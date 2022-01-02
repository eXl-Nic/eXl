#include <engine/gfx/gfxguirendernode.hpp>

#include <engine/gui/fontresource.hpp>

#include <engine/common/transforms.hpp>

#include <ogl/renderer/oglshaderdata.hpp>
#include <ogl/renderer/oglrendercommand.hpp>
#include <ogl/renderer/ogldisplaylist.hpp>
#include <ogl/renderer/oglcompiledprogram.hpp>
#include <ogl/oglspritealgo.hpp>

#include <utf8.h>

namespace eXl
{
  IMPLEMENT_RTTI(GfxGUIRenderNode);

  struct Text
  {
    String m_Text;
    uint32_t m_Size;
    Resource::UUID m_FontId;

    Matrix4f m_Transform;
    uint32_t m_NumElems;
    IntrusivePtr<OGLBuffer> m_TextData;
    OGLVAssembly m_Assembly;
    OGLShaderData* m_TextureData;
    OGLShaderData m_ShaderData;
  };

  struct FontCache
  {
    struct Entry
    {
      IntrusivePtr<OGLTexture> m_Texture;
      OGLShaderData m_TextureData;
      UnorderedMap<uint32_t, AABB2Df> m_AtlasLoc;
      Vector2i m_CurAtlasLoc;
      uint32_t m_CurMaxY = 0;
    };

    struct Key
    {
      FontResource const* m_Font;
      uint32_t m_Size;

      bool operator == (Key const& iOther) const
      {
        return m_Font == iOther.m_Font && m_Size == iOther.m_Size;
      }
    };

    using FontMap = UnorderedMap<Key, Entry>;
    FontMap m_Fonts;
    using FontEntry = FontMap::value_type;
    Vector<uint8_t> m_GlyphCopyBuffer;

    FontEntry& GetOrCreateEntry(Key const& iKey)
    {
      auto iter = m_Fonts.find(iKey);
      if (iter != m_Fonts.end())
      {
        return *iter;
      }

      Entry newEntry;

      Image::Size texSize(16 * iKey.m_Size, 16 * iKey.m_Size);
      OGLTextureType textureType = OGLTextureType::TEXTURE_2D;
      OGLInternalTextureFormat format = OGLInternalTextureFormat::RED;
      newEntry.m_Texture = MakeRefCounted<OGLTexture>(texSize, textureType, format);
      newEntry.m_Texture->AllocateTexture();
      newEntry.m_TextureData.AddTexture(OGLSpriteAlgo::GetUnfilteredTexture(), newEntry.m_Texture);

      iter = m_Fonts.insert(std::make_pair(iKey, newEntry)).first;

      // Bootstrap texture
      for (uint32_t glyph = 'a'; glyph <= 'z'; glyph++)
      {
        AddGlyph(*iter, glyph);
      }

      for (uint32_t glyph = 'A'; glyph <= 'Z'; glyph++)
      {
        AddGlyph(*iter, glyph);
      }

      for (uint32_t glyph = '0'; glyph <= '9'; glyph++)
      {
        AddGlyph(*iter, glyph);
      }

      AddGlyph(*iter, '.');
      AddGlyph(*iter, ',');
      AddGlyph(*iter, ' ');

      return *iter;
    }

    Font::GlyphDesc GetGlyph(FontEntry& iEntry, uint32_t iGlyph, AABB2Df& oTexBox)
    {
      auto iter = iEntry.second.m_AtlasLoc.find(iGlyph);
      if (iter != iEntry.second.m_AtlasLoc.end())
      {
        oTexBox = iter->second;
      }
      else
      {
        oTexBox = AddGlyph(iEntry, iGlyph);
      }
      return iEntry.first.m_Font->GetGlyphDesc(iGlyph, iEntry.first.m_Size);
    }

