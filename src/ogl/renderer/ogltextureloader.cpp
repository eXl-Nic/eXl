/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ogl/renderer/ogltextureloader.hpp>
#include <ogl/renderer/oglinclude.hpp>
#include <core/image/image.hpp>
#include <core/log.hpp>
#include <ogl/oglutils.hpp>
#include <ogl/renderer/ogltypesconv.hpp>

namespace eXl
{
  
  void UpdateFromImage(Image const& iImage, GLuint iTexId, bool iGenMipMap, int iFace);

  //OGLTextureLoader::OGLTextureLoader(std::list<DataVault*> const& iSource):m_Source(iSource)
  //{
  //
  //}
  //
  //void OGLTextureLoader::ForgetTexture(OGLTexture const* iTexture)
  //{
  //  TextureMap::iterator iter = m_TextureCache.begin();
  //  TextureMap::iterator iterEnd = m_TextureCache.end();
  //  for(; iter != iterEnd; ++iter)
  //  {
  //    if(iTexture == iter->second)
  //    {
  //      m_TextureCache.erase(iter);
  //      return;
  //    }
  //  }
  //}

  OGLTexture* OGLTextureLoader::Create(Image::Size const& iSize, Format iFormat)
  {
    return CreateFromBuffer(iSize,iFormat,NULL, false);
  }

  OGLTexture* OGLTextureLoader::CreateFromBuffer(Image::Size const& iSize, Format iFormat, void const* iData, bool iGenMipMap)
  {
    OGLTexture* res = NULL;

    if(iSize.X() > 0 && iSize.Y() > 0)
    {
      GLuint texId;
      glGenTextures(1, &texId);
      glBindTexture(GL_TEXTURE_2D,texId);
      OGLInternalTextureFormat internalFormat;
      GLuint dataType = GL_UNSIGNED_BYTE;
      GLuint dataComponents;
#ifdef __ANDROID__
      switch(iFormat)
      {
      case R8:
        internalFormat = GL_LUMINANCE;
        dataComponents = GL_LUMINANCE;
        break;
      case RG8:
        internalFormat = GL_LUMINANCE_ALPHA;
        dataComponents = GL_LUMINANCE_ALPHA;
        break;
      case RGB8:
        internalFormat = GL_RGB;
        dataComponents = GL_RGB;
        break;
      case RGBA8:
        internalFormat = GL_RGBA;
        dataComponents = GL_RGBA;
        break;
      }
#else
      switch(iFormat)
      {
      case R8:
        internalFormat = OGLInternalTextureFormat::RED;
        dataComponents = GL_RED;
        break;
      case RG8:
        internalFormat = OGLInternalTextureFormat::RG;
        dataComponents = GL_RG;
        break;
      case RGB8:
        internalFormat = OGLInternalTextureFormat::RGB;
        dataComponents = GL_RGB;
        break;
      case RGBA8:
        internalFormat = OGLInternalTextureFormat::RGBA;
        dataComponents = GL_RGBA;
        break;
      case R32F:
        internalFormat = OGLInternalTextureFormat::R32F;
        dataComponents = GL_RED;
        dataType = GL_FLOAT;
        break;
      case RG32F:
        internalFormat = OGLInternalTextureFormat::RG32F;
        dataComponents = GL_RG;
        dataType = GL_FLOAT;
        break;
      case RGB32F:
        internalFormat = OGLInternalTextureFormat::RGB32F;
        dataComponents = GL_RGB;
        dataType = GL_FLOAT;
        break;
      case RGBA32F:
        internalFormat = OGLInternalTextureFormat::RGBA32F;
        dataComponents = GL_RGBA;
        dataType = GL_FLOAT;
        break;
      }
#endif
      //Arbitrary alignment
      glPixelStorei(GL_UNPACK_ALIGNMENT,1);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
#ifndef __ANDROID__
      if(!iGenMipMap)
      {
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_BASE_LEVEL,0);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
      }
      else
      {
        glGenerateMipmap(GL_TEXTURE_2D);
      }
#endif
      glTexImage2D(GL_TEXTURE_2D,0, GetGLInternalTextureFormat(internalFormat), iSize.X(), iSize.Y(), 0, dataComponents, dataType, iData);

      res = eXl_NEW OGLTexture(iSize, OGLTextureType::TEXTURE_2D, internalFormat);
      res->m_TexId = texId;
    }
    return res;
  }

  template <class T, uint32_t iNumCol>
  void SwapRB(Vector2<uint32_t> const& iSize, void* iData, uint32_t iRowStride)
  {
    uint8_t* dataPtr = reinterpret_cast<uint8_t*>(iData);
    for(uint32_t i = 0; i < iSize.Y(); ++i)
    {
      T* curRow = (T*)dataPtr;
      for(uint32_t j = 0; j<iSize.X(); ++j)
      {
        std::swap(curRow[0], curRow[2]);
        curRow += iNumCol;
      }
      dataPtr += iRowStride;
    }
  }

#ifdef __ANDROID__
#define GL_BGR 0x80E0
#define GL_BGRA 0x80E1
#endif

  void GetOGLTypes(Image const& iImage, 
    OGLInternalTextureFormat& oInternalFormat, 
    OGLTextureElementType& oDataType, 
    OGLTextureFormat& oDataComponents)
  {
    switch(iImage.GetFormat())
    {
      case Image::Char:   oDataType = OGLTextureElementType::UNSIGNED_BYTE;  break;
      case Image::Short:  oDataType = OGLTextureElementType::UNSIGNED_SHORT; break;
      case Image::Int:    oDataType = OGLTextureElementType::UNSIGNED_INT;   break;
      case Image::Float:  oDataType = OGLTextureElementType::FLOAT;          break;
    }

    switch(iImage.GetComponents())
    {
    case Image::R:
    case Image::RG:
      //Pas envie de le gérer pour le moment.
      LOG_ERROR << "Unsupported format for image"<<"\n";
      break;
      //              
      //        internalFormat = GL_RGB;
      //        dataComponents = GL_RGB;
      //        switch(iImage->GetFormat())
      //        {
      //        case Image::Char:  dataType = GL_UNSIGNED_BYTE;  break;
      //        case Image::Short: dataType = GL_UNSIGNED_SHORT; break;
      //        case Image::Int:   dataType = GL_UNSIGNED_INT;   break;
      //        default : LOG_WARNING << "Strange format for Luminance texture : "<< bytesPP << " bytes per component" << "\n"; break;
      //        }
      //        break;

    case Image::RGB:
      oInternalFormat = OGLInternalTextureFormat::RGB;
      oDataComponents = OGLTextureFormat::RGB;
      break;
    case Image::BGR:
      oInternalFormat = OGLInternalTextureFormat::RGB;
      oDataComponents = OGLTextureFormat::BGR;
      break;
    case Image::RGBA:
      oInternalFormat = OGLInternalTextureFormat::RGBA;
      oDataComponents = OGLTextureFormat::RGBA;
      break;
    case Image::BGRA:
      oInternalFormat = OGLInternalTextureFormat::RGBA;
      oDataComponents = OGLTextureFormat::BGRA;
      break;
    }

  }

  OGLTexture* OGLTextureLoader::CreateFromImage(Image const* iImage, bool iGenMipMap)
  {
    if (iImage == NULL)
      return NULL;

    GLuint texId;
    glGenTextures(1, &texId);

    OGLInternalTextureFormat internalFormat;
    OGLTextureElementType dataType;
    OGLTextureFormat dataComponents;

    GetOGLTypes(*iImage, internalFormat, dataType, dataComponents);
    
    UpdateFromImage(*iImage, texId, false, -1);

    Image::Size const& imgSize = iImage->GetSize();

    OGLTexture* newTex = eXl_NEW OGLTexture(imgSize, OGLTextureType::TEXTURE_2D, internalFormat);
    //mouais....
    newTex->m_TexId = texId;

    return newTex;
  }

  OGLTexture* OGLTextureLoader::CreateCubeMap(Image const* const* iImage, bool iGenMipMap)
  {

    if(iImage[0] == NULL)
      return NULL;

    OGLInternalTextureFormat internalFormat;
    OGLTextureElementType dataType;
    OGLTextureFormat dataComponents;

    GetOGLTypes(*iImage[0], internalFormat, dataType, dataComponents);

    Image::Size const& imgSize = iImage[0]->GetSize();

    for(uint32_t i = 1; i<6; ++i)
    {
      if(iImage[i] == NULL)
        return NULL;

      Image::Size const& testImgSize = iImage[i]->GetSize();

      if(testImgSize != imgSize)
        return NULL;

      OGLInternalTextureFormat testInternalFormat;
      OGLTextureElementType testDataType;
      OGLTextureFormat testDataComponents;

      GetOGLTypes(*iImage[i], testInternalFormat, testDataType, testDataComponents);

      if(testInternalFormat != internalFormat
      || testDataType != dataType)
      {
        return NULL;
      }
    }

    GLuint texId;
    glGenTextures(1, &texId);

    for(uint32_t i = 0; i<6; ++i)
    {
      UpdateFromImage(*iImage[i], texId, false, i);
    }

    OGLTexture* newTex = eXl_NEW OGLTexture(imgSize, OGLTextureType::TEXTURE_CUBE_MAP, internalFormat);
    //mouais....
    newTex->m_TexId = texId;

    return newTex;
  }

  void UpdateFromImage(Image const& iImage, GLuint iTexId, bool iGenMipMap, int iFace)
  {
    if(iTexId == 0)
      return;

    GLenum textureTarget = iFace >= 0 ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    GLenum textureFaceUpdate = iFace >= 0 ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + iFace : GL_TEXTURE_2D;

    glBindTexture(textureTarget,iTexId);

    size_t totSize = iImage.GetByteSize();

    OGLInternalTextureFormat internalFormat;
    OGLTextureElementType dataType;
    OGLTextureFormat dataComponents;

    GetOGLTypes(iImage, internalFormat, dataType, dataComponents);

    Image* replImage = nullptr;
  
#if defined(__ANDROID__) || 1
    if(dataComponents == OGLTextureFormat::BGR)
    {
      dataComponents = OGLTextureFormat::RGB;
      switch(iImage.GetFormat())
      {
      case Image::Char:   
        replImage = eXl_NEW Image(iImage);
        SwapRB<uint8_t, 3>(replImage->GetSize(), replImage->GetImageData(), replImage->GetRowStride());
        break;
      case Image::Short:  
        replImage = eXl_NEW Image(iImage);
        SwapRB<unsigned short, 3>(replImage->GetSize(), replImage->GetImageData(), replImage->GetRowStride());
        break;
      case Image::Int:    
        replImage = eXl_NEW Image(iImage);
         SwapRB<uint32_t, 3>(replImage->GetSize(), replImage->GetImageData(), replImage->GetRowStride());
        break;
      }
    }

    if(dataComponents == OGLTextureFormat::BGRA)
    {
      dataComponents = OGLTextureFormat::RGBA;
      switch(iImage.GetFormat())
      {
      case Image::Char:   
        replImage = eXl_NEW Image(iImage);
        SwapRB<uint8_t, 4>(replImage->GetSize(), replImage->GetImageData(), replImage->GetRowStride());
        break;
      case Image::Short:  
        replImage = eXl_NEW Image(iImage);
        SwapRB<unsigned short, 4>(replImage->GetSize(), replImage->GetImageData(), replImage->GetRowStride());
        break;
      case Image::Int:    
        replImage = eXl_NEW Image(iImage);
        SwapRB<uint32_t, 4>(replImage->GetSize(), replImage->GetImageData(), replImage->GetRowStride());
        break;
      }
    }
#endif 

    uint8_t* texCopy = nullptr;

    Image::Size const& imgSize = iImage.GetSize();

    uint8_t const* pixelsData = reinterpret_cast<uint8_t const*>(replImage != nullptr ? replImage->GetImageData() : iImage.GetImageData());

    if(iImage.GetRowStride() % 8 == 0)
      glPixelStorei(GL_UNPACK_ALIGNMENT,8);
    else if(iImage.GetRowStride() % 4 == 0)
      glPixelStorei(GL_UNPACK_ALIGNMENT,4);
    else if(iImage.GetRowStride() % 2 == 0)
      glPixelStorei(GL_UNPACK_ALIGNMENT,2);
    else
      glPixelStorei(GL_UNPACK_ALIGNMENT,1);

    glTexImage2D(textureFaceUpdate, 0, 
      GetGLInternalTextureFormat(internalFormat), 
      imgSize.X(), imgSize.Y(), 0, 
      GetGLTextureFormat(dataComponents), 
      GetGLTextureElementType(dataType), 
      nullptr);
    for(uint32_t i = 0; i < imgSize.Y(); ++i)
    {
      //uint8_t const* pixelsDataRev = pixelsData + ((imgSize.Y() - 1) - i) * iImage->GetRowStride();
      uint8_t const* pixelRow = pixelsData + i * iImage.GetRowStride();
      glTexSubImage2D(textureFaceUpdate, 0, 0, i, imgSize.X(), 1, GetGLTextureFormat(dataComponents), GetGLTextureElementType(dataType), pixelRow);
    }

    //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);

#ifndef __ANDROID__
    if(!iGenMipMap)
    {
      glTexParameteri(textureTarget,GL_TEXTURE_BASE_LEVEL,0);
      glTexParameteri(textureTarget,GL_TEXTURE_MAX_LEVEL,0);
    }
    else
#endif
    {
      glGenerateMipmap(textureTarget);
    }

    
    //newTex->m_Loader = this;

    if(replImage != NULL)
      eXl_DELETE replImage;
  }

  Err OGLTextureLoader::ReadTexture(OGLTexture* iTexture, Image*& oImage, int iFace)
  {
#ifdef __ANDROID__
    return Err::Failure;
#endif

    if(iTexture)
    {
      GLenum textureTarget;
      GLenum textureFaceTarget;

      if(iFace == -1 && !iTexture->IsCubeMap())
      {
        textureTarget = GL_TEXTURE_2D;
        textureFaceTarget = GL_TEXTURE_2D;
      }
      else if(iFace < 6 && iTexture->IsCubeMap())
      {
        textureTarget = GL_TEXTURE_CUBE_MAP;
        textureFaceTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + iFace;
      }
      else
        return Err::Failure;

      OGLInternalTextureFormat imgInternalFormat;
      OGLTextureElementType imgDataType;
      OGLTextureFormat imgDataComponents;
      Image::Components comps;
      Image::Format fmt;

      if(oImage)
      {
        GetOGLTypes(*oImage, imgInternalFormat, imgDataType, imgDataComponents);

        if(imgInternalFormat != iTexture->m_InternalFormat 
        || imgDataType != iTexture->GetElementType()
        || oImage->GetSize() != iTexture->GetSize()
        || (oImage->GetRowStride() % 8 != 0 
         && oImage->GetRowStride() % 4 != 0 
         && oImage->GetRowStride() % 2 != 0 
         && oImage->GetRowStride() != oImage->GetSize().X()))
          oImage = NULL;
      }

      if(!oImage)
      {
        imgInternalFormat = iTexture->m_InternalFormat;
        imgDataType = iTexture->GetElementType();
        imgDataComponents = iTexture->GetElementFormat();
      
#ifdef __ANDROID__
        switch(imgInternalFormat)
        {
        case GL_LUMINANCE:
          comps = Image::R;
          break;
        case GL_LUMINANCE_ALPHA:
          comps = Image::RG;
          break;
        case GL_RGB:
          comps = Image::RGB;
          break;
        case GL_RGBA:
          comps = Image::RGBA;
          break;
        }
#else
        switch(imgInternalFormat)
        {
        case OGLInternalTextureFormat::RED:
          comps = Image::R;
          break;
        case OGLInternalTextureFormat::RG:
          comps = Image::RG;
          break;
        case OGLInternalTextureFormat::RGB:
          comps = Image::RGB;
          break;
        case OGLInternalTextureFormat::RGBA:
          comps = Image::RGBA;
          break;
        }
#endif

        switch(imgDataType)
        {
        case OGLTextureElementType::UNSIGNED_BYTE:
          fmt = Image::Char;
          break;
        case OGLTextureElementType::UNSIGNED_SHORT:
          fmt = Image::Short;
          break;
        case OGLTextureElementType::UNSIGNED_INT:
          fmt = Image::Int;
          break;
        case OGLTextureElementType::FLOAT:
          fmt = Image::Float;
          break;
        }

        oImage = eXl_NEW Image(NULL, iTexture->GetSize(), comps, fmt, 4);
      }
      uint32_t align = oImage->GetRowStride() - oImage->GetSize().X();

      if(align > 4)
        glPixelStorei(GL_PACK_ALIGNMENT,8);
      else if(align > 2)
        glPixelStorei(GL_PACK_ALIGNMENT,4);
      else if(align > 1)
        glPixelStorei(GL_PACK_ALIGNMENT,2);
      else
        glPixelStorei(GL_PACK_ALIGNMENT,1);

      glBindTexture(textureTarget, iTexture->GetId());
#ifndef __ANDROID__
      glGetTexImage(textureFaceTarget, 0, GetGLTextureFormat(imgDataComponents), GetGLTextureElementType(imgDataType), oImage->GetPixel(0,0));
#endif

      //glReadPixels(iBox.m_Data[0].X(), iBox.m_Data[0].Y(), iBox.GetSize().X(), iBox.GetSize().Y(), imgInternalFormat, imgDataType, oImage->GetPixel(0,0));
      glBindTexture(textureTarget, 0);
      return Err::Success;
    }
    return Err::Failure;
  }
}
