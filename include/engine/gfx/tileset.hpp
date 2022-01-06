/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/enginelib.hpp>
#include <core/resource/resource.hpp>
#include <core/image/image.hpp>
#include <core/stream/serializer.hpp>
#include <core/path.hpp>
#include <ogl/renderer/ogltexture.hpp>
#include <math/aabb2d.hpp>

namespace eXl
{
  MAKE_NAME(TileName);
  MAKE_NAME(ImageName);

  enum class AnimationType
  {
    None,
    Loop,
    Pingpong,
  };

  struct EXL_ENGINE_API Tile
  {
    static Type const* GetType();
    static constexpr bool eXl_Reflected = true;

    static ImageName EmptyName();

    Err Stream(Streamer& iStreamer) const;
    Err Unstream(Unstreamer& iStreamer);

    ImageName m_ImageName = EmptyName();

    Vector2i m_Size;
    Vector<Vector2i> m_Frames;
    float m_FrameDuration = 0.15;
    AnimationType m_AnimType = AnimationType::None;

    Vector2f m_Offset = Vector2f::ZERO;
    Vector2f m_Scale = Vector2f::ONE;
  };

  class TilesetLoader;

  class EXL_ENGINE_API Tileset : public Resource
  {
    DECLARE_RTTI(Tileset, Resource)
  public:

    using TileMap = UnorderedMap<TileName, Tile>;

    static void Init();

#ifndef EXL_IS_BAKED_PLATFORM
    static Tileset* Create(Path const& iDir, String const& iName);
    boost::optional<ImageName> ImageNameFromImagePath(Path const& iImagePath);
    Path GetSrcImagePath(ImageName iImage) const;
#endif

    static ResourceLoaderName StaticLoaderName();

    Image const* GetImage(ImageName iImage) const;
    IntrusivePtr<OGLTexture> GetTexture(ImageName iImage) const;

    Vector2i GetImageSize(ImageName iImage) const;

    Err Stream_Data(Streamer& iStreamer) const override;
    Err Unstream_Data(Unstreamer& iStreamer) override;

    uint32_t ComputeHash() override;

    TileMap::const_iterator begin() const { return m_Tiles.begin(); }
    TileMap::const_iterator end() const { return m_Tiles.end(); }

    Tile const* Find(TileName iTile) const 
    { 
      auto iter = m_Tiles.find(iTile);
      if (iter != m_Tiles.end())
      {
        return &iter->second;
      }
      return nullptr;
    }

    Err AddTile(TileName iName, Tile const& iTile);
    void RemoveTile(TileName iName);

  private:
    Err Serialize(Serializer iStreamer);

    UnorderedMap<TileName, Tile> m_Tiles;

    friend TilesetLoader;
    Tileset(ResourceMetaData&);

    void PostLoad() override;
#ifndef EXL_IS_BAKED_PLATFORM
    UnorderedMap<ImageName, Path> m_ImagePathCache;
#endif
    UnorderedMap<ImageName, std::unique_ptr<Image>> m_Images;
#ifdef EXL_WITH_OGL
    UnorderedMap<ImageName, IntrusivePtr<OGLTexture>> m_Textures;
#endif
  };

  EXL_REFLECT_ENUM(AnimationType, eXl__AnimationType, EXL_ENGINE_API)
}