﻿#include "DPS.h"
#include <vector>
#include <iostream>
#include <string> 
#include "BulletCollision/Gimpact/btGImpactShape.h"
#include "BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h"
#include "Mesh/BunnyMesh.h"
#include "GUI.h"
#include <string>
#include <sstream>
#include <Terrain/OgreTerrain.h>
#include <Terrain/OgreTerrainGroup.h>

DPS::DPS(void)
{
	initPhysics();
	rayNode = nullptr;
	runClothDome_1 = false;
	runClothDome_2 = false;
	runClothDome_3 = false;
	runClothDome_4 = false;
	runClothDome_5 = false;
	runClothDome_6 = false;
	runClothDome_7 = false;

	runDeformDome_1 = false;
	runDeformDome_2 = false;
	runDeformDome_3 = false;
	runDeformDome_4 = false;
	runDeformDome_5 = false;
	runDeformDome_6 = false;
	runDeformDome_7 = false;

	runPlaygroud_1 = false;
	runPlaygroud_2 = false;
	runPlaygroud_3 = false;
	runPlaygroud_4 = false;
	runPlaygroud_5 = false;

	playgroundCount = 0;
	maxProxies = 32766;
	hit_rel_pos = btVector3(0,0,0);
	shot_imp = btVector3(0,0,0);
	timeLine = 0;

	
	mTerrainGlobals = 0;
	mTerrainGroup = 0;
	mTerrainsImported = false;
	mInfoLabel =  0;

	mVideoFrame   = NULL;
	mVideoCapture = NULL;

}

DPS::~DPS(void)
{
	deletePhysicsShapes();
	exitPhysics();
	cvReleaseCapture(&mVideoCapture);
	cvDestroyWindow("video");
}

void DPS::initPhysics(void)
{
	//Bullet initialisation.
	m_dispatcher=0;
	///register some softbody collision algorithms on top of the default btDefaultCollisionConfiguration
	m_collisionConfiguration = new btSoftBodyRigidBodyCollisionConfiguration();
	m_dispatcher = new	btCollisionDispatcher(m_collisionConfiguration);
	m_softBodyWorldInfo.m_dispatcher = m_dispatcher;
	btVector3 worldAabbMin(-1000,-1000,-1000);
	btVector3 worldAabbMax(1000,1000,1000);
	m_broadphase = new btAxisSweep3(worldAabbMin,worldAabbMax,maxProxies);
	btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver();
	m_solver = solver;
	btSoftBodySolver* softBodySolver = 0;
	Globals::phyWorld = new btSoftRigidDynamicsWorld(m_dispatcher,m_broadphase,m_solver,m_collisionConfiguration,softBodySolver);
	btGImpactCollisionAlgorithm::registerAlgorithm(m_dispatcher);

	//phyWorld->setInternalTickCallback(pickingPreTickCallback,this,true);
	Globals::phyWorld->getDispatchInfo().m_enableSPU = true;
	Globals::phyWorld->setGravity(btVector3(0,-10,0));
	m_softBodyWorldInfo.m_gravity.setValue(0,-10,0);
	m_softBodyWorldInfo.m_sparsesdf.Initialize();

	dt = evt.timeSinceLastFrame;
}


void DPS::exitPhysics(void)
{
	delete Globals::phyWorld;
	delete m_solver;
	delete m_broadphase;
	delete m_dispatcher;
	delete m_collisionConfiguration;
	delete Globals::dbgdraw;
}


void DPS::deletePhysicsShapes(void)
{
	int i;
	for (i=Globals::phyWorld->getNumCollisionObjects()-1 ; i>=0 ; i--)
	{
		btCollisionObject* obj = Globals::phyWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState())
		{
			delete body->getMotionState();
		}
		Globals::phyWorld->removeCollisionObject(obj);
		delete obj;
	}

	//delete collision shapes
	for (int j=0;j<m_collisionShapes.size();j++)
	{
		btCollisionShape* shape = m_collisionShapes[j];
		m_collisionShapes[j] = 0;
		delete shape;
	}
}


void DPS::createScene(void)
{
	mGUI->createGUI(1);

	// Basic Ogre stuff.
	//mSceneMgr->setAmbientLight(Ogre::ColourValue(1.0f, 1.0f, 1.0f));
// 	mCamera->setPosition(Ogre::Vector3(0,5,20));
// 	mCamera->lookAt(Ogre::Vector3(0,5,-10));
// 	mCamera->setNearClipDistance(0.05f);
	//LogManager::getSingleton().setLogDetail(LL_BOREME);




	dpsHelper = std::make_shared<DPSHelper>(Globals::phyWorld, mCamera, mSceneMgr);
	dpsSoftbodyHelper = std::make_shared<DPSSoftBodyHelper>(Globals::phyWorld, mCamera, mSceneMgr, m_collisionShapes);


// 	dpsHelper->createWorld();
	//Create Ogre stuff.
	Ogre::Vector3 lightdir(0.55, -0.3, 0.75);
	lightdir.normalise();
	Ogre::Light* light = mSceneMgr->createLight("tstLight");
	light->setType(Ogre::Light::LT_DIRECTIONAL);
	light->setDirection(lightdir);
	light->setDiffuseColour(Ogre::ColourValue::White);
	//light->setSpecularColour(Ogre::ColourValue(0.4, 0.4, 0.4));

	mSceneMgr->setAmbientLight(Ogre::ColourValue(0.2, 0.2, 0.2));
	mTerrainGlobals = OGRE_NEW Ogre::TerrainGlobalOptions();

	mTerrainGroup = OGRE_NEW Ogre::TerrainGroup(mSceneMgr, Ogre::Terrain::ALIGN_X_Z, 513, 12000.0f);
	mTerrainGroup->setFilenameConvention(Ogre::String("BasicTutorial3Terrain"), Ogre::String("dat"));
	mTerrainGroup->setOrigin(Ogre::Vector3::ZERO);

	configureTerrainDefaults(light);

	for (long x = 0; x <= 0; ++x)
		for (long y = 0; y <= 0; ++y)
			defineTerrain(x, y);

	// sync load since we want everything in place when we start
	mTerrainGroup->loadAllTerrains(true);

	if (mTerrainsImported)
	{
		Ogre::TerrainGroup::TerrainIterator ti = mTerrainGroup->getTerrainIterator();
		while(ti.hasMoreElements())
		{
			Ogre::Terrain* t = ti.getNext()->instance;
			initBlendMaps(t);
		}
	}

	mTerrainGroup->freeTemporaryResources();

// 	Ogre::Entity* entGround = mSceneMgr->createEntity("GroundEntity", "ground");
// 
// 	entGround ->setMaterialName("Examples/Wood");
// 	entGround ->setCastShadows(false);

	btCollisionShape* shape = new btBoxShape (btVector3(5000,1,5000));
	btDefaultMotionState* motionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1), btVector3(0,-1,0)));
	btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(
		0,                  // mass
		motionState,        // initial position
		shape,              // collision shape of body
		btVector3(0,0,0)    // local inertia
		);
	btRigidBody *GroundBody = new btRigidBody(rigidBodyCI);
	Globals::phyWorld->addRigidBody(GroundBody);

