#include "DPS.h"
#include <vector>
#include <iostream>
#include <fstream> 
#include <string> 
#include "BulletCollision/Gimpact/btGImpactShape.h"
#include "BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h"
#include "Mesh/barrel.h"
#include "Mesh/BunnyMesh.h"
//#include <memory>

const int maxProxies = 32766;

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

	//phyWorld->setInternalTickCallback(pickingPreTickCallback,this,true);
	Globals::phyWorld->getDispatchInfo().m_enableSPU = true;
	Globals::phyWorld->setGravity(btVector3(0,-10,0));
	m_softBodyWorldInfo.m_gravity.setValue(0,-10,0);
	//	clientResetScene();
	m_softBodyWorldInfo.m_sparsesdf.Initialize();
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
	// Basic Ogre stuff.
	mSceneMgr->setAmbientLight(ColourValue(0.9f,0.9f,0.9f));
	mCamera->setPosition(Vector3(0,20,20));
	mCamera->lookAt(Vector3(0,20,0));
	mCamera->setNearClipDistance(0.05f);
	//LogManager::getSingleton().setLogDetail(LL_BOREME);

	dpsHelper = std::make_shared<DPSHelper>(Globals::phyWorld, mCamera, mSceneMgr);
	dpsSoftbodyHelper = std::make_shared<DPSSoftBodyHelper>(Globals::phyWorld, mCamera, mSceneMgr);

	dpsHelper->createGround();

	// Main light in scene
	dpsHelper->createDirectionLight("mainLight",Ogre::Vector3(60,180,100),Ogre::Vector3(-60,-80,-100));
	dpsHelper->createDirectionLight("mainLight1",Ogre::Vector3(0,200,0),Ogre::Vector3(0,0,0));

	// Debug drawing
	Globals::dbgdraw = new BtOgre::DebugDrawer(mSceneMgr->getRootSceneNode(), Globals::phyWorld);
	Globals::phyWorld->setDebugDrawer(Globals::dbgdraw);

	mSceneMgr->setSkyBox(true, "Examples/SpaceSkyBox");

	//initSoftBody(dpsSoftbodyHelper->createSoftBody(btVector3(0,20,0)));
	//initSoftBody(dpsSoftbodyHelper->createCloth());
	//initSoftBody(dpsSoftbodyHelper->createDeformableModel());
	//initSoftBody(dpsSoftbodyHelper->createBunny());

	btTransform tr;
	tr.setIdentity();
	tr.setOrigin(btVector3(0,1.0f,20.0f));

	btTriangleIndexVertexArray * mesh_interface = new btTriangleIndexVertexArray(
		faces_size,
		barrel_ia,
		3*sizeof(int),
		vertices_size,
		(btScalar *) &barrel_va[0][0],
		sizeof(btVector3));

	// 	btTriangleIndexVertexArray * mesh_interface = new btTriangleIndexVertexArray(
	// 		BUNNY_NUM_TRIANGLES,
	// 		&gIndicesBunny[0][0],
	// 		3*sizeof(int),
	// 		BUNNY_NUM_VERTICES,
	// 		(btScalar *) &gVerticesBunny[0],
	// 		sizeof(btScalar)*3);

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
	//body->translate(btVector3(0,0,-50));
	Globals::phyWorld->addRigidBody(body);



// 	m_ManualObject = mSceneMgr->createManualObject("barrel");
// 
// 
// 	//btSoftBody::tNodeArray& nodes(body->m_nodes);
// 	//btSoftBody::tFaceArray& faces(body->m_faces);
// 
// 	m_ManualObject->estimateVertexCount(BUNNY_NUM_VERTICES);
// 	m_ManualObject->estimateIndexCount(BUNNY_NUM_INDICES);
// 
// 	m_ManualObject->begin("BaseWhiteNoLighting", Ogre::RenderOperation::OT_TRIANGLE_LIST);
// 
// 	//btSoftBody::Node *node0 = 0, *node1 = 0, *node2 = 0;
// 	int count = 0;
// 	for (int i = 0; i < BUNNY_NUM_TRIANGLES; i+=9)
// 	{
// 		//node0 = faces[i].m_n[0];
// 		//node1 = faces[i].m_n[1];
// 		//node2 = faces[i].m_n[2];
// 
// 		m_ManualObject->position(Ogre::Vector3(gVerticesBunny[i],gVerticesBunny[i+1],gVerticesBunny[i+2]));
// 		m_ManualObject->position(Ogre::Vector3(gVerticesBunny[i+3],gVerticesBunny[i+4],gVerticesBunny[i+5]));
// 		m_ManualObject->position(Ogre::Vector3(gVerticesBunny[i+6],gVerticesBunny[i+7],gVerticesBunny[i+8]));
// 
// 		m_ManualObject->normal(Ogre::Vector3(gIndicesBunny[i][0],gIndicesBunny[i][1],gIndicesBunny[i][2]));
// 		m_ManualObject->normal(Ogre::Vector3(gIndicesBunny[i+1][0],gIndicesBunny[i+1][1],gIndicesBunny[i+1][2]));
// 		m_ManualObject->normal(Ogre::Vector3(gIndicesBunny[i+2][0],gIndicesBunny[i+2][1],gIndicesBunny[i+2][2]));
// 
// 		//m_ManualObject->position(node0->m_x[0], node0->m_x[1], node0->m_x[2]);
// 		//m_ManualObject->textureCoord(1,0);
// 		//m_ManualObject->normal(node0->m_n[0], node0->m_n[1], node0->m_n[2]);
// 
// 		//m_ManualObject->position(node1->m_x[0], node1->m_x[1], node1->m_x[2]);
// 		//m_ManualObject->textureCoord(0,1);
// 		//m_ManualObject->normal(node1->m_n[0], node1->m_n[1], node1->m_n[2]);
// 
// 		//m_ManualObject->position(node2->m_x[0], node2->m_x[1], node2->m_x[2]);
// 		//m_ManualObject->textureCoord(1,1);
// 		//m_ManualObject->normal(node2->m_n[0], node2->m_n[1], node2->m_n[2]);
// 
// 		m_ManualObject->index(i*3);
// 		m_ManualObject->index(i*3+1);
// 		m_ManualObject->index(i*3+2);
// 	}
// 	// 	m_ManualObject->textureCoord(1,0);
// 	// 	m_ManualObject->textureCoord(0,0);
// 	// 	m_ManualObject->textureCoord(0,1);
// 	// 	m_ManualObject->textureCoord(1,1);
// 	m_ManualObject->end();
// 	m_ManualObject->setDynamic(true);
// 
// 	Ogre::SceneNode* mLiquidBodyNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
// 	mLiquidBodyNode->attachObject(m_ManualObject);
}


