#include <opencv2/opencv.hpp>

int main(int argc, char** argv)
{
    cv::UMat img;                                       // Image object
    cv::VideoCapture cap(1);                            // Open first webcam found
    
    while(true)                                         // Forever
    {
        cap >> img;                                     // Take image

        cv::imshow("Meetup Vigolabs", img);             // Show on window
        if((uchar) cv::waitKey(40) == 27) return 0;     // Exit on "esc" key
    }
    
    return 0;                                           // End of program
}