// 	mSceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(entGround);

	//mSceneMgr->setSkyBox(true, "Examples/MorningSkyBox");
	mSceneMgr->setSkyBox(true, "Examples/SpaceSkyBox", 5000);

	//dpsHelper->createDirectionLight("mainLight",Ogre::Vector3(60,180,100),Ogre::Vector3(-60,-80,-100));
	//dpsHelper->createDirectionLight("mainLight1",Ogre::Vector3(0,50,-3),Ogre::Vector3(0,0,0));

	// Debug drawing
	Globals::dbgdraw = new BtOgre::DebugDrawer(mSceneMgr->getRootSceneNode(), Globals::phyWorld);
	Globals::phyWorld->setDebugDrawer(Globals::dbgdraw);

// 	leapMotionInit();
	setMiniCamPosition(Ogre::Vector3(10,10,10));


	// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$  GAME CA $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
	GameCALoadModel();

	// Set the camera to look at our handiwork
	mCamera->setPosition(Ogre::Vector3(1645.0f,94.0f,2520.0f));
	mCamera->setNearClipDistance(0.05f);


	//** Setup OpenCV for reading an video
	mVideoCapture = cvCreateFileCapture("..\\media\\test.avi");
	createVideoTexture();
	
	//** Implementing a video screen
	mVideoScreen = new Ogre::Rectangle2D(true);
	mVideoScreen->setCorners(-0.5f, 1.0f, 0.5f, -1.0f);
	mVideoScreen->setBoundingBox(Ogre::AxisAlignedBox(-100000.0f * Ogre::Vector3::UNIT_SCALE, 100000.0f * Ogre::Vector3::UNIT_SCALE));
	mVideoScreen->setMaterial( material->getName() );
    // Attach it to a SceneNode
	Ogre::SceneNode* videoScreenNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("VideoScreenNode");
	videoScreenNode->attachObject(mVideoScreen);

	mCameraMan->setStyle(OgreBites::CS_ORBIT);


}



void DPS::GameCALoadModel(void)
{
	Ogre::Vector3 pos = Ogre::Vector3(1500,200,2000);
	Ogre::Quaternion rot = Ogre::Quaternion::IDENTITY;

	//$$$$$$$$$$$$$$$$$$$$$$$$$$  RZR  $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

	Ogre::Entity* RZR_001 = mSceneMgr->createEntity("RZR-001", "RZR-002.mesh");
	Ogre::SceneNode* entRZR_001 = mSceneMgr->getRootSceneNode()->createChildSceneNode(Ogre::Vector3(1600,200,1420), rot);
	entRZR_001->attachObject(RZR_001);

	//$$$$$$$$$$$$$$$$$$$$$$$$$$  Robot  $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
	// Create the entity
	mEntity = mSceneMgr->createEntity("Robot", "robot.mesh");

	// Create the scene node
	mNode = mSceneMgr->getRootSceneNode()->
		createChildSceneNode("RobotNode", Ogre::Vector3(604.0f, 16.0f, 4245.0f),rot);
	mNode->attachObject(mEntity);

	// Create the walking list
	mWalkList.push_back(Ogre::Vector3(1235.0f,  16.0f, 3334.0f ));
	mWalkList.push_back(Ogre::Vector3(1526.0f,  16.0f, 3328.0f));

	// Create objects so we can see movement
	Ogre::Entity *ent;
	Ogre::SceneNode *node;

	ent = mSceneMgr->createEntity("Knot1", "knot.mesh");
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode("Knot1Node",
		Ogre::Vector3(0.0f, -10.0f,  25.0f));
	node->attachObject(ent);
	node->setScale(0.1f, 0.1f, 0.1f);

	ent = mSceneMgr->createEntity("Knot2", "knot.mesh");
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode("Knot2Node",
		Ogre::Vector3(550.0f, -10.0f,  50.0f));
	node->attachObject(ent);
	node->setScale(0.1f, 0.1f, 0.1f);

	ent = mSceneMgr->createEntity("Knot3", "knot.mesh");
	node = mSceneMgr->getRootSceneNode()->createChildSceneNode("Knot3Node",
		Ogre::Vector3(-100.0f, -10.0f,-200.0f));
	node->attachObject(ent);
	node->setScale(0.1f, 0.1f, 0.1f);

	//$$$$$$$$$$$$$$$$$$$$$$$$$$  tudorhouse  $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
	Ogre::Vector3 mTerrainPos = Ogre::Vector3(0,0,0);

	Ogre::Entity* e1= mSceneMgr->createEntity("tudorhouse.mesh");
	Ogre::Vector3 entPos1 = Ogre::Vector3(mTerrainPos.x + 2043, 65, mTerrainPos.z + 1715);
	Ogre::SceneNode* sn1 = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos1, rot);
	sn1->setScale(Vector3(0.12, 0.12, 0.12));
	sn1->attachObject(e1);

	Ogre::Entity* e2= mSceneMgr->createEntity("tudorhouse.mesh");
	Ogre::Vector3 entPos2 = Ogre::Vector3(mTerrainPos.x + 1850, 65, mTerrainPos.z + 1478);
	Ogre::SceneNode* sn2 = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos2, rot);
	sn2->setScale(Vector3(0.12, 0.12, 0.12));
	sn2->attachObject(e2);

	Ogre::Entity* e3= mSceneMgr->createEntity("tudorhouse.mesh");
	Ogre::Vector3 entPos3 = Ogre::Vector3(mTerrainPos.x + 1970, 85, mTerrainPos.z + 2180);
	Ogre::SceneNode* sn3 = mSceneMgr->getRootSceneNode()->createChildSceneNode(entPos3, rot);
	sn3->setScale(Vector3(0.12, 0.12, 0.12));
	sn3->attachObject(e3);

	//$$$$$$$$$$$$$$$$$$$$$$$$$$  Sinbad  $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
	mSinbadNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(Ogre::Vector3(223,452,1976));
	mBodyEnt = mSceneMgr->createEntity("SinbadBody", "Sinbad.mesh");
	mSinbadNode->attachObject(mBodyEnt);
	mSinbadNode->yaw(Ogre::Degree(180));

	// create swords and attach to sheath
	LogManager::getSingleton().logMessage("Creating swords");
	mSword1 = mSceneMgr->createEntity("SinbadSword1", "Sword.mesh");
	mSword2 = mSceneMgr->createEntity("SinbadSword2", "Sword.mesh");
	mBodyEnt->attachObjectToBone("Sheath.L", mSword1);
	mBodyEnt->attachObjectToBone("Sheath.R", mSword2);

	LogManager::getSingleton().logMessage("Creating the chains");
	// create a couple of ribbon trails for the swords, just for fun
	NameValuePairList params;
	params["numberOfChains"] = "2";
	params["maxElements"] = "80";
	mSwordTrail = (RibbonTrail*)mSceneMgr->createMovableObject("RibbonTrail", &params);
	mSwordTrail->setMaterialName("Examples/LightRibbonTrail");
	mSwordTrail->setTrailLength(20);
	mSwordTrail->setVisible(false);
	mSceneMgr->getRootSceneNode()->attachObject(mSwordTrail);


	for (int i = 0; i < 2; i++)
	{
		mSwordTrail->setInitialColour(i, 1, 0.8, 0);
		mSwordTrail->setColourChange(i, 0.75, 1.25, 1.25, 1.25);
		mSwordTrail->setWidthChange(i, 1);
		mSwordTrail->setInitialWidth(i, 0.5);
	}
}


