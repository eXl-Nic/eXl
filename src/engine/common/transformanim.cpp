#include <engine/common/transformanim.hpp>
#include <engine/common/world.hpp>
#include <engine/common/transforms.hpp>

#include <engine/common/animation.cxx>
#include <math/mathtools.hpp>

namespace eXl
{
  template class TimelineManager<TransformAnimation, TransformAnimManager>;

  IMPLEMENT_RTTI(TransformAnimManager)

  bool TransformAnimation::Update(float iTime, TransformAnimManager& iManager)
  {
    if (!iManager.GetWorld().IsObjectValid(m_Object))
    {
      return false;
    }
    bool animEnded = false;
    Vector3f newPos = m_Anim->GetValue(iTime, &animEnded);

    if (animEnded)
    {
      return false;
    }

    Matrix4f basePos = Matrix4f::IDENTITY;
    Vector4f trans = m_PreTransform * Vector4f(newPos.X(), newPos.Y(), newPos.Z(), 1.0);
    MathTools::GetPosition(basePos) = reinterpret_cast<Vector3f&>(trans);
    iManager.m_Transforms->UpdateTransform(m_Object, m_PostTransform * basePos);

    return true;
  }

  void TransformAnimManager::Register(World& iWorld)
  {
    ParentRttiClass::Register(iWorld);
    m_Transforms = iWorld.GetSystem<Transforms>();
    iWorld.AddTick(World::PrePhysics, [this](World& iWorld, float)
    {
      Tick();
    });
  }

  TransformAnimManager::TimelineHandle TransformAnimManager::Start(ObjectHandle iObject, LinearPositionAnimation& iAnim, Matrix4f const& iPreTrans, Matrix4f const& iPostTrans)
  {
    TimelineEntry* newEntry;
    TimelineHandle newHandle = Impl_Start(newEntry);
    newEntry->m_Object = iObject;
    newEntry->m_Anim = &iAnim;
    newEntry->m_PreTransform = iPreTrans;
    newEntry->m_PostTransform = iPostTrans;

    return newHandle;
  }

  TransformAnimManager::TimelineHandle TransformAnimManager::StartLooping(ObjectHandle iObject, LinearPositionAnimation& iAnim, float iLoopTime, Matrix4f const& iPreTrans, Matrix4f const& iPostTrans)
  {
    TimelineEntry* newEntry;
    TimelineHandle newHandle = Impl_StartLooping(iLoopTime, newEntry);
    newEntry->m_Object = iObject;
    newEntry->m_Anim = &iAnim;
    newEntry->m_PreTransform = iPreTrans;
    newEntry->m_PostTransform = iPostTrans;

    return newHandle;
  }
}