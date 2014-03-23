﻿#include "DPS.h"
#include <vector>
#include <iostream>
#include <fstream> 
#include <string> 
#include "BulletCollision/Gimpact/btGImpactShape.h"
#include "BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h"
#include "Mesh/barrel.h"
#include "Mesh/BunnyMesh.h"
#include "GUI.h"
//#include <memory>

const int maxProxies = 32766;
btRigidBody * hit_body = 0;
btVector3 hit_rel_pos;
btVector3 shot_imp = btVector3(0,0,0);

DPS::DPS(void)
{
	initPhysics();
}

DPS::~DPS(void)
{
	exitPhysics();
	leapMotionCleanup();
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
	//	clientResetScene();
	m_softBodyWorldInfo.m_sparsesdf.Initialize();

	dt = evt.timeSinceLastFrame;
	//mGUI->Simulation_Default = true;
	//std::vector<float> texCoords = dpsSoftbodyHelper->texCoord;
}

void DPS::exitPhysics(void)
{
	//cleanup in the reverse order of creation/initialization
	//remove the rigidbodies from the dynamics world and delete them
	int i;
	for (i=Globals::phyWorld->getNumCollisionObjects()-1; i>=0 ;i--)
	{
		btCollisionObject* obj = Globals::phyWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState())
		{
			delete body->getMotionState();
		}
		Globals::phyWorld->removeCollisionObject( obj );
		delete obj;
	}

	//delete collision shapes
	for (int j=0;j<m_collisionShapes.size();j++)
	{
		btCollisionShape* shape = m_collisionShapes[j];
		m_collisionShapes[j] = 0;
		delete shape;
	}

	delete Globals::phyWorld;
	delete m_solver;
	delete m_broadphase;
	delete m_dispatcher;
	delete m_collisionConfiguration;
	delete Globals::dbgdraw;
	//texCoords = NULL;
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


/*
void DPS::createCamera(void)
{
    // create the camera
    mCamera = mSceneMgr->createCamera("PlayerCam");
    // set its position, direction  
    mCamera->setPosition(Ogre::Vector3(0,10,500));
    mCamera->lookAt(Ogre::Vector3(0,0,0));
    // set the near clip distance
    mCamera->setNearClipDistance(5);
 
    mCameraMan = new OgreBites::SdkCameraMan(mCamera);   // create a default camera controller
}
//-------------------------------------------------------------------------------------
void DPS::createViewports(void)
{
    // Create one viewport, entire window
    Ogre::Viewport* vp = mWindow->addViewport(mCamera);
    vp->setBackgroundColour(Ogre::ColourValue(0,0,0));
    // Alter the camera aspect ratio to match the viewport
    mCamera->setAspectRatio(Ogre::Real(vp->getActualWidth()) / Ogre::Real(vp->getActualHeight()));    
}
*/


void DPS::createScene(void)
{
	mGUI->createGUI(1);

	// Basic Ogre stuff.
	mSceneMgr->setAmbientLight(ColourValue(0.9f,0.9f,0.9f));
	mCamera->setPosition(Vector3(0,5,20));
	mCamera->lookAt(Vector3(0,5,-10));
	mCamera->setNearClipDistance(0.05f);
	//LogManager::getSingleton().setLogDetail(LL_BOREME);

	dpsHelper = std::make_shared<DPSHelper>(Globals::phyWorld, mCamera, mSceneMgr);
	dpsSoftbodyHelper = std::make_shared<DPSSoftBodyHelper>(Globals::phyWorld, mCamera, mSceneMgr);

	dpsHelper->createWorld();

	// Debug drawing
	Globals::dbgdraw = new BtOgre::DebugDrawer(mSceneMgr->getRootSceneNode(), Globals::phyWorld);
	Globals::phyWorld->setDebugDrawer(Globals::dbgdraw);

	//initSoftBody(dpsSoftbodyHelper->createSoftBody(btVector3(0,20,0)));
	//initSoftBody(dpsSoftbodyHelper->createCloth());
	//initSoftBody(dpsSoftbodyHelper->createDeformableModel());
	//initSoftBody(dpsSoftbodyHelper->createBunny());
	//initSoftBody(dpsSoftbodyHelper->createMesh());

	createGimpactBarrel();
	createGimpactBuuny();

	leapMotionInit();
}