bool DPS::nextLocation(void){

	if (mWalkList.empty()) return false;
	mDestination = mWalkList.front();  // this gets the front of the deque
	mWalkList.pop_front();             // this removes the front of the deque
	mDirection = mDestination - mNode->getPosition();
	mDistance = mDirection.normalise();
	return true;
}

void DPS::createFrameListener(void){
	BaseApplication::createFrameListener();

	// Set idle animation
	mAnimationState = mEntity->getAnimationState("Idle");
	mAnimationState->setLoop(true);
	mAnimationState->setEnabled(true);

	// Set default values for variables
	mWalkSpeed = 35.0f;
	mDirection = Ogre::Vector3::ZERO;
}

Ogre::Vector3 DPS::ogreLerp (Ogre::Vector3 srcLocation, Ogre::Vector3 destLocation, Ogre::Real Time)
{
	Ogre::Vector3 mDest; 
	mDest.x = srcLocation.x + (destLocation.x - srcLocation.x) * Time;
	mDest.y = srcLocation.y + (destLocation.y - srcLocation.y) * Time;
	mDest.z = srcLocation.z + (destLocation.z - srcLocation.z) * Time;
	return mDest;
}

void DPS::GameCA(int timeLine, Ogre::Real Time)
{
	//houses
	if(timeLine > 0 && timeLine < 300)
	{
		camPos = mCamera->getPosition();
		targetPos = Ogre::Vector3(1399.0f,90.0f,1311.0f);
		camLerpPos = ogreLerp(camPos, targetPos, Time);
		mCamera->setPosition(camLerpPos);
		mCamera->lookAt(Ogre::Vector3(2000.0f,50.0f,1715.0f));
	}

	if (timeLine == 300)
	{
		mCamera->setPosition(Ogre::Vector3(264,452,2268));
		mCamera->lookAt(mSinbadNode->getPosition());
	}

	//hilltop
	if(timeLine > 300 && timeLine < 420 )
	{
		camPos = mCamera->getPosition();
		targetPos = Ogre::Vector3(237,452,2037);
		camLerpPos = ogreLerp(camPos, targetPos, Time);
		mCamera->setPosition(camLerpPos);
		mCamera->lookAt(mSinbadNode->getPosition());
	}
	if (timeLine == 420)
	{
		mCamera->setPosition(Ogre::Vector3(237,452,2037));
		mCamera->lookAt(mSinbadNode->getPosition());
	}

	if(timeLine > 420 && timeLine < 540 )
	{
		camPos = mCamera->getPosition();
		targetPos = Ogre::Vector3(314,452,1984);
		camLerpPos = ogreLerp(camPos, targetPos, Time);
		mCamera->setPosition(camLerpPos);
		mCamera->lookAt(mSinbadNode->getPosition());
	}
	if (timeLine == 540)
	{
		mCamera->setPosition(Ogre::Vector3(314,452,1984));
		mCamera->lookAt(mSinbadNode->getPosition());
	}

	if(timeLine > 540 && timeLine < 660 )
	{
		camPos = mCamera->getPosition();
		targetPos = Ogre::Vector3(237,452,1890);
		camLerpPos = ogreLerp(camPos, targetPos, Time);
		mCamera->setPosition(camLerpPos);
		mCamera->lookAt(mSinbadNode->getPosition());
	}

	if (timeLine == 660)
	{
		mCamera->setPosition(Ogre::Vector3(237,452,1890));
		mCamera->lookAt(mSinbadNode->getPosition());
	}
	if(timeLine > 660 && timeLine < 840 )
	{
		camPos = mCamera->getPosition();
		targetPos = Ogre::Vector3(223,455,1966);
		camLerpPos = ogreLerp(camPos, targetPos, Time);
		mCamera->setPosition(camLerpPos);
		mCamera->lookAt(mSinbadNode->getPosition());
	}

}


bool DPS::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
	if (mDirection == Ogre::Vector3::ZERO) {
		if (nextLocation()) {
			// Set walking animation
			mAnimationState = mEntity->getAnimationState("Walk");
			mAnimationState->setLoop(true);
			mAnimationState->setEnabled(true);				
		}//if
	}else{
		Ogre::Real move = mWalkSpeed * evt.timeSinceLastFrame;
		mDistance -= move;
		if (mDistance <= 0.0f){                 
			mNode->setPosition(mDestination);
			mDirection = Ogre::Vector3::ZERO;				
			// Set animation based on if the robot has another point to walk to. 
			if (!nextLocation()){
				// Set Idle animation                     
				mAnimationState = mEntity->getAnimationState("Idle");
				mAnimationState->setLoop(true);
				mAnimationState->setEnabled(true);
			}else{
				// Rotation Code will go here later
				Ogre::Vector3 src = mNode->getOrientation() * Ogre::Vector3::UNIT_X;
				if ((1.0f + src.dotProduct(mDirection)) < 0.0001f) {
					mNode->yaw(Ogre::Degree(180));						
				}else{
					Ogre::Quaternion quat = src.getRotationTo(mDirection);
					mNode->rotate(quat);
				} // else
			}//else
		}else{
			mNode->translate(mDirection * move);
		} // else
	} // if


	if(!mVideoFrame)
	{
		return false;
	}
	else
	{
		updateVideoTexture();
	}


	mAnimationState->addTime(evt.timeSinceLastFrame);

	times = evt.timeSinceLastFrame;

	++timeLine;

	GameCA(timeLine, times);

	if(mGUI->Simulation_Stop)
	{
		dt = 0;
	}
	else
	{	
		if(mGUI->Command_Enable_Slow)
		{
			dt = 0.0002;
		}
		else
		{
			dt = evt.timeSinceLastFrame + mGUI->demoDt;
		}
	}

	GUIeventHandler();
	demoController();

	if(rayNode)
	{
		setMiniCamPosition(rayNode->getPosition());
	}

	//Update Bullet world
	Globals::phyWorld->stepSimulation(dt,0); 
	Globals::phyWorld->debugDrawWorld();

	//Shows debug if F3 key down.
	//Globals::dbgdraw->setDebugMode(mKeyboard->isKeyDown(OIS::KC_F3));
	Globals::dbgdraw->setDebugMode(mGUI->Command_Bullet_Debug_Mode);
	Globals::dbgdraw->step();

