#include <engine/gui/fontresource.hpp>

#include <core/resource/resourceloader.hpp>
#include <core/resource/resourcemanager.hpp>
#include <core/stream/inputstream.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SYSTEM_H
#include FT_GLYPH_H
#include FT_BITMAP_H

namespace eXl
{
  IMPLEMENT_RTTI(FontResource);

  class FontLoader : public ResourceLoader
  {
  public:

    static FontLoader& Get()
    {
      static FontLoader s_This;
      return s_This;
    }

    FontLoader()
      : ResourceLoader(FontResource::StaticLoaderName(), 1)
    {}

#ifndef EXL_IS_BAKED_PLATFORM
    FontResource* Create(Path const& iDir, String const& iName) const
    {
      ResourceMetaData* metaData = CreateNewMetaData(iName);

      Path rscPath = iDir / Path(iName.c_str());
      rscPath.replace_extension(ResourceManager::GetAssetExtension().c_str());

      FontResource* newTileset = eXl_NEW FontResource(*metaData);
      newTileset->SetFlags(Resource::LockPathDirectory);
      if (ResourceManager::SetPath(newTileset, rscPath))
      {
        return newTileset;
      }

      return nullptr;
    }
#endif

    FontResource* Create_Impl(ResourceMetaData* iMetaData) const override
    {
      FontResource* tileset = eXl_NEW FontResource(*iMetaData);

      return tileset;
    }

    bool NeedsBaking(Resource* iRsc) const override
    {
      return true;
    }

    FontResource* CreateBakedResource(Resource* iRsc) const override
    {
      eXl_FAIL_MSG("Not ready yet");
      FontResource* fontToBake = FontResource::DynamicCast(iRsc);
      FontResource* bakedFont = eXl_NEW FontResource(*CreateBakedMetaData(iRsc->GetMetaData()));

      bakedFont->PostLoad();

      return bakedFont;
    }

  };

  void FontResource::Init()
  {
    ResourceManager::AddLoader(&FontLoader::Get(), FontResource::StaticRtti());
  }


#ifndef EXL_IS_BAKED_PLATFORM
  FontResource* FontResource::Create(Path const& iDir, String const& iName, Path const& iFontFilePath)
  {
    Path resourceName = iDir / iName;
    Path fontFilePath = iFontFilePath;
    if (!Filesystem::exists(fontFilePath))
    {
      fontFilePath = iDir / iFontFilePath;
    }

    std::error_code ec;
    eXl_ASSERT_MSG_REPAIR_RET(Filesystem::exists(fontFilePath, ec) && Filesystem::is_regular_file(fontFilePath, ec),
      eXl_FORMAT("Font file %s does not exist", iFontFilePath.c_str()), nullptr);

    fontFilePath = Filesystem::absolute(Filesystem::canonical(fontFilePath));
    Path parentPath = fontFilePath.parent_path();
    while (!parentPath.empty())
    {
      if (parentPath == iDir)
      {
        break;
      }
    }

    eXl_ASSERT_MSG_REPAIR_RET(!parentPath.empty(),
      eXl_FORMAT("Font file %s is not contained in directory %s", iFontFilePath.c_str(), iDir.c_str()), nullptr);

    FontResource* newResource = FontLoader::Get().Create(iDir, iName);
    newResource->m_FontFileName = Filesystem::relative(fontFilePath, iDir, ec).u8string();
    newResource->PostLoad();
    return newResource;
  }

  Path FontResource::GetFontPath() const
  {
    Path resourcePath = ResourceManager::GetPath(GetHeader().m_ResourceId);
    Path resourceDir = resourcePath.parent_path();
    resourceDir /= m_FontFileName;

    return resourceDir;
  }
#endif

  ResourceLoaderName FontResource::StaticLoaderName()
  {
    ResourceLoaderName s_Name("Font");
    return s_Name;
  }

  FontResource::FontResource(ResourceMetaData& iMetadata)
    : Resource(iMetadata)
  {}

  FontResource::~FontResource() = default;

  Err FontResource::Stream_Data(Streamer& iStreamer) const
  {
    return const_cast<FontResource*>(this)->Serialize(Serializer(iStreamer));
  }

  Err FontResource::Unstream_Data(Unstreamer& iStreamer)
  {
    return Serialize(Serializer(iStreamer));
  }

  Err FontResource::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("FilePath");
    iStreamer &= m_FontFileName;
    iStreamer.PopKey();
    if (GetHeader().m_Flags & BakedResource)
    {
      iStreamer.PushKey("FileData");
      iStreamer &= m_FontFile;
      iStreamer.PopKey();
    }
    iStreamer.EndStruct();