bool DPS::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
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

	//Update Bullet world
	Globals::phyWorld->stepSimulation(dt,0); 
	Globals::phyWorld->debugDrawWorld();

	//Shows debug if F3 key down.
	//Globals::dbgdraw->setDebugMode(mKeyboard->isKeyDown(OIS::KC_F3));
	Globals::dbgdraw->setDebugMode(mGUI->Command_Bullet_Debug_Mode);
	Globals::dbgdraw->step();

	//Globals::app->updateSoftBody(dpsSoftbodyHelper->m_cloth);
	//Globals::app->updateSoftBody(dpsSoftbodyHelper->m_SoftBody);
	//Globals::app->updateSoftBody(dpsSoftbodyHelper->m_deformableModel);
	//Globals::app->updateSoftBody(dpsSoftbodyHelper->m_bunny);
	//Globals::app->updateSoftBody(dpsSoftbodyHelper->m_mesh);

	Ogre::Vector3 camPos = mCamera->getDerivedPosition();
// 	Ogre::Vector3 camPos = mCamera->get
 	Ogre::Vector3 camDir = mCamera->getDerivedDirection();

	if (camPos.y<0.5)
	{
		mCamera->setPosition(camPos.x, 0.5, camPos.z);
	}

	miniCamPos(camPos,camDir);

	leapMotionUpdate();

	return BaseApplication::frameRenderingQueued(evt);
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
// 	if (arg.key == OIS::KC_4) 
// 	{
// 
// 	}
	if (arg.key == OIS::KC_SPACE) 
	{
		GimpactRayCallBack();
	}
	return BaseApplication::keyPressed(arg);
}


