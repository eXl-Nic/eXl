/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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
    Vec3 newPos = m_Anim->GetValue(iTime, &animEnded);

    if (animEnded)
    {
      return false;
    }

    Mat4 basePos = Identity<Mat4>();
    Vec4 trans = m_PreTransform * Vec4(newPos.x, newPos.y, newPos.z, 1.0);
    basePos[3] = trans;
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

  TransformAnimManager::TimelineHandle TransformAnimManager::Start(ObjectHandle iObject, LinearPositionAnimation& iAnim, Mat4 const& iPreTrans, Mat4 const& iPostTrans)
  {
    TimelineEntry* newEntry;
    TimelineHandle newHandle = Impl_Start(newEntry);
    newEntry->m_Object = iObject;
    newEntry->m_Anim = &iAnim;
    newEntry->m_PreTransform = iPreTrans;
    newEntry->m_PostTransform = iPostTrans;

    return newHandle;
  }

  TransformAnimManager::TimelineHandle TransformAnimManager::StartLooping(ObjectHandle iObject, LinearPositionAnimation& iAnim, float iLoopTime, Mat4 const& iPreTrans, Mat4 const& iPostTrans)
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