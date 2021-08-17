/**
  Copyright Nicolas Colombe
  2009-2014

  This file is part of eXl.

  eXl is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  eXl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with eXl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <ogl/renderer/ogltextureloader.hpp>
#include <ogl/renderer/oglinclude.hpp>
#include <core/image/image.hpp>
#include <core/log.hpp>
#include <ogl/oglutils.hpp>

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
      GLuint internalFormat;
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
        internalFormat = GL_RED;
        dataComponents = GL_RED;
        break;
      case RG8:
        internalFormat = GL_RG;
        dataComponents = GL_RG;
        break;
      case RGB8:
        internalFormat = GL_RGB;
        dataComponents = GL_RGB;
        break;
      case RGBA8:
        internalFormat = GL_RGBA;
        dataComponents = GL_RGBA;
        break;
      case R32F:
        internalFormat = GL_R32F;
        dataComponents = GL_RED;
        dataType = GL_FLOAT;
        break;
      case RG32F:
        internalFormat = GL_RG32F;
        dataComponents = GL_RG;
        dataType = GL_FLOAT;
        break;
      case RGB32F:
        internalFormat = GL_RGB32F;
        dataComponents = GL_RGB;
        dataType = GL_FLOAT;
        break;
      case RGBA32F:
        internalFormat = GL_RGBA32F;
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
      glTexImage2D(GL_TEXTURE_2D,0,internalFormat,iSize.X(),iSize.Y(),0,dataComponents,dataType,iData);

      res = eXl_NEW OGLTexture(iSize,dataType);
      //res->m_TextureData = NULL;
      res->m_TextureFormat = dataComponents;
      res->m_TextureType = dataType;
      res->m_TexId = texId;
    }
    return res;
  }

  template <class T, unsigned int iNumCol>
  void SwapRB(Vector2<uint32_t> const& iSize, void* iData, unsigned int iRowStride)
  {
    unsigned char* dataPtr = reinterpret_cast<unsigned char*>(iData);
    for(unsigned int i = 0; i < iSize.Y(); ++i)
    {
      T* curRow = (T*)dataPtr;
      for(unsigned int j = 0; j<iSize.X(); ++j)
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

  void GetOGLTypes(Image const& iImage, GLuint& oInternalFormat, GLuint& oDataType, GLuint& oDataComponents)
  {
    switch(iImage.GetFormat())
    {
      case Image::Char:   oDataType = GL_UNSIGNED_BYTE;  break;
      case Image::Short:  oDataType = GL_UNSIGNED_SHORT; break;
      case Image::Int:    oDataType = GL_UNSIGNED_INT;   break;
      case Image::Float:  oDataType = GL_FLOAT;          break;
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
      oInternalFormat = GL_RGB;
      oDataComponents = GL_RGB;
      break;
    case Image::BGR:
      oInternalFormat = GL_RGB;
      oDataComponents = GL_BGR;
      break;
    case Image::RGBA:
      oInternalFormat = GL_RGBA;
      oDataComponents = GL_RGBA;
      break;
    case Image::BGRA:
      oInternalFormat = GL_RGBA;
      oDataComponents = GL_BGRA;
      break;
    }

  }

  OGLTexture* OGLTextureLoader::CreateFromImage(Image const* iImage, bool iGenMipMap)
  {
    if (iImage == NULL)
      return NULL;

    GLuint texId;
    glGenTextures(1, &texId);

    GLuint internalFormat;
    GLuint dataType;
    GLuint dataComponents;

    GetOGLTypes(*iImage, internalFormat, dataType, dataComponents);
    
    UpdateFromImage(*iImage, texId, false, -1);

    Image::Size const& imgSize = iImage->GetSize();

    OGLTexture* newTex = eXl_NEW OGLTexture(imgSize,dataType);
    //mouais....
    newTex->m_TextureFormat = internalFormat;
    newTex->m_TextureType = dataType;
    newTex->m_TexId = texId;

    return newTex;
  }

  OGLTexture* OGLTextureLoader::CreateCubeMap(Image const* const* iImage, bool iGenMipMap)
  {

    if(iImage[0] == NULL)
      return NULL;

    GLuint internalFormat;
    GLuint dataType;
    GLuint dataComponents;

    GetOGLTypes(*iImage[0], internalFormat, dataType, dataComponents);

    Image::Size const& imgSize = iImage[0]->GetSize();

    for(unsigned int i = 1; i<6; ++i)
    {
      if(iImage[i] == NULL)
        return NULL;

      Image::Size const& testImgSize = iImage[i]->GetSize();

      if(testImgSize != imgSize)
        return NULL;

      GLuint testInternalFormat;
      GLuint testDataType;
      GLuint testDataComponents;

      GetOGLTypes(*iImage[i], testInternalFormat, testDataType, testDataComponents);

      if(testInternalFormat != internalFormat
      || testDataType != dataType)
      {
        return NULL;
      }
    }

    GLuint texId;
    glGenTextures(1, &texId);

    for(unsigned int i = 0; i<6; ++i)
    {
      UpdateFromImage(*iImage[i], texId, false, i);
    }

    OGLTexture* newTex = eXl_NEW OGLTexture(imgSize,dataType);
    //mouais....
    newTex->m_TextureFormat = internalFormat;
    newTex->m_TextureType = dataType;
    newTex->m_TexId = texId;
    newTex->m_CubeMap = true;

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

    GLuint internalFormat;
    GLuint dataType;
    GLuint dataComponents;

    GetOGLTypes(iImage, internalFormat, dataType, dataComponents);

    Image* replImage = nullptr;
  
#if defined(__ANDROID__) || 1
    if(dataComponents == GL_BGR)
    {
      dataComponents = GL_RGB;
      switch(iImage.GetFormat())
      {
      case Image::Char:   
        replImage = eXl_NEW Image(iImage);
        SwapRB<unsigned char, 3>(replImage->GetSize(), replImage->GetImageData(), replImage->GetRowStride());
        break;
      case Image::Short:  
        replImage = eXl_NEW Image(iImage);
        SwapRB<unsigned short, 3>(replImage->GetSize(), replImage->GetImageData(), replImage->GetRowStride());
        break;
      case Image::Int:    
        replImage = eXl_NEW Image(iImage);
         SwapRB<unsigned int, 3>(replImage->GetSize(), replImage->GetImageData(), replImage->GetRowStride());
        break;
      }
    }

    if(dataComponents == GL_BGRA)
    {
      dataComponents = GL_RGBA;
      switch(iImage.GetFormat())
      {
      case Image::Char:   
        replImage = eXl_NEW Image(iImage);
        SwapRB<unsigned char, 4>(replImage->GetSize(), replImage->GetImageData(), replImage->GetRowStride());
        break;
      case Image::Short:  
        replImage = eXl_NEW Image(iImage);
        SwapRB<unsigned short, 4>(replImage->GetSize(), replImage->GetImageData(), replImage->GetRowStride());
        break;
      case Image::Int:    
        replImage = eXl_NEW Image(iImage);
        SwapRB<unsigned int, 4>(replImage->GetSize(), replImage->GetImageData(), replImage->GetRowStride());
        break;
      }
    }
#endif 

    unsigned char* texCopy = nullptr;

    Image::Size const& imgSize = iImage.GetSize();

    unsigned char const* pixelsData = reinterpret_cast<unsigned char const*>(replImage != nullptr ? replImage->GetImageData() : iImage.GetImageData());

    if(iImage.GetRowStride() % 8 == 0)
      glPixelStorei(GL_UNPACK_ALIGNMENT,8);
    else if(iImage.GetRowStride() % 4 == 0)
      glPixelStorei(GL_UNPACK_ALIGNMENT,4);
    else if(iImage.GetRowStride() % 2 == 0)
      glPixelStorei(GL_UNPACK_ALIGNMENT,2);
    else
      glPixelStorei(GL_UNPACK_ALIGNMENT,1);

    glTexImage2D(textureFaceUpdate, 0, internalFormat, imgSize.X(), imgSize.Y(), 0, dataComponents, dataType, nullptr);
    for(unsigned int i = 0; i < imgSize.Y(); ++i)
    {
      //unsigned char const* pixelsDataRev = pixelsData + ((imgSize.Y() - 1) - i) * iImage->GetRowStride();
      unsigned char const* pixelRow = pixelsData + i * iImage.GetRowStride();
      glTexSubImage2D(textureFaceUpdate, 0, 0, i, imgSize.X(), 1, dataComponents, dataType, pixelRow);
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

      GLuint imgInternalFormat;
      GLuint imgDataType;
      GLuint imgDataComponents;
      Image::Components comps;
      Image::Format fmt;

      if(oImage)
      {
        GetOGLTypes(*oImage, imgInternalFormat, imgDataType, imgDataComponents);
        if(imgInternalFormat != iTexture->m_TextureFormat 
        || imgDataType != iTexture->m_TextureType
        || oImage->GetSize() != iTexture->GetSize()
        || (oImage->GetRowStride() % 8 != 0 
         && oImage->GetRowStride() % 4 != 0 
         && oImage->GetRowStride() % 2 != 0 
         && oImage->GetRowStride() != oImage->GetSize().X()))
          oImage = NULL;
      }

      if(!oImage)
      {
        imgInternalFormat = iTexture->m_TextureFormat;
        imgDataType = iTexture->m_TextureType;
        imgDataComponents = iTexture->m_TextureFormat;
      
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
        case GL_R:
          comps = Image::R;
          break;
        case GL_RG:
          comps = Image::RG;
          break;
        case GL_RGB:
          comps = Image::RGB;
          break;
        case GL_RGBA:
          comps = Image::RGBA;
          break;
        }
#endif

        switch(imgDataType)
        {
        case GL_UNSIGNED_BYTE:
          fmt = Image::Char;
          break;
        case GL_UNSIGNED_SHORT:
          fmt = Image::Short;
          break;
        case GL_UNSIGNED_INT:
          fmt = Image::Int;
          break;
        case GL_FLOAT:
          fmt = Image::Float;
          break;
        }

        oImage = eXl_NEW Image(NULL, iTexture->GetSize(), comps, fmt, 4);
      }
      unsigned int align = oImage->GetRowStride() - oImage->GetSize().X();

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
      glGetTexImage(textureFaceTarget, 0, imgInternalFormat, imgDataType, oImage->GetPixel(0,0));
#endif

      //glReadPixels(iBox.m_Data[0].X(), iBox.m_Data[0].Y(), iBox.GetSize().X(), iBox.GetSize().Y(), imgInternalFormat, imgDataType, oImage->GetPixel(0,0));
      glBindTexture(textureTarget, 0);
      return Err::Success;
    }
    return Err::Failure;
  }
}