void DPS::initSoftBody(btSoftBody* body)
{
	//manual objects are used to generate new meshes based on raw vertex data
	//this is used for the liquid form
	m_ManualObject = mSceneMgr->createManualObject("liquidBody");
	m_ManualObject->setDynamic(true);
	m_ManualObject->setCastShadows(true);

	btSoftBody::tNodeArray& nodes(body->m_nodes);
	btSoftBody::tFaceArray& faces(body->m_faces);

	m_ManualObject->estimateVertexCount(faces.size()*3);
	m_ManualObject->estimateIndexCount(faces.size()*3);

	//m_ManualObject->begin("CharacterMaterials/LiquidBody", Ogre::RenderOperation::OT_TRIANGLE_LIST);
	//m_ManualObject->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);
	//m_ManualObject->begin("FlatVertexColour", Ogre::RenderOperation::OT_TRIANGLE_LIST);
	m_ManualObject->begin("ClothMaterial", Ogre::RenderOperation::OT_TRIANGLE_LIST);

	//btSoftBody::Node *node0 = 0, *node1 = 0, *node2 = 0;

	//http://www.ogre3d.org/tikiwiki/ManualObject
	for (int i = 0; i < faces.size(); ++i)
	{
		//node0 = faces[i].m_n[0];
		//node1 = faces[i].m_n[1];
		//node2 = faces[i].m_n[2];
		
		//problem of rendering texture - DO NOT USE NORMALS IN UPDATE FUNCTION FOR NOW! 
		m_ManualObject->position(body->m_faces[i].m_n[0]->m_x.x(),body->m_faces[i].m_n[0]->m_x.y(),body->m_faces[i].m_n[0]->m_x.z());
		//m_ManualObject->colour(0.5f,0.5f,0.5f);
		m_ManualObject->textureCoord(dpsSoftbodyHelper->texCoord[i],dpsSoftbodyHelper->texCoord[i+1]);
		m_ManualObject->position(body->m_faces[i].m_n[1]->m_x.x(),body->m_faces[i].m_n[1]->m_x.y(),body->m_faces[i].m_n[1]->m_x.z());
		//m_ManualObject->colour(0.5f,0.5f,0.5f);
		m_ManualObject->textureCoord(dpsSoftbodyHelper->texCoord[i+1],dpsSoftbodyHelper->texCoord[i+2]);
		m_ManualObject->position(body->m_faces[i].m_n[2]->m_x.x(),body->m_faces[i].m_n[2]->m_x.y(),body->m_faces[i].m_n[2]->m_x.z());
		//m_ManualObject->colour(0.5f,0.5f,0.5f);
		m_ManualObject->textureCoord(dpsSoftbodyHelper->texCoord[i+2],dpsSoftbodyHelper->texCoord[i+3]);
// 
		// 		m_ManualObject->position(body->m_faces[i].m_n[0]->m_x[0],body->m_faces[i].m_n[0]->m_x[1],body->m_faces[i].m_n[0]->m_x[2]);
		// 		m_ManualObject->position(body->m_faces[i].m_n[1]->m_x[0],body->m_faces[i].m_n[1]->m_x[1],body->m_faces[i].m_n[1]->m_x[2]);
		// 		m_ManualObject->position(body->m_faces[i].m_n[2]->m_x[0],body->m_faces[i].m_n[2]->m_x[1],body->m_faces[i].m_n[2]->m_x[2]);
		// 
		m_ManualObject->normal(body->m_faces[i].m_n[0]->m_n[0], body->m_faces[i].m_n[0]->m_n[1], body->m_faces[i].m_n[0]->m_n[2]);
		m_ManualObject->normal(body->m_faces[i].m_n[1]->m_n[0], body->m_faces[i].m_n[1]->m_n[1], body->m_faces[i].m_n[1]->m_n[2]);
		m_ManualObject->normal(body->m_faces[i].m_n[2]->m_n[0], body->m_faces[i].m_n[2]->m_n[1], body->m_faces[i].m_n[2]->m_n[2]);



		//m_ManualObject->position(node0->m_x[0], node0->m_x[1], node0->m_x[2]);
		//m_ManualObject->textureCoord(1,0);
		//m_ManualObject->normal(node0->m_n[0], node0->m_n[1], node0->m_n[2]);
				
		//m_ManualObject->position(node1->m_x[0], node1->m_x[1], node1->m_x[2]);
		//m_ManualObject->textureCoord(0,1);
		//m_ManualObject->normal(node1->m_n[0], node1->m_n[1], node1->m_n[2]);
				
		//m_ManualObject->position(node2->m_x[0], node2->m_x[1], node2->m_x[2]);
		//m_ManualObject->textureCoord(1,1);
		//m_ManualObject->normal(node2->m_n[0], node2->m_n[1], node2->m_n[2]);

		m_ManualObject->triangle(i*3,i*3+1,i*3+2);
// 		m_ManualObject->triangle(i*3+1);
// 		m_ManualObject->triangle(i*3+2);
	}
// 	m_ManualObject->textureCoord(1,0);
// 	m_ManualObject->textureCoord(0,0);
// 	m_ManualObject->textureCoord(0,1);
// 	m_ManualObject->textureCoord(1,1);
	m_ManualObject->end();

	Ogre::SceneNode* mLiquidBodyNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
	mLiquidBodyNode->attachObject(m_ManualObject);
}


