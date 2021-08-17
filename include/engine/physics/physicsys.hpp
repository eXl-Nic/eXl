#pragma once

#include <engine/enginelib.hpp>
#include <math/vector3.hpp>
#include <math/quaternion.hpp>
#include <core/heapobject.hpp>
#include <engine/common/world.hpp>
#include <engine/common/transforms.hpp>
#include <engine/common/neighbours.hpp>

#include "physicsdef.hpp"

namespace eXl
{
  struct PhysicComponent_Impl;
  struct PhysicsSystem_Impl;
  class CollisionComponent;
  class KinematicController;

  namespace DebugTool
  {
    class Drawer;
  }

  class BulletDebugDraw;

  class EXL_ENGINE_API PhysicsSystem : public ComponentManager
  {
    DECLARE_RTTI(PhysicsSystem, ComponentManager);
  public:

    PhysicsSystem(Transforms& iTransforms);
    ~PhysicsSystem();

    void RayQuery(List<CollisionData>& oRes, const Vector3f& iOrig,const Vector3f& iEnd,unsigned int maxEnt=0,unsigned short category = 1,unsigned short mask=-1L);

    void SphereQuery(List<CollisionData>& oRes, float iRadius,const Vector3f& iOrig,unsigned int maxEnt=0,unsigned short category = 1,unsigned short mask=-1);

    void CylinderQuery(List<CollisionData>& oRes, float iRay,float iH,const Vector3f& iPos,const Quaternionf& iOrient=Quaternionf::IDENTITY,unsigned int maxEnt=0,unsigned short category = 1,unsigned short mask=-1);

    void BoxQuery(List<CollisionData>& oRes, const Vector3f& iDim,const Vector3f& iPos,const Quaternionf& iOrient=Quaternionf::IDENTITY,unsigned int maxEnt=0,unsigned short category = 1,unsigned short mask=-1);

    void Step(float iTime);

    void SyncTriggersTransforms();

    void AddTrigger(ObjectHandle iObj, TriggerDef const& iDef, TriggerCallbackHandle iCallback, ContactFilterCallback iFilter = ContactFilterCallback());

    TriggerCallbackHandle AddTriggerCallback(std::unique_ptr<TriggerCallback> iCallback);

    /*PhysicComponent**/Err CreateComponent(ObjectHandle iTransform, PhysicInitData const& iInit);

    PhysicComponent_Impl* GetCompImpl(ObjectHandle iObj);

    void SetComponentEnabled(ObjectHandle iObj, bool iEnabled);

    void DeleteComponent(ObjectHandle) override;

    void AddContactCb(ObjectHandle iObj, ContactFilterCallback iCb);

    void RemoveContactCb(ObjectHandle iObj);

    //Err AddCollisionDispatcher(PhysicComponent& iComp, CollisionComponent* iDispatcher);

    Err AddKinematicController(KinematicController* iController);

    Vector<CollisionData> const& GetLastCollisions() const { return m_LastCollisions; }

    Transforms& GetTransforms() const { return m_Transforms; }

    void EnableDebugDraw(DebugTool::Drawer& iDrawer);

    void DisableDebugDraw();

    NeighborhoodExtraction& GetNeighborhoodExtraction() const;

    PhysicsSystem_Impl& GetImpl() const {return *m_Impl;}

  protected:

    PhysicsSystem(PhysicsSystem const&) = delete;
    PhysicsSystem& operator=(PhysicsSystem const&) = delete;

    friend struct PhysicComponent_Impl;

    PhysicsSystem_Impl* m_Impl;
    Transforms& m_Transforms;

    Vector<IntrusivePtr<PhysicComponent_Impl>> m_Components;

    Vector<Matrix4f> m_MovedTransform;
    Vector<ObjectHandle> m_MovedObject;

    Vector<CollisionData> m_LastCollisions;

    BulletDebugDraw* m_DebugDrawer = nullptr;
  };
}