// 	Ogre::Vector3 camPos = mCamera->getDerivedPosition();
// 
// 	if (camPos.y<0.5)
// 	{
// 		mCamera->setPosition(camPos.x, 0.5, camPos.z);
// 	}

// 	if(leapMotionRunning)
// 	{
// 		leapMotionUpdate();
// 	}

	return BaseApplication::frameRenderingQueued(evt);
}


void DPS::createVideoTexture(void)
{
	mVideoFrame = cvQueryFrame(mVideoCapture);
	if(!mVideoFrame)
	{
		return;
	}

	// Implementing a video texture
	texture = Ogre::TextureManager::getSingleton().createManual(
		"VideoTexture",      // name
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D,   // type
		mVideoFrame->width,  // width
		mVideoFrame->height, // height
		0,                   // number of mipmaps
		Ogre::PF_B8G8R8A8,   // pixel format
		Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE // usage, for textures updated very often
		);
	// Get the pixel buffer
	pixelBuffer = texture->getBuffer();
	// Lock the pixel buffer and get a pixel box
	unsigned char* buffer = static_cast<unsigned char*>(
		pixelBuffer->lock(0, mVideoFrame->width*mVideoFrame->height*4, Ogre::HardwareBuffer::HBL_DISCARD) );
	for(int y = 0; y < mVideoFrame->height; ++y)
	{
		for(int x = 0; x < mVideoFrame->width; ++x)
		{
			buffer[ ((y*mVideoFrame->width)+x)*4 + 0 ] = mVideoFrame->imageData[ ((y*mVideoFrame->width)+x)*3 + 0 ]; // B
			buffer[ ((y*mVideoFrame->width)+x)*4 + 1 ] = mVideoFrame->imageData[ ((y*mVideoFrame->width)+x)*3 + 1 ]; // G
			buffer[ ((y*mVideoFrame->width)+x)*4 + 2 ] = mVideoFrame->imageData[ ((y*mVideoFrame->width)+x)*3 + 2 ]; // R
			buffer[ ((y*mVideoFrame->width)+x)*4 + 3 ] = 255;                                                        // A
		}
	}
	// Unlock the pixel buffer
	pixelBuffer->unlock();

	// Create a materail using the texture
	material = Ogre::MaterialManager::getSingleton().create(
		"VideoTextureMaterial", // name
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	material->getTechnique(0)->getPass(0)->createTextureUnitState("VideoTexture");
	material->getTechnique(0)->getPass(0)->setDepthCheckEnabled(false);
	material->getTechnique(0)->getPass(0)->setDepthWriteEnabled(false);
	material->getTechnique(0)->getPass(0)->setLightingEnabled(false);
}

//-------------------------------------------------------------------------------------
void DPS::updateVideoTexture(void)
{
	mVideoFrame = cvQueryFrame(mVideoCapture);
	if(!mVideoFrame)
	{
		return;
	}

	// Lock the pixel buffer
	unsigned char* buffer = static_cast<unsigned char*>(
		pixelBuffer->lock(0, mVideoFrame->width*mVideoFrame->height*4, Ogre::HardwareBuffer::HBL_DISCARD) );
	for(int y = 0; y < mVideoFrame->height; ++y)
	{
		for(int x = 0; x < mVideoFrame->width; ++x)
		{
			buffer[ ((y*mVideoFrame->width)+x)*4 + 0 ] = mVideoFrame->imageData[ ((y*mVideoFrame->width)+x)*3 + 0 ]; // B
			buffer[ ((y*mVideoFrame->width)+x)*4 + 1 ] = mVideoFrame->imageData[ ((y*mVideoFrame->width)+x)*3 + 1 ]; // G
			buffer[ ((y*mVideoFrame->width)+x)*4 + 2 ] = mVideoFrame->imageData[ ((y*mVideoFrame->width)+x)*3 + 2 ]; // R
			buffer[ ((y*mVideoFrame->width)+x)*4 + 3 ] = 255;                                                        // A
		}
	}
	// Unlock the pixel buffer
	pixelBuffer->unlock();
}


// ------------------------------- Seek -----------------------------------
// 
//  Given a target, this behavior returns a steering force which will
//  direct the agent towards the target
// ------------------------------------------------------------------------
// void DPS::seek(Ogre::Vector3 TargetPosition, Ogre::Quaternion TargetOrientation)
// {
// 	Ogre::Vector3 mAgentPosition      = mGlobalResource->LocalPlayerObjectScene->getInventoryPosition();
// 	Ogre::Quaternion mAgentOrientation   = mGlobalResource->LocalPlayerObjectScene->getInventoryOrientation();
// 	Ogre::Vector3 mVectorToTarget      = TargetPosition - mAgentPosition; // A-B = B->A
// 	mAgentPosition.normalise();
// 	mAgentOrientation.normalise();
// 
// 	Ogre::Vector3 mAgentHeading      = mAgentOrientation * mAgentPosition;
// 	Ogre::Vector3 mTargetHeading      = TargetOrientation * TargetPosition;
// 	mAgentHeading.normalise();
// 	mTargetHeading.normalise();
// 
// 	// Orientation control - Ogre::Vector3::UNIT_Y is common up vector.
// 	Ogre::Vector3 mAgentVO      = mAgentOrientation.Inverse() * Ogre::Vector3::UNIT_Y;
// 	Ogre::Vector3 mTargetVO      = TargetOrientation * Ogre::Vector3::UNIT_Y;
// 
// 	// Compute new torque scalar (-1.0 to 1.0) based on heading vector to target.
// 	Ogre::Vector3 mSteeringForce = mAgentOrientation.Inverse() * mVectorToTarget;
// 	mSteeringForce.normalise();
// 
// 	float mYaw      = mSteeringForce.x;
// 	float mPitch   = mSteeringForce.y;
// 	float mRoll      = mTargetVO.getRotationTo( mAgentVO ).getRoll().valueRadians();
// 
// 	clsSystemControls::getSingleton().setPitchControl( mPitch );
// 	clsSystemControls::getSingleton().setYawControl( mYaw );
// 	clsSystemControls::getSingleton().setRollControl( mRoll );
// 
// } // Seek
// 
// 
// ----------------------------- Flee -------------------------------------
// 
//  Does the opposite of Seek
// ------------------------------------------------------------------------
// void DPS::flee(Ogre::Vector3 TargetPosition, Ogre::Quaternion TargetOrientation)
// {
// 	Ogre::Vector3 mAgentPosition      = mGlobalResource->LocalPlayerObjectScene->getInventoryPosition();
// 	Ogre::Quaternion mAgentOrientation   = mGlobalResource->LocalPlayerObjectScene->getInventoryOrientation();
// 	Ogre::Vector3 mVectorToTarget      = TargetPosition - mAgentPosition; // A-B = B->A
// 	mAgentPosition.normalise();
// 	mAgentOrientation.normalise();
// 
// 	Ogre::Vector3 mAgentHeading      = mAgentOrientation * mAgentPosition;
// 	Ogre::Vector3 mTargetHeading      = TargetOrientation * TargetPosition;
// 	mAgentHeading.normalise();
// 	mTargetHeading.normalise();
// 
// 	// Orientation control - Ogre::Vector3::UNIT_Y is common up vector.
// 	Ogre::Vector3 mAgentVO      = mAgentOrientation.Inverse() * Ogre::Vector3::UNIT_Y;
// 	Ogre::Vector3 mTargetVO      = TargetOrientation * Ogre::Vector3::UNIT_Y;
// 
// 	// Compute new torque scalar (-1.0 to 1.0) based on heading vector to target.
// 	Ogre::Vector3 mSteeringForce = mAgentOrientation * mVectorToTarget;
// 	mSteeringForce.normalise();
// 
// 	float mYaw      = mSteeringForce.x;
// 	float mPitch   = mSteeringForce.y;
// 	float mRoll      = mTargetVO.getRotationTo( mAgentVO ).getRoll().valueRadians();
// 
// 	clsSystemControls::getSingleton().setPitchControl( mPitch );
// 	clsSystemControls::getSingleton().setYawControl( mYaw );
// 	clsSystemControls::getSingleton().setRollControl( mRoll );
// 
// } // Flee

void DPS::configureTerrainDefaults(Ogre::Light* light)
{
	// Configure global
	mTerrainGlobals->setMaxPixelError(8);
	// testing composite map
	mTerrainGlobals->setCompositeMapDistance(3000);

	// Important to set these so that the terrain knows what to use for derived (non-realtime) data
	mTerrainGlobals->setLightMapDirection(light->getDerivedDirection());
	mTerrainGlobals->setCompositeMapAmbient(mSceneMgr->getAmbientLight());
	mTerrainGlobals->setCompositeMapDiffuse(light->getDiffuseColour());

	// Configure default import settings for if we use imported image
	Ogre::Terrain::ImportData& defaultimp = mTerrainGroup->getDefaultImportSettings();
	defaultimp.terrainSize = 513;
	defaultimp.worldSize = 12000.0f;
	defaultimp.inputScale = 600;
	defaultimp.minBatchSize = 33;
	defaultimp.maxBatchSize = 65;
	// textures
	defaultimp.layerList.resize(3);
	defaultimp.layerList[0].worldSize = 100;
	defaultimp.layerList[0].textureNames.push_back("dirt_grayrocky_diffusespecular.dds");
	defaultimp.layerList[0].textureNames.push_back("dirt_grayrocky_normalheight.dds");
	defaultimp.layerList[1].worldSize = 30;
	defaultimp.layerList[1].textureNames.push_back("grass_green-01_diffusespecular.dds");
	defaultimp.layerList[1].textureNames.push_back("grass_green-01_normalheight.dds");
	defaultimp.layerList[2].worldSize = 200;
	defaultimp.layerList[2].textureNames.push_back("growth_weirdfungus-03_diffusespecular.dds");
	defaultimp.layerList[2].textureNames.push_back("growth_weirdfungus-03_normalheight.dds");
}

void DPS::defineTerrain(long x, long y)
{
	Ogre::String filename = mTerrainGroup->generateFilename(x, y);
	if (Ogre::ResourceGroupManager::getSingleton().resourceExists(mTerrainGroup->getResourceGroup(), filename))
	{
		mTerrainGroup->defineTerrain(x, y);
	}
	else
	{
		Ogre::Image img;
		getTerrainImage(x % 2 != 0, y % 2 != 0, img);
		mTerrainGroup->defineTerrain(x, y, &img);
		mTerrainsImported = true;
	}
}

void DPS::initBlendMaps(Ogre::Terrain* terrain)
{
	Ogre::TerrainLayerBlendMap* blendMap0 = terrain->getLayerBlendMap(1);
	Ogre::TerrainLayerBlendMap* blendMap1 = terrain->getLayerBlendMap(2);
	Ogre::Real minHeight0 = 70;
	Ogre::Real fadeDist0 = 40;
	Ogre::Real minHeight1 = 70;
	Ogre::Real fadeDist1 = 15;
	float* pBlend0 = blendMap0->getBlendPointer();
	float* pBlend1 = blendMap1->getBlendPointer();
	for (Ogre::uint16 y = 0; y < terrain->getLayerBlendMapSize(); ++y)
	{
		for (Ogre::uint16 x = 0; x < terrain->getLayerBlendMapSize(); ++x)
		{
			Ogre::Real tx, ty;

			blendMap0->convertImageToTerrainSpace(x, y, &tx, &ty);
			Ogre::Real height = terrain->getHeightAtTerrainPosition(tx, ty);
			Ogre::Real val = (height - minHeight0) / fadeDist0;
			val = Ogre::Math::Clamp(val, (Ogre::Real)0, (Ogre::Real)1);
			*pBlend0++ = val;

			val = (height - minHeight1) / fadeDist1;
			val = Ogre::Math::Clamp(val, (Ogre::Real)0, (Ogre::Real)1);
			*pBlend1++ = val;
		}
	}
	blendMap0->dirty();
	blendMap1->dirty();
	blendMap0->update();
	blendMap1->update();
}

void DPS::getTerrainImage(bool flipX, bool flipY, Ogre::Image& img)
{
	img.load("terrain.png", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	if (flipX)
		img.flipAroundY();
	if (flipY)
		img.flipAroundX();
}


void DPS::clearScreen(void)
{
	mGUI->Command_Clear_Screen = false;

	deletePhysicsShapes();
	deleteOgreEntities();

	//stop demos loops
	runClothDome_1 = false;
	runClothDome_2 = false;
	runClothDome_3 = false;
	runClothDome_4 = false;
	runClothDome_5 = false;
	runClothDome_6 = false;
	runClothDome_7 = false;

 	runDeformDome_1 = false;
	runDeformDome_2 = false;
	runDeformDome_3 = false;
 	runDeformDome_4 = false;
 	runDeformDome_5 = false;
	runDeformDome_6 = false;
	runDeformDome_7 = false;

	runPlaygroud_1 = false;
	runPlaygroud_2 = false;
	runPlaygroud_3 = false;
	runPlaygroud_4 = false;
	runPlaygroud_5 = false;

	//enable re click for all demos simply delete all following code.
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Cloth_Demo_1")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Cloth_Demo_2")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Cloth_Demo_3")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Cloth_Demo_4")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Cloth_Demo_5")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Cloth_Demo_6")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Cloth_Demo_7")->setEnabled(true);

	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Deformable_Demo_1")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Deformable_Demo_2")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Deformable_Demo_3")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Deformable_Demo_4")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Deformable_Demo_5")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Deformable_Demo_6")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Deformable_Demo_7")->setEnabled(true);

	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Playgroud_1")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Playgroud_2")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Playgroud_3")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Playgroud_4")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Playgroud_5")->setEnabled(true);

	mGUI->Command_Disable_Mini_Camera = true;
	mGUI->Command_Enable_Mini_Camera = false;
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Enable_Mini_Camera")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Disable_Mini_Camera")->setEnabled(false);
	mGUI->miniCameraWindow->setVisible(false);


	solidScreen();
}


