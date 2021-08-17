#pragma once

#include <engine/enginelib.hpp>
#include "physiccomponent.hpp"
#include "physicsys.hpp"
#include <engine/common/transforms.hpp>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

#include <boost/scoped_ptr.hpp>
#include <boost/container/small_vector.hpp>

class btPairCachingGhostObject;

namespace eXl
{
  class PhysicsSystem;

  class PhysicInternalActionInterface : public btActionInterface, public HeapObject
  {
  public:
    PhysicInternalActionInterface(KinematicController* iItf, PhysicsSystem* iSys)
      : m_ClItf(iItf)
      , m_Host(iSys)
    {
    }
    ~PhysicInternalActionInterface()
    {
    }

    void SetItf(KinematicController* iItf)
    {
      m_ClItf = iItf;
    }

  protected:

    virtual void updateAction(btCollisionWorld* collisionWorld, btScalar deltaTimeStep)
    {
      if(m_ClItf)
      {
        m_ClItf->Step(m_Host,deltaTimeStep);
      }
    }

    void debugDraw(btIDebugDraw *){}
    KinematicController* m_ClItf;
    PhysicsSystem* m_Host;
  };

  struct PhysicComponent_Impl : btMotionState, HeapObject
  {
    DECLARE_RefC;
  public:
    PhysicComponent_Impl(PhysicsSystem& iSys) 
      : m_System(iSys)
    {}
    ~PhysicComponent_Impl();

    void Build(ObjectHandle iObject, PhysicInitData& iInitData);

    inline unsigned int GetFlags() const {return m_InitData->GetFlags();}

    inline btCollisionObject* GetObject() const {return m_Object;}

    void getWorldTransform(btTransform &worldTrans) const override;

    void setWorldTransform(const btTransform &worldTrans) override;

    bool SweepTest(Vector3f const& iFrom, Vector3f const& iTo, CollisionData& oRes, std::function<bool(PhysicComponent_Impl*)> const& iIgnore, uint16_t iMask);

    void OnNullRefC() const;

    bool IsEnabled() const { return m_Enabled; }

    void SetEnabled(bool iEnabled) { m_Enabled = iEnabled; }

    void PushController(KinematicController* iController)
    {
      m_ControllerStack.push_back(iController);
    }

    void PopController(KinematicController* iController)
    {
      eXl_ASSERT(m_ControllerStack.back() == iController);
      m_ControllerStack.pop_back();
    }

    bool IsControlledBy(KinematicController* iController)
    {
      return !m_ControllerStack.empty() && m_ControllerStack.back() == iController;
    }

    ObjectHandle m_ObjectId;

    btCollisionObject* m_Object = nullptr;
    btPairCachingGhostObject* m_Sensor = nullptr;
    btCollisionShape* m_Shape = nullptr;
    btTransform m_CachedTransform;
    PhysicInitData* m_InitData;
    //unsigned int m_Flags = 0;
    //uint32_t m_GroupMask;
    float m_Mass = 0.0;

    PhysicsSystem& m_System;
    SmallVector<KinematicController*, 1> m_ControllerStack;
    IntrusivePtr<PhysicComponent_Impl> m_ShapeHolder;
    bool m_MovedLastFrame = false;
    bool m_Enabled = true;
  };
}