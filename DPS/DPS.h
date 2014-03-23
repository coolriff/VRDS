﻿#ifndef __DPS_h_
#define __DPS_h_

#include "BaseApplication.h"
#include "DPSHelper.h"
#include "DPSSoftBodyHelper.h"
#include "BtOgrePG.h"
#include "BtOgreGP.h"
#include "BtOgreExtras.h"
#include "ExampleApplication.h"
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <BulletSoftBody/btDefaultSoftBodySolver.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include "Leap.h"
#include "LeapListener.h"

class DPS;
class btBroadphaseInterface;
class btCollisionShape;
class btOverlappingPairCache;
class btCollisionDispatcher;
class btConstraintSolver;
struct btCollisionAlgorithmCreateFunc;
class btDefaultCollisionConfiguration;

///collisions between two btSoftBody's
class btSoftSoftCollisionAlgorithm;

///collisions between a btSoftBody and a btRigidBody
class btSoftRididCollisionAlgorithm;
class btSoftRigidDynamicsWorld;


namespace Globals
{
	DPS* app;
	btSoftRigidDynamicsWorld* phyWorld;
    BtOgre::DebugDrawer* dbgdraw;
}

class DPS : public BaseApplication
{
	public:

		DPS(void);
		~DPS(void);

		// No CCD (continuous collision detection) for Gimpact shapes
		struct MyClosestRayResultCallback : public btCollisionWorld::ClosestRayResultCallback
		{
			const btCollisionShape * m_hitTriangleShape;
			int					  m_hitTriangleIndex;
			int					  m_hitShapePart;

			MyClosestRayResultCallback (const btVector3 & rayFrom,const btVector3 & rayTo)
				: btCollisionWorld::ClosestRayResultCallback(rayFrom, rayTo),
				m_hitTriangleShape(NULL),
				m_hitTriangleIndex(0),
				m_hitShapePart(0)
			{
			}

			virtual ~MyClosestRayResultCallback()
			{
			}

			virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult & rayResult, bool normalInWorldSpace)
			{
				if (rayResult.m_localShapeInfo)
				{
					m_hitTriangleShape = rayResult.m_collisionObject->getCollisionShape();
					m_hitTriangleIndex = rayResult.m_localShapeInfo->m_triangleIndex;
					m_hitShapePart = rayResult.m_localShapeInfo->m_shapePart;
				} else 
				{
					m_hitTriangleShape = NULL;
					m_hitTriangleIndex = 0;
					m_hitShapePart = 0;
				}
				return ClosestRayResultCallback::addSingleResult(rayResult,normalInWorldSpace);
			}
		};

		btAlignedObjectArray<btSoftSoftCollisionAlgorithm*> m_SoftSoftCollisionAlgorithms;
		btAlignedObjectArray<btSoftRididCollisionAlgorithm*> m_SoftRigidCollisionAlgorithms;
		btSoftBodyWorldInfo	m_softBodyWorldInfo;
		//keep the collision shapes, for deletion/cleanup
		btAlignedObjectArray<btCollisionShape*>		m_collisionShapes;
		btBroadphaseInterface*	m_broadphase;
		btCollisionDispatcher*	m_dispatcher;
		btConstraintSolver*	m_solver;
		btCollisionAlgorithmCreateFunc*	m_boxBoxCF;
		btDefaultCollisionConfiguration* m_collisionConfiguration;

		void initPhysics(void);
		void exitPhysics(void);
		void resetCamera(void);
		void deleteOgreEntities(void);
		void deletePhysicsShapes(void);
		void GimpactRayCallBack(void);
		void createGimpactBarrel(void);
		void createGimpactBuuny(void);
		void createGimpactTorus(void);
		bool process_triangle(btCollisionShape * shape, int hitTriangleIndex);
		void GUIeventHandler(void);
		void miniCamPos(Ogre::Vector3 camPos,Ogre::Vector3 camDir);

		Ogre::FrameEvent evt;
		double dt;
		int leapMotionCounter;

		LeapListener leapMotionListener;
		Leap::Controller leapMotionController;
		Leap::Frame leapFrameData;
		Leap::Hand leapHand;

		bool leapMotionInit(void);
		void leapMotionUpdate(void);
		void leapMotionCleanup(void);

	protected:

		Ogre::ManualObject* m_ManualObject;
// 		Ogre;;ManualObject* barrelManualObject;
// 		Ogre;;ManualObject* bunnyManualObject;

		std::shared_ptr<DPSHelper> dpsHelper;
		std::shared_ptr<DPSSoftBodyHelper> dpsSoftbodyHelper;

		virtual void createScene(void);
		virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt);

		// OIS::MouseListener
		virtual bool mouseMoved(const OIS::MouseEvent &arg);
		virtual bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
		virtual bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);

		void initSoftBody(btSoftBody* body);
		void updateSoftBody(btSoftBody* body);
// 		void initRigidBody(btRigidBody* body);
// 		void updateRigidBody(btRigidBody* body);
		bool keyPressed(const OIS::KeyEvent &arg);

};



#endif