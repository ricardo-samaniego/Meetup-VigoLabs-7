# USAGE
# python ball_tracking.py --video ball_tracking_example.mp4
# python ball_tracking.py

# import the necessary packages
from collections import deque
import numpy as np
import argparse
import imutils
import cv2
import sys

PY3 = sys.version_info[0] == 3
if PY3:
    xrange = range

# construct the argument parse and parse the arguments
ap = argparse.ArgumentParser()
ap.add_argument("-v", "--video",
	help="path to the (optional) video file")
ap.add_argument("-b", "--buffer", type=int, default=32,
	help="max buffer size")
args = vars(ap.parse_args())

# define the lower and upper boundaries of the "green"
# ball in the HSV color space, then initialize the
# list of tracked points
# Green ball:
greenLower = (7, 86, 94)
greenUpper = (43, 255, 221)

# Blue ball:
blueLower = (82, 92, 74)
blueUpper = (146, 214, 198)

minRadius = 5

greenPts = deque(maxlen=args["buffer"])
bluePts = deque(maxlen=args["buffer"])

# Finds a ball and returns position and radius
def findBall(hsvFrame, lowerColor, upperColor):
        # construct a mask for the given color, then perform
	# a series of dilations and erosions to remove any small
	# blobs left in the mask
	mask = cv2.inRange(hsvFrame, lowerColor, upperColor)
	mask = cv2.erode(mask, None, iterations=2)
	mask = cv2.dilate(mask, None, iterations=2)
	
	# find contours in the mask and initialize the current
	# (x, y) center of the ball
	cnts = cv2.findContours(mask.copy(), cv2.RETR_EXTERNAL,
		cv2.CHAIN_APPROX_SIMPLE)[-2]
	center = None
	radius = 0
	x = 0
	y = 0
	
	# only proceed if at least one contour was found
	if len(cnts) > 0:
		# find the largest contour in the mask, then use
		# it to compute the minimum enclosing circle and
		# centroid
		centroid = max(cnts, key=cv2.contourArea)
		((x, y), radius) = cv2.minEnclosingCircle(centroid)
		moments = cv2.moments(centroid)
		center = (int(moments["m10"] / moments["m00"]), int(moments["m01"] / moments["m00"]))
        
	return (center, radius)

def drawCircle(frame, center, radius, color):
	# only proceed if the radius meets a minimum size
	if radius > minRadius:
		# draw the circle and centroid on the frame,
		# then update the list of tracked points
		cv2.circle(frame, center, int(radius), (255, 255, 255), 2)
		cv2.circle(frame, center, 5, color, -1)

def drawTail(frame, pts, radius, color):
	# loop over the set of tracked points
	for i in xrange(1, len(pts)):
		# if either of the tracked points are None, ignore
		# them
		if pts[i - 1] is None or pts[i] is None:
			continue

		# otherwise, compute the thickness of the line and
		# draw the connecting lines
		thickness = int(np.sqrt(args["buffer"] / float(i + 1)) * radius/5)
		cv2.line(frame, pts[i - 1], pts[i], color, thickness)

# if a video path was not supplied, grab the reference
# to the webcam
if not args.get("video", False):
	camera = cv2.VideoCapture(1)

# otherwise, grab a reference to the video file
else:
	camera = cv2.VideoCapture(args["video"])

# keep looping
while True:
	# grab the current frame
	(grabbed, frame) = camera.read()

	# if we are viewing a video and we did not grab a frame,
	# then we have reached the end of the video
	if args.get("video") and not grabbed:
		break

	# resize the frame, blur it, and convert it to the HSV
	# color space
	frame = imutils.resize(frame, width=600)
	# blurred = cv2.GaussianBlur(frame, (11, 11), 0)
	hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

	# Find color balls, if present
	(greenCenter, greenRadius) = findBall(hsv, greenLower, greenUpper)
	(blueCenter, blueRadius) = findBall(hsv, blueLower, blueUpper)

	# Draw circles
	drawCircle(frame, greenCenter, greenRadius, (0, 255, 0))
	drawCircle(frame, blueCenter, blueRadius, (255, 0, 0))

	# update the points queue
	greenPts.appendleft(greenCenter)
	bluePts.appendleft(blueCenter)

	# draw tails
	drawTail(frame, greenPts, greenRadius, (0, 255, 0))
	drawTail(frame, bluePts, blueRadius, (255, 0, 0))

	# show the frame to our screen
	cv2.imshow("Frame", frame)
	key = cv2.waitKey(1) & 0xFF

	# if the "Esc" key is pressed, stop the loop
	if key == 27:
		break

# cleanup the camera and close any open windows
camera.release()
cv2.destroyAllWindows()