void DPS::GUIeventHandler(void)
{
	if(mGUI->Command_Clear_Screen)
	{
		clearScreen();
	}
	if(mGUI->Command_Enable_FPS)
	{
		mGUI->Command_Enable_FPS = false;
		mTrayMgr->showFrameStats(OgreBites::TL_BOTTOMLEFT);
	}
	if(mGUI->Command_Disable_FPS)
	{
		mGUI->Command_Disable_FPS = false;
		mTrayMgr->hideFrameStats();
	}
	if(mGUI->Command_Quit)
	{
		mGUI->Command_Quit = false;
		mShutDown = true;
	}
	if(mGUI->Command_ScreenShot)
	{
		mGUI->Command_ScreenShot = false;
		mWindow->writeContentsToTimestampedFile("screenshot", ".jpg");
	}
	if(mGUI->Command_Disable_Slow)
	{
		mGUI->Command_Disable_Slow = false;
		mGUI->Command_Enable_Slow = false;
	}
	if(mGUI->Command_Reset_Camera)
	{
		resetCamera();
		mGUI->Command_Reset_Camera = false;
	}
	if(mGUI->Command_Cloth_Demo_1)
	{
		mGUI->Command_Cloth_Demo_1 = false;

		//clean screen before create new demo
		clearScreen();

		resetCamera(Ogre::Vector3(0.0f,8.0f,25.0f));

		//create demo
		dpsSoftbodyHelper->createClothDemo_1();

		//run demo after ceate
		runClothDome_1 = true;
	}
	if(mGUI->Command_Cloth_Demo_2)
	{
		mGUI->Command_Cloth_Demo_2 = false;

		//clean screen before create new demo
		clearScreen();

		
		//create demo
		resetCamera(Ogre::Vector3(0.0f,12.0f,25.0f));
		dpsSoftbodyHelper->createClothDemo_2();
		for(int i=0 ; i<26; i+=5)
		{
			dpsHelper->createCube(Ogre::Vector3(0,40+i,0),Ogre::Vector3(2,2,2),1);
		}

		//run demo after ceate
		runClothDome_2 = true;
	}
	if(mGUI->Command_Cloth_Demo_3)
	{
		mGUI->Command_Cloth_Demo_3 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,6.0f,25.0f));
		dpsSoftbodyHelper->createClothDemo_3();
		dpsHelper->createFixedSphere(Ogre::Vector3(0,5,0),0);

		//run demo after ceate
		runClothDome_3 = true;
	}
	if(mGUI->Command_Cloth_Demo_4)
	{
		mGUI->Command_Cloth_Demo_4 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,12.0f,25.0f));
		dpsSoftbodyHelper->createClothDemo_4(dpsHelper->createCubeAndReturnBody(Ogre::Vector3(0,15,-7.5),Ogre::Vector3(10,1,3),5));

		//run demo after ceate
		runClothDome_4 = true;
	}
	if(mGUI->Command_Cloth_Demo_5)
	{
		mGUI->Command_Cloth_Demo_5 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,12.0f,25.0f));
		dpsSoftbodyHelper->createClothDemo_5();

		//run demo after ceate
		runClothDome_5 = true;
	}
	if(mGUI->Command_Cloth_Demo_6)
	{
		mGUI->Command_Cloth_Demo_6 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,30.0f,150.0f));
		dpsSoftbodyHelper->createClothDemo_6();

		//run demo after ceate
		runClothDome_6 = true;
	}
	if(mGUI->Command_Cloth_Demo_7)
	{
		mGUI->Command_Cloth_Demo_7 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,45.0f,40.0f));
		dpsSoftbodyHelper->createClothDemo_7();

		//run demo after ceate
		runClothDome_7 = true;
	}


	//deformable demos
	if(mGUI->Command_Deformable_Demo_1)
	{
		mGUI->Command_Deformable_Demo_1 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,6.0f,40.0f));
		dpsSoftbodyHelper->createDeformDemo_1(btVector3(2,1.1,0));
		//dpsHelper->createCube(Ogre::Vector3(-5,1.5,0),Ogre::Vector3(4,4,4),1);
		dpsHelper->createMesh(Ogre::Vector3(-3,1,0),20,"softcube.mesh",Ogre::Vector3(2,2,2));

		//run demo after ceate
		runDeformDome_1 = true;
	}

	if(mGUI->Command_Deformable_Demo_2)
	{
		mGUI->Command_Deformable_Demo_2 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,10.0f,40.0f));
		dpsSoftbodyHelper->createDeformDemo_2(btVector3(0,7,0));
		dpsHelper->createMesh(Ogre::Vector3(-8,2,0),1,"Barrel.mesh",Ogre::Vector3(1,1,1));

		//run demo after ceate
		runDeformDome_2 = true;
	}
	if(mGUI->Command_Deformable_Demo_3)
	{
		mGUI->Command_Deformable_Demo_3 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,6.0f,20.0f));
		dpsSoftbodyHelper->createDeformDemo_3(btVector3(3,1,0));
		dpsHelper->createMesh(Ogre::Vector3(-4,2,0),1,"m845.mesh",Ogre::Vector3(2,2,2));

		//run demo after ceate
		runDeformDome_3 = true;
	}
	if(mGUI->Command_Deformable_Demo_4)
	{
		mGUI->Command_Deformable_Demo_4 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,6.0f,40.0f));
		dpsSoftbodyHelper->createDeformDemo_4(btVector3(0,3,0));

		//run demo after ceate
		runDeformDome_4 = true;
	}
	if(mGUI->Command_Deformable_Demo_5)
	{
		mGUI->Command_Deformable_Demo_5 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,17.0f,20.0f));
		dpsSoftbodyHelper->createDeformDemo_5(btVector3(0,20,0));

		//run demo after ceate
		runDeformDome_5 = true;
	}
	if(mGUI->Command_Deformable_Demo_6)
	{
		mGUI->Command_Deformable_Demo_6 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,6.0f,20.0f));
		dpsSoftbodyHelper->createDeformDemo_6(btVector3(0,1,0));

		//run demo after ceate
		runDeformDome_6 = true;
	}
	if(mGUI->Command_Deformable_Demo_7)
	{
		mGUI->Command_Deformable_Demo_7 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,6.0f,20.0f));
		dpsSoftbodyHelper->createGimpactBuuny();

		//might not needed. 
		bulletDebugScreen();

		//run demo after ceate
		runDeformDome_7 = true;
	}


	if(mGUI->Command_Playgroud_1)
	{
		mGUI->Command_Playgroud_1 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,6.0f,50.0f));
		dpsSoftbodyHelper->createPlayground_1(btVector3(0,10,0));

		//run demo after ceate
		runPlaygroud_1 = true;
	}
	if(mGUI->Command_Playgroud_2)
	{
		mGUI->Command_Playgroud_2 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,6.0f,40.0f));
		dpsSoftbodyHelper->createPlayground_2();
		dpsHelper->createMesh(Ogre::Vector3(0,2,0),50,"defCube.mesh",Ogre::Vector3(10,2,10));
 		dpsHelper->createMesh(Ogre::Vector3(0,10,0),10,"defCube.mesh",Ogre::Vector3(10,2,10));
 		dpsHelper->createMesh(Ogre::Vector3(0,17,0),10,"defCube.mesh",Ogre::Vector3(10,2,10));


		//run demo after ceate
		runPlaygroud_2 = true;
	}
	if(mGUI->Command_Playgroud_3)
	{
		mGUI->Command_Playgroud_3 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,10.0f,40.0f));
		dpsSoftbodyHelper->createPlayground_3();
		dpsHelper->createMesh(Ogre::Vector3(-10,0.1,0),50,"car.mesh",Ogre::Vector3(1,1,1));
		//createFromMesh(m_playgroundManualObject_3, m_playgroundBody_3, btVector3(0,0,0), btVector3(0,0,0), btVector3(1,1,1));

		if (playgroundCount == 0)
		{
			dpsHelper->createMesh(Ogre::Vector3(-5,50,0),50,"defCube.mesh",Ogre::Vector3(10,2,10));
			dpsHelper->createMesh(Ogre::Vector3(15,50,0),50,"defCube.mesh",Ogre::Vector3(10,2,10));
			playgroundCount++;
		}
		else if(playgroundCount == 1)
		{
			dpsHelper->createMesh(Ogre::Vector3(-10,50,0),50,"defCube.mesh",Ogre::Vector3(10,2,10));
			dpsHelper->createMesh(Ogre::Vector3(10,50,0),50,"defCube.mesh",Ogre::Vector3(10,2,10));
			playgroundCount++;
		}
		else if (playgroundCount == 2)
		{
			dpsHelper->createMesh(Ogre::Vector3(-15,50,0),50,"defCube.mesh",Ogre::Vector3(10,2,10));
			dpsHelper->createMesh(Ogre::Vector3(5,50,0),50,"defCube.mesh",Ogre::Vector3(10,2,10));
			playgroundCount++;
		}
		else if(playgroundCount == 3)
		{
			dpsHelper->createMesh(Ogre::Vector3(-10,50,0),50,"defCube.mesh",Ogre::Vector3(2,10,10));
			dpsHelper->createMesh(Ogre::Vector3(10,50,0),50,"defCube.mesh",Ogre::Vector3(2,10,10));
			playgroundCount = 0;
		}

		//run demo after ceate
		runPlaygroud_3 = true;
	}
	if(mGUI->Command_Playgroud_4)
	{
		mGUI->Command_Playgroud_4 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,10.0f,40.0f));
		dpsSoftbodyHelper->createPlayground_4(btVector3(8,7,0));
		dpsHelper->createMesh(Ogre::Vector3(-8,5,0),50,"cylinder_high.mesh",Ogre::Vector3(0.7,0.7,0.7));
		dpsHelper->createMesh(Ogre::Vector3(-8,50,0),400,"defCube.mesh",Ogre::Vector3(10,2,10));
		dpsHelper->createMesh(Ogre::Vector3(6,50,0),400,"defCube.mesh",Ogre::Vector3(10,2,10));


		//run demo after ceate
		runPlaygroud_4 = true;
	}
	if(mGUI->Command_Playgroud_5)
	{
		mGUI->Command_Playgroud_5 = false;

		//clean screen before create new demo
		clearScreen();

		//create demo
		resetCamera(Ogre::Vector3(0.0f,10.0f,40.0f));

		//dpsSoftbodyHelper->createPlayground_4(btVector3(8,7,0));

		//dpsHelper->createMesh(Ogre::Vector3(-8,5,0),50,"cylinder_high.mesh",Ogre::Vector3(0.7,0.7,0.7));

		for(int i = 0; i<500 ;i++)
		{
			dpsHelper->createMesh(Ogre::Vector3(-10,i,0),1,"defCube.mesh",Ogre::Vector3(1,1,1));
			dpsHelper->createMesh(Ogre::Vector3(10,i,0),1,"defCube.mesh",Ogre::Vector3(1,1,1));
		}


		//run demo after ceate
		runPlaygroud_5 = true;
	}
}


