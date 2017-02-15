/* 
 * File:   main.cpp
 * Author: camilo, rsamaniego
 *
 * Created on 10 de mayo de 2010, 17:28
 */

#include <stdio.h>
#include <sys/time.h>
#include <cv.h>
#include <highgui.h>

#define CALIBRATION_HEIGHT_MM 27.90 // Height of our calibration reference  object
#define DEFAULT_Y_BEGIN 100                 // Frame's upper dead zone (unused zone)
#define PREFILTER_THRESHOLD 200     // Ignore all pixels below this intensity
#define PEAK_THRESHOLD 210          // Pixels' intensity below this value are not considered as valid laser reflections
#define MAX_TEXT_TO_DISPLAY 64
#define CAMERA_ID 1

CvFont myFont;
char* textToDisplay;
struct timeval t_current, t_prev;
IplImage* annotated;
IplImage* workingFrame;

bool showText;
int calibrationPixels;
int baseLine;
bool calibrated;
bool showResult;
float resolution; // mm per pixel
time_t t0, t1;
int yBegin;

void init()
{
  cvInitFont(&myFont, CV_FONT_HERSHEY_COMPLEX_SMALL, 0.75f, 0.75f, 0, 1, CV_AA);
  textToDisplay = (char*) malloc(MAX_TEXT_TO_DISPLAY * sizeof (char));

  gettimeofday(&t_current, 0);
  gettimeofday(&t_prev, 0);

  showText = true;
  calibrated = false;
  showResult = false;
  resolution = 0.0f;
  calibrationPixels = 0;
  baseLine = 0;
  workingFrame = 0;
  yBegin = DEFAULT_Y_BEGIN;
}

/** Time difference, in seconds, between two timeval structs.
 * 
 * @param a last timeval
 * @param b first timeval
 * @return a minus b in seconds (with sign).
 */
static double getTimeDiff(const struct timeval *a,
                          const struct timeval *b)
{
  return (double) (a->tv_sec + (double) a->tv_usec / 1000000) -
          (double) (b->tv_sec + (double) b->tv_usec / 1000000);
}