void DPS::updateSoftBody(btSoftBody* body)
{
	//grab the calculated mesh data from the physics body
	btSoftBody::tNodeArray& nodes(body->m_nodes);
	btSoftBody::tFaceArray& faces(body->m_faces);

	m_ManualObject->beginUpdate(0);
/*	btSoftBody::Node *node0 = 0, *node1 = 0, *node2 = 0;*/
	for (int i = 0; i < faces.size(); i++)
	{
		m_ManualObject->position(body->m_faces[i].m_n[0]->m_x.x(),body->m_faces[i].m_n[0]->m_x.y(),body->m_faces[i].m_n[0]->m_x.z());
		//m_ManualObject->colour(0.5f,0.5f,0.5f);
		m_ManualObject->position(body->m_faces[i].m_n[1]->m_x.x(),body->m_faces[i].m_n[1]->m_x.y(),body->m_faces[i].m_n[1]->m_x.z());
		//m_ManualObject->colour(0.5f,0.5f,0.5f);
		m_ManualObject->position(body->m_faces[i].m_n[2]->m_x.x(),body->m_faces[i].m_n[2]->m_x.y(),body->m_faces[i].m_n[2]->m_x.z());
		//m_ManualObject->colour(0.5f,0.5f,0.5f);
		// 
// 		m_ManualObject->textureCoord(body->m_faces[i].m_n[1]->m_x.x(), body->m_faces[i].m_n[1]->m_x.y(), body->m_faces[i].m_n[1]->m_x.z());
// 		m_ManualObject->textureCoord(body->m_faces[i].m_n[2]->m_x.x(), body->m_faces[i].m_n[2]->m_x.y(), body->m_faces[i].m_n[2]->m_x.z());

// 		m_ManualObject->position(body->m_faces[i].m_n[0]->m_x[0],body->m_faces[i].m_n[0]->m_x[1],body->m_faces[i].m_n[0]->m_x[2]);
// 		m_ManualObject->position(body->m_faces[i].m_n[1]->m_x[0],body->m_faces[i].m_n[1]->m_x[1],body->m_faces[i].m_n[1]->m_x[2]);
// 		m_ManualObject->position(body->m_faces[i].m_n[2]->m_x[0],body->m_faces[i].m_n[2]->m_x[1],body->m_faces[i].m_n[2]->m_x[2]);
// 
		m_ManualObject->normal(body->m_faces[i].m_n[0]->m_n[0], body->m_faces[i].m_n[0]->m_n[1], body->m_faces[i].m_n[0]->m_n[2]);
		m_ManualObject->normal(body->m_faces[i].m_n[1]->m_n[0], body->m_faces[i].m_n[1]->m_n[1], body->m_faces[i].m_n[1]->m_n[2]);
		m_ManualObject->normal(body->m_faces[i].m_n[2]->m_n[0], body->m_faces[i].m_n[2]->m_n[1], body->m_faces[i].m_n[2]->m_n[2]);

// 		node0 = faces[i].m_n[0];
// 		node1 = faces[i].m_n[1];
// 		node2 = faces[i].m_n[2];
				
// 		m_ManualObject->position(node0->m_x[0], node0->m_x[1], node0->m_x[2]);
// 		m_ManualObject->normal(node0->m_n[0], node0->m_n[1], node0->m_n[2]);
// 				
// 		m_ManualObject->position(node1->m_x[0], node1->m_x[1], node1->m_x[2]);
// 		m_ManualObject->normal(node1->m_n[0], node1->m_n[1], node1->m_n[2]);
// 				
// 		m_ManualObject->position(node2->m_x[0], node2->m_x[1], node2->m_x[2]);
// 		m_ManualObject->normal(node2->m_n[0], node2->m_n[1], node2->m_n[2]);

		m_ManualObject->index(i*3);
		m_ManualObject->index(i*3+1);
		m_ManualObject->index(i*3+2);
	}

	m_ManualObject->end();
}


// void DPS::initRigidBody(btRigidBody* body)
// {
// 
// }
// 
// void DPS::updateRigidBody(btRigidBody* body)
// {
// 
// }
void DPS::GimpactRayCallBack(void)
{
	Ogre::Vector3 camPos = mCamera->getPosition();
	Ogre::Vector3 viewPos = mCamera->getDirection();
	btVector3 to = btVector3(viewPos.x,viewPos.y,viewPos.z);
	to.normalize();
	to *= 1000;
	MyClosestRayResultCallback rayCallback(btVector3(camPos.x,camPos.y,camPos.z), to);
	Globals::phyWorld->rayTest(btVector3(camPos.x,camPos.y,camPos.z), to, rayCallback);

	if (rayCallback.hasHit())
	{
		btVector3 hit = rayCallback.m_hitPointWorld;
		btCollisionObject obj = *rayCallback.m_collisionObject;
		//downcast
		btRigidBody* body = btRigidBody::upcast(&obj);
		if (body)
		{
			hit_body = body;
			if (rayCallback.m_hitTriangleShape)
			{
				bool triangle_ok = process_triangle(hit_body->getCollisionShape(),
					rayCallback.m_hitTriangleIndex);
			}
		}
	}
}


