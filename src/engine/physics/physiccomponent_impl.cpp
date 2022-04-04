/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


#include <engine/physics/physiccomponent_impl.hpp>
#include <engine/physics/physicsys_impl.hpp>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <engine/physics/physics_detail.hpp>
#include <engine/physics/physicsys.hpp>

#include <math/math.hpp>
#include <math/mathtools.hpp>

namespace eXl
{
  IMPLEMENT_RefC(PhysicComponent_Impl)

  PhysicComponent_Impl::~PhysicComponent_Impl()
  {
    if (m_Sensor)
    {
      eXl_Bullet_DELETE(btPairCachingGhostObject, m_Sensor);
    }

    btCollisionShape* shape = m_Object->getCollisionShape();
    eXl_Bullet_DELETE(btCollisionObject, m_Object);
    if (shape != nullptr && m_ShapeHolder == nullptr)
    {
      m_System.GetImpl().m_ShapesCache.ReleaseShape(shape);
    }
  }

  void PhysicComponent_Impl::getWorldTransform(btTransform &worldTrans) const 
  {
    worldTrans = m_CachedTransform;
  }

  void PhysicComponent_Impl::setWorldTransform(const btTransform &worldTrans)
  {
    if(!m_Enabled)
    {
      return;
    }
    if (m_Sensor)
    {
      m_Sensor->setWorldTransform(worldTrans);
    }

    {
      Vec3 upDir = FROM_BTVECT(m_CachedTransform.getBasis()[1]);

      m_CachedTransform = worldTrans;
      
      Quaternion rot = FROM_BTQUAT(worldTrans.getRotation());

      if ((m_InitData.GetFlags() & PhysicFlags::AlignRotToVelocity)
        && (m_InitData.GetFlags() & (PhysicFlags::IsGhost | PhysicFlags::Kinematic)) == 0
        && !m_Object->getInterpolationLinearVelocity().isZero())
      {
        Vec3 frontDir = normalize(FROM_BTVECT(m_Object->getInterpolationLinearVelocity()));
        
        if (Mathf::Abs(dot(frontDir, upDir) > 1 - Mathf::ZeroTolerance()))
        {
          upDir = FROM_BTVECT(m_CachedTransform.getBasis()[0]);
        }
        Vec3 sideDir = cross(frontDir, upDir);
        rot = Quaternion(frontDir, sideDir);
      }
      m_System.m_MovedTransform.push_back(translate(Identity<Mat4>(), FROM_BTVECT(worldTrans.getOrigin())) * Mat4(rot));
      m_System.m_MovedObject.push_back(m_ObjectId);
    }
  }

  bool PhysicComponent_Impl::SweepTest(Vec3 const& iFrom, Vec3 const& iTo, CollisionData& oRes, std::function<bool(PhysicComponent_Impl*)> const& iIgnore, uint16_t iMask)
  {
    return m_System.m_Impl->SweepTest(this, iFrom, iTo, oRes, iIgnore, iMask);
  }

  void PhysicComponent_Impl::Build(ObjectHandle iTransform, PhysicInitData const& iInitData)
  {
    m_ObjectId = iTransform;
    m_InitData = iInitData;

    //m_GroupMask = iInitData.GetFilter();
    //m_GroupMask <<= 16;
    //m_GroupMask |= iInitData.GetCategory();

    if (iInitData.GetShapeObj().IsAssigned())
    {
      PhysicComponent_Impl* shapeComp = m_System.GetCompImpl(iInitData.GetShapeObj());
      eXl_ASSERT(shapeComp);
      while (shapeComp->m_ShapeHolder)
      {
        shapeComp = shapeComp->m_ShapeHolder.get();
      }
      m_Shape = shapeComp->m_Shape;
      m_ShapeHolder = shapeComp;
    }
    if(m_Shape == nullptr)
    {
      m_Shape = m_System.GetImpl().m_ShapesCache.BuildCollisionShape(iInitData);
    }
    uint32_t flags = iInitData.GetFlags();

    Mat4 const& transform = m_System.m_Transforms.GetWorldTransform(iTransform);

    btTransform trans;
    trans.setOrigin(TO_BTVECT(transform[3]));
    trans.getBasis().setFromOpenGLSubMatrix(value_ptr(transpose(transform)));

    if(flags & PhysicFlags::IsGhost)
    {
      m_Object = eXl_Bullet_NEW(btGhostObject); 

      m_Object->setCollisionShape(m_Shape);
      m_Object->setUserPointer(this);
      if(!(flags & PhysicFlags::Static || flags & PhysicFlags::Kinematic) || flags & PhysicFlags::IsGhost)
      {
        m_Object->setCollisionFlags(m_Object->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
      }
      if(!(flags & PhysicFlags::Static) && (flags & PhysicFlags::Kinematic))
      {
        m_Object->setCollisionFlags(m_Object->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
      }
      m_Object->setWorldTransform(trans);
    }
    else
    {
      //Phys object
      btVector3 localInertia(0,0,0);
      btScalar mass = flags & PhysicFlags::Static ? 0:iInitData.GetMass();
      m_Mass = mass;
      if(m_Shape != nullptr
        && !(flags & (PhysicFlags::Compound | PhysicFlags::MasterCompound)))
      {
        if(mass!=0 && !(flags & (PhysicFlags::Static|PhysicFlags::Kinematic)))
        {
          m_Shape->calculateLocalInertia(mass,localInertia);
        }
      }
      m_CachedTransform = trans;
      btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, this, m_Shape, localInertia);
      rbInfo.m_restitution = 1.0;
      btRigidBody * rBody = eXl_Bullet_NEW(btRigidBody)(rbInfo);
      m_Object = rBody;

      btVector3 linearFactor = rBody->getLinearFactor();
      if(flags & PhysicFlags::LockX)
      {
        linearFactor.setX(0.0);
      }
      if(flags & PhysicFlags::LockY)
      {
        linearFactor.setY(0.0);
      }
      if(flags & PhysicFlags::LockZ)
      {
        linearFactor.setZ(0.0);
      }
      if(flags & PhysicFlags::LockRotation)
      {
        rBody->setAngularFactor(0.0);
      }

      rBody->setLinearFactor(linearFactor);

      rBody->setWorldTransform(trans);
      
      if(mass!=0 && flags & PhysicFlags::NoGravity)
      {
        rBody->setFlags(rBody->getFlags() | BT_DISABLE_WORLD_GRAVITY);
      }
      if(mass==0 && !(flags & PhysicFlags::Static))
      {
        flags |= PhysicFlags::Static;
      }
      //else
      //m_Flags &= ~BTC_Kinematic;
      
      m_Object->setUserPointer(this);
      if(iInitData.GetFlags() & (PhysicFlags::Static | PhysicFlags::Kinematic))
      {
        m_Object->setCollisionFlags(m_Object->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        m_Object->setActivationState(DISABLE_DEACTIVATION);
      }
      else
      {
        m_Object->setActivationState(ACTIVE_TAG);
      }
    }

    if (flags & PhysicFlags::AddSensor)
    {
      m_Sensor = eXl_Bullet_NEW(btPairCachingGhostObject);
      m_Sensor->setCollisionShape(m_Shape);
      m_Sensor->setUserPointer(this);
      m_Sensor->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE | btCollisionObject::CF_KINEMATIC_OBJECT);
      m_Sensor->setWorldTransform(trans);
    }
  }
}