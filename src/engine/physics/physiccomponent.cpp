/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/physics/physiccomponent.hpp>
#include <engine/physics/physiccomponent_impl.hpp>
#include <engine/physics/physics_detail.hpp>
#include <engine/physics/physicsys_impl.hpp>

#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <math/mathtools.hpp>

namespace eXl
{

  //IMPLEMENT_WEAKREF(PhysicComponent);
  //
  //PhysicComponent::PhysicComponent(PhysicComponent_Impl* iImpl)
  //  : m_Impl(iImpl)
  //{
  //  //AddProperty(GetPositionProperty(), TypeFieldName("PhysicComponent::Position"), &PhysicComponent::m_Position);
  //}
  //
  //PhysicComponent::~PhysicComponent()
  //{
  //
  //}

  KinematicController::KinematicController()
  {

  }

  KinematicController::~KinematicController()
  {
    //if(m_Impl)
    //{
    //  if(m_Impl->m_Controller)
    //  {
    //    m_Impl->m_Controller->SetItf(nullptr);
    //  }
    //}
  }

  bool KinematicController::SweepTest(PhysicComponent_Impl& iComp, Vector3f const& iFrom, Vector3f const& iTo, CollisionData& oRes, std::function<bool(PhysicComponent_Impl*)> const& iIgnore, uint16_t iMask)
  {
    return iComp.SweepTest(iFrom, iTo, oRes, iIgnore, iMask);
  }

  Vector3f KinematicController::GetPosition(PhysicComponent_Impl& iComp) const
  {
    btTransform trans = iComp.m_Object->getWorldTransform();

    return FROM_BTVECT(trans.getOrigin());
  }

  bool KinematicController::IsControlled(PhysicComponent_Impl& iComp)
  {
    return iComp.IsControlledBy(this);
  }

  //void KinematicController::SetTransform(PhysicComponent_Impl& iComp, Quaternionf const& iOrient, Vector3f const& iPos)
  void KinematicController::ApplyLinearVelocity(PhysicComponent_Impl& iComp, Vector3f const& iLinVel, float iTimeStep)
  {
    btVector3 linVel = TO_BTVECT(iLinVel);
    btRigidBody* body = btRigidBody::upcast(iComp.m_Object);
    if (body && (iComp.GetFlags() & PhysicFlags::Kinematic) == 0)
    {
      btVector3 currentVelocity = body->getLinearVelocity();

      body->activate();
      body->applyCentralImpulse((linVel - currentVelocity));
    }
    else
    {
      btTransform trans = iComp.m_Object->getWorldTransform();

      trans.setOrigin(trans.getOrigin() + linVel * iTimeStep);

      if ((iComp.GetFlags() & PhysicFlags::AlignRotToVelocity) && !linVel.isZero())
      {
        btMatrix3x3 rotMat;
        rotMat[0] = linVel;
        rotMat[0].normalize();
        rotMat[2] = btVector3(0.0, 0.0 ,1.0);
        rotMat[1] = rotMat[2].cross(rotMat[0]);
        rotMat = rotMat.transpose();

        btQuaternion quat;
        //quat.setRotation(btVector3(0.0, 0.0, 1.0), MathTools::GetAngleFromVec(MathTools::As2DVec(iLinVel)));
        rotMat.getRotation(quat);
        trans.setRotation(quat);
      }
      iComp.m_Object->setWorldTransform(trans);
      iComp.m_Object->setInterpolationWorldTransform(trans);
      //if (iComp.GetFlags() & PhysicFlags::IsGhost)
      {
        iComp.setWorldTransform(trans);
      }

      //btCollisionWorld* collisionWorld = iComp.m_System.GetImpl().m_dynamicsWorld;
      //btCollisionShape* shape = iComp.m_Object->getCollisionShape();
      //btVector3 minAabb, maxAabb;
      //shape->getAabb(trans, minAabb, maxAabb);
      //collisionWorld->getBroadphase()->setAabb(iComp.m_Object->getBroadphaseHandle(),
      //  minAabb,
      //  maxAabb,
      //  collisionWorld->getDispatcher());

    }
  }