void DPS::solidScreen(void)
{
	vp->getCamera()->setPolygonMode(Ogre::PM_SOLID);
	mGUI->Command_Bullet_Debug_Mode = false;
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Solid")->setEnabled(false);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Wireframe")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Points")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Bullet_Debug_Mode")->setEnabled(true);
}


void DPS::bulletDebugScreen(void)
{
	mGUI->Command_Bullet_Debug_Mode = true;
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Solid")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Wireframe")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Points")->setEnabled(true);
	mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Bullet_Debug_Mode")->setEnabled(false);
}


void DPS::demoController(void)
{
	//cloth demos
	if(runClothDome_1)
	{
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_clothManualObject_1, dpsSoftbodyHelper->m_clothBody_1);
	}
	if(runClothDome_2)
	{
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_clothManualObject_2, dpsSoftbodyHelper->m_clothBody_2);
	}
	if(runClothDome_3)
	{
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_clothManualObject_3, dpsSoftbodyHelper->m_clothBody_3);
	}
	if(runClothDome_4)
	{
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_clothManualObject_4, dpsSoftbodyHelper->m_clothBody_4);
	}
	if(runClothDome_5)
	{
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_clothManualObject_5_0, dpsSoftbodyHelper->m_clothBody_5_0);
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_clothManualObject_5_1, dpsSoftbodyHelper->m_clothBody_5_1);
	}
	if(runClothDome_6)
	{
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_clothManualObject_6_0, dpsSoftbodyHelper->m_clothBody_6_0);
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_clothManualObject_6_1, dpsSoftbodyHelper->m_clothBody_6_1);
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_clothManualObject_6_2, dpsSoftbodyHelper->m_clothBody_6_2);
	}
	if(runClothDome_7)
	{
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_clothManualObject_7_0, dpsSoftbodyHelper->m_clothBody_7_0);
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_clothManualObject_7_1, dpsSoftbodyHelper->m_clothBody_7_1);
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_clothManualObject_7_2, dpsSoftbodyHelper->m_clothBody_7_2);
	}


	//deformation
	if(runDeformDome_1)
	{
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_deformManualObject_1, dpsSoftbodyHelper->m_deformBody_1);
	}
	if(runDeformDome_2)
	{
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_deformManualObject_2, dpsSoftbodyHelper->m_deformBody_2);
	}
	if(runDeformDome_3)
	{
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_deformManualObject_3, dpsSoftbodyHelper->m_deformBody_3);
	}
	if(runDeformDome_4)
	{
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_deformManualObject_4, dpsSoftbodyHelper->m_deformBody_4);
	}
	if(runDeformDome_5)
	{
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_deformManualObject_5, dpsSoftbodyHelper->m_deformBody_5);
	}
	if(runDeformDome_6)
	{
		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_deformManualObject_6, dpsSoftbodyHelper->m_deformBody_6);
	}