    return Err::Success;
  }

  uint32_t FontResource::ComputeHash()
  {
    return 0;
  }

  struct FontResource::Impl
  {
    static unsigned long FT_IoFunc(FT_Stream stream, unsigned long offset, unsigned char* buffer, unsigned long count)
    {
      InputStream* memStream = (InputStream*)stream->descriptor.pointer;
      return memStream->Read(offset, count, buffer);
    }

    static void FT_CloseFunc(FT_Stream stream)
    {
      InputStream* memStream = (InputStream*)stream->descriptor.pointer;
      eXl_DELETE memStream;
      stream->descriptor.pointer = NULL;
      eXl_FREE(stream);
    }

    struct FTLibraryHolder
    {
      FTLibraryHolder()
      {
        FT_Error error = FT_Init_FreeType(&m_Library);
        if (error)
        {
          LOG_WARNING << "Error during freetype init" << "\n";
        }
      }
      FT_Library m_Library;
    };

    static FT_Library& GetLibrary()
    {
      static FTLibraryHolder s_Holder;
      return s_Holder.m_Library;
    }

    FT_Face m_Face;
    UniquePtr<InputStream> m_FileHandle;
    UnorderedMap<uint32_t, Font::GlyphDesc> m_GlyphMap;
  };

  void FontResource::PostLoad()
  {
    m_Impl = std::make_unique<Impl>();

    if (GetHeader().m_Flags & Resource::BakedResource)
    {
      m_Impl->m_FileHandle.reset(eXl_NEW BinaryInputStream(m_FontFile.data(), m_FontFile.size()));
    }
    else
    {
      m_Impl->m_FileHandle.reset(eXl_NEW FileInputStream(GetFontPath()));
    }
    eXl_ASSERT(m_Impl->m_FileHandle != nullptr);
    FT_StreamRec* streamRec = (FT_StreamRec*)eXl_ALLOC(sizeof(FT_StreamRec));
    memset(streamRec, 0, sizeof(FT_StreamRec));
    streamRec->close = &Impl::FT_CloseFunc;
    streamRec->read = &Impl::FT_IoFunc;
    streamRec->descriptor.pointer = m_Impl->m_FileHandle.get();
    streamRec->size = m_Impl->m_FileHandle->GetSize();
    streamRec->pos = 0;

    FT_Open_Args openArgs;
    memset(&openArgs, 0, sizeof(FT_Open_Args));
    openArgs.stream = streamRec;
    openArgs.flags = FT_OPEN_STREAM;

    FT_Error error = FT_Open_Face(Impl::GetLibrary(), &openArgs, 0, &m_Impl->m_Face);

    eXl_ASSERT(error != FT_Err_Unknown_File_Format);
    eXl_ASSERT(!error);
    FT_Select_Charmap(m_Impl->m_Face, FT_ENCODING_UNICODE);
  }

  Font::GlyphDesc FontResource::RenderGlyph(uint32_t iChar, uint32_t iSize, RenderCallback iRender) const
  {
    auto iter = m_Impl->m_GlyphMap.find(iChar);
    if (iter != m_Impl->m_GlyphMap.end() && !iRender)
    {
      return iter->second;
    }

    FT_Set_Pixel_Sizes(m_Impl->m_Face, 0, iSize);
    FT_UInt charIndex = FT_Get_Char_Index(m_Impl->m_Face, iChar);
    FT_Load_Glyph(m_Impl->m_Face, charIndex, FT_LOAD_DEFAULT);
    if (m_Impl->m_Face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
    {
      FT_Render_Glyph(m_Impl->m_Face->glyph, FT_RENDER_MODE_NORMAL);
    }
    Font::GlyphDesc newDesc;
    newDesc.penOffset = Vector2i(m_Impl->m_Face->glyph->bitmap_left, m_Impl->m_Face->glyph->bitmap_top);
    newDesc.penAdvance = Vector2i(m_Impl->m_Face->glyph->advance.x >> 6, m_Impl->m_Face->glyph->advance.y >> 6);
    newDesc.glyphSize = Vector2i(m_Impl->m_Face->glyph->bitmap.width, m_Impl->m_Face->glyph->bitmap.rows);

    if (iter == m_Impl->m_GlyphMap.end())
    {
      m_Impl->m_GlyphMap.insert(std::make_pair(iChar, newDesc));
    }

    if (iRender)
    {
      FT_Bitmap& bitmap = m_Impl->m_Face->glyph->bitmap;
      if (bitmap.pixel_mode == FT_PIXEL_MODE_GRAY)
      {
        iRender(newDesc, bitmap.buffer);
      }
      else
      {
        eXl_FAIL_MSG("TODO : Unsupported pixel mode for font rendering");
      }
    }

    return newDesc;
  }
}