#include <stdlib.h>
#include <string>
#include <iostream>

#include <XnCppWrapper.h>


using namespace std;


struct SNode {
	char *sGestureToUse;
	char *sGestureToPress;
	xn::DepthGenerator mDepth;
	xn::HandsGenerator mHand;
	xn::GestureGenerator mGesture;
};

void XN_CALLBACK_TYPE GestureRecognized(xn::GestureGenerator &generator,
				const XnChar *strGesture,
				const XnPoint3D *pIDPostion,
				const XnPoint3D *pEndPosition,
				void *pCookie)
{
	SNode *pNodes = ((SNode *)pCookie);
	if(strcmp(strGesture, pNodes->sGestureToPress) == 0)
	{
		cout << "Left Button" << endl;
	} 
	else if(strcmp(strGesture, pNodes->sGestureToUse) == 0)
	{
		cout << "Start moving" << endl;
		generator.RemoveGesture(strGesture);
		pNodes->mHand.StartTracking(*pEndPosition);
	}
}

void XN_CALLBACK_TYPE HandCreate(xn::HandsGenerator &generator,
		XnUserID nId, const XnPoint3D *pPosition, XnFloat fTime, void *pCookie)
{
	SNode *pNodes = ((SNode *)pCookie);
	cout << "New Hand: " << nId << " detected!" << endl;
	cout << pPosition->X << "/" << pPosition->Y << "/" << pPosition->Z << endl;
	pNodes->mGesture.AddGesture(pNodes->sGestureToPress, NULL);
}

void XN_CALLBACK_TYPE HandUpdate(xn::HandsGenerator &generator,
	XnUserID nId, const XnPoint3D *pPosition, XnFloat fTime, void *pCookie)
{
	SNode *pNodes = ((SNode *)pCookie);
	XnPoint3D wPos;
	pNodes->mDepth.ConvertRealWorldToProjective(1, pPosition, &wPos);
	cout << wPos.X << "/" << wPos.Y << endl;
}


void XN_CALLBACK_TYPE HandDestroy(xn::HandsGenerator &generator,
	XnUserID nId, XnFloat fTime, void *pCookie)
{
	SNode *pNodes = ((SNode *)pCookie);
	cout << "Lost Hand: " << nId << endl;
	pNodes->mGesture.AddGesture(pNodes->sGestureToUse, NULL);
	pNodes->mGesture.RemoveGesture(pNodes->sGestureToPress);
}

int main(int argc, char *argv[])
{
	xn::Context mContext;
	mContext.Init();

	SNode mNodes;
	mNodes.mDepth.Create(mContext);
	mNodes.mGesture.Create(mContext);
	mNodes.mHand.Create(mContext);

	mNodes.mHand.SetSmoothing(0.5f);
	mNodes.sGestureToPress = "Click";
	mNodes.sGestureToUse = "RaiseHand";

	XnCallbackHandle hGesture;
	mNodes.mGesture.RegisterGestureCallbacks(GestureRecognized, NULL,
		&mNodes, hGesture);
	mNodes.mGesture.AddGesture(mNodes.sGestureToUse, NULL);

	XnCallbackHandle hHandle;
	mNodes.mHand.RegisterHandCallbacks(HandCreate, HandUpdate, HandDestroy,
				&mNodes, hHandle);

	mContext.StartGeneratingAll();
	while(true)
	{
		mContext.WaitAndUpdateAll();
	}

	mContext.StopGeneratingAll();
	mContext.Release();

	return 0;
}