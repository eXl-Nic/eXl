/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ogl/renderer/ogltexture.hpp>
#include <ogl/renderer/ogltextureloader.hpp>
#include <ogl/renderer/ogltypesconv.hpp>
#include <ogl/renderer/oglinclude.hpp>

#include <core/log.hpp>

namespace eXl
{
  IMPLEMENT_RefC(OGLTexture);

  OGLTexture::OGLTexture(Image::Size const& iSize, OGLTextureType iTextureType, OGLInternalTextureFormat iFormat, uint32_t iNumSlices)
    : m_Size(iSize)
    , m_NumSlices(iNumSlices)
    , m_InternalFormat(iFormat)
    , m_TextureType(iTextureType)
    , m_TexId(0)
    , m_Loader(NULL)
  {
    eXl_ASSERT((m_TextureType != OGLTextureType::TEXTURE_2D_ARRAY 
      && m_TextureType != OGLTextureType::TEXTURE_3D) || iNumSlices > 0);
    
  }

  OGLTextureElementType OGLTexture::GetElementType() const
  {
    switch (m_InternalFormat)
    {
    case OGLInternalTextureFormat::DEPTH_COMPONENT:
      return OGLTextureElementType::UNSIGNED_INT;
      break;
    case OGLInternalTextureFormat::DEPTH_STENCIL:
      return OGLTextureElementType::UNSIGNED_INT;
      break;
    case OGLInternalTextureFormat::RED:
    case OGLInternalTextureFormat::RG:
    case OGLInternalTextureFormat::RGB:
    case OGLInternalTextureFormat::RGBA:
      return OGLTextureElementType::UNSIGNED_BYTE;
      break;
    case OGLInternalTextureFormat::R32F:
    case OGLInternalTextureFormat::RG32F:
    case OGLInternalTextureFormat::RGB32F:
    case OGLInternalTextureFormat::RGBA32F:
      return OGLTextureElementType::FLOAT;
      break;
    default:
      eXl_FAIL_MSG_RET("Unrecognized Type", OGLTextureElementType::UNSIGNED_INT);
    }
  }

  OGLTextureFormat OGLTexture::GetElementFormat() const
  {
    switch (m_InternalFormat)
    {
    case OGLInternalTextureFormat::DEPTH_COMPONENT:
      return OGLTextureFormat::DEPTH_COMPONENT;
      break;
    case OGLInternalTextureFormat::DEPTH_STENCIL:
      return OGLTextureFormat::DEPTH_STENCIL;
      break;
    case OGLInternalTextureFormat::RED:
    case OGLInternalTextureFormat::R32F:
      return OGLTextureFormat::RED;
      break;
    case OGLInternalTextureFormat::RG:
    case OGLInternalTextureFormat::RG32F:
      return OGLTextureFormat::RG;
      break;
    case OGLInternalTextureFormat::RGB:
    case OGLInternalTextureFormat::RGB32F:
      return OGLTextureFormat::RGB;
      break;
    case OGLInternalTextureFormat::RGBA:
    case OGLInternalTextureFormat::RGBA32F:
      return OGLTextureFormat::RGBA;
      break;
    default:
      eXl_FAIL_MSG_RET("Unrecognized Type", OGLTextureFormat::DEPTH_COMPONENT);
    }
  }

  uint32_t OGLTexture::GetGLElementType() const
  {
    return GetGLTextureElementType(GetElementType());
  }

  uint32_t OGLTexture::GetGLElementFormat() const
  {
    return GetGLTextureFormat(GetElementFormat());
  }

  //void OGLTexture::OnNullRefC() const
  //{
  //  eXl_DELETE this;
  //}

  OGLTexture::~OGLTexture()
  {
    //if(m_TextureData != NULL)
    //{
    //  eXl_FREE(m_TextureData);
    //}
    if(m_TexId != 0)
    {
      glDeleteTextures(1,&m_TexId);
    }
    if(m_Loader != NULL)
    {
      //m_Loader->ForgetTexture(this);
    }
  }

  //void OGLTexture::DropTextureData()
  //{
  //  if(m_TextureData != NULL)
  //  {
  //    eXl_FREE(m_TextureData);
  //    m_TextureData = NULL;
  //  }
  //}

  void OGLTexture::Update(AABB2Di iBox, OGLTextureElementType iType, OGLTextureFormat iFormat, void const* iData, uint32_t iMip, uint32_t iSlice)
  {
    if(m_TexId != 0 && iData != NULL)
    {
      GLenum textureTarget = GetGLTextureType(m_TextureType);
      
      GLenum inputType = GetGLTextureElementType(iType);
      GLenum inputFormat = GetGLTextureFormat(iFormat);

      if (m_TextureType == OGLTextureType::TEXTURE_1D)
      {
        Vector2i boxSize = iBox.GetSize();
        if (iBox.MaxX() > m_Size.X()
          || iBox.MinX() < 0
          || boxSize.X() <= 0)
        {
          eXl_FAIL_MSG_RET("Invalid box dimensions", void());
        }
        
        glBindTexture(textureTarget, m_TexId);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage1D(textureTarget, iMip, iBox.MinX(), boxSize.X(), GetGLTextureFormat(iFormat), GetGLTextureElementType(iType), iData);
      }

      if (m_TextureType == OGLTextureType::TEXTURE_1D_ARRAY
        || m_TextureType == OGLTextureType::TEXTURE_2D
        || m_TextureType == OGLTextureType::TEXTURE_CUBE_MAP)
      {
        GLenum textureFaceUpdate = IsCubeMap() ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + iSlice : GL_TEXTURE_2D;

        Vector2i boxSize = iBox.GetSize();
        if (iBox.MaxX() > m_Size.X() || iBox.MaxY() > m_Size.Y()
          || iBox.MinX() < 0 || iBox.MinY() < 0
          || boxSize.X() <= 0 && boxSize.Y() <= 0)
        {
          eXl_FAIL_MSG_RET("Invalid box dimensions", void());
        }
        eXl_ASSERT_REPAIR_RET(!IsCubeMap() || (iSlice >= 0 && iSlice < 6), void());
        
        glBindTexture(textureTarget, m_TexId);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(textureFaceUpdate, iMip, iBox.MinX(), iBox.MinY(), boxSize.X(), boxSize.Y(), inputFormat, inputType, iData);
      }

      if (m_TextureType == OGLTextureType::TEXTURE_2D_ARRAY
        || m_TextureType == OGLTextureType::TEXTURE_3D)
      {
        Vector2i boxSize = iBox.GetSize();
        if (iBox.MaxX() > m_Size.X() || iBox.MaxY() > m_Size.Y()
          || iBox.MinX() < 0 || iBox.MinY() < 0
          || boxSize.X() <= 0 && boxSize.Y() <= 0)
        {
          eXl_FAIL_MSG_RET("Invalid box dimensions", void());
        }
        eXl_ASSERT_REPAIR_RET(iSlice < m_NumSlices, void());

        glBindTexture(textureTarget, m_TexId);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        glTexSubImage3D(textureTarget, iMip, iBox.MinX(), iBox.MinY(), iSlice, boxSize.X(), boxSize.Y(), 1, inputFormat, inputType, iData);
        
      }
    }
  }
}