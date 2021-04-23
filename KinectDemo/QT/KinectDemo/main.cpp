// Standard C++ header
#include <stdlib.h>
#include <iostream>
#include <vector>

// Qt Header
#include <QApplication>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

// OpenNI Header
#include <XnCppWrapper.h>

// namespace
using namespace std;

/* Class for control OpenNI device */
class COpenNI
{
public:
	/* Destructor */
	~COpenNI()
	{
		m_Context.Release();
	}

	/* Initial OpenNI context and create nodes. */
	bool Initial()
	{
		// Initial OpenNI Context
		m_eResult = m_Context.Init();
		if( CheckError( "Context Initial failed" ) )
			return false;

        m_eResult = m_Context.SetGlobalMirror(true);
        if(CheckError( "Set Global Mirror Error" ))
            return false;

		// create image node
		m_eResult = m_Image.Create( m_Context );
		if( CheckError( "Create Image Generator Error" ) )
			return false;

		// create depth node
		m_eResult = m_Depth.Create( m_Context );
		if( CheckError( "Create Depth Generator Error" ) )
			return false;

		// create user node
		m_eResult = m_User.Create( m_Context );
		if( CheckError( "Create User Generator Error" ) )
			return false;

		// set nodes
		m_eResult = m_Depth.GetAlternativeViewPointCap().SetViewPoint( m_Image );
		CheckError( "Can't set the alternative view point on depth generator" );

		XnCallbackHandle hUserCB;
		m_User.RegisterUserCallbacks( CB_NewUser, NULL, NULL, hUserCB );

        m_User.GetSkeletonCap().SetSkeletonProfile( XN_SKEL_PROFILE_ALL );
        //m_User.GetSkeletonCap().SetSkeletonProfile( XN_SKEL_PROFILE_UPPER );
		XnCallbackHandle hCalibCB;
		m_User.GetSkeletonCap().RegisterToCalibrationComplete( CB_CalibrationComplete, &m_User, hCalibCB );

		XnCallbackHandle hPoseCB;
		m_User.GetPoseDetectionCap().RegisterToPoseDetected( CB_PoseDetected, &m_User, hPoseCB );

		return true;
	}

	/* Start to get the data from device */
	bool Start()
	{
		m_eResult = m_Context.StartGeneratingAll();
		return !CheckError( "Start Generating" );
	}

	/* Update / Get new data */
	bool UpdateData()
	{
		// update
		m_eResult = m_Context.WaitNoneUpdateAll();
		if( CheckError( "Update Data" ) )
			return false;

		// get new data
		m_Depth.GetMetaData( m_DepthMD );
		m_Image.GetMetaData( m_ImageMD );

		return true;
	}

	/* Get User generator */
	xn::UserGenerator& GetUserGenerator()
	{
		return m_User;
	}

	/* Get Depth generator */
	xn::DepthGenerator& GetDepthGenerator()
	{
		return m_Depth;
	}

public:
	xn::DepthMetaData		m_DepthMD;
	xn::ImageMetaData		m_ImageMD;

private:
	/* Check return status m_eResult.
	 * return false if the value is XN_STATUS_OK, true for error */
	bool CheckError( const char* sError )
	{
		if( m_eResult != XN_STATUS_OK )
		{
			cerr << sError << ": " << xnGetStatusString( m_eResult ) << endl;
			return true;
		}
		return false;
	}

private:
	static void XN_CALLBACK_TYPE CB_NewUser( xn::UserGenerator& generator, XnUserID user, void* pCookie )
	{
		cout << "New user identified: " << user << endl;
		generator.GetPoseDetectionCap().StartPoseDetection("Psi", user);
	}

	static void XN_CALLBACK_TYPE CB_CalibrationComplete( xn::SkeletonCapability& skeleton, XnUserID user, XnCalibrationStatus calibrationError, void* pCookie )
	{
		cout << "Calibration complete for user " <<  user << ", ";
		if( calibrationError == XN_CALIBRATION_STATUS_OK )
		{
			cout << "Success" << endl;
			skeleton.StartTracking( user );
		}
		else
		{
			cout << "Failure" << endl;
			xn::UserGenerator* pUser = (xn::UserGenerator*)pCookie;
			pUser->GetPoseDetectionCap().StartPoseDetection( "Psi", user );
		}
	}