  bool KinematicController::RecoverFromPenetration(PhysicComponent_Impl& iComp, Vector3f& OutNewPos)
  {
    btCollisionShape* shape = iComp.m_Shape;
    btPairCachingGhostObject* sensor = iComp.m_Sensor;
    if (!sensor)
    {
      return false;
    }

    PhysicsSystem_Impl& phSysImpl = iComp.m_System.GetImpl();
    btCollisionWorld* collisionWorld = phSysImpl.m_dynamicsWorld;

    // Here we must refresh the overlapping paircache as the penetrating movement itself or the
    // previous recovery iteration might have used setWorldTransform and pushed us into an object
    // that is not in the previous cache contents from the last timestep, as will happen if we
    // are pushed into a new AABB overlap. Unhandled this means the next convex sweep gets stuck.
    //
    // Do this by calling the broadphase's setAabb with the moved AABB, this will update the broadphase
    // paircache and the ghostobject's internal paircache at the same time.    /BW

    btVector3 minAabb, maxAabb;
    shape->getAabb(sensor->getWorldTransform(), minAabb, maxAabb);
    collisionWorld->getBroadphase()->setAabb(sensor->getBroadphaseHandle(),
      minAabb,
      maxAabb,
      collisionWorld->getDispatcher());

    bool penetration = false;
    bool neededCorrection = false;

    collisionWorld->getDispatcher()->dispatchAllCollisionPairs(sensor->getOverlappingPairCache(), collisionWorld->getDispatchInfo(), collisionWorld->getDispatcher());

    btVector3 currentPosition = sensor->getWorldTransform().getOrigin();
    OutNewPos = FROM_BTVECT(currentPosition);
    btVector3 touchingNormal;
    // --> may need caching
    btManifoldArray manifoldArray;

    btScalar maxPen = btScalar(0.0);
    for (int i = 0; i < sensor->getOverlappingPairCache()->getNumOverlappingPairs(); i++)
    {
      manifoldArray.resize(0);

      btBroadphasePair* collisionPair = &sensor->getOverlappingPairCache()->getOverlappingPairArray()[i];

      btCollisionObject* obj0 = static_cast<btCollisionObject*>(collisionPair->m_pProxy0->m_clientObject);
      btCollisionObject* obj1 = static_cast<btCollisionObject*>(collisionPair->m_pProxy1->m_clientObject);

      if (obj0->getUserPointer() == obj1->getUserPointer())
      {
        continue;
      }

      //if ((obj0 && !obj0->hasContactResponse()) || (obj1 && !obj1->hasContactResponse()))
      //  continue;

      if (collisionPair->m_algorithm)
        collisionPair->m_algorithm->getAllContactManifolds(manifoldArray);

      for (int j = 0; j < manifoldArray.size(); j++)
      {
        btPersistentManifold* manifold = manifoldArray[j];
        btScalar directionSign = manifold->getBody0() == sensor ? btScalar(-1.0) : btScalar(1.0);
        for (int p = 0; p < manifold->getNumContacts(); p++)
        {
          const btManifoldPoint&pt = manifold->getContactPoint(p);

          btScalar dist = pt.getDistance();

          if (dist < 0.0)
          {
            if (dist < maxPen)
            {
              maxPen = dist;
              touchingNormal = pt.m_normalWorldOnB * directionSign;//??

            }
            neededCorrection = true;
            currentPosition += pt.m_normalWorldOnB * directionSign * dist * btScalar(0.5);
            penetration = true;
          }
          else {
            //printf("touching %f\n", dist);
          }
        }

        //manifold->clearManifold();
      }
    }
    if (neededCorrection)
    {
      btTransform newTrans = sensor->getWorldTransform();
      newTrans.setOrigin(currentPosition);
      //m_ghostObject->setWorldTransform(newTrans);
      //	printf("m_touchingNormal = %f,%f,%f\n",m_touchingNormal[0],m_touchingNormal[1],m_touchingNormal[2]);
      OutNewPos = FROM_BTVECT(currentPosition);
      sensor->setWorldTransform(newTrans);
      //iComp.m_Object->setWorldTransform(newTrans);
    }
    return penetration;
  }
}