int main()
{
  init();

  CvCapture *camera = cvCreateCameraCapture(CAMERA_ID);
  IplImage* original = 0;
  cvNamedWindow("Laser measurement", CV_WINDOW_AUTOSIZE);

  while (true)
  {
    original = cvQueryFrame(camera);

    if (!original)
    {
      fprintf(stderr, "ERROR: Could not acquire image from camera\n");
      break;
    }

    // Filter laser pixels
    if (!workingFrame)
    {
      workingFrame = cvCreateImage(cvGetSize(original), 8, 1);
    }
    cvCvtColor(original, workingFrame, CV_RGB2GRAY);
    cvThreshold(workingFrame, workingFrame, PREFILTER_THRESHOLD, 0, CV_THRESH_TOZERO);

    int maxHeight = 0;
    int upper = workingFrame->height; // Highest position of the laser line
    int lower = 0; // Lowest position of the laser line

    // From left to right...
    for (int x = 0; x < workingFrame->width; x++)
    {
      int startY = -1;
      int endY = -1;
      int maxY = -1;
      uchar maxValue = 0;

      // ... find laser line center
      for (int y = yBegin; y < workingFrame->height; y++)
      {
        uchar val = ((uchar*) (workingFrame->imageData + y * workingFrame->widthStep))[x];

        if (startY < 0 && val >= PEAK_THRESHOLD)
        {
          startY = y;
        }

        if (startY >= 0 && endY < 0)
        {
          if (val > maxValue)
          {
            maxValue = val;
            maxY = y;
          }

          if (val < PEAK_THRESHOLD)
          {
            endY = y;
            maxY = startY + (endY - startY) / 2;
            cvLine(original, cvPoint(x, startY), cvPoint(x, endY), CV_RGB(0, 255, 0), 1, 16); // Paint detected laser area
            cvCircle(original, cvPoint(x, maxY), 1, CV_RGB(255, 0, 0), -1, 16); // Paint center of laser line

            break;
          }
        }
      }

      if (maxY > 0 && maxY < upper)
      {
        upper = maxY;
      }

      if (maxY > lower)
      {
        lower = maxY;
      }
    }

    maxHeight = lower - upper; // Max laser line difference found
    cvLine(original, CvPoint(0, lower), CvPoint(original->width - 1, lower), CV_RGB(255, 128, 128), 1, CV_AA); // Paint zero reference
    cvLine(original, CvPoint(0, upper), CvPoint(original->width - 1, upper), CV_RGB(255, 128, 128), 1, CV_AA); // Paint measured height

    // Our rig may be not yet calibrated
    calibrationPixels = maxHeight;
    float maxHeightMm = calibrated ? maxHeight * resolution : 0.0f;
    baseLine = calibrated ? baseLine : upper;
    cvLine(original, CvPoint(0, baseLine), CvPoint(original->width - 1, baseLine), CV_RGB(128, 255, 128), 1, CV_AA);

    //  Mask non used zone
    for (int y = 0; y < yBegin; y++)
    {
      cvLine(original, CvPoint(0, y), CvPoint(original->width - 1, y), CV_RGB(128, 128, 255), 1, CV_AA);
    }

    // Calculate and display height data, as well as _*REAL*_ fps obtained
    if (showText)
    {
      gettimeofday(&t_current, 0);
      if (calibrated)
      {
        cvRectangle(original, cvPoint(0, 0), cvPoint(original->width, 20), CV_RGB(50, 50, 50), CV_FILLED, CV_AA);
        sprintf(textToDisplay, "%.2f max. height -- %.2f mm per pixel -- %d x %d pixels, %d fps", maxHeightMm, resolution,
                original->width, original->height, (int) (1.0f / getTimeDiff(&t_current, &t_prev)));
      } else
      {
        cvRectangle(original, cvPoint(0, 0), cvPoint(original->width, 20), CV_RGB(200, 50, 50), CV_FILLED, CV_AA);
        sprintf(textToDisplay, "%d max. height -- Uncalibrated -- %d x %d pixels, %d fps", maxHeight,
                original->width, original->height, (int) (1.0f / getTimeDiff(&t_current, &t_prev)));
      }
      t_prev = t_current;
      cvPutText(original, textToDisplay, cvPoint(6, 16), &myFont, CV_RGB(0, 0, 0));
      cvPutText(original, textToDisplay, cvPoint(5, 15), &myFont, CV_RGB(255, 255, 255));
    }

    cvShowImage("Laser measurement", showResult ? original : workingFrame);

    int key = cvWaitKey(33);
    bool end = false;
    switch ((char) key) {
    case -1:
      break;
    case 32: // space
      cvWaitKey(0);
      break;
    case 27: // esc
      end = true;
      break;
    case 116: // t
    {
      showText = !showText;
      break;
    }
    case 99: // c
    {
      if (calibrationPixels == 0)
      {
        printf("Measured pixel difference is zero. Calibration not done.\n");
      } else
      {
        resolution = CALIBRATION_HEIGHT_MM / calibrationPixels;
        printf("Calibration result: %f mm per pixel\n", resolution);
        calibrated = true;
      }
      break;
    }
    case 82: // Down arrow
    {
      yBegin += 5;
      break;
    }
    case 84: // Up arrow
    {
      yBegin -= 5;
      break;
    }
    case 109: // m
    {
      showResult = !showResult;
      break;
    }
    default:
    {
      printf("No action for key code %d\n", (char) key);
      printf("   t - Toggle text visualization\n");
      printf("   c - (re)Calibrate Z resolution\n");
      printf("   m - Toggle visualization mode\n");
      printf("   space - Freeze until next key press\n");
      printf("   ESC - Exit\n");
    }
    }

    if (end) break;

    cvReleaseImage(&workingFrame);
  }

  // Free resources and exit
  cvReleaseImage(&original);
  cvReleaseImage(&workingFrame);
  cvReleaseCapture(&camera);
  cvDestroyWindow("Laser measurement");
  free(textToDisplay);

  return 0;
}