    AABB2Df AddGlyph(FontEntry& iEntry, uint32_t iGlyph)
    {
      AABB2Df glyphBox;
      auto renderCb = [&glyphBox, &iEntry, iGlyph, this](Font::GlyphDesc iDesc, uint8_t const* iData)
      {
        Entry& fontEntry = iEntry.second;
        if ((iDesc.glyphSize.Y() + 2) > fontEntry.m_CurMaxY)
        {
          fontEntry.m_CurMaxY = (iDesc.glyphSize.Y() + 2);
        }

        float halfPixelOffsetX = iDesc.glyphSize.X() > 1 ? 0.5 : 0.0;
        float halfPixelOffsetY = iDesc.glyphSize.Y() > 1 ? 0.5 : 0.0;

        Image::Size texSize = fontEntry.m_Texture->GetSize();
        if (fontEntry.m_CurAtlasLoc.X() + iDesc.glyphSize.X() + 2 >= texSize.X())
        {
          fontEntry.m_CurAtlasLoc.X() = 0;
          fontEntry.m_CurAtlasLoc.Y() += fontEntry.m_CurMaxY;
          fontEntry.m_CurMaxY = 0;
          eXl_ASSERT(fontEntry.m_CurAtlasLoc.Y() < texSize.Y());
        }

        glyphBox = AABB2Df(fontEntry.m_CurAtlasLoc.X() + (1.0 - halfPixelOffsetX),
          fontEntry.m_CurAtlasLoc.Y() + (1.0 - halfPixelOffsetY),
          fontEntry.m_CurAtlasLoc.X() + (1.0 - halfPixelOffsetX) + iDesc.glyphSize.X(),
          fontEntry.m_CurAtlasLoc.Y() + (1.0 - halfPixelOffsetY) + iDesc.glyphSize.Y());

        glyphBox.m_Data[0].X() *= 1.0 / texSize.X();
        glyphBox.m_Data[0].Y() *= 1.0 / texSize.Y();
        glyphBox.m_Data[1].X() *= 1.0 / texSize.X();
        glyphBox.m_Data[1].Y() *= 1.0 / texSize.Y();
        {
          AABB2Di texLoc(fontEntry.m_CurAtlasLoc.X(),
            fontEntry.m_CurAtlasLoc.Y(),
            fontEntry.m_CurAtlasLoc.X() + iDesc.glyphSize.X() + 2,
            fontEntry.m_CurAtlasLoc.Y() + iDesc.glyphSize.Y() + 2);

          size_t totSize = texLoc.GetSize().X() * texLoc.GetSize().Y();
          if (m_GlyphCopyBuffer.size() <= totSize)
          {
            m_GlyphCopyBuffer.resize(totSize);
          }
          uint8_t const* src = iData;
          uint8_t* dst = m_GlyphCopyBuffer.data();
#if 0
          memset(dst, 0, margin * (bitmap.width + 2 * margin) + margin);
          dst += margin * (bitmap.width + 2 * margin) + margin;

          for (uint32_t i = 0; i < bitmap.rows; ++i)
          {
            memcpy(dst, src, bitmap.width);
            dst += bitmap.width;
            memset(dst, 0, 2 * margin);
            dst += 2 * margin;
            src += bitmap.width;
          }
          memset(dst, 0, margin * bitmap.width + margin);

          //Image filter(filterCoeff, Vector2i(3,3), Image::R, Image::Float, 1, Image::Reference);
          Image filter(filterCoeff5, Image::Size(5, 5), Image::R, Image::Float, 1, Image::Reference);
          Image newBitmap(m_Impl->m_GlyphCopyBuffer, Image::Size(bitmap.width + 2 * margin, bitmap.rows + 2 * margin), Image::R, Image::Char, 1, Image::Reference);
          //newBitmap.Convolve(filter, Vector2i(-1,-1));
          //newBitmap.Convolve(filter, Vector2i(-2,-2));

          dst = m_Impl->m_GlyphCopyBuffer;
          src = reinterpret_cast<unsigned char const*>(newBitmap.GetImageData());
          src += margin * (bitmap.width + 2 * margin) + margin;
#endif
          memset(dst, 0, iDesc.glyphSize.X() + 2 + 1);
          dst += iDesc.glyphSize.X() + 2 + 1;

          for (uint32_t i = 0; i < iDesc.glyphSize.Y(); ++i)
          {
            memcpy(dst, src, iDesc.glyphSize.X());
            dst += iDesc.glyphSize.X();
            dst[0] = 0;
            dst[1] = 1;
            dst += 2;
            src += iDesc.glyphSize.X();
          }
          memset(dst, 0, iDesc.glyphSize.X() + 1);

          //Assume 256 grey level format
          fontEntry.m_Texture->Update(texLoc, OGLTextureElementType::UNSIGNED_BYTE, OGLTextureFormat::RED, m_GlyphCopyBuffer.data());
          fontEntry.m_AtlasLoc.insert(std::make_pair(iGlyph, glyphBox));
          fontEntry.m_CurAtlasLoc.X() += iDesc.glyphSize.X() + 2;
          if (fontEntry.m_CurAtlasLoc.X() >= texSize.X())
          {
            fontEntry.m_CurAtlasLoc.X() = 0;
            fontEntry.m_CurAtlasLoc.Y() += fontEntry.m_CurMaxY;
            fontEntry.m_CurMaxY = 0;
            eXl_ASSERT(fontEntry.m_CurAtlasLoc.Y() < texSize.Y());
          }
        }
      };
      iEntry.first.m_Font->RenderGlyph(iGlyph, iEntry.first.m_Size, renderCb);
      return glyphBox;
    }
  };

