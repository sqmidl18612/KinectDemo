#include <stdlib.h>
#include <iostream>

#include "opencv/cv.h"
#include "opencv/highgui.h"

#include <XnCppWrapper.h>


using namespace std;
using namespace cv;

ostream& operator<<(ostream &out, const XnPoint3D &rPoint)
{
	out << "(" << rPoint.X << "," << rPoint.Y << "," << rPoint.Z << ")";
	return out;
}

void XN_CALLBACK_TYPE gestureRecog(xn::GestureGenerator &generator,
						const XnChar *strGesture,
						const XnPoint3D *pIDPosition,
						const XnPoint3D *pEndPosition,
						void *pCookie)
{
	cout << strGesture << " from " << *pIDPosition << " to " << *pEndPosition << endl;

	int imgStartX = 0;
	int imgStartY = 0;
	int imgEndX = 0;
	int imgEndY = 0;

	char locationInfo[100];

	imgStartX = (int)(640 / 2 - (pIDPosition->X));
	imgStartY = (int)(640 / 2 - (pIDPosition->Y));
	imgEndX = (int)(640 / 2 - (pEndPosition->X));
	imgEndY = (int)(640 / 2 - (pEndPosition->Y));

	IplImage * refImage = (IplImage *)pCookie;

	if(strcmp(strGesture, "RaiseHand") == 0)
	{
		cvCircle(refImage, cvPoint(imgStartX, imgStartY), 1, CV_RGB(255, 0, 0), 2);
	}
	else if(strcmp(strGesture, "Wave") == 0)
	{
		cvLine(refImage, cvPoint(imgStartX, imgStartY), cvPoint(imgEndX, imgEndY),
			CV_RGB(255, 255, 0), 6);
	}
	else if(strcmp(strGesture, "Click") == 0)
	{
		cvCircle(refImage, cvPoint(imgStartX, imgStartY), 6, CV_RGB(0, 0, 255), 12);
	}

	cvSetImageROI(refImage, cvRect(40, 450, 640, 30));
	CvFont font;
	cvInitFont(&font, CV_FONT_VECTOR0, 1, 1, 0, 3, 5);
	cvSet(refImage, cvScalar(255, 255, 255));
	sprintf(locationInfo, "From: %d,%d to %d,%d", (int)pIDPosition->X, (int)pIDPosition->Y,
		(int)(pEndPosition->X), (int)(pEndPosition->Y));
	cvPutText(refImage, locationInfo, cvPoint(30, 30), &font, CV_RGB(0, 0, 0));
	cvResetImageROI(refImage);

}


void clearImg(IplImage *inputImg)
{
	CvFont font;
	cvInitFont(&font, CV_FONT_VECTOR0, 1, 1, 0, 3, 5);
	memset(inputImg->imageData, 255, 640 * 480 * 3);
	cvPutText(inputImg, "Hand Raise!", cvPoint(20, 20), &font, CV_RGB(255, 0, 0));
	cvPutText(inputImg, "Hand Wave!", cvPoint(20, 50), &font, CV_RGB(255, 255, 0));
	cvPutText(inputImg, "Hand Push!", cvPoint(20, 80), &font, CV_RGB(0, 0, 255));
}

void XN_CALLBACK_TYPE gestureProcess(xn::GestureGenerator &generator,
								const XnChar *strGesture,
								const XnPoint3D *pPosition,
								XnFloat fProgress,
								void *PCookie)
{
	cout << strGesture << ":" << fProgress << " at " << *pPosition << endl;
}

int main(int argc, char *argv[])
{
	IplImage *drawImg = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 3);
	IplImage *cameraImg = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 3);

	cvNamedWindow("Gesture", 1);
	cvNamedWindow("Camera", 1);

	clearImg(drawImg);

	XnStatus res;
	char key = 0;

	xn::Context context;
	res = context.Init();

	xn::ImageMetaData imageMD;

	xn::ImageGenerator imageGenerator;
	res = imageGenerator.Create(context);

	xn::GestureGenerator gestureGenerator;
	res = gestureGenerator.Create(context);

	gestureGenerator.AddGesture("Wave", NULL);
	gestureGenerator.AddGesture("Click", NULL);
	gestureGenerator.AddGesture("RaiseHand", NULL);

	XnCallbackHandle handle;
	gestureGenerator.RegisterGestureCallbacks(gestureRecog, 
		gestureProcess, (void *)drawImg, handle);

	context.StartGeneratingAll();
	res = context.WaitAndUpdateAll();

	while((key != 27) && !(res = context.WaitAndUpdateAll()))
	{
		if(key == 'c')
		{
			clearImg(drawImg);
		}

		imageGenerator.GetMetaData(imageMD);
		memcpy(cameraImg->imageData, imageMD.Data(), 640 * 480 * 3);
		cvCvtColor(cameraImg, cameraImg, CV_RGB2BGR);

		cvShowImage("Gesture", drawImg);
		cvShowImage("Camera", cameraImg);

		key = waitKey(20);
	}

	cvDestroyWindow("Gesture");
	cvDestroyWindow("Camera");
	cvReleaseImage(&drawImg);
	cvReleaseImage(&cameraImg);

	context.StopGeneratingAll();
	context.Shutdown();

	return 0;
}

