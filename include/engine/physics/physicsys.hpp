/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/enginelib.hpp>
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

    void RayQuery(List<CollisionData>& oRes, const Vec3& iOrig,const Vec3& iEnd,unsigned int maxEnt=0,unsigned short category = 1,unsigned short mask=-1L);

    void SphereQuery(List<CollisionData>& oRes, float iRadius,const Vec3& iOrig,unsigned int maxEnt=0,unsigned short category = 1,unsigned short mask=-1);

    void CylinderQuery(List<CollisionData>& oRes, float iRay,float iH,const Vec3& iPos,const Quaternion& iOrient=Identity<Quaternion>(),unsigned int maxEnt=0,unsigned short category = 1,unsigned short mask=-1);

    void BoxQuery(List<CollisionData>& oRes, const Vec3& iDim,const Vec3& iPos,const Quaternion& iOrient=Identity<Quaternion>(),unsigned int maxEnt=0,unsigned short category = 1,unsigned short mask=-1);

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

    Vector<Mat4> m_MovedTransform;
    Vector<ObjectHandle> m_MovedObject;

    Vector<CollisionData> m_LastCollisions;

    BulletDebugDraw* m_DebugDrawer = nullptr;
  };
}