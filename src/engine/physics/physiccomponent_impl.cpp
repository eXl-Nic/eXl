
#include <engine/physics/physiccomponent_impl.hpp>
#include <engine/physics/physicsys_impl.hpp>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include <engine/physics/physics_detail.hpp>
#include <engine/physics/physicsys.hpp>

#include <math/quaternion.hpp>
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
      m_CachedTransform = worldTrans;
      Matrix4f mat;
      mat.m_Data[15] = 1;
      worldTrans.getOpenGLMatrix(mat.m_Data);

      if ((m_InitData->GetFlags() & PhysicFlags::AlignRotToVelocity)
        && (m_InitData->GetFlags() & (PhysicFlags::IsGhost | PhysicFlags::Kinematic)) == 0
        && !m_Object->getInterpolationLinearVelocity().isZero())
      {
        Vector3f* rotMat[] =
        {
          reinterpret_cast<Vector3f*>(mat.m_Data + 0),
          reinterpret_cast<Vector3f*>(mat.m_Data + 4),
          reinterpret_cast<Vector3f*>(mat.m_Data + 8),
        };
        *rotMat[0] = FROM_BTVECT(m_Object->getInterpolationLinearVelocity());
        rotMat[0]->Normalize();
        *rotMat[2] = Vector3f::UNIT_Z;
        *rotMat[1] = rotMat[2]->Cross(*rotMat[0]);
      }

      m_System.m_MovedTransform.push_back(mat);
      m_System.m_MovedObject.push_back(m_ObjectId);
    }
  }

  bool PhysicComponent_Impl::SweepTest(Vector3f const& iFrom, Vector3f const& iTo, CollisionData& oRes, std::function<bool(PhysicComponent_Impl*)> const& iIgnore, uint16_t iMask)
  {
    return m_System.m_Impl->SweepTest(this, iFrom, iTo, oRes, iIgnore, iMask);
  }

  void PhysicComponent_Impl::Build(ObjectHandle iTransform, PhysicInitData& iInitData)
  {
    m_ObjectId = iTransform;
    m_InitData = &iInitData;

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

    Matrix4f const& transform = m_System.m_Transforms.GetWorldTransform(iTransform);

    btTransform trans;
    trans.setFromOpenGLMatrix(transform.m_Data);

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

  //BulletCompoundComponent* BulletComponent::GetCompound()
  //{
  //  if(m_Flags & BTC_MasterCompound)
  //  {
  //    if(!(m_Flags & (BTC_Static|BTC_Kinematic)))
  //    {
  //      return ((MasterRigidMotionState*)m_MState)->GetCompound();
  //    }
  //    else
  //    {
  //      return m_Compound;
  //    }
  //  }
  //  return NULL;
  //}

  
}