// 	if(runDeformDome_7)
// 	{
// 		dpsSoftbodyHelper->updateSoftBody(dpsSoftbodyHelper->m_deformManualObject_7, dpsSoftbodyHelper->m_deformBody_7);
// 	}

	//playground
	if(runPlaygroud_1)
	{
		dpsSoftbodyHelper->updateCar(dpsSoftbodyHelper->m_playgroundManualObject_fl, dpsSoftbodyHelper->m_playgroundBody_fl);
		dpsSoftbodyHelper->updateCar(dpsSoftbodyHelper->m_playgroundManualObject_fr, dpsSoftbodyHelper->m_playgroundBody_fr);
		dpsSoftbodyHelper->updateCar(dpsSoftbodyHelper->m_playgroundManualObject_rl, dpsSoftbodyHelper->m_playgroundBody_rl);
		dpsSoftbodyHelper->updateCar(dpsSoftbodyHelper->m_playgroundManualObject_rr, dpsSoftbodyHelper->m_playgroundBody_rr);
		dpsSoftbodyHelper->updateCar(dpsSoftbodyHelper->m_playgroundManualObject_carBody, dpsSoftbodyHelper->m_playgroundBody_carBody);
	}
	if(runPlaygroud_2)
	{
		dpsSoftbodyHelper->updateCar(dpsSoftbodyHelper->m_playgroundManualObject_2_1, dpsSoftbodyHelper->m_playgroundBody_2_1);
 		dpsSoftbodyHelper->updateCar(dpsSoftbodyHelper->m_playgroundManualObject_2_2, dpsSoftbodyHelper->m_playgroundBody_2_2);
	}
	if(runPlaygroud_3)
	{
		dpsSoftbodyHelper->updateCar(dpsSoftbodyHelper->m_playgroundManualObject_3, dpsSoftbodyHelper->m_playgroundBody_3);
	}
	if(runPlaygroud_4)
	{
		dpsSoftbodyHelper->updateCar(dpsSoftbodyHelper->m_playgroundManualObject_4, dpsSoftbodyHelper->m_playgroundBody_4);
	}
}


