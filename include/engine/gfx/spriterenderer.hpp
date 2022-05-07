#pragma once

#include <engine/gfx/gfxsystem.hpp>
#include <engine/gfx/gfxcomponent.hpp>

namespace eXl
{
  struct SpriteRenderer
  {
    SpriteRenderer(GfxSystem& iSys, GameDataView<GfxSpriteComponent::Desc> const& iSpriteDescView, DenseGameDataView<GfxSpriteData>& iSpriteData);
    ~SpriteRenderer();

    void UpdateTransforms(ObjectHandle const* iObjects, Mat4 const** iTransforms, uint32_t iNum);
    void PrepareSprites();

    void TickAnimation(ObjectHandle, GfxSpriteData& iData, float iDelta);

    GameDataView<GfxSpriteComponent::Desc> const& m_SpriteDescView;
    DenseGameDataView<GfxSpriteData>& m_SpriteData;
    Transforms& m_Transforms;

    UnorderedMap<Vec3, IntrusivePtr<GeometryInfo>> m_SpriteGeomCache;

    UniquePtr<OGLCompiledProgram const> m_SpriteProgram;
    IntrusivePtr<OGLBuffer> m_DefaultSpriteIdxBuffer;

    UnorderedSet<ObjectHandle> m_DirtyComponents;
  };
}