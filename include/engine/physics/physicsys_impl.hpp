#pragma once

#include <engine/enginelib.hpp>
#include "physicsdef.hpp"
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/BroadphaseCollision/btDbvt.h>
#include "physicsys.hpp"
#include "trigger.hpp"

struct btDbvtBroadphase;

namespace eXl
{
  struct PhysicComponent_Impl;

  struct NeighborhoodExtractionImpl : public NeighborhoodExtraction
  {
    NeighborhoodExtractionImpl(PhysicsSystem& iSys) : m_Sys(iSys) {}

    void AddObject(ObjectHandle iObj, float iRadius, bool iPopulate) override;

    void AddObject(ObjectHandle iObj, Vector3f const& iBoxDim, bool iPopulateNeigh) override;

    void Remove(ObjectHandle iObj, PhysicComponent_Impl* iPhObj);

    void Run(Vector3f const& iForwardOffset, float iRadiusSearch) override;

    void Run(btDbvt& iTree, Vector3f const& iForwardOffset, float iRadiusSearch);

    UnorderedMap<void*, uint32_t> m_UsrDataToNum;

    btAlignedObjectArray<btDbvtVolume> m_Volumes;
    btAlignedObjectArray<btTransform> m_Trans;
    PhysicsSystem& m_Sys;
  };

  struct ShapesCache
  {
    btCollisionShape* MakeGeom(const GeomDef& iDef, btCompoundShape* iCont);
    btCollisionShape* BuildCollisionShape(PhysicInitData const& iData);

    void ReleaseShape(btCollisionShape* iShape);

    UnorderedMap<GeomDef, btCollisionShape*> m_Shapes;
  };

  class eXlCustomDispatcher;

  struct PhysicsSystem_Impl : public HeapObject
  {
  public:

    PhysicsSystem_Impl(PhysicsSystem& iSys);

    //BulletRootComp                          m_Comp;
    btAlignedObjectArray<btCollisionShape*> m_collisionShapes;
    btDbvtBroadphase*                       m_broadphase;
    eXlCustomDispatcher*                    m_dispatcher;
    btConstraintSolver*                     m_solver;
    btDefaultCollisionConfiguration*        m_collisionConfiguration;
    btDynamicsWorld*                        m_dynamicsWorld;

    NeighborhoodExtractionImpl              m_NeighExtraction;

    UnorderedSet<IntrusivePtr<PhysicComponent_Impl>> m_ToDelete;
    PhysicsSystem& m_HostSys;

    ShapesCache m_ShapesCache;
    TriggerManager m_Triggers;

    unsigned int m_DebugDrawHandle;

    void AddContactCb(ObjectHandle iObj, ContactFilterCallback&& iCb);
    void RemoveContactCb(ObjectHandle iObj);

    bool SweepTest(PhysicComponent_Impl* iShape, Vector3f const& iFrom, Vector3f const& iTo, CollisionData& oRes, std::function<bool(PhysicComponent_Impl*)> const& iIgnore, uint16_t iMask);

    void HandleColQuery(const GeomDef& iDef, unsigned int iMax, unsigned int iCat, List<CollisionData>& oRes);

    // Remove everything in m_toDelete;
    void Cleanup();

    void Step(float iTime);
  };
}