#include <stdlib.h>
#include <iostream>
#include <vector>

#include <XnCppWrapper.h>
#include <XnModuleCppInterface.h>

#include "opencv/cv.h"
#include "opencv/highgui.h"

using namespace std;
using namespace cv;

xn::UserGenerator userGenerator;
xn::DepthGenerator depthGenerator;
xn::ImageGenerator imageGenerator;

int startSkelPoints[14] = {1, 2, 6, 6, 12, 17, 6, 7, 12, 13, 17, 18, 21, 22};  
int endSkelPoints[14] = {2, 3, 12, 21, 17, 21, 7, 9, 13, 15, 18, 20, 22, 24};  

void XN_CALLBACK_TYPE NewUser(xn::UserGenerator &generator, XnUserID user, void *pCookie)
{
	cout << "New user identified: " << user << endl;
	generator.GetPoseDetectionCap().StartPoseDetection("Psi", user);
}

void XN_CALLBACK_TYPE LostUser(xn::UserGenerator &generator, XnUserID user, void *pCookie)
{
	cout << "User " << user << " lost" << endl;
}

void XN_CALLBACK_TYPE CalibrationStart(xn::SkeletonCapability &skeleton, XnUserID user, void *pCookie)
{
	cout << "Calibration start for user" << user << endl;
}

void XN_CALLBACK_TYPE CalibrationEnd(xn::SkeletonCapability &skeleton, XnUserID user, 
			XnCalibrationStatus calibrationError, void *pCookie)
{
	cout << "Calibration complete for user" << user << ",";
	if(calibrationError == XN_CALIBRATION_STATUS_OK)
	{
		cout << "Success" << endl;
		skeleton.StartTracking(user);
	}
	else
	{
		cout << "Failure" << endl;
		((xn::UserGenerator *)pCookie)->GetPoseDetectionCap().StartPoseDetection("Psi", user);
	}
}

void XN_CALLBACK_TYPE PoseDetected(xn::PoseDetectionCapability &poseDetection, 
		const XnChar *strPose, XnUserID user, void *pCookie)
{
	cout << "Pose " << strPose << " detected for user" << user << endl;
	((xn::UserGenerator *)pCookie)->GetSkeletonCap().RequestCalibration(user, FALSE);
}

void clearImg(IplImage *inputImg)
{
	CvFont font;
	cvInitFont(&font, CV_FONT_VECTOR0, 1, 1, 0, 3, 5);
	memset(inputImg->imageData, 255, 640 * 480 * 3);
}

int main(int argc, char *argv[])
{
	char key = 0;
	int imgPosX = 0;
	int imgPosY = 0;

	xn::Context context;
	context.Init();
	xn::ImageMetaData imageMD;

	IplImage *cameraImg = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 3);
	cvNamedWindow("Camera", 1);

	XnMapOutputMode mapMode;
	mapMode.nXRes = 640;
	mapMode.nYRes = 480;
	mapMode.nFPS = 30;


	depthGenerator.Create(context);
	depthGenerator.SetMapOutputMode(mapMode);
	imageGenerator.Create(context);
	userGenerator.Create(context);

	XnCallbackHandle userCBHandle;
	userGenerator.RegisterUserCallbacks(NewUser, LostUser, NULL, userCBHandle);

	xn::SkeletonCapability skeletonCap = userGenerator.GetSkeletonCap();
	skeletonCap.SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
	XnCallbackHandle calibCBHandle;
	skeletonCap.RegisterToCalibrationStart(CalibrationStart, &userGenerator, calibCBHandle);
	skeletonCap.RegisterToCalibrationComplete(CalibrationEnd, &userGenerator, calibCBHandle);

	XnCallbackHandle poseCBHandle;
	userGenerator.GetPoseDetectionCap().RegisterToPoseDetected(PoseDetected, &userGenerator, poseCBHandle);

	context.StartGeneratingAll();
	while(key != 27)
	{
		context.WaitAndUpdateAll();

		imageGenerator.GetMetaData(imageMD);
		memcpy(cameraImg->imageData, imageMD.Data(), 640 * 480 * 3);
		cvCvtColor(cameraImg, cameraImg, CV_RGB2BGR);

		XnUInt16 userCounts = userGenerator.GetNumberOfUsers();
		if(userCounts > 0)
		{
			XnUserID *userID = new XnUserID(userCounts);
			userGenerator.GetUsers(userID, userCounts);
			for(int i = 0; i <userCounts; ++i)
			{
				if(skeletonCap.IsTracking(userID[i]))
				{
					XnPoint3D skelPointsIn[24], skelPointsOut[24];
					XnSkeletonJointTransformation mJointTram;
					for(int iter = 0; iter < 24; iter++)
					{
						skeletonCap.GetSkeletonJoint(userID[i], XnSkeletonJoint(iter + 1), mJointTram);
						skelPointsIn[iter] = mJointTram.position.position;
					}
					depthGenerator.ConvertRealWorldToProjective(24, skelPointsIn, skelPointsOut);
				
					for(int j = 0; j < 14; j++)
					{
						CvPoint startPoint = cvPoint(skelPointsOut[startSkelPoints[j] - 1].X,
							skelPointsOut[startSkelPoints[j] - 1].Y);
						CvPoint endPoint = cvPoint(skelPointsOut[endSkelPoints[j] - 1].X,
							skelPointsOut[endSkelPoints[j] - 1].Y);

						cvCircle(cameraImg, startPoint, 3, CV_RGB(0, 0, 255), 12);
						cvCircle(cameraImg, endPoint, 3, CV_RGB(0, 0, 255), 12);
						cvLine(cameraImg, startPoint, endPoint, CV_RGB(0, 0, 255), 4);
					}
							
				
				}

								
			}

			delete [] userID;
			cvShowImage("Camera", cameraImg);
			key = cvWaitKey(20);
		}
	}

	cvDestroyWindow("Camera");
	cvReleaseImage(&cameraImg);
	context.StopGeneratingAll();
	context.Shutdown();

	return 0;
}