  size_t hash_value(FontCache::Key const& iKey)
  {
    size_t hash = 0;
    boost::hash_combine(hash, iKey.m_Font);
    boost::hash_combine(hash, iKey.m_Size);
    return hash;
  }

  struct GfxGUIRenderNode::Impl
  {
    Impl(GfxSystem& iSys)
      : m_TextElements(iSys.GetWorld())
    {
      m_FontProgram.reset(OGLSpriteAlgo::CreateFontProgram(iSys.GetSemanticManager()));
    }

    UniquePtr<OGLCompiledProgram const> m_FontProgram;
    SpriteColor m_DefaultColor;
    OGLShaderData m_DefaultData;
    FontCache m_Fonts;
    DenseGameDataStorage<Text> m_TextElements;
  };

  GfxGUIRenderNode::GfxGUIRenderNode() = default;
  GfxGUIRenderNode::~GfxGUIRenderNode() = default;

  struct TextLine
  {
    uint32_t length;
    uint32_t begin;
    uint32_t end;
    float linePos;
  };
  
  enum Anchor
  {
    AnchorLeft,
    AnchorMiddle,
    AnchorRight
  };

  template <typename Iterator>
  uint32_t GetNextSymbol(Iterator& iIter, Iterator const& iIterEnd)
  {
    uint32_t cp = 0;
    utf8::internal::utf_error err_code = utf8::internal::validate_next(iIter, iIterEnd, cp);
    switch (err_code) 
    {
    case utf8::internal::UTF8_OK:
      break;
    case utf8::internal::NOT_ENOUGH_ROOM:
      LOG_ERROR << "";
      break;
    case utf8::internal::INVALID_LEAD:
    case utf8::internal::INCOMPLETE_SEQUENCE:
    case utf8::internal::OVERLONG_SEQUENCE:
      LOG_ERROR << "Invalid utf8 :" << *iIter;
      break;
    case utf8::internal::INVALID_CODE_POINT:
      LOG_ERROR << "Invalid symbol :" << cp;
      cp = 0;
      break;
    }
    return cp;
  }
  

