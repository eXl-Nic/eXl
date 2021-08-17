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