	static void XN_CALLBACK_TYPE CB_PoseDetected( xn::PoseDetectionCapability& poseDetection, const XnChar* strPose, XnUserID user, void* pCookie)
	{
		cout << "Pose " << strPose << " detected for user " <<  user << endl;
		xn::UserGenerator* pUser = (xn::UserGenerator*)pCookie;
		pUser->GetSkeletonCap().RequestCalibration( user, FALSE );
		poseDetection.StopPoseDetection( user );
	}
	
private:
	XnStatus			m_eResult;
	xn::Context			m_Context;
	xn::DepthGenerator	m_Depth;
	xn::ImageGenerator	m_Image;
	xn::UserGenerator	m_User;
};

/* Class for draw skeleton */
class CSkelItem : public QGraphicsItem
{
public:
	/* Constructor */
	CSkelItem( XnUserID& uid, COpenNI& rOpenNI ) : QGraphicsItem(), m_UserID( uid ), m_OpenNI( rOpenNI )
	{
		// build lines connection table
		// body and head
		{
			m_aConnection[0][0] = 0;
			m_aConnection[0][1] = 1;

			m_aConnection[1][0] = 1;
			m_aConnection[1][1] = 2;

			m_aConnection[2][0] = 1;
			m_aConnection[2][1] = 3;
		}
		// left hand
		{
			m_aConnection[3][0] = 1;
			m_aConnection[3][1] = 3;

			m_aConnection[4][0] = 3;
			m_aConnection[4][1] = 4;

			m_aConnection[5][0] = 4;
			m_aConnection[5][1] = 5;
		}
		// right hand
		{
			m_aConnection[6][0] = 1;
			m_aConnection[6][1] = 6;

			m_aConnection[7][0] = 6;
			m_aConnection[7][1] = 7;

			m_aConnection[8][0] = 7;
			m_aConnection[8][1] = 8;
		}
		// left leg
		{
			m_aConnection[9][0] = 2;
			m_aConnection[9][1] = 9;

			m_aConnection[10][0] = 9;
			m_aConnection[10][1] = 10;

			m_aConnection[11][0] = 10;
			m_aConnection[11][1] = 11;
		}
		// right leg
		{
			m_aConnection[12][0] = 2;
			m_aConnection[12][1] = 12;

			m_aConnection[13][0] = 12;
			m_aConnection[13][1] = 13;

			m_aConnection[14][0] = 13;
			m_aConnection[14][1] = 14;
		}
	}

	/* update skeleton data */
    void UpdateSkeleton(XnPoint3D &pos)
	{
		// read the position in real world
		XnPoint3D	JointsReal[15];
		JointsReal[ 0] = GetSkeletonPos( XN_SKEL_HEAD			);
		JointsReal[ 1] = GetSkeletonPos( XN_SKEL_NECK			);
		JointsReal[ 2] = GetSkeletonPos( XN_SKEL_TORSO			);
		JointsReal[ 3] = GetSkeletonPos( XN_SKEL_LEFT_SHOULDER	);
		JointsReal[ 4] = GetSkeletonPos( XN_SKEL_LEFT_ELBOW		);
		JointsReal[ 5] = GetSkeletonPos( XN_SKEL_LEFT_HAND		);
		JointsReal[ 6] = GetSkeletonPos( XN_SKEL_RIGHT_SHOULDER	);
		JointsReal[ 7] = GetSkeletonPos( XN_SKEL_RIGHT_ELBOW	);
		JointsReal[ 8] = GetSkeletonPos( XN_SKEL_RIGHT_HAND		);
		JointsReal[ 9] = GetSkeletonPos( XN_SKEL_LEFT_HIP		);
		JointsReal[10] = GetSkeletonPos( XN_SKEL_LEFT_KNEE		);
		JointsReal[11] = GetSkeletonPos( XN_SKEL_LEFT_FOOT		);
		JointsReal[12] = GetSkeletonPos( XN_SKEL_RIGHT_HIP		);
		JointsReal[13] = GetSkeletonPos( XN_SKEL_RIGHT_KNEE		);
		JointsReal[14] = GetSkeletonPos( XN_SKEL_RIGHT_FOOT		);

		// convert form real world to projective
		m_OpenNI.GetDepthGenerator().ConvertRealWorldToProjective( 15, JointsReal, m_aJoints );
        //qDebug("Left hand: (%f, %f)", JointsReal[8].X, JointsReal[8].Y);
        //captureAction(JointsReal[8]);
        pos = JointsReal[8];
	}

public:
	COpenNI&	m_OpenNI;
	XnUserID	m_UserID;
	XnPoint3D	m_aJoints[15];
    //static XnPoint3D m_lastHandJoint;
    int			m_aConnection[15][2];

private:
	QRectF boundingRect() const
	{
		QRectF qRect( m_aJoints[0].X, m_aJoints[0].Y, 0, 0 );
		for( unsigned int i = 1; i < 15; ++ i )
		{
			if( m_aJoints[i].X < qRect.left() )
				qRect.setLeft( m_aJoints[i].X );
			if( m_aJoints[i].X > qRect.right() )
				qRect.setRight( m_aJoints[i].X );

			if( m_aJoints[i].Y < qRect.top() )
				qRect.setTop( m_aJoints[i].Y );
			if( m_aJoints[i].Y > qRect.bottom() )
				qRect.setBottom( m_aJoints[i].Y );
		}
		return qRect;
	}

