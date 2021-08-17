#pragma once

#include <engine/common/world.hpp>
#include <engine/gfx/tileset.hpp>
#include <math/aabb2dpolygon.hpp>

namespace eXl
{
  class GfxSystem;
  class TilingGroup;

  namespace MapTiler
  {
    struct EXL_ENGINE_API TexGroup
    {
      Tileset const* m_Tileset;
      ImageName m_Name;
      Vector2f m_VtxScaling = Vector2f::ONE;
      Vector2f m_VtxOffset = Vector2f::ZERO;
      Vector2f m_Tiling = Vector2f::ONE;

      bool operator==(TexGroup const& iOther) const;
    };

    EXL_ENGINE_API size_t hash_value(TexGroup const& iGroup);

    struct EXL_ENGINE_API Batcher
    {
      UnorderedMap<TexGroup, uint32_t> textures;
      Vector<Vector<float>> allTiles;

      Vector<float>& GetGroup(TexGroup const& iGroup);

      void Finalize(GfxSystem& iGfx, ObjectHandle iObject, uint8_t iLayer);
    };

    struct Blocks
    {
      Vector<AABB2DPolygoni> islands;
      TilingGroup const* group;
    };

    EXL_ENGINE_API void ComputeGfxForBlock(Batcher& oBatcher, AABB2Di const& iFullSize, Blocks const& iBlock);
  }
}