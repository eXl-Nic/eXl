/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#include <engine/physics/physicsys_impl.hpp>
#include <engine/physics/physiccomponent_impl.hpp>
#include <engine/physics/physics_detail.hpp>
#include <engine/physics/physicsys.hpp>

#include <math/mathtools.hpp>

#include <BulletCollision/CollisionDispatch/btGhostObject.h>

namespace eXl
{
  const Quaternionf RXD(Mathf::Cos(Mathf::PI / 4.0f), 1.0*Mathf::Sin(Mathf::PI / 4.0f), 0.0, 0.0);
  const Quaternionf RXA(Mathf::Cos(Mathf::PI / 4.0f), -1.0*Mathf::Sin(Mathf::PI / 4.0f), 0.0, 0.0);
  const Quaternionf RYD(Mathf::Cos(Mathf::PI / 4.0f), 0.0, 1.0*Mathf::Sin(Mathf::PI / 4.0f), 0.0);
  const Quaternionf RYA(Mathf::Cos(Mathf::PI / 4.0f), 0.0, -1.0*Mathf::Sin(Mathf::PI / 4.0f), 0.0);
  const Quaternionf RZD(Mathf::Cos(Mathf::PI / 4.0f), 0.0, 0.0, 1.0*Mathf::Sin(Mathf::PI / 4.0f));
  const Quaternionf RZA(Mathf::Cos(Mathf::PI / 4.0f), 0.0, 0.0, -1.0*Mathf::Sin(Mathf::PI / 4.0f));

