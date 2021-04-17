#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>

#include <XnCppWrapper.h>
#include "opencv/cv.h"
#include "opencv/highgui.h"

using namespace std;
using namespace cv;

void CheckOpenNIError(XnStatus result, string status)
{
	if(result != XN_STATUS_OK)
		cerr << status << "Error: " << xnGetStatusString(result) << endl;
}

// callback function of user generator: new user
void XN_CALLBACK_TYPE NewUser( xn::UserGenerator& generator,
	XnUserID user,
	void* pCookie )
{
	cout << "New user identified: " << user << endl;
	generator.GetSkeletonCap().RequestCalibration( user, true );
}

// callback function of skeleton: calibration end 
void XN_CALLBACK_TYPE CalibrationEnd( xn::SkeletonCapability& skeleton,
	XnUserID user,
	XnCalibrationStatus eStatus,
	void* pCookie )
{
	cout << "Calibration complete for user " <<  user << ", ";
	if( eStatus == XN_CALIBRATION_STATUS_OK )
	{
		cout << "Success" << endl;
		skeleton.StartTracking( user );
	}
	else
	{
		cout << "Failure" << endl;
		skeleton.RequestCalibration( user, true );
	}
}

void XN_CALLBACK_TYPE LostUser(xn::UserGenerator &generator, XnUserID user, void *pCookie)
{
	cout << "User " << user << " lost" << endl;
}

void clearImg(IplImage *inputImg)
{
	CvFont font;
	cvInitFont(&font, CV_FONT_VECTOR0, 1, 1, 0, 3, 5);
	memset(inputImg->imageData, 255, 640 * 480 * 3);
}


int main( int argc, char** argv )
{
	XnStatus result = XN_STATUS_OK;

	// 1. initial context
	xn::Context mContext;
	result = mContext.Init();
	CheckOpenNIError(result, "initialize context");

	mContext.SetGlobalMirror(true);

	xn::ImageMetaData imageMD;
	IplImage *cameraImg = cvCreateImage(cvSize(640, 480), IPL_DEPTH_8U, 3);
	cvNamedWindow("Camera", 1);

	XnMapOutputMode mapMode;
	mapMode.nXRes = 640;
	mapMode.nYRes = 480;
	mapMode.nFPS = 30;

	xn::DepthGenerator mDepthGenerator;
	mDepthGenerator.Create(mContext);
	mDepthGenerator.SetMapOutputMode(mapMode);

	xn::ImageGenerator mImageGenerator;
	mImageGenerator.Create(mContext);

	// 2. create user generator
	xn::UserGenerator mUserGenerator;
	result = mUserGenerator.Create( mContext );
	CheckOpenNIError(result, "Create user generator");


	// 3. Register callback functions of user generator
	XnCallbackHandle hUserCB;
	mUserGenerator.RegisterUserCallbacks( NewUser, LostUser, NULL, hUserCB );

	// 4. Register callback functions of skeleton capability
	xn::SkeletonCapability mSC = mUserGenerator.GetSkeletonCap();
	//mSC.SetSkeletonProfile( XN_SKEL_PROFILE_ALL );
	mSC.SetSkeletonProfile( XN_SKEL_PROFILE_UPPER );
	XnCallbackHandle hCalibCB;
	mSC.RegisterToCalibrationComplete( CalibrationEnd, &mUserGenerator, hCalibCB );


	// 5. start generate data
	mContext.StartGeneratingAll();
	while( true )
	{
		// 6. Update date
		mContext.WaitAndUpdateAll();

		mImageGenerator.GetMetaData(imageMD);
		memcpy(cameraImg->imageData, imageMD.Data(), 640 * 480 * 3);
		cvCvtColor(cameraImg, cameraImg, CV_RGB2BGR);

		// 7. get user information
		XnUInt16 nUsers = mUserGenerator.GetNumberOfUsers();
		if( nUsers > 0 )
		{
			// 8. get users
			XnUserID* aUserID = new XnUserID[nUsers];
			mUserGenerator.GetUsers( aUserID, nUsers );

			// 9. check each user
			for( int i = 0; i < nUsers; ++i )
			{
				// 10. if is tracking skeleton
				if( mSC.IsTracking( aUserID[i] ) )
				{
					// 11. get skeleton joint data
					XnSkeletonJointTransformation mJointTran;
					//mSC.GetSkeletonJoint( aUserID[i], XN_SKEL_HEAD, mJointTran);
					mSC.GetSkeletonJoint( aUserID[i], XN_SKEL_RIGHT_HAND, mJointTran);

					XnPoint3D skelPointIn, skelPointOut;
					skelPointIn = mJointTran.position.position;

					mDepthGenerator.ConvertRealWorldToProjective(1, &skelPointIn, &skelPointOut);
					cvCircle(cameraImg, cvPoint(skelPointOut.X, skelPointOut.Y),
						3, CV_RGB(0, 0, 255), 12);

					#if 0
// 12. output information
					cout << "The head of user " << aUserID[i] << " is at (";
					cout << mJointTran.position.position.X << ", ";
					cout << mJointTran.position.position.Y << ", ";
					cout << mJointTran.position.position.Z << ")" << endl;
#endif
				}
			}
			delete [] aUserID;
			cvShowImage("Camera", cameraImg);
			cvWaitKey(20);
		}

	}
	// 13. stop and shutdown
	mContext.StopGeneratingAll();
	mContext.Release();

	return 0;
}