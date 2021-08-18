/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
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