bool DPS::process_triangle(btCollisionShape* shape, int hitTriangleIndex)
{
	btStridingMeshInterface * meshInterface = NULL;

	if (shape->getShapeType() == GIMPACT_SHAPE_PROXYTYPE)
	{
		meshInterface = (static_cast<btGImpactMeshShape*>(shape))->getMeshInterface();
	}
	else if (shape->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE)
	{
		meshInterface = (static_cast<btBvhTriangleMeshShape*>(shape))->getMeshInterface();
	}
	else
	{
		return false;
	}

	if (!meshInterface) return false;

	unsigned char *vertexbase;
	int numverts;
	PHY_ScalarType type;
	int stride;
	unsigned char *indexbase;
	int indexstride;
	int numfaces;
	PHY_ScalarType indicestype;

	meshInterface->getLockedVertexIndexBase(
		&vertexbase,
		numverts,
		type,
		stride,
		&indexbase,
		indexstride,
		numfaces,
		indicestype,
		0);

	unsigned int * gfxbase = (unsigned int*)(indexbase+hitTriangleIndex*indexstride);
	const btVector3 & meshScaling = shape->getLocalScaling();
	btVector3 triangle_v[3];

	for (int j=2;j>=0;j--)
	{
		int graphicsindex = indicestype==PHY_SHORT?((unsigned short*)gfxbase)[j]:gfxbase[j];

		btScalar * graphicsbase = (btScalar*)(vertexbase+graphicsindex*stride);

		if (shape->getShapeType() == GIMPACT_SHAPE_PROXYTYPE)
		{
			graphicsbase[0] *= btScalar(0.85);
			graphicsbase[1] *= btScalar(0.85);
			graphicsbase[2] *= btScalar(0.85);
		}
	}

	meshInterface->unLockVertexBase(0);

	if (shape->getShapeType() == GIMPACT_SHAPE_PROXYTYPE)
	{
		btGImpactMeshShape * gimp_shape = static_cast<btGImpactMeshShape*>(shape);
		gimp_shape->postUpdate();
	}
	return true;
}