	void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget )
	{
		// set pen for drawing
		QPen pen( QColor::fromRgb( 0, 0, 255 ) );
		pen.setWidth( 3 );
		painter->setPen( pen );
		
		// draw lines
		for( unsigned int i = 0; i < 15; ++ i )
		{
			XnPoint3D	&p1 = m_aJoints[ m_aConnection[i][0] ],
						&p2 = m_aJoints[ m_aConnection[i][1] ];

			painter->drawLine( p1.X, p1.Y, p2.X, p2.Y );
		}
		
		// draw joints
		for( unsigned int i = 0; i < 15; ++ i )
			painter->drawEllipse( QPointF( m_aJoints[i].X, m_aJoints[i].Y ), 5, 5 );
	}

	XnPoint3D GetSkeletonPos( XnSkeletonJoint eJointName )
	{
		// get position
		XnSkeletonJointPosition mPos;
		m_OpenNI.GetUserGenerator().GetSkeletonCap().GetSkeletonJointPosition( m_UserID, eJointName, mPos );

		// convert to XnPoint3D
		return xnCreatePoint3D( mPos.position.X, mPos.position.Y, mPos.position.Z );
	}
};

/* Timer to update image in scene from OpenNI */
class CKinectReader: public QObject
{
public:
	/* Constructor */
	CKinectReader( COpenNI& rOpenNI, QGraphicsScene& rScene )
		: m_OpenNI( rOpenNI ), m_Scene( rScene )
	{}

	/* Destructor */
	~CKinectReader()
	{
		m_Scene.removeItem( m_pItemImage );
		m_Scene.removeItem( m_pItemDepth );
		delete [] m_pDepthARGB;
	}

	/* Start to update Qt Scene from OpenNI device */
    bool Start( int iInterval = 33 )
    //bool Start( int iInterval = 1000 )
	{
		m_OpenNI.Start();

		// add an empty Image to scene
		m_pItemImage = m_Scene.addPixmap( QPixmap() );
		m_pItemImage->setZValue( 1 );

		// add an empty Depth to scene
		m_pItemDepth = m_Scene.addPixmap( QPixmap() );
		m_pItemDepth->setZValue( 2 );

		// update first to get the depth map size
		m_OpenNI.UpdateData();
		m_pDepthARGB = new uchar[4*m_OpenNI.m_DepthMD.XRes()*m_OpenNI.m_DepthMD.YRes()];

		startTimer( iInterval );
		return true;
	}

private:
	COpenNI&				m_OpenNI;
	QGraphicsScene&			m_Scene;
	QGraphicsPixmapItem*	m_pItemDepth;
	QGraphicsPixmapItem*	m_pItemImage;
	uchar*					m_pDepthARGB;
	vector<CSkelItem*>		m_vSkeleton;
    XnPoint3D m_lastHandJoint;
    enum {D = 20};

private:
    void captureAction(XnPoint3D &pos)
    {
        double diffX = pos.X - m_lastHandJoint.X;
        double diffY = pos.Y - m_lastHandJoint.Y;

        m_lastHandJoint.X = pos.X;
        m_lastHandJoint.Y = pos.Y;

        if(diffX > D)
        {
            qDebug("(%f,%f) - (%f,%f)", pos.X, pos.Y,
                   m_lastHandJoint.X, m_lastHandJoint.Y);
            cout << "Right: " << diffX << endl;
            return;
        }
        else if(diffX < -D)
        {
            cout << "Left" << endl;
            return;
        }
        else if(diffY > D)
        {
            cout << "Up" << endl;
            return;
        }
        else if(diffY < -D)
        {
            cout << "Down" << endl;
            return;
        }


    }