  void GfxGUIRenderNode::AddText(ObjectHandle iObject, String const& iText, uint32_t iSize, FontResource const* iFont)
  {
    if (iFont == nullptr)
    {
      return;
    }

    FontCache::Key key({ iFont, iSize });
    auto& entry = m_Impl->m_Fonts.GetOrCreateEntry(key);

    uint32_t curLineBegin = 0;
    uint32_t curLineEnd = 0;
    float curLineMin = 0;
    float curLineMax = 0;

    bool beginningOfLine = true;
    std::vector<TextLine> lines;
    int32_t maxLength = 0;

    Vector2i penPos(0, iSize);

    Anchor iAnchor = AnchorLeft;

    Vector<AABB2Df> oPos;
    Vector<AABB2Df> oTexCoords;

    String::const_iterator iterTxt = iText.begin();
    String::const_iterator iterTxtEnd = iText.end();
    while (iterTxt != iterTxtEnd)
    {
      uint32_t symbol = GetNextSymbol(iterTxt, iterTxtEnd);

      if (symbol == '\n')
      {
        if (!oPos.empty() && curLineBegin < curLineEnd)
        {
          TextLine curLine = { uint32_t(penPos.X()), curLineBegin, curLineEnd, penPos.Y() - curLineMax};
          if (maxLength < penPos.X())
          {
            maxLength = penPos.X();
          }
          lines.push_back(curLine);
        }
        penPos = Vector2i(0, penPos.Y() - (curLineMax - curLineMin) - 1);
        curLineBegin = curLineEnd = oPos.size();
        beginningOfLine = true;
        curLineMin = curLineMax = 0;
      }
      else
      {
        AABB2Df glyphBox;
        Font::GlyphDesc desc = m_Impl->m_Fonts.GetGlyph(entry, symbol, glyphBox);
        if (desc.glyphSize.X() != 0 && desc.glyphSize.Y() != 0)
        {
          oPos.push_back(AABB2Df(penPos.X() + desc.penOffset.X(),
            desc.penOffset.Y() - desc.glyphSize.Y(),
            penPos.X() + (desc.penOffset.X() + desc.glyphSize.X()),
            desc.penOffset.Y()));
          oTexCoords.push_back(glyphBox);

          curLineMin = Mathf::Min(oPos.back().MinY(), curLineMin);
          curLineMax = Mathf::Max(oPos.back().MaxY(), curLineMax);

          curLineEnd++;
          penPos = penPos + Vector2i(desc.penAdvance.X(), desc.penAdvance.Y());
          beginningOfLine = false;
        }
        else
        {
          if (!beginningOfLine)
          {
            penPos = penPos + Vector2i(desc.penAdvance.X(), desc.penAdvance.Y());
          }
        }
      }
    }

    if (!oPos.empty() && curLineBegin < curLineEnd)
    {
      TextLine curLine = { uint32_t(penPos.X()), curLineBegin, curLineEnd, penPos.Y() - curLineMax };
      if (maxLength < penPos.X())
      {
        maxLength = penPos.X();
      }
      lines.push_back(curLine);
    }

    for (uint32_t i = 0; i < lines.size(); ++i)
    {
      float offsetX = 0;
      if (iAnchor == AnchorMiddle)
      {
        offsetX = (float(maxLength) - float(lines[i].length)) / 2.0f - maxLength / 2;
      }
      else if (iAnchor == AnchorRight)
      {
        offsetX = (float(maxLength) - float(lines[i].length)) - maxLength;
      }
      for (uint32_t j = lines[i].begin; j < lines[i].end; ++j)
      {
        Vector2f glyphSize = oPos[j].GetSize();
        oPos[j].m_Data[0].X() += offsetX;
        oPos[j].m_Data[1].X() += offsetX;
        oPos[j].m_Data[0].Y() += lines[i].linePos;
        oPos[j].m_Data[1].Y() += lines[i].linePos;
      }
    }

    Text& textElem = m_Impl->m_TextElements.GetOrCreate(iObject);
    textElem.m_Text = iText;
    textElem.m_Size = iSize;
    textElem.m_FontId = iFont->GetHeader().m_ResourceId;

    size_t totBufferSize = oPos.size() * 4 * 5 * sizeof(float);
    size_t totIdxBufferSize = oPos.size() * 6 * sizeof(uint32_t);

    Vector<float> geomData;
    Vector<uint32_t> geomIdxData;
    geomData.reserve(oPos.size() * 4 * 5);
    geomIdxData.reserve(oPos.size() * 6);
    for (uint32_t i = 0; i < oPos.size(); ++i)
    {
      AABB2Df const& geomBox = oPos[i];
      AABB2Df const& texBox = oTexCoords[i];

      int32_t const permutation[] = { 0, 0, 1, 0, 1, 1, 0, 1 };
      for (uint32_t j = 0; j < 4; ++j)
      {
        geomData.push_back(geomBox.m_Data[permutation[2 * j + 0]].X());
        geomData.push_back(geomBox.m_Data[permutation[2 * j + 1]].Y());
        geomData.push_back(0.0);
        geomData.push_back(texBox.m_Data[permutation[2 * j + 0]].X());
        geomData.push_back(texBox.m_Data[1 - permutation[2 * j + 1]].Y());
      }
      uint32_t basePos = i * 4;
      uint32_t const quad[] = { 0, 1, 2, 0, 2, 3};
      for (uint32_t j = 0; j < 6; ++j)
      {
        geomIdxData.push_back(basePos + quad[j]);
      }
    }

    textElem.m_NumElems = geomIdxData.size();
    textElem.m_TextData = IntrusivePtr<OGLBuffer>(OGLBuffer::CreateBuffer(OGLBufferUsage::ARRAY_BUFFER, totBufferSize, geomData.data()));
    textElem.m_Assembly.m_IBuffer = IntrusivePtr<OGLBuffer>(OGLBuffer::CreateBuffer(OGLBufferUsage::ELEMENT_ARRAY_BUFFER, totIdxBufferSize, geomIdxData.data()));
    textElem.m_Assembly.m_IOffset = 0;
    textElem.m_Assembly.AddAttrib(textElem.m_TextData, OGLBaseAlgo::GetPosAttrib(), 3, 5 * sizeof(float), 0);
    textElem.m_Assembly.AddAttrib(textElem.m_TextData, OGLBaseAlgo::GetTexCoordAttrib(), 2, 5 * sizeof(float), 3 * sizeof(float));

    textElem.m_TextureData = &entry.second.m_TextureData;
    textElem.m_Transform = m_Sys->GetTransforms().GetWorldTransform(iObject);
    textElem.m_ShaderData.AddData(OGLBaseAlgo::GetWorldMatUniform(), &textElem.m_Transform);

    AddObject(iObject);
  }

