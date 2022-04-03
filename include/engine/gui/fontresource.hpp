#pragma once

#include <engine/engineexp.hpp>
#include <math/math.hpp>
#include <math/aabb2d.hpp>
#include <core/resource/resource.hpp>
#include <core/path.hpp>
#include <core/stream/serializer.hpp>

namespace eXl
{
  class FontLoader;

  namespace Font
  {
    struct GlyphDesc
    {
      Vec2i penOffset;
      Vec2i penAdvance;
      Vec2i glyphSize;
    };
  }

  class EXL_ENGINE_API FontResource : public Resource
  {
    DECLARE_RTTI(FontResource, Resource);
  public:

    static void Init();

#ifndef EXL_IS_BAKED_PLATFORM
    static FontResource* Create(Path const& iDir, String const& iName, Path const& iFontFilePath);
    Path GetFontPath() const;
#endif

    ~FontResource();

    static ResourceLoaderName StaticLoaderName();
    uint32_t ComputeHash() override;

    using RenderCallback = std::function<void(Font::GlyphDesc, uint8_t const*)>;
    Font::GlyphDesc RenderGlyph(uint32_t iChar, uint32_t iSize, RenderCallback iRender = RenderCallback()) const;

    Font::GlyphDesc GetGlyphDesc(uint32_t iChar, uint32_t iSize) const
    {
      return RenderGlyph(iChar, iSize);
    }

  private:
    friend FontLoader;

    FontResource(ResourceMetaData&);

    Err Stream_Data(Streamer& iStreamer) const override;
    Err Unstream_Data(Unstreamer& iStreamer) override;
    Err Serialize(Serializer iStreamer);

    void PostLoad() override;

    String m_FontFileName;
    Vector<uint8_t> m_FontFile;

    struct Impl;
    UniquePtr<Impl> m_Impl;
  };

}