void DPS::createGimpactBarrel(void)
{
	btTransform tr;
	tr.setIdentity();
	tr.setOrigin(btVector3(0,0,0));

	btTriangleIndexVertexArray * mesh_interface = new btTriangleIndexVertexArray(
		faces_size,
		barrel_ia,
		3*sizeof(int),
		vertices_size,
		(btScalar *) &barrel_va[0][0],
		sizeof(btVector3));

	btGImpactMeshShape * shape = new btGImpactMeshShape(mesh_interface);
	shape->setMargin(btScalar(0.1));
	shape->updateBound();
	m_collisionShapes.push_back(shape);
	/*	btRigidBody * body = localCreateRigidBody(btScalar(20.0),tr,shape);*/
	btVector3 localInertia(0,0,0);
	//btScalar m_defaultContactProcessingThreshold;
	shape->calculateLocalInertia(btScalar(20.0),localInertia);
	btDefaultMotionState* myMotionState = new btDefaultMotionState(tr);
	btRigidBody::btRigidBodyConstructionInfo cInfo(btScalar(20.0),myMotionState,shape,localInertia);
	btRigidBody* body = new btRigidBody(cInfo);
	body->setContactProcessingThreshold(btScalar(BT_LARGE_FLOAT));
	//body->translate(btVector3(0,50,0));
	Globals::phyWorld->addRigidBody(body);

	m_ManualObject = mSceneMgr->createManualObject("barrel");
	m_ManualObject->setDynamic(true);

	//m_ManualObject->estimateVertexCount(BUNNY_NUM_VERTICES);
	//m_ManualObject->estimateIndexCount(BUNNY_NUM_INDICES);

	m_ManualObject->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);

	for (int i = 0; i < 83; i++)
	{
		btVector3 face = barrel_va[i];

		m_ManualObject->position(face.x(),face.y(),face.z());

// 		m_ManualObject->normal(Ogre::Vector3(gIndicesBunny[i][0],gIndicesBunny[i][1],gIndicesBunny[i][2]));
// 		m_ManualObject->normal(Ogre::Vector3(gIndicesBunny[i+1][0],gIndicesBunny[i+1][1],gIndicesBunny[i+1][2]));
// 		m_ManualObject->normal(Ogre::Vector3(gIndicesBunny[i+2][0],gIndicesBunny[i+2][1],gIndicesBunny[i+2][2]));

		//m_ManualObject->position(node0->m_x[0], node0->m_x[1], node0->m_x[2]);
		//m_ManualObject->textureCoord(1,0);
		//m_ManualObject->normal(node0->m_n[0], node0->m_n[1], node0->m_n[2]);

		//m_ManualObject->position(node1->m_x[0], node1->m_x[1], node1->m_x[2]);
		//m_ManualObject->textureCoord(0,1);
		//m_ManualObject->normal(node1->m_n[0], node1->m_n[1], node1->m_n[2]);

		//m_ManualObject->position(node2->m_x[0], node2->m_x[1], node2->m_x[2]);
		//m_ManualObject->textureCoord(1,1);
		//m_ManualObject->normal(node2->m_n[0], node2->m_n[1], node2->m_n[2]);
// 
// 		m_ManualObject->index(i*3);
// 		m_ManualObject->index(i*3+1);
// 		m_ManualObject->index(i*3+2);
	}
	// 	m_ManualObject->textureCoord(1,0);
	// 	m_ManualObject->textureCoord(0,0);
	// 	m_ManualObject->textureCoord(0,1);
	// 	m_ManualObject->textureCoord(1,1);
	m_ManualObject->end();

	Ogre::SceneNode* mLiquidBodyNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
	mLiquidBodyNode->attachObject(m_ManualObject);

	
}