  void GfxGUIRenderNode::Init(GfxSystem& iSys, GfxRenderNodeHandle iHandle)
  {
    GfxRenderNode::Init(iSys, iHandle);
    m_Impl = std::make_unique<Impl>(iSys);
    m_Impl->m_DefaultColor.tint = Vector4f::ONE;
    m_Impl->m_DefaultData.AddData(OGLSpriteAlgo::GetSpriteColorUniform(), &m_Impl->m_DefaultColor);
  }

  GfxRenderNode::TransformUpdateCallback GfxGUIRenderNode::GetTransformUpdateCallback()
  {
    return [this](ObjectHandle const* iObjects, Matrix4f const** iTransforms, uint32_t iNum)
    {
      for (uint32_t i = 0; i < iNum; ++i, ++iObjects, ++iTransforms)
      {
        if (Text* txt = m_Impl->m_TextElements.Get(*iObjects))
        {
          txt->m_Transform = (**iTransforms);
        }
      }
    };
  }

  GfxRenderNode::UpdateCallback GfxGUIRenderNode::GetDeleteCallback()
  {
    return [this](ObjectHandle const* iObjects, uint32_t iNum)
    {
      for (uint32_t i = 0; i < iNum; ++i, ++iObjects)
      {
        m_Impl->m_TextElements.Erase(*iObjects);
      }
    };
  }

  void GfxGUIRenderNode::Push(OGLDisplayList& iList, float iDelta)
  {
    iList.SetDepth(false, false);
    iList.SetProgram(m_Impl->m_FontProgram.get());
    
    iList.PushData(&m_Impl->m_DefaultData);

    m_Impl->m_TextElements.Iterate([&iList](ObjectHandle iObject, Text const& iText)
      {
        iList.PushData(iText.m_TextureData);
        iList.PushData(&iText.m_ShaderData);
        iList.SetVAssembly(&iText.m_Assembly);
        iList.PushDraw(0x1000, OGLDraw::TriangleList, iText.m_NumElems, 0, 0);
        iList.PopData();
        iList.PopData();
      });
    
    iList.PopData();
  }
  
}