/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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