void DPS::createGimpactBuuny(void)
{
	btTransform tr;
	tr.setIdentity();
	tr.setOrigin(btVector3(0,0,0));

	btTriangleIndexVertexArray * mesh_interface = new btTriangleIndexVertexArray(
		BUNNY_NUM_TRIANGLES,
		&gIndicesBunny[0][0],
		3*sizeof(int),
		BUNNY_NUM_VERTICES,
		(btScalar *) &gVerticesBunny[0],
		sizeof(btScalar)*3);

	btGImpactMeshShape * shape = new btGImpactMeshShape(mesh_interface);
	shape->setMargin(btScalar(0.1));
	shape->updateBound();
	m_collisionShapes.push_back(shape);
	/*	btRigidBody * body = localCreateRigidBody(btScalar(20.0),tr,shape);*/
	btVector3 localInertia(0,0,0);
	//btScalar m_defaultContactProcessingThreshold;
	shape->calculateLocalInertia(btScalar(20.0),localInertia);
	btDefaultMotionState* myMotionState = new btDefaultMotionState(tr);
	btRigidBody::btRigidBodyConstructionInfo cInfo(btScalar(20.0),myMotionState,shape,localInertia);
	btRigidBody* body = new btRigidBody(cInfo);
	body->setContactProcessingThreshold(btScalar(BT_LARGE_FLOAT));
	body->translate(btVector3(0,50,0));
	Globals::phyWorld->addRigidBody(body);

	m_ManualObject = mSceneMgr->createManualObject("bunny");
	m_ManualObject->setDynamic(true);

	//m_ManualObject->estimateVertexCount(BUNNY_NUM_VERTICES);
	//m_ManualObject->estimateIndexCount(BUNNY_NUM_INDICES);

	m_ManualObject->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);

	for (int i = 0; i < 1358; i+=9)
	{
		//node0 = faces[i].m_n[0];
		//node1 = faces[i].m_n[1];
		//node2 = faces[i].m_n[2];

		m_ManualObject->position(Ogre::Vector3(gVerticesBunny[i],gVerticesBunny[i+1],gVerticesBunny[i+2]));
		m_ManualObject->position(Ogre::Vector3(gVerticesBunny[i+3],gVerticesBunny[i+4],gVerticesBunny[i+5]));
		m_ManualObject->position(Ogre::Vector3(gVerticesBunny[i+6],gVerticesBunny[i+7],gVerticesBunny[i+8]));


		m_ManualObject->normal(Ogre::Vector3(gIndicesBunny[i][0],gIndicesBunny[i][1],gIndicesBunny[i][2]));
		m_ManualObject->normal(Ogre::Vector3(gIndicesBunny[i+1][0],gIndicesBunny[i+1][1],gIndicesBunny[i+1][2]));
		m_ManualObject->normal(Ogre::Vector3(gIndicesBunny[i+2][0],gIndicesBunny[i+2][1],gIndicesBunny[i+2][2]));

		//m_ManualObject->position(node0->m_x[0], node0->m_x[1], node0->m_x[2]);
		//m_ManualObject->textureCoord(1,0);
		//m_ManualObject->normal(node0->m_n[0], node0->m_n[1], node0->m_n[2]);

		//m_ManualObject->position(node1->m_x[0], node1->m_x[1], node1->m_x[2]);
		//m_ManualObject->textureCoord(0,1);
		//m_ManualObject->normal(node1->m_n[0], node1->m_n[1], node1->m_n[2]);

		//m_ManualObject->position(node2->m_x[0], node2->m_x[1], node2->m_x[2]);
		//m_ManualObject->textureCoord(1,1);
		//m_ManualObject->normal(node2->m_n[0], node2->m_n[1], node2->m_n[2]);

		m_ManualObject->index(i*3);
		m_ManualObject->index(i*3+1);
		m_ManualObject->index(i*3+2);
	}
	// 	m_ManualObject->textureCoord(1,0);
	// 	m_ManualObject->textureCoord(0,0);
	// 	m_ManualObject->textureCoord(0,1);
	// 	m_ManualObject->textureCoord(1,1);
	m_ManualObject->end();

	Ogre::SceneNode* mLiquidBodyNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
	mLiquidBodyNode->attachObject(m_ManualObject);
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
			Ogre::RaySceneQuery* rayQuery=mSceneMgr->createRayQuery(Ogre::Ray());
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
	mCamera->setPosition(0,16,20);
	mCamera->lookAt(0,5,0);

	vp->setCamera(mCamera);
}