bool DPS::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
	//Update Bullet world
	Globals::phyWorld->stepSimulation(evt.timeSinceLastFrame, 10); 
	Globals::phyWorld->debugDrawWorld();

	//Shows debug if F3 key down.
	Globals::dbgdraw->setDebugMode(mKeyboard->isKeyDown(OIS::KC_F3));
	Globals::dbgdraw->step();

	//Globals::app->updateSoftBody(dpsSoftbodyHelper->m_cloth);
	//Globals::app->updateSoftBody(dpsSoftbodyHelper->m_SoftBody);
	//Globals::app->updateSoftBody(dpsSoftbodyHelper->m_deformableModel);
	//Globals::app->updateSoftBody(dpsSoftbodyHelper->m_bunny);

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
	return BaseApplication::keyPressed(arg);
}


void DPS::initSoftBody(btSoftBody* body)
{
	//manual objects are used to generate new meshes based on raw vertex data
	//this is used for the liquid form
	m_ManualObject = mSceneMgr->createManualObject("liquidBody");


	btSoftBody::tNodeArray& nodes(body->m_nodes);
	btSoftBody::tFaceArray& faces(body->m_faces);

	m_ManualObject->estimateVertexCount(faces.size()*3);
	m_ManualObject->estimateIndexCount(faces.size()*3);

	m_ManualObject->begin("CharacterMaterials/LiquidBody", Ogre::RenderOperation::OT_TRIANGLE_LIST);

	//btSoftBody::Node *node0 = 0, *node1 = 0, *node2 = 0;

	//http://www.ogre3d.org/tikiwiki/ManualObject
	for (int i = 0; i < faces.size(); ++i)
	{
		//node0 = faces[i].m_n[0];
		//node1 = faces[i].m_n[1];
		//node2 = faces[i].m_n[2];
		
		m_ManualObject->position(body->m_faces[i].m_n[0]->m_x[0],body->m_faces[i].m_n[0]->m_x[1],body->m_faces[i].m_n[0]->m_x[2]);
		m_ManualObject->position(body->m_faces[i].m_n[1]->m_x[0],body->m_faces[i].m_n[1]->m_x[1],body->m_faces[i].m_n[1]->m_x[2]);
		m_ManualObject->position(body->m_faces[i].m_n[2]->m_x[0],body->m_faces[i].m_n[2]->m_x[1],body->m_faces[i].m_n[2]->m_x[2]);

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
		//m_ManualObject->triangle(i*3+1);
		//m_ManualObject->triangle(i*3+2);
	}
// 	m_ManualObject->textureCoord(1,0);
// 	m_ManualObject->textureCoord(0,0);
// 	m_ManualObject->textureCoord(0,1);
// 	m_ManualObject->textureCoord(1,1);
	m_ManualObject->end();
	m_ManualObject->setDynamic(true);

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
		m_ManualObject->position(body->m_faces[i].m_n[0]->m_x[0],body->m_faces[i].m_n[0]->m_x[1],body->m_faces[i].m_n[0]->m_x[2]);
		m_ManualObject->position(body->m_faces[i].m_n[1]->m_x[0],body->m_faces[i].m_n[1]->m_x[1],body->m_faces[i].m_n[1]->m_x[2]);
		m_ManualObject->position(body->m_faces[i].m_n[2]->m_x[0],body->m_faces[i].m_n[2]->m_x[1],body->m_faces[i].m_n[2]->m_x[2]);

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
	m_ManualObject->setCastShadows(true);
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