bool DPS::keyPressed(const OIS::KeyEvent &arg)
{
	if (arg.key == OIS::KC_1) 
	{
		dpsHelper->throwSphere();
	}
	if (arg.key == OIS::KC_2) 
	{
		dpsHelper->throwCube();
	}
	if (arg.key == OIS::KC_3) 
	{
		dpsHelper->createOgreHead();
	}
	return BaseApplication::keyPressed(arg);
}


void DPS::deleteOgreEntities(void)
{
	mSceneMgr->destroyAllManualObjects();
	mSceneMgr->destroyAllEntities();
	mSceneMgr->destroyAllLights();

	dpsHelper->createWorld();
}


bool DPS::mouseMoved(const OIS::MouseEvent &arg)
{
	//mouse move on screen
	bool mouseOnWidget = MyGUI::InputManager::getInstance().injectMouseMove(arg.state.X.abs, arg.state.Y.abs, arg.state.Z.abs);

	//MyGUI::IntPoint mousePos = MyGUI::InputManager::getInstance().getMousePosition();

	if (arg.state.buttonDown(OIS::MB_Right))
	{
		mCameraMan->injectMouseMove(arg);
	}

	return BaseApplication::mouseMoved(arg);
}


bool DPS::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	bool mouseOnWidget = MyGUI::InputManager::getInstance().injectMousePress(arg.state.X.abs, arg.state.Y.abs, MyGUI::MouseButton::Enum(id));

	if(!mouseOnWidget)
	{
		if (id == OIS::MB_Left)
		{
			MyGUI::IntPoint mousePos = MyGUI::InputManager::getInstance().getMousePosition();

			//do the raycast
			Ogre::Ray mouseRay = mCamera->getCameraToViewportRay(mousePos.left/float(arg.state.width),mousePos.top/float(arg.state.height));
			Ogre::RaySceneQuery* mRayScnQuery = mSceneMgr->createRayQuery(Ogre::Ray());
			
			mRayScnQuery->setRay(mouseRay);
			mRayScnQuery->setSortByDistance(true, 1);

			Ogre::RaySceneQueryResult &result = mRayScnQuery->execute();
			Ogre::RaySceneQueryResult::iterator iter = result.begin();

			if (iter != result.end() && !iter->worldFragment)
			{
				rayObject = iter->movable;
				rayNode = rayObject->getParentSceneNode();
				mGUI->miniCameraWindow->setVisible(true);
				mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Enable_Mini_Camera")->setEnabled(false);
				mGUI->mGuiSystem->findWidget<MyGUI::Widget>("Command_Disable_Mini_Camera")->setEnabled(true);
			}
		}
	}

	return BaseApplication::mousePressed(arg, id);
}


bool DPS::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	MyGUI::InputManager::getInstance().injectMouseRelease(arg.state.X.abs, arg.state.Y.abs, MyGUI::MouseButton::Enum(id));

	return BaseApplication::mouseReleased(arg, id);
}


void DPS::resetCamera(void)
{
	mCamera->setPosition(0,5,20);
	mCamera->lookAt(0,5,-10);
}


void DPS::resetCamera(Ogre::Vector3 camPos)
{
	mCamera->setPosition(camPos);
	mCamera->lookAt(Ogre::Vector3(camPos.x,camPos.y,-camPos.z));
}


void DPS::setMiniCamPosition(Ogre::Vector3 camPos)
{
	miniCam->setPosition(10+camPos);
	miniCam->lookAt(camPos);
	miniCam->setNearClipDistance(5);
}




#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif
 
#ifdef __cplusplus
	extern "C" {
#endif
 
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
		int main(int argc, char *argv[])
#endif
		{
			// Create application object
			//DPS app;
			Globals::app = new DPS();
			try {
				Globals::app->go();
			} catch( Ogre::Exception& e ) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
				//MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
				std::cerr << "An exception has occured: " <<
					e.getFullDescription().c_str() << std::endl;
#endif
			}
 
			return 0;
		}
 
#ifdef __cplusplus
	}
#endif