void DPS::GUIeventHandler(void)
{
	if(mGUI->Command_Clear_Screen)
	{
		mGUI->Command_Clear_Screen = false;
		deletePhysicsShapes();
		deleteOgreEntities();
		

/*		leapMotionCleanup();*/

// 		dpsHelper->createLeapMotionSphere_0("finger_0",Ogre::Vector3(-12.f,100.0f,0.0f));
// 		dpsHelper->createLeapMotionSphere_1("finger_1",Ogre::Vector3(-9.f,100.0f,0.0f));
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
}


void DPS::miniCamPos(Ogre::Vector3 camPos,Ogre::Vector3 camDir)
{
// 	Ogre::Vector3 absPos = mSceneMgr->getCamera("PlayerCam")->getDerivedPosition();
// 	Ogre::Quaternion absOrient = mSceneMgr->getCamera("PlayerCam")->getDerivedOrientation();

	miniCam->setPosition(Ogre::Vector3(-(camPos.x),camDir.y,-(camPos.z)));
	miniCam->setDirection(camPos);
	miniCam->setNearClipDistance(5);
}


bool DPS::leapMotionInit(void)
{
	leapMotionController.addListener(leapMotionListener);


	dpsHelper->createLeapMotionSphere_0("finger_0",Ogre::Vector3(-12.f,100.0f,0.0f));
 	dpsHelper->createLeapMotionSphere_1("finger_1",Ogre::Vector3(-9.f,100.0f,0.0f));

	return true;
}


void DPS::leapMotionUpdate(void)
{

	if(leapMotionController.isConnected())
	{
		Leap::Frame leapFrameData = leapMotionController.frame();
		Leap::Hand leapHand = leapFrameData.hands().rightmost();

		if (leapFrameData.hands().isEmpty())
		{
			mSceneMgr->getSceneNode("finger_0")->setPosition(Ogre::Vector3(-12.f,100.0f,0.0f));
 			mSceneMgr->getSceneNode("finger_1")->setPosition(Ogre::Vector3(-9.f,100.0f,0.0f));

			btTransform tr0 = dpsHelper->sphereBody_0->getWorldTransform();
			tr0.setOrigin(btVector3(-12.f,100.0f,0.0f));
			dpsHelper->sphereBody_0->applyCentralForce(btVector3(0,0,0));
			dpsHelper->sphereBody_0->setLinearVelocity(btVector3(0,0,0));
			dpsHelper->sphereBody_0->setWorldTransform(tr0);

			btTransform tr1 = dpsHelper->sphereBody_1->getWorldTransform();
			tr1.setOrigin(btVector3(-9.f,100.0f,0.0f));
			dpsHelper->sphereBody_1->applyCentralForce(btVector3(0,0,0));
			dpsHelper->sphereBody_1->setLinearVelocity(btVector3(0,0,0));
			dpsHelper->sphereBody_1->setWorldTransform(tr1);
		}
		else
		{
			Leap::Finger finger0 = leapHand.fingers()[0];
			Leap::Vector fp0 = finger0.tipPosition();
			mSceneMgr->getSceneNode("finger_0")->setPosition(Ogre::Vector3(fp0.x * 0.5,(fp0.y-100) * 0.5,fp0.z * 0.5));
			Ogre::Vector3 absPos0 = dpsHelper->sphereNode_0->_getDerivedPosition();
			Ogre::Quaternion absQuat0 = dpsHelper->sphereNode_0->_getDerivedOrientation();

			btTransform tr0 = dpsHelper->sphereBody_0->getWorldTransform();
			tr0.setOrigin(btVector3(absPos0.x,absPos0.y,absPos0.z));
			tr0.setRotation(btQuaternion(absQuat0.x, absQuat0.y, absQuat0.z, absQuat0.w));
			dpsHelper->sphereBody_0->applyCentralForce(btVector3(0,0,0));
			dpsHelper->sphereBody_0->setLinearVelocity(btVector3(0,0,0));
			dpsHelper->sphereBody_0->activate(true);
			dpsHelper->sphereBody_0->setWorldTransform(tr0);



			Leap::Finger finger1 = leapHand.fingers()[1];
			Leap::Vector fp1 = finger1.tipPosition();
			mSceneMgr->getSceneNode("finger_1")->setPosition(Ogre::Vector3(fp1.x * 0.5,(fp1.y-100) * 0.5,fp1.z * 0.5));
			Ogre::Vector3 absPos1 = dpsHelper->sphereNode_1->_getDerivedPosition();
			Ogre::Quaternion absQuat1 = dpsHelper->sphereNode_1->_getDerivedOrientation();

			btTransform tr1 = dpsHelper->sphereBody_1->getWorldTransform();
			tr1.setOrigin(btVector3(absPos1.x,absPos1.y,absPos1.z));
			tr1.setRotation(btQuaternion(absQuat1.x, absQuat1.y, absQuat1.z, absQuat1.w));
			dpsHelper->sphereBody_1->applyCentralForce(btVector3(0,0,0));
			dpsHelper->sphereBody_1->setLinearVelocity(btVector3(0,0,0));
			dpsHelper->sphereBody_1->activate(true);
			dpsHelper->sphereBody_1->setWorldTransform(tr1);
		}
	}
}


void DPS::leapMotionCleanup(void)
{
	leapMotionController.removeListener(leapMotionListener);
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