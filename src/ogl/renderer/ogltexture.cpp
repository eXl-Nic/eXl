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
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with eXl.  If not, see <http://www.gnu.org/licenses/>.
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