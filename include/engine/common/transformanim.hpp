/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/common/animation.hpp>
#include <engine/common/world.hpp>

namespace eXl
{
  class Transforms;
  class TransformAnimManager;
  struct EXL_ENGINE_API TransformAnimation
  {
    bool Update(float iTime, TransformAnimManager&);

    LinearPositionAnimation* m_Anim;
    ObjectHandle m_Object;
    Matrix4f m_PreTransform;
    Matrix4f m_PostTransform;
  };

  class EXL_ENGINE_API TransformAnimManager : public WorldSystem, public TimelineManager<TransformAnimation, TransformAnimManager>
  {
    DECLARE_RTTI(TransformAnimManager, WorldSystem);
    friend TransformAnimation;
  public:

    TimelineHandle Start(ObjectHandle iObject
      , LinearPositionAnimation& iAnim
      , Matrix4f const& iPreTrans = Matrix4f::IDENTITY
      , Matrix4f const& iPostTrans = Matrix4f::IDENTITY);

    TimelineHandle StartLooping(ObjectHandle iObject, LinearPositionAnimation& iAnim, float iLoopTime
      , Matrix4f const& iPreTrans = Matrix4f::IDENTITY
      , Matrix4f const& iPostTrans = Matrix4f::IDENTITY);

  protected:

    void Register(World& iWorld) override;

    Transforms* m_Transforms;
  };
}