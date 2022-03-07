/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <ogl/renderer/oglshaderdata.hpp>

namespace eXl
{
  MAKE_NAME(TextureKey);

  class OGLTextureCache;

  struct EXL_OGL_API OGLTextureCacheEntry : public HeapObject
  {
    DECLARE_RefC;
  public:
    OGLTextureCacheEntry(OGLTextureCache& iCache);
    ~OGLTextureCacheEntry();

    void Touch();

    OGLTexture const* GetTexture() const { return m_Texture.get(); }

    OGLShaderData const* GetShaderData(TextureName iName);

  protected:

    void OnNullRefC() const;

    friend OGLTextureCache;

    IntrusivePtr<OGLTexture> m_Texture;
    OGLTextureCache& m_Cache;
    uint32_t m_Timestamp;
    UnorderedMap<TextureName, OGLShaderData> m_BoundTextures;
  };

  class EXL_OGL_API OGLTextureCache
  {
  public:

    using Entry = OGLTextureCacheEntry;
    using LoadingCallback = std::function<IntrusivePtr<OGLTexture>(TextureKey)>;

    OGLTextureCache();
    OGLTextureCache(OGLTextureCache const&) = delete;
    OGLTextureCache operator=(OGLTextureCache const&) = delete;

    IntrusivePtr<Entry> GetTexture(TextureKey iKey, LoadingCallback const& iCb);
    void Tick();

  protected:
    friend OGLTextureCacheEntry;
    UnorderedMap<TextureKey, std::unique_ptr<OGLTextureCacheEntry>> m_Entries;
    uint32_t m_Timestamp = 0;
  };
}
