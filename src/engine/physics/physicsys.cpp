/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/physics/physicsys.hpp>
#include <engine/physics/physiccomponent.hpp>
#include <engine/physics/physics_detail.hpp>
#include <engine/physics/physicsys_impl.hpp>
#include <engine/physics/physiccomponent_impl.hpp>

#include <core/coredef.hpp>
#include <core/log.hpp>

#include <math/mathtools.hpp>

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#ifndef WIN32 //Fix crado en attendant de synchroniser les versions de bullet.
#include <BulletCollision/CollisionDispatch/btCollisionObjectWrapper.h>
#endif

#include <engine/common/debugtool.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(PhysicsSystem);

  PhysicsSystem::PhysicsSystem(Transforms& iTransforms)
    : m_Impl(eXl_NEW PhysicsSystem_Impl(*this))
    , m_Transforms(iTransforms)
  {
  }

  PhysicsSystem::~PhysicsSystem()
  {}

  class BulletDebugDraw : public btIDebugDraw, public HeapObject
  {
  public:
    BulletDebugDraw():
      m_debugMode(btIDebugDraw::DBG_DrawAabb | btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawContactPoints)
    {

    }

    DebugTool::Drawer* m_Drawer;

  protected:
    int m_debugMode;

    virtual void	drawLine(const btVector3& from,const btVector3& to,const btVector3& color)
    {
      Vector4f compColor(color.x(), color.y(), color.z(), 1.0);
      m_Drawer->DrawLine(FROM_BTVECT(from), FROM_BTVECT(to), compColor);
    }

    virtual void	drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color)
    {
      Vector4f compColor(color.x(), color.y(), color.z(), 1.0);
      m_Drawer->DrawLine(FROM_BTVECT(PointOnB) + Vector3f(-1.0, -1.0 , 0.0), FROM_BTVECT(PointOnB) + Vector3f(1.0,  1.0 , 0.0), compColor, true);
      m_Drawer->DrawLine(FROM_BTVECT(PointOnB) + Vector3f(-1.0,  1.0 , 0.0), FROM_BTVECT(PointOnB) + Vector3f(1.0, -1.0 , 0.0), compColor, true);
    }

    virtual void	drawBox (const btVector3& boxMin, const btVector3& boxMax, const btVector3& color, btScalar alpha)
    {
      Vector4f compColor(color.x(), color.y(), color.z(), alpha);

      Vector3f boxPts[8] = {
        Vector3f(boxMin.x(), boxMin.y(), boxMin.z()),
        Vector3f(boxMin.x(), boxMin.y(), boxMax.z()),
        Vector3f(boxMin.x(), boxMax.y(), boxMin.z()),
        Vector3f(boxMin.x(), boxMax.y(), boxMax.z()),
        Vector3f(boxMax.x(), boxMin.y(), boxMin.z()),
        Vector3f(boxMax.x(), boxMin.y(), boxMax.z()),
        Vector3f(boxMax.x(), boxMax.y(), boxMin.z()),
        Vector3f(boxMax.x(), boxMax.y(), boxMax.z()),
      };

      unsigned int lines[12][2] =
      { {0, 1}, {0, 2}, {0, 4},
        {1, 3}, {1, 5},
        {2, 3}, {2, 6},
        {3, 7},
        {4, 5}, {4, 6},
        {5, 7},
        {6, 7}
      };

      for(auto line : lines)
      {
        m_Drawer->DrawLine(boxPts[line[0]], boxPts[line[1]], compColor);
      }
    }

    virtual void	reportErrorWarning(const char* warningString)
    {
      LOG_WARNING<<warningString<<"\n";
    }

    virtual void	draw3dText(const btVector3& location,const char* textString)
    {

    }

    virtual void	setDebugMode(int debugMode)
    {
      m_debugMode = debugMode;
    }

    virtual int		getDebugMode() const
    {
      return m_debugMode;
    }
  };

  void PhysicsSystem::EnableDebugDraw(DebugTool::Drawer& iDrawer)
  {
    if(!m_DebugDrawer)
    {
      m_DebugDrawer = eXl_NEW BulletDebugDraw;
      m_DebugDrawer->m_Drawer = &iDrawer;
    }
  }

  void PhysicsSystem::DisableDebugDraw()
  {
    if(m_DebugDrawer)
    {
      eXl_DELETE m_DebugDrawer;
      m_DebugDrawer = nullptr;
    }
  }

  NeighborhoodExtraction& PhysicsSystem::GetNeighborhoodExtraction() const
  {
    return m_Impl->m_NeighExtraction;
  }

  void PhysicsSystem::AddContactCb(ObjectHandle iObj, ContactFilterCallback iCb)
  {
    m_Impl->AddContactCb(iObj, std::move(iCb));
  }

  void PhysicsSystem::RemoveContactCb(ObjectHandle iObj)
  {
    m_Impl->RemoveContactCb(iObj);
  }

  Err PhysicsSystem::CreateComponent(ObjectHandle iObject, PhysicInitData const& iInit)
  {
    if(!GetWorld().IsObjectValid(iObject))
    {
      return Err::Failure;
    }

    PhysicComponent_Impl* compImpl = eXl_NEW PhysicComponent_Impl(*this);

    PhysicInitData& initData = GetPhysicsInitDataView(GetWorld())->GetOrCreate(iObject);
    initData = iInit;

    compImpl->Build(iObject, initData);
    if(iInit.GetFlags() & PhysicFlags::IsGhost)
    {
      m_Impl->m_dynamicsWorld->addCollisionObject(compImpl->m_Object, iInit.GetCategory(), iInit.GetFilter());
    }
    else
    {
      m_Impl->m_dynamicsWorld->addRigidBody(static_cast<btRigidBody*>(compImpl->m_Object), iInit.GetCategory(), iInit.GetFilter());
    }
    if (compImpl->m_Sensor)
    {
      m_Impl->m_dynamicsWorld->addCollisionObject(compImpl->m_Sensor, iInit.GetCategory(), EngineCommon::s_MovementSensorMask);
    }

    while(m_Components.size() <= iObject.GetId())
    {
      m_Components.push_back(IntrusivePtr<PhysicComponent_Impl>());
    }

    m_Components[iObject.GetId()] = compImpl;
    ComponentManager::CreateComponent(iObject);
    return Err::Success;
  }

  void PhysicsSystem::AddTrigger(ObjectHandle iObj, TriggerDef const& iDef, TriggerCallbackHandle iCallback, ContactFilterCallback iFilter)
  {
    m_Impl->m_Triggers.AddTrigger(iObj, iDef, iCallback, iFilter);
    ComponentManager::CreateComponent(iObj);
  }

  TriggerCallbackHandle PhysicsSystem::AddTriggerCallback(std::unique_ptr<TriggerCallback> iCallback)
  {
    return m_Impl->m_Triggers.AddCallback(std::move(iCallback));
  }

  void PhysicsSystem::DeleteComponent(ObjectHandle iObj)
  {
    m_Impl->m_Triggers.DeleteComponent(iObj);

    if (m_Components.size() > iObj.GetId() && m_Components[iObj.GetId()] != nullptr)
    {
      m_Impl->m_ToDelete.insert(m_Components[iObj.GetId()]);
      m_Components[iObj.GetId()] = nullptr;
      ComponentManager::DeleteComponent(iObj);
    }
  }

  void PhysicsSystem::SetComponentEnabled(ObjectHandle iObj, bool iEnabled)
  {
    if (m_Components.size() > iObj.GetId() && m_Components[iObj.GetId()] != nullptr)
    {
      PhysicComponent_Impl* compImpl = m_Components[iObj.GetId()].get();
      if (!iEnabled && compImpl->IsEnabled())
      {
        if ((compImpl->GetFlags() & PhysicFlags::IsGhost) == 0)
        {
          m_Impl->m_dynamicsWorld->removeRigidBody(static_cast<btRigidBody*>(compImpl->GetObject()));
        }
        else
        {
          m_Impl->m_dynamicsWorld->removeCollisionObject(compImpl->GetObject());
        }
        if (compImpl->m_Sensor)
        {
          m_Impl->m_dynamicsWorld->removeCollisionObject(compImpl->m_Sensor);
        }
        compImpl->SetEnabled(iEnabled);
      }
      else if(iEnabled && !compImpl->IsEnabled())
      {
        Matrix4f const& trans = m_Transforms.GetWorldTransform(iObj);
        btTransform btTrans;
        btTrans.setFromOpenGLMatrix(trans.m_Data);
        compImpl->m_Object->setWorldTransform(btTrans);
        compImpl->m_Object->setInterpolationWorldTransform(btTrans);

        uint16_t group = compImpl->m_InitData->GetCategory();
        uint16_t mask = compImpl->m_InitData->GetFilter();

        if ((compImpl->GetFlags() & PhysicFlags::IsGhost) == 0)
        {
          m_Impl->m_dynamicsWorld->addRigidBody(static_cast<btRigidBody*>(compImpl->GetObject()), group, mask);
        }
        else
        {
          m_Impl->m_dynamicsWorld->addCollisionObject(compImpl->GetObject(), group, mask);
        }
        if (compImpl->m_Sensor)
        {
          compImpl->m_Sensor->setWorldTransform(btTrans);
          m_Impl->m_dynamicsWorld->addCollisionObject(compImpl->m_Sensor);
        }
        compImpl->SetEnabled(iEnabled);
        compImpl->setWorldTransform(btTrans);
      }
    }
  }

  PhysicComponent_Impl* PhysicsSystem::GetCompImpl(ObjectHandle iObj)
  {
    if(GetWorld().IsObjectValid(iObj))
    {
      uint32_t objId = iObj.GetId();
      if(objId < m_Components.size())
      {
        return m_Components[objId].get();
      }
    }

    return nullptr;
  }

  Err PhysicsSystem::AddKinematicController(KinematicController* iController)
  {
    if(iController /*&& iComp.GetImpl()->m_Flags & PhysicFlags::Kinematic*/)
    {
      auto actItf = eXl_NEW PhysicInternalActionInterface(iController, this);
      m_Impl->m_dynamicsWorld->addAction( actItf );
      return Err::Success;
    }
    return Err::Failure;
  }

  void PhysicsSystem::RayQuery(List<CollisionData>& oRes, const Vector3f& iOrig,const Vector3f& iEnd,unsigned int maxEnt,unsigned short category,unsigned short mask)
  {
    GeomDef temp;
    temp.position=iOrig;
    temp.shape = GeomDef::Ray;
    temp.geomData = iEnd;
    unsigned int completeCat = (unsigned int)category | (((unsigned int)mask)<<16);
    m_Impl->HandleColQuery(temp, maxEnt, completeCat, oRes);
  }

  void PhysicsSystem::SphereQuery(List<CollisionData>& oRes, float iRadius,const Vector3f& iOrig,unsigned int maxEnt,unsigned short category,unsigned short mask)
  {
    GeomDef temp;
    temp.position=iOrig;
    temp.shape = GeomDef::Sphere;
    temp.geomData = Vector3f(iRadius,0,0);
    unsigned int completeCat = (unsigned int)category | (((unsigned int)mask)<<16);
    m_Impl->HandleColQuery(temp, maxEnt, completeCat, oRes);
  }

  void PhysicsSystem::CylinderQuery(List<CollisionData>& oRes, float iRay,float iH,const Vector3f& iPos,const Quaternionf& iOrient,unsigned int maxEnt,unsigned short category,unsigned short mask)
  {
    GeomDef temp;
    temp.position=iPos;
    temp.orient=iOrient;
    temp.shape = GeomDef::Cylinder;
    temp.geomData = Vector3f(iRay,iH,0);
    unsigned int completeCat = (unsigned int)category | (((unsigned int)mask)<<16);
    m_Impl->HandleColQuery(temp, maxEnt, completeCat, oRes);
  }

  void PhysicsSystem::BoxQuery(List<CollisionData>& oRes, const Vector3f& iDim,const Vector3f& iPos,const Quaternionf& iOrient,unsigned int maxEnt,unsigned short category,unsigned short mask)
  {
    GeomDef temp;
    temp.position=iPos;
    temp.orient=iOrient;
    temp.shape = GeomDef::Box;
    temp.geomData = iDim;
    unsigned int completeCat = (unsigned int)category | (((unsigned int)mask)<<16);
    m_Impl->HandleColQuery(temp, maxEnt, completeCat, oRes);
  }

  void PhysicsSystem::SyncTriggersTransforms()
  {
    m_Transforms.IterateOverDirtyTransforms([this](Matrix4f const& iTrans, ObjectHandle iObj)
    {
      m_Impl->m_Triggers.UpdateTransform(iObj, iTrans);
    });
  }

  void PhysicsSystem::Step(float iTime)
  {
    m_MovedTransform.clear();
    m_MovedObject.clear();

    // Should be handled as kinematic objects.
    //m_Transforms.IterateOverDirtyTransforms([this](Matrix4f const& iTrans, ObjectHandle iObj)
    //{
    //  if (m_Components.size() > iObj.GetId() && m_Components[iObj.GetId()] != nullptr)
    //  {
    //    PhysicComponent_Impl* obj = m_Components[iObj.GetId()].get();
    //    if (obj->GetFlags() & PhysicFlags::IsGhost)
    //    {
    //      Vector3f const& pos = MathTools::GetPosition(iTrans);
    //      obj->m_Object->getWorldTransform().getBasis().setValue(
    //        iTrans.m_Data[0], iTrans.m_Data[1], iTrans.m_Data[2],
    //        iTrans.m_Data[4], iTrans.m_Data[5], iTrans.m_Data[6],
    //        iTrans.m_Data[8], iTrans.m_Data[9], iTrans.m_Data[10]);
    //      obj->m_Object->getWorldTransform().setOrigin(TO_BTVECT(pos));
    //    }
    //  }
    //});

    m_Impl->Step(iTime);
    m_Impl->m_Triggers.Tick(GetWorld(), iTime);

#ifndef __ANDROID__
    if(m_DebugDrawer!=NULL)
    {
      m_Impl->m_dynamicsWorld->setDebugDrawer(m_DebugDrawer);
      m_Impl->m_dynamicsWorld->debugDrawWorld();
      m_Impl->m_dynamicsWorld->setDebugDrawer(NULL);
    }
#endif

    NeighborhoodExtractionImpl& neighExt = m_Impl->m_NeighExtraction;
    for(unsigned int i = 0; i<m_MovedTransform.size(); ++i)
    {
      m_Transforms.UpdateTransform(m_MovedObject[i], m_MovedTransform[i]);

      auto iter = neighExt.GetObjects().find(m_MovedObject[i]);
      if(iter != neighExt.GetObjects().end())
      {
        Matrix4f const& trans = m_MovedTransform[i];
        //auto& pos = neighExt.m_Pos[iter->second];
        btVector3 pos(trans.m_Data[12], trans.m_Data[13], trans.m_Data[14]);
        neighExt.m_Trans[iter->second].setFromOpenGLMatrix(trans.m_Data);

        neighExt.m_Volumes[iter->second] = btDbvtVolume::FromCE(pos, neighExt.m_Volumes[iter->second].Extents());
      }
    }

#if 0
    m_LastCollisions.clear();

    int numManifolds = m_Impl->m_dynamicsWorld->getDispatcher()->getNumManifolds();
    for (int i = 0; i<numManifolds; i++)
    {
      btPersistentManifold* contactManifold =  m_Impl->m_dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
      btCollisionObject const* obA = static_cast<btCollisionObject const*>(contactManifold->getBody0());
      btCollisionObject const* obB = static_cast<btCollisionObject const*>(contactManifold->getBody1());
      PhysicComponent_Impl* bcA = static_cast<PhysicComponent_Impl*>(obA->getUserPointer());
      PhysicComponent_Impl* bcB = static_cast<PhysicComponent_Impl*>(obB->getUserPointer());
      if(bcA==NULL || bcB==NULL)
      {
        LOG_WARNING<<"Foreign bullet object in physic loop"<<"\n";
        continue;
      }
      if((bcA->GetFlags() | bcB->GetFlags()) & PhysicFlags::NeedContactNotify)
      {
        //CollisionComponent* collisionDispatcher1 = bcA->GetFlags() & PhysicFlags::NeedContactNotify ? bcA->GetCollisionDispatcher() : nullptr;
        //CollisionComponent* collisionDispatcher2 = bcB->GetFlags() & PhysicFlags::NeedContactNotify ? bcB->GetCollisionDispatcher() : nullptr;

        bool recordCollision1 = bcA->GetFlags() & PhysicFlags::NeedContactNotify;
        bool recordCollision2 = bcB->GetFlags() & PhysicFlags::NeedContactNotify;

        int numContacts = contactManifold->getNumContacts();
        
        CollisionData colData;
        colData.obj1=bcA->m_ObjectId;
        colData.obj2=bcB->m_ObjectId;

        float energy=0.0;

        int numContactToLoop = ((bcA->GetFlags() | bcB->GetFlags()) & PhysicFlags::FullContactNotify) == 0 ? 0 : numContacts;

        for (int j=0; j<numContactToLoop; j++)
        {
          btManifoldPoint& pt = contactManifold->getContactPoint(j);
          if (pt.getDistance()<0.f)
          {
            const btVector3& ptA = pt.getPositionWorldOnA();
            colData.contactPosOn1+=Vector3f(ptA.x(),ptA.y(),ptA.z());
            const btVector3& normalOnB = pt.m_normalWorldOnB;
            colData.normal1To2+=Vector3f(-normalOnB.x(),-normalOnB.y(),-normalOnB.z());
            colData.depth-=pt.getDistance();

            //Là problème, besoin de dispatcher spécifiques.
            //Suppose collision parfaitement inelastique
            //float massA = bcA->GetMass();
            //float massB = bcB->GetMass();
            //if(!(massA+massB<=0))
            //{
            //  float contribA = bcA->GetLinearVelocity().Dot(colData.normal1To2);
            //  float contribB = bcB->GetLinearVelocity().Dot(colData.normal1To2);
            //  float origEC = (massA*contribA*contribA + massB*contribB*contribB);
            //  float newEC = (contribA*massA+contribB*massB);
            //  if(massA==0 || massB==0)
            //    newEC = 0;
            //  else
            //    newEC = newEC*newEC/(massA+massB);
            //  colData.energy = origEC-newEC;
            //}
            //else

            colData.energy=0.0;

            if(recordCollision1 && bcA->GetFlags() & PhysicFlags::FullContactNotify)
            {
              CollisionData nColData(colData);
              nColData.contactPosOn1=Vector3f(ptA.x(),ptA.y(),ptA.z());
              nColData.normal1To2=Vector3f(-normalOnB.x(),-normalOnB.y(),-normalOnB.z());
              nColData.depth=-pt.getDistance();

              m_LastCollisions.push_back(nColData);
            }
            if(bcB->GetFlags() & PhysicFlags::FullContactNotify)
            {
              CollisionData nColData(colData);
              nColData.obj1=bcB->m_ObjectId;
              nColData.obj2=bcA->m_ObjectId;
              nColData.contactPosOn1=Vector3f(ptA.x()-normalOnB.x()*colData.depth,
                ptA.y()-normalOnB.y()*colData.depth,
                ptA.z()-normalOnB.z()*colData.depth);
              nColData.normal1To2=Vector3f(normalOnB.x(),normalOnB.y(),normalOnB.z());
              nColData.depth=-pt.getDistance();

              m_LastCollisions.push_back(nColData);
            }
          }
        }

        if(numContacts>0)
        {
          if(recordCollision1 && !(bcA->GetFlags() & PhysicFlags::FullContactNotify))
          {
            m_LastCollisions.push_back(colData);
          }

          if(recordCollision2 && !(bcB->GetFlags() & PhysicFlags::FullContactNotify))
          {
            colData.obj1=bcB->m_ObjectId;
            colData.obj2=bcA->m_ObjectId;
            colData.contactPosOn1+=colData.normal1To2*colData.depth;
            colData.normal1To2=colData.normal1To2*-1.0;
          
            m_LastCollisions.push_back(colData);
          }
        }
      }
    }
#endif
  }
}