	void timerEvent( QTimerEvent *event )
	{
        QApplication::processEvents();
		// Read OpenNI data
		m_OpenNI.UpdateData();

		// Read Image
		{
			// convert to RGBA format
			const XnDepthPixel*  pDepth = m_OpenNI.m_DepthMD.Data();
			unsigned int iSize=m_OpenNI.m_DepthMD.XRes()*m_OpenNI.m_DepthMD.YRes();

			// fin the max value
			XnDepthPixel tMax = *pDepth;
			for( unsigned int i = 1; i < iSize; ++ i )
			{
				if( pDepth[i] > tMax )
					tMax = pDepth[i];
			}

			// redistribute data to 0-255
			int idx = 0;
			for( unsigned int i = 1; i < iSize; ++ i )
			{
				if( (*pDepth) != 0 )
				{
					m_pDepthARGB[ idx++ ] = 0;									// Blue
					m_pDepthARGB[ idx++ ] = 255 * ( tMax - *pDepth ) / tMax;		// Green
					m_pDepthARGB[ idx++ ] = 255 * *pDepth / tMax;				// Red
					m_pDepthARGB[ idx++ ] = 255 * ( tMax - *pDepth ) / tMax;		// Alpha
				}
				else
				{
					m_pDepthARGB[ idx++ ] = 0;
					m_pDepthARGB[ idx++ ] = 0;
					m_pDepthARGB[ idx++ ] = 0;
					m_pDepthARGB[ idx++ ] = 0;
				}
				++pDepth;
			}

			// Update Depth data
			m_pItemDepth->setPixmap( QPixmap::fromImage( QImage( m_pDepthARGB, m_OpenNI.m_DepthMD.XRes(), m_OpenNI.m_DepthMD.YRes(), QImage::Format_ARGB32 ) ) );

			// Update Image data
			m_pItemImage->setPixmap( QPixmap::fromImage( QImage( m_OpenNI.m_ImageMD.Data(), m_OpenNI.m_ImageMD.XRes(), m_OpenNI.m_ImageMD.YRes(), QImage::Format_RGB888 ) ) );
		}

		// Read Skeleton
		xn::UserGenerator& rUser = m_OpenNI.GetUserGenerator();
		XnUInt16 nUsers = rUser.GetNumberOfUsers();
		if( nUsers > 0 )
		{
			// get user id
			XnUserID* aUserID = new XnUserID[nUsers];
			rUser.GetUsers( aUserID, nUsers );

			// get skeleton for each user
			unsigned int counter = 0;
			xn::SkeletonCapability& rSC = rUser.GetSkeletonCap();
			for( int i = 0; i < nUsers; ++i )
			{
				// if is tracking skeleton
				if( rSC.IsTracking( aUserID[i] ) )
				{
					++counter;
					if( counter > m_vSkeleton.size() )
					{
						// create new skeleton item
						CSkelItem* pSkeleton = new CSkelItem( aUserID[i], m_OpenNI );
						m_Scene.addItem( pSkeleton );
						m_vSkeleton.push_back( pSkeleton );
						pSkeleton->setZValue( 10 );
					}
					else
						m_vSkeleton[ counter-1 ]->m_UserID = aUserID[i];

					// update skeleton item data
                    XnPoint3D pos;
                    m_vSkeleton[ counter-1 ]->UpdateSkeleton(pos);
                    captureAction(pos);
					m_vSkeleton[ counter-1 ]->setVisible( true );
				}
			}
			// hide un-used skeleton items
			for( unsigned int i = counter; i < m_vSkeleton.size(); ++ i )
				m_vSkeleton[i]->setVisible( false );

			// release user id area
			delete [] aUserID;
		}
	}
};

/* Main function */
int main( int argc, char** argv )
{
	// initial OpenNI
	COpenNI mOpenNI;
	bool bStatus = true;
	if( !mOpenNI.Initial() )
		return 1;

	// Qt Application
	QApplication App( argc, argv );
	QGraphicsScene  qScene;

	// Qt View
	QGraphicsView qView( &qScene );
	qView.resize( 650, 540 );
	qView.show();

	// Timer to update image
	CKinectReader KReader( mOpenNI, qScene );

	// start!
	KReader.Start();
	return App.exec();
}