  btCollisionShape* ShapesCache::MakeGeom(const GeomDef& iDef, btCompoundShape* iCont)
  {
    auto existingShape = m_Shapes.find(iDef);
    if (existingShape != m_Shapes.end())
    {
      uint8_t* counter = (uint8_t*)existingShape->second->getUserPointer();
      existingShape->second->setUserPointer(counter + 1);
      return existingShape->second;
    }

    btCollisionShape* newGeom = NULL;
    switch (iDef.shape)
    {
    case GeomDef::Sphere:
      newGeom = eXl_Bullet_NEW(btSphereShape)(iDef.geomData.X());
      if (iCont)
      {
        iCont->addChildShape(btTransform(TO_BTQUAT(Quaternionf::IDENTITY), TO_BTVECT(iDef.position)), newGeom);
      }
      else if (iDef.position != Vector3f::ZERO)
      {
        iCont = eXl_Bullet_NEW(btCompoundShape);
        iCont->addChildShape(btTransform(TO_BTQUAT(Quaternionf::IDENTITY), TO_BTVECT(iDef.position)), newGeom);
      }
      break;
    case GeomDef::Cylinder:
      if (iCont)
      {
        newGeom = eXl_Bullet_NEW(btCylinderShape)(btVector3(iDef.geomData.X(), iDef.geomData.Y() / 2.0, iDef.geomData.Z()));
        iCont->addChildShape(btTransform(TO_BTQUAT(iDef.orient), TO_BTVECT(iDef.position)), newGeom);
      }
      else if (iDef.position != Vector3f::ZERO || iDef.orient != Quaternionf::IDENTITY)
      {
        //Should optim with Y,Z rotation but version in bullet look buggy (halfExtent? whattt?)
        iCont = eXl_Bullet_NEW(btCompoundShape);
        newGeom = eXl_Bullet_NEW(btCylinderShape)(btVector3(iDef.geomData.X(), iDef.geomData.Y() / 2.0, iDef.geomData.Z()));
        iCont->addChildShape(btTransform(TO_BTQUAT(iDef.orient), TO_BTVECT(iDef.position)), newGeom);
      }
      else
      {
        newGeom = eXl_Bullet_NEW(btCylinderShape)(btVector3(iDef.geomData.X(), iDef.geomData.Y() / 2.0, iDef.geomData.Z()));
      }
      break;
    case GeomDef::Box:
      if (iCont)
      {
        newGeom = eXl_Bullet_NEW(btBoxShape)(TO_BTVECT((iDef.geomData / 2.0)));
        iCont->addChildShape(btTransform(TO_BTQUAT(iDef.orient), TO_BTVECT(iDef.position)), newGeom);
      }
      else if (iDef.position != Vector3f::ZERO)
      {
        iCont = eXl_Bullet_NEW(btCompoundShape);
        newGeom = eXl_Bullet_NEW(btBoxShape)(TO_BTVECT((iDef.geomData / 2.0)));
        iCont->addChildShape(btTransform(TO_BTQUAT(iDef.orient), TO_BTVECT(iDef.position)), newGeom);
      }
      else if (iDef.orient == Quaternionf::IDENTITY || iDef.orient == Quaternionf(-1.0, 0.0, 0.0, 0.0) ||
        iDef.orient == Quaternionf(0.0, 1.0, 0.0, 0.0) || iDef.orient == Quaternionf(0.0, -1.0, 0.0, 0.0) ||
        iDef.orient == Quaternionf(0.0, 0.0, 1.0, 0.0) || iDef.orient == Quaternionf(0.0, 0.0, -1.0, 0.0) ||
        iDef.orient == Quaternionf(0.0, 0.0, 0.0, 1.0) || iDef.orient == Quaternionf(0.0, 0.0, 0.0, -1.0))
      {
        newGeom = eXl_Bullet_NEW(btBoxShape)(TO_BTVECT((iDef.geomData / 2.0)));
      }
      else if (iDef.orient == RXD || iDef.orient == RXA)
      {
        newGeom = eXl_Bullet_NEW(btBoxShape)(btVector3(iDef.geomData.X() / 2.0, iDef.geomData.Z() / 2.0, iDef.geomData.Y() / 2.0));
      }
      else if (iDef.orient == RYD || iDef.orient == RYA)
      {
        newGeom = eXl_Bullet_NEW(btBoxShape)(btVector3(iDef.geomData.Z() / 2.0, iDef.geomData.Y() / 2.0, iDef.geomData.X() / 2.0));
      }
      else if (iDef.orient == RZD || iDef.orient == RZA)
      {
        newGeom = eXl_Bullet_NEW(btBoxShape)(btVector3(iDef.geomData.Y() / 2.0, iDef.geomData.X() / 2.0, iDef.geomData.Z() / 2.0));
      }
      else
      {
        iCont = eXl_Bullet_NEW(btCompoundShape);
        newGeom = eXl_Bullet_NEW(btBoxShape)(TO_BTVECT((iDef.geomData / 2.0)));
        iCont->addChildShape(btTransform(TO_BTQUAT(iDef.orient), TO_BTVECT(iDef.position)), newGeom);
      }
      break;
    case GeomDef::Capsule:
      if (iCont)
      {
        newGeom = eXl_Bullet_NEW(btCapsuleShape)(iDef.geomData.X(), iDef.geomData.Y() - 2.0*iDef.geomData.X());
        iCont->addChildShape(btTransform(TO_BTQUAT(iDef.orient), TO_BTVECT(iDef.position)), newGeom);
      }
      else if (iDef.position != Vector3f::ZERO || iDef.orient != Quaternionf::IDENTITY)
      {
        //Should optim with Y,Z rotation but version in bullet look buggy (halfExtent? whattt?)
        iCont = eXl_Bullet_NEW(btCompoundShape);
        newGeom = eXl_Bullet_NEW(btCapsuleShape)(iDef.geomData.X(), iDef.geomData.Y() - 2.0*iDef.geomData.X());
        iCont->addChildShape(btTransform(TO_BTQUAT(iDef.orient), TO_BTVECT(iDef.position)), newGeom);
      }
      else
      {
        newGeom = eXl_Bullet_NEW(btCapsuleShape)(iDef.geomData.X(), iDef.geomData.Y() - 2.0*iDef.geomData.X());
      }
      break;
    default:
      eXl_ASSERT_MSG(false, "Wrong Shape");
      break;
    }

    if (iCont)
    {
      m_Shapes.insert(std::make_pair(iDef, iCont));
      iCont->setUserPointer((void*)2);
      return iCont;
    }
    m_Shapes.insert(std::make_pair(iDef, newGeom));
    newGeom->setUserPointer((void*)2);
    return newGeom;
  }

  btCollisionShape* ShapesCache::BuildCollisionShape(PhysicInitData const& iData)
  {
    auto const& gList = iData.GetGeoms();
    auto iter = gList.begin();
    auto iterEnd = gList.end();

    if (iter == iterEnd)
      return NULL;

    iter++;

    if (iter == iterEnd)
    {
      iter = gList.begin();
      return MakeGeom(*iter, NULL);
    }
    else
    {
      iter = gList.begin();
      iterEnd = gList.end();
      btCompoundShape* ret = eXl_Bullet_NEW(btCompoundShape);
      for (; iter != iterEnd; iter++)
      {
        MakeGeom(*iter, ret);
      }
      return ret;
    }
  }

  void ShapesCache::ReleaseShape(btCollisionShape* iShape)
  {
    if (iShape->getUserPointer() == 0)
    {
      eXl_Bullet_DELETE(btCollisionShape, iShape);
    }
    else
    {
      uint8_t* counter = (uint8_t*)iShape->getUserPointer();
      iShape->setUserPointer(counter - 1);
    }
  }

  struct OrderedColDat
  {
    CollisionData m_ColDat;
    //Reverse comparison to sort the results more easily.
    bool operator<(const OrderedColDat& iRec)const
    {
      if(m_ColDat.depth==0)return false;
      else if(iRec.m_ColDat.depth==0)return true;
      else return m_ColDat.depth>iRec.m_ColDat.depth;
    }
  };

  struct XClosestContactResultCallback : public btCollisionWorld::ContactResultCallback
  {
    XClosestContactResultCallback(btCollisionObject* iMe, Multiset<OrderedColDat>& iRec, unsigned int iLimit, uint16_t iMask):
      me(iMe),
      m_Rec(iRec),
      count(0),
      m_Limit(iLimit)
    {
      m_collisionFilterMask = iMask;
      m_collisionFilterGroup = -1;
    }
    btCollisionObject* me;
    Multiset<OrderedColDat>& m_Rec;
    unsigned int count;
    unsigned int m_Limit;

    btScalar addSingleResult(btManifoldPoint& pt,const btCollisionObjectWrapper* colObj0,int,int,const btCollisionObjectWrapper* colObj1,int,int) override
    {

      if (pt.getDistance()<0.f)
      {
        OrderedColDat res;
        CollisionData& colData=res.m_ColDat;
        if(colObj0->m_collisionObject==me)
        {
          PhysicComponent_Impl* bcA = static_cast<PhysicComponent_Impl*>(colObj1->m_collisionObject->getUserPointer());
          eXl_ASSERT_MSG(bcA!=NULL,"Error with object in collision query");
          colData.obj1 = bcA->m_ObjectId ;

          const btVector3& ptA = pt.getPositionWorldOnA();
          colData.contactPosOn1=Vector3f(ptA.x(),ptA.y(),ptA.z());
          const btVector3& normalOnB = pt.m_normalWorldOnB;
          colData.normal1To2=Vector3f(-normalOnB.x(),-normalOnB.y(),-normalOnB.z());
          colData.depth= -pt.getDistance();
        }
        else
        {
          PhysicComponent_Impl* bcA = static_cast<PhysicComponent_Impl*>(colObj0->m_collisionObject->getUserPointer());
          eXl_ASSERT_MSG(bcA!=NULL,"Error with object in collision query");
          colData.obj1 = bcA->m_ObjectId;

          const btVector3& ptB = pt.getPositionWorldOnB();
          colData.contactPosOn1=Vector3f(ptB.x(),ptB.y(),ptB.z());
          const btVector3& normalOnB = pt.m_normalWorldOnB;
          colData.normal1To2=Vector3f(normalOnB.x(),normalOnB.y(),normalOnB.z());
          colData.depth= -pt.getDistance();
        }

        m_Rec.insert(res);
        if(m_Limit!=0 && count>m_Limit)
        {
          count--;
          m_Rec.erase(m_Rec.begin());
        }
        count++;
      }
      //Whatever
      return 1.0;
    }
  };

  struct XClosestRayResultCallback : public btCollisionWorld::RayResultCallback
  {
    XClosestRayResultCallback(Multiset<OrderedColDat>& iRec,unsigned int iLimit,const btVector3& iRayFromWorld,const btVector3& iRayToWorld, uint16_t iMask):
      m_Rec(iRec),
      count(0),
      m_Limit(iLimit),
      m_ClosestHitFrac(1.0),
      m_rayFromWorld(iRayFromWorld),
      m_rayToWorld(iRayToWorld)
    {
      m_collisionFilterMask = iMask;
      m_collisionFilterGroup = -1;
    }

    const btVector3	m_rayFromWorld;
    const btVector3	m_rayToWorld;
    Multiset<OrderedColDat>& m_Rec;
    unsigned int count;
    unsigned int m_Limit;
    float m_ClosestHitFrac;

    btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace) override
    {
      OrderedColDat res;
      CollisionData& colData=res.m_ColDat;
      PhysicComponent_Impl* bcA = static_cast<PhysicComponent_Impl*>(rayResult.m_collisionObject->getUserPointer());
      eXl_ASSERT_MSG(bcA!=NULL,"Error with object in collision query");
      colData.obj1 = bcA->m_ObjectId;
      btVector3 ptA;
      ptA.setInterpolate3(m_rayFromWorld,m_rayToWorld,rayResult.m_hitFraction);
      colData.contactPosOn1=Vector3f(ptA.x(),ptA.y(),ptA.z());
      btVector3 normalOnB;
      if (normalInWorldSpace)
      {
        normalOnB = rayResult.m_hitNormalLocal;
      }
      else
      {
        ///need to transform normal into worldspace
        normalOnB = rayResult.m_collisionObject->getWorldTransform().getBasis()*rayResult.m_hitNormalLocal;
      }
      colData.normal1To2=Vector3f(-normalOnB.x(),-normalOnB.y(),-normalOnB.z());
      colData.depth = rayResult.m_hitFraction;


      m_Rec.insert(res);
      if(m_Limit!=0 && count>m_Limit)
      {
        count--;
        m_Rec.erase(m_Rec.begin());
        //the farest is at the beginning.
        m_ClosestHitFrac = m_Rec.begin()->m_ColDat.depth;
      }
      count++;

      return m_ClosestHitFrac;
    }
  };

  struct ClosestSweepTestResultCallback : public btCollisionWorld::ClosestConvexResultCallback
  {
    ClosestSweepTestResultCallback(btCollisionObject* iMe, std::function<bool(PhysicComponent_Impl*)> const& iIgnore, uint16_t iMask)
      : btCollisionWorld::ClosestConvexResultCallback(btVector3(0.0, 0.0, 0.0), btVector3(0.0, 0.0, 0.0))
      , me(iMe)
      , m_Ignore(iIgnore)
      
    {
      m_collisionFilterMask = iMask;
      m_collisionFilterGroup = -1;
    }
    btCollisionObject* me;
    std::function<bool(PhysicComponent_Impl*)> const& m_Ignore;

    virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace)
    {
      if (convexResult.m_hitCollisionObject == me
        || convexResult.m_hitCollisionObject->getUserPointer() == me->getUserPointer())
      {
        return btScalar(1.0);
      }

      PhysicComponent_Impl* bc = static_cast<PhysicComponent_Impl*>(convexResult.m_hitCollisionObject->getUserPointer());
      if (!bc)
      {
        //if (convexResult.m_hitCollisionObject->getInternalType() == btCollisionObject::CO_GHOST_OBJECT)
        {
          return btScalar(1.0);
        }
      }

      if (m_Ignore && m_Ignore(bc))
      {
        return btScalar(1.0);
      }

      btVector3 hitNormalWorld;
      if (normalInWorldSpace)
      {
        hitNormalWorld = convexResult.m_hitNormalLocal;
      }
      else
      {
        ///need to transform normal into worldspace
        hitNormalWorld = m_hitCollisionObject->getWorldTransform().getBasis()*convexResult.m_hitNormalLocal;
      }

      if(bc->GetFlags() & PhysicFlags::IsGhost)
      {
        return btScalar(1.0);
      }
      //btScalar dotUp = m_up.dot(hitNormalWorld);
      //if(dotUp < m_minSlopeDot || m_Comp->OnCollision(m_Sys,bc,FROM_BTVECT(convexResult.m_hitPointLocal),FROM_BTVECT(hitNormalWorld)))
      //{
      //  return btScalar(1.0);
      //}

      return ClosestConvexResultCallback::addSingleResult (convexResult, normalInWorldSpace);
    }
  };

  class eXlCustomDispatcher : public btCollisionDispatcher
  {
  public:
    eXlCustomDispatcher(btCollisionConfiguration* config)
      : btCollisionDispatcher(config)
    {

    }

    bool needsCollision(const btCollisionObject* body0, const btCollisionObject* body1) override
    {
      if (body0->getUserPointer() == body1->getUserPointer())
      {
        return false;
      }

      if (btCollisionDispatcher::needsCollision(body0, body1))
      {
        PhysicComponent_Impl* obj0 = reinterpret_cast<PhysicComponent_Impl*>(body0->getUserPointer());
        PhysicComponent_Impl* obj1 = reinterpret_cast<PhysicComponent_Impl*>(body1->getUserPointer());

        if (obj0 == nullptr
          || obj1 == nullptr)
        {
          return true;
        }

        auto iterCb = m_CustomCB.find(obj0->m_ObjectId);
        if (iterCb != m_CustomCB.end())
        {
          if (!iterCb->second(obj0->m_ObjectId, obj1->m_ObjectId))
          {
            return false;
          }
        }
        
        iterCb = m_CustomCB.find(obj1->m_ObjectId);
        if (iterCb != m_CustomCB.end())
        {
          return iterCb->second(obj1->m_ObjectId, obj0->m_ObjectId);
        }

        return true;
      }
      return false;
    }

    UnorderedMap<ObjectHandle, ContactFilterCallback> m_CustomCB;
  };

  PhysicsSystem_Impl::PhysicsSystem_Impl(PhysicsSystem& iSys)
    : m_HostSys(iSys)
    , m_NeighExtraction(iSys)
    , m_Triggers(*this)
  {
    m_collisionConfiguration = new btDefaultCollisionConfiguration();
    m_collisionConfiguration->setPlaneConvexMultipointIterations();
    m_dispatcher = new eXlCustomDispatcher(m_collisionConfiguration);
    m_broadphase = new btDbvtBroadphase();
    m_broadphase->m_deferedcollide = true;

    btSequentialImpulseConstraintSolver* sol = new btSequentialImpulseConstraintSolver;

    m_solver = sol;
    m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration);

    //Support for ghost object
    m_dynamicsWorld->getBroadphase()->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
    m_dynamicsWorld->setGravity(btVector3(0,-10,0));
  }

  void PhysicsSystem_Impl::Cleanup()
  {
    for(auto comp : m_ToDelete)
    {
      ObjectHandle id = comp->m_ObjectId;
      m_NeighExtraction.Remove(id, comp.get());
      RemoveContactCb(id);

      if(!(comp->m_InitData->GetFlags() & PhysicFlags::IsGhost))
      {
        btRigidBody* object = btRigidBody::upcast(comp->m_Object);
        eXl_ASSERT_MSG(object!=NULL,"Bad upcast");
        m_dynamicsWorld->removeRigidBody(object);
      }
      else
      {
        m_dynamicsWorld->removeCollisionObject(comp->m_Object);
      }

      if (comp->m_Sensor)
      {
        m_dynamicsWorld->removeCollisionObject(comp->m_Sensor);
      }
    }
    m_ToDelete.clear();
  }

  void PhysicsSystem_Impl::AddContactCb(ObjectHandle iObj, ContactFilterCallback&& iCb)
  {
    m_dispatcher->m_CustomCB.emplace(std::make_pair(iObj, std::move(iCb)));
  }

  void PhysicsSystem_Impl::RemoveContactCb(ObjectHandle iObj)
  {
    m_dispatcher->m_CustomCB.erase(iObj);
  }

  void PhysicsSystem_Impl::Step(float iTime)
  {
    Cleanup();
    m_dynamicsWorld->stepSimulation(iTime);
  }

  bool PhysicsSystem_Impl::SweepTest(PhysicComponent_Impl* iShape, Vector3f const& iFrom, Vector3f const& iTo, CollisionData& oRes, std::function<bool(PhysicComponent_Impl*)> const& iIgnore, uint16_t iMask)
  {
    Cleanup();
    oRes.obj1 = ObjectHandle();
    oRes.obj2 = ObjectHandle();
    oRes.depth = 0.0;

    btCollisionShape* shape = iShape->GetObject()->getCollisionShape();
    eXl_ASSERT_MSG(shape && shape->isConvex(), "Bad shape for convex sweep");
    
    btTransform transFrom(TO_BTQUAT(Quaternionf::IDENTITY), TO_BTVECT(iFrom));
    btTransform transTo(TO_BTQUAT(Quaternionf::IDENTITY), TO_BTVECT(iTo));

    ClosestSweepTestResultCallback callback(iShape->GetObject(), iIgnore, iMask);

    auto castShape = static_cast<btConvexShape*>(shape);

    m_dynamicsWorld->convexSweepTest(castShape, transFrom, transTo, callback);
   
    if(callback.m_hitCollisionObject)
    {
      PhysicComponent_Impl* bcA = static_cast<PhysicComponent_Impl*>(callback.m_hitCollisionObject->getUserPointer());
        
      oRes.obj1 = iShape->m_ObjectId;
      oRes.obj2 = bcA->m_ObjectId;
      oRes.contactPosOn1 = FROM_BTVECT(callback.m_hitPointWorld);
      oRes.normal1To2 = FROM_BTVECT(callback.m_hitNormalWorld);
      oRes.depth = callback.m_closestHitFraction;

      return true;
    }
    
    return false;
  }

  void PhysicsSystem_Impl::HandleColQuery(const GeomDef& iDef, unsigned int iMax, unsigned int iCat, List<CollisionData>& oRes)
  {
    Cleanup();
    //Agent* agent = iInfo.GetSender().Get();
    Multiset<OrderedColDat> rec;
    //if(agent==NULL)
    //  return;

    uint16_t collisionFilterGroup = (unsigned short)(iCat & 0xFFFF);
    uint16_t collisionFilterMask = (unsigned short)(iCat >> 16);

    if(iDef.shape == GeomDef::Ray)
    {
      XClosestRayResultCallback contactCB(rec,iMax,TO_BTVECT(iDef.position),TO_BTVECT(iDef.geomData), collisionFilterMask);
      contactCB.m_collisionFilterGroup = collisionFilterGroup;
      
      m_dynamicsWorld->rayTest(contactCB.m_rayFromWorld,contactCB.m_rayToWorld,contactCB);

      float length = (iDef.position-iDef.geomData).Length();
      Multiset<OrderedColDat>::iterator iter = rec.begin();
      Multiset<OrderedColDat>::iterator iterEnd = rec.end();
      for(;iter!=iterEnd;iter++)
      {
        ((CollisionData*)(&iter->m_ColDat))->depth*=length;
      }
    }
    else
    {
      btTransform trans;
      trans.setOrigin(TO_BTVECT(iDef.position));
      trans.setRotation(TO_BTQUAT(iDef.orient));
      GeomDef localDef = iDef;
      localDef.position=Vector3f::ZERO;
      localDef.orient=Quaternionf::IDENTITY;
      btCollisionShape* shape = m_ShapesCache.MakeGeom(localDef, nullptr);
      {
        btCollisionObject obj;
        XClosestContactResultCallback contactCB(&obj,rec,iMax, collisionFilterMask);
        contactCB.m_collisionFilterGroup = collisionFilterGroup;
        obj.setCollisionShape(shape);
        obj.setWorldTransform(trans);
        contactCB.m_collisionFilterGroup = (unsigned short)(iCat & 0xFFFF);
        contactCB.m_collisionFilterMask = (unsigned short)(iCat>>16);

        m_dynamicsWorld->contactTest(&obj,contactCB);
      }
      m_ShapesCache.ReleaseShape(shape);
    }
    Multiset<OrderedColDat>::iterator iter = rec.begin();
    Multiset<OrderedColDat>::iterator iterEnd = rec.end();
    for(;iter!=iterEnd;iter++)
    {
      oRes.push_front(iter->m_ColDat);
    }
  }

  void NeighborhoodExtractionImpl::AddObject(ObjectHandle iObj, float iRadius, bool iPopulate)
  {
    if(auto physObj = m_Sys.GetCompImpl(iObj))
    {
      m_UsrDataToNum.insert(std::make_pair((void*)physObj->m_Object->getBroadphaseHandle(), m_ObjectsNeigh.size()));
      m_Objects.insert(std::make_pair(iObj, m_ObjectsNeigh.size()));
      m_ObjectsNeigh.push_back(Neigh());
      Neigh& newObj = m_ObjectsNeigh.back();
      newObj.populateNeigh = iPopulate;
      newObj.m_Shape = Neigh::Sphere;
      m_Handles.push_back(iObj);

      auto& trans = m_Sys.GetTransforms().GetWorldTransform(iObj);

      btVector3 pos(trans.m_Data[12], trans.m_Data[13], trans.m_Data[14]);

      m_Trans.push_back(btTransform());
      m_Trans[m_Trans.size() - 1].setFromOpenGLMatrix(trans.m_Data);
      
      m_Volumes.expand(btDbvtVolume::FromCR(pos, iRadius));
    } 
  }

  void NeighborhoodExtractionImpl::AddObject(ObjectHandle iObj, Vector3f const& iDims, bool iPopulate)
  {
    if(auto physObj = m_Sys.GetCompImpl(iObj))
    {
      m_UsrDataToNum.insert(std::make_pair((void*)physObj->m_Object->getBroadphaseHandle(), m_ObjectsNeigh.size()));
      m_Objects.insert(std::make_pair(iObj, m_ObjectsNeigh.size()));
      m_ObjectsNeigh.push_back(Neigh());
      Neigh& newObj = m_ObjectsNeigh.back();
      newObj.populateNeigh = iPopulate;
      newObj.m_Shape = Neigh::Box;
      m_Handles.push_back(iObj);

      auto& trans = m_Sys.GetTransforms().GetWorldTransform(iObj);

      btVector3 pos(trans.m_Data[12], trans.m_Data[13], trans.m_Data[14]);

      m_Trans.push_back(btTransform());
      m_Trans[m_Trans.size() - 1].setFromOpenGLMatrix(trans.m_Data);

      m_Volumes.expand(btDbvtVolume::FromCE(pos, TO_BTVECT(iDims) * 0.5));
    }
  }

  void NeighborhoodExtractionImpl::Remove(ObjectHandle iObj, PhysicComponent_Impl* iPhObj)
  {
    auto neighIter = m_Objects.find(iObj);
    if (neighIter != m_Objects.end())
    {
      uint32_t toErase = neighIter->second;
      if (m_Handles.size() > 1 && toErase < m_Handles.size() - 1)
      {
        ObjectHandle toReplace = m_Handles.back();

        m_Handles[toErase] = m_Handles.back();
        if (auto toReplacePhObj = m_Sys.GetCompImpl(toReplace))
        {
          m_ObjectsNeigh[toErase] = m_ObjectsNeigh.back();
          m_Volumes[toErase] = m_Volumes[m_Volumes.size() - 1];
          m_Trans[toErase] = m_Trans[m_Trans.size() - 1];
          m_UsrDataToNum[(void*)toReplacePhObj->m_Object->getBroadphaseHandle()] = toErase;
        }
        m_Objects[toReplace] = toErase;
      }

      m_Handles.pop_back();
      m_ObjectsNeigh.pop_back();
      m_Volumes.pop_back();
      m_Trans.pop_back();
      m_UsrDataToNum.erase((void*)iPhObj->m_Object->getBroadphaseHandle());
      m_Objects.erase(iObj);
    }
  }

  struct ObjSort : btDbvt::ICollide
  {
    ObjSort(UnorderedMap<void*, uint32_t> const& iIdMap, Vector<NeighborhoodExtraction::Neigh> const& iDescs, Vector<uint32_t>& oIds)
      : m_IdMap(iIdMap)
      , m_OutIds(oIds)
      , m_Descs(iDescs)
    {
    }
    UnorderedMap<void*, uint32_t> const& m_IdMap;
    Vector<NeighborhoodExtraction::Neigh> const& m_Descs;
    Vector<uint32_t>& m_OutIds;
    void Process(const btDbvtNode* leaf)
    {
      auto iter = m_IdMap.find(leaf->data);
      if(iter != m_IdMap.end() && m_Descs[iter->second].populateNeigh)
      {
        m_OutIds.push_back(iter->second);
      }
    }
  };

  struct VisitStackElem
  {
    VisitStackElem(btDbvtNode* iNode, uint32_t iListEnd)
      : node(iNode)
      , objListEnd(iListEnd)
    {
    }

    btDbvtNode* node;
    uint32_t objListEnd;
    int32_t curChild = -1;
  };

  void NeighborhoodExtractionImpl::Run(Vector3f const& iForwardOffset, float iRadiusSearch)
  {
    m_Sys.GetImpl().Cleanup();
    Run(((btDbvtBroadphase*)(m_Sys.GetImpl().m_broadphase))->m_sets[btDbvtBroadphase::DYNAMIC_SET], iForwardOffset, iRadiusSearch);
  }

  void NeighborhoodExtractionImpl::Run(btDbvt& iTree, Vector3f const& iForwardOffset, float iRadiusSearch)
  {
    Vector<uint32_t> objects;
    objects.reserve(m_Objects.size());
    {
      ObjSort sort(m_UsrDataToNum, m_ObjectsNeigh, objects);
      btDbvt::enumLeaves(iTree.m_root, sort);
    }

    if(objects.empty())
    {
      return;
    }

    for(auto& neigh : m_ObjectsNeigh)
    {
      neigh.numNeigh = 0;
    }

    Vector<VisitStackElem> nodes;
    nodes.reserve(64);

    nodes.push_back(VisitStackElem(iTree.m_root, objects.size()));

    Neigh* neighborsArray = m_ObjectsNeigh.data();
    btTransform* transArray = &m_Trans[0];
    btDbvtVolume* volumesArray = &m_Volumes[0];

    btVector3 searchOffset = TO_BTVECT(iForwardOffset);

    float inflateRadius = iRadiusSearch + searchOffset.length();

    float const sqRadius = iRadiusSearch * iRadiusSearch;

    while(!nodes.empty())
    {
      auto& curNode = nodes.back();
      if(curNode.curChild == 1)
      {
        nodes.pop_back();
      }
      else
      {
        curNode.curChild++;
        btDbvtNode* child = curNode.node->childs[curNode.curChild];

        btDbvtVolume nodeExpandedVolume = child->volume;
        nodeExpandedVolume.Expand(btVector3(inflateRadius, inflateRadius, inflateRadius));

        if(child->isleaf())
        {
          auto iter = m_UsrDataToNum.find(child->data);
          if(iter != m_UsrDataToNum.end())
          {
            uint32_t otherObj = iter->second;
            btVector3 const& otherPos = (transArray + otherObj)->getOrigin();
            
            // Update neighborhood with remaining object.
            for(uint32_t i = 0; i<curNode.objListEnd; ++i)
            {
              uint32_t curObject = objects[i];
              Neigh& neighToUpdate = *(neighborsArray + curObject);

              if(curObject != otherObj)
              {
                btDbvtVolume const& objVolume = *(volumesArray + objects[i]);
                
                if(Intersect(nodeExpandedVolume, objVolume))
                {
                  Neigh& otherNeigh = *(neighborsArray + otherObj);
                  btTransform const& curObjTrans = transArray[curObject];
                  //btVector3 curObjPos = *(transArray + curObject);

                  btVector3 curObjPos = curObjTrans.getOrigin();
                  btVector3 curObjFrontDir = curObjTrans.getBasis().getColumn(0);

                  float sqLen;
                  //float conePos;
                  if(neighToUpdate.m_Shape == Neigh::Sphere)
                  {
                    if(otherNeigh.m_Shape == Neigh::Sphere)
                    {
                      sqLen = curObjPos.distance2(otherPos);
                      //conePos = (otherPos - curObjPos).normalized().dot(curObjFrontDir);
                    }
                    else if(otherNeigh.m_Shape == Neigh::Box)
                    {
                      //Pretend we are on a plane :/
                      AABB2Df box2D;
                      box2D.m_Data[0] = MathTools::As2DVec(FROM_BTVECT(child->volume.Mins()));
                      box2D.m_Data[1] = MathTools::As2DVec(FROM_BTVECT(child->volume.Maxs()));

                      Vector2f ptDists = box2D.Classify(MathTools::As2DVec(FROM_BTVECT(curObjPos)));

                      sqLen = ptDists.SquaredLength();
                    }
                  }
                  else
                  {
                    eXl_ASSERT(false);
                  }

                  float* vecBegin = neighToUpdate.neighSqDist;
                  float* vecEnd = neighToUpdate.neighSqDist + neighToUpdate.numNeigh;
                  float* vecInsert = std::lower_bound(vecBegin, vecEnd, sqLen);

                  //float* vecBegin = neighToUpdate.neighRelPos;
                  //float* vecEnd = neighToUpdate.neighRelPos + neighToUpdate.numNeigh;
                  //float* vecInsert = std::lower_bound(vecBegin, vecEnd, 1.0 - conePos);

                  if(vecInsert != vecEnd)
                  {
                    uint32_t* neighInsert = neighToUpdate.neighbors + (vecInsert - vecBegin);
                    uint32_t sizeCopy = Mathi::Min(neighToUpdate.numNeigh, s_NumNeigh - (vecInsert - vecBegin) - 1);
                    memmove(vecInsert + 1, vecInsert, sizeCopy * sizeof(float));
                    memmove(neighInsert + 1, neighInsert, sizeCopy * sizeof(uint32_t));

                    *neighInsert = otherObj;
                    *vecInsert = sqLen;
                    //*vecInsert = 1.0 - conePos;
                    if(neighToUpdate.numNeigh < s_NumNeigh )
                    {
                      ++neighToUpdate.numNeigh;
                    }
                  }
                  else if(neighToUpdate.numNeigh < s_NumNeigh )
                  {
                    neighToUpdate.neighbors[neighToUpdate.numNeigh] = otherObj;
                    neighToUpdate.neighSqDist[neighToUpdate.numNeigh] = sqLen;
                    //neighToUpdate.neighRelPos[neighToUpdate.numNeigh] = 1.0 - conePos;
                    ++neighToUpdate.numNeigh;
                  }
                }
              }
            }
          }
        }
        else
        {
          //Cull object list by children expanded volume
          nodes.push_back(VisitStackElem(child, curNode.objListEnd));

          auto& stackElem = nodes.back();
          
          for(int i = 0; i < (int)stackElem.objListEnd; ++i)
          {
            btDbvtVolume const& objVolume = *(volumesArray + objects[i]);
            if(!Intersect(nodeExpandedVolume, objVolume))
            {
              std::swap(objects[i], objects[stackElem.objListEnd - 1]);
              --stackElem.objListEnd;
              --i;
            }
          }
        }
      }
    }
  }
}