#include <ogl/ogltexturecache.hpp>

namespace eXl
{
  IMPLEMENT_RefCCustom(OGLTextureCacheEntry);

  OGLTextureCacheEntry::OGLTextureCacheEntry(OGLTextureCache& iCache)
    : m_Cache(iCache)
  {

  }

  OGLTextureCacheEntry::~OGLTextureCacheEntry()
  {

  }

  void OGLTextureCacheEntry::Touch()
  {
    m_Timestamp = m_Cache.m_Timestamp;
  }

  void OGLTextureCacheEntry::OnNullRefC() const
  {
    
  }

  OGLShaderData const* OGLTextureCacheEntry::GetShaderData(TextureName iName)
  {
    Touch();
    auto insertRes = m_BoundTextures.insert(std::make_pair(iName, OGLShaderData()));
    auto iter = insertRes.first;
    if (!insertRes.second)
    {
      return &iter->second;
    }

    iter->second.AddTexture(iName, m_Texture);

    return &iter->second;
  }

  OGLTextureCache::OGLTextureCache()
  {

  }

  void OGLTextureCache::Tick()
  {
    ++m_Timestamp;
  }

  IntrusivePtr<OGLTextureCacheEntry> OGLTextureCache::GetTexture(TextureKey iKey, LoadingCallback const& iCb)
  {
    auto iter = m_Entries.find(iKey);
    if (iter != m_Entries.end())
    {
      return IntrusivePtr<Entry>(iter->second.get());
    }

    IntrusivePtr<OGLTexture> loadedTexture = iCb(iKey);
    if (!loadedTexture)
    {
      return nullptr;
    }

    Entry* newEntry = eXl_NEW Entry(*this);
    newEntry->m_Texture = std::move(loadedTexture);

    m_Entries.insert(std::make_pair(iKey, newEntry));

    return IntrusivePtr<Entry>(newEntry);
  }
}
