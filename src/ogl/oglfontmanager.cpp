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

#include EXL_TEXT("oglfontmanager.hpp")
#include EXL_TEXT("renderer/ogltextureloader.hpp")
#include EXL_TEXT("renderer/ogltexture.hpp")

namespace eXl
{

  OGLFontManager::OGLFontTexture::~OGLFontTexture()
  {
    eXl_DELETE m_TextureObject;
  }

  void OGLFontManager::OGLFontTexture::Update(AABB2Di const& iBox, void const* iData)
  {
    m_TextureObject->Update(iBox,iData);
  }

  void OGLFontManager::ShutdownAPITexture(FontManager::FontTexture* iTexture)
  {
    OGLFontTexture* oglTex = reinterpret_cast<OGLFontTexture*>(iTexture);
    eXl_DELETE oglTex->m_TextureObject;
    oglTex->m_TextureObject = NULL;
  }

  void OGLFontManager::RestoreAPITexture(FontManager::FontTexture* iTexture)
  {
    OGLFontTexture* oglTex = reinterpret_cast<OGLFontTexture*>(iTexture);
    oglTex->m_TextureObject = m_Loader->Create(oglTex->m_SizeInPixels,OGLTextureLoader::R8);
  }

  OGLFontManager::OGLFontManager(StorageInfo const* iSource, String const& iDefaultFont, OGLTextureLoader* iLoader):FontManager(iSource,iDefaultFont),m_Loader(iLoader)
  {

  }

  OGLFontManager::OGLFontTexture* OGLFontManager::CreateTexture(String const& iName)
  {
    OGLFontTexture* tex = eXl_NEW OGLFontTexture;
    tex->m_FontName = iName;
    tex->m_TextureObject = m_Loader->Create(Vector2i(1024,1024),OGLTextureLoader::R8);
    tex->m_SizeInPixels = Vector2i(1024,1024);
    return tex;
  }

  OGLTexture* OGLFontManager::WriteText(String const& iFontName,String const& iText, Vector2f const& iScale, std::vector<AABB2Df>& oPos, std::vector<AABB2Df>& oTexCoord, FontManager::Anchor iAnchor)
  {
    FontTexture* tex = FontManager::WriteText(iFontName,iText,iScale,oPos,oTexCoord,iAnchor);
    if(tex != NULL)
      return ((OGLFontTexture*)tex)->m_TextureObject;
    return NULL;
  }
}