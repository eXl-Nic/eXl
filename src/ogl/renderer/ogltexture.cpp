/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ogl/renderer/oglinclude.hpp>
#include <ogl/renderer/ogltexture.hpp>
#include <ogl/renderer/ogltextureloader.hpp>

namespace eXl
{
  IMPLEMENT_RefC(OGLTexture);

  OGLTexture::OGLTexture(Image::Size const& iSize, unsigned int iType)
    : m_Size(iSize)
    , m_TextureType(iType)
    , m_TexId(0)
    , m_Loader(NULL)
  {
      
    m_CubeMap = false;
  }

  //void OGLTexture::OnNullRefC() const
  //{
  //  //Pour le moment...
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

  void OGLTexture::Update(AABB2Di iBox, void const* iData, unsigned int iMip, unsigned int iFace)
  {
    if(m_TexId != 0 && iData != NULL && iFace < 6)
    {
      GLenum textureTarget = IsCubeMap()? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
      GLenum textureFaceUpdate = IsCubeMap() ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + iFace : GL_TEXTURE_2D;

      Vector2i boxSize = iBox.GetSize();
      if(boxSize.X() > 0 && boxSize.Y() > 0)
      {
        glBindTexture(textureTarget,m_TexId);
        glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        if(iBox.MinX() == 0 && iBox.MinY() == 0)
          glTexImage2D(textureFaceUpdate,iMip, m_TextureFormat,boxSize.X(),boxSize.Y(), 0, m_TextureFormat,m_TextureType,iData);
        else
          glTexSubImage2D(textureFaceUpdate,iMip,iBox.MinX(),iBox.MinY(),boxSize.X(),boxSize.Y(),m_TextureFormat,m_TextureType,iData);
      }
    }
  }
}