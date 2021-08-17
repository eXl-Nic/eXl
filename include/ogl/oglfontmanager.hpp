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

#include <gametk/gfx/fontmanager.hpp>

namespace eXl
{
  class OGLTexture;
  class OGLTextureLoader;

  class OGLFontManager : public FontManager
  {
  public:
    OGLFontManager(StorageInfo const* iSource, String const& iDefaultFont,OGLTextureLoader* iLoader);

    OGLTexture* WriteText(String const& iFontName,String const& iText, Vector2f const& iScale, std::vector<AABB2Df>& oPos, std::vector<AABB2Df>& oTexCoord, FontManager::Anchor iAnchor);

  protected:

    struct OGLFontTexture : public FontManager::FontTexture
    {
      ~OGLFontTexture();

      void Update(AABB2Di const& iBox, void const* iData);

      OGLTexture* m_TextureObject;
    };

    void ShutdownAPITexture(FontManager::FontTexture* iTexture);

    void RestoreAPITexture(FontManager::FontTexture* iTexture);

    OGLFontTexture* CreateTexture(String const& iName);

    OGLTextureLoader* m_Loader;
  };
}