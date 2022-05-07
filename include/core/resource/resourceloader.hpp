/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/resource/resource.hpp>
#include <core/stream/reader.hpp>
#include <core/stream/writer.hpp>

#include <core/path.hpp>

namespace eXl
{
  class ResourceLoader;

  namespace ResourceManager
  {
    EXL_CORE_API void AddLoader(ResourceLoader* iLoader, Rtti const& iRtti);
    EXL_CORE_API String const& GetAssetExtension();
#ifdef EXL_RSC_HAS_FILESYSTEM
    EXL_CORE_API Err SetPath(Resource* iRsc, Path const& iPath);
#endif
  }

  class EXL_CORE_API ResourceLoader
  {
  public:

    virtual bool CanCreateDefaultResource() const { return false; }

    Resource* Create(String const& iName) const 
    { 
      eXl_ASSERT_REPAIR_RET(CanCreateDefaultResource(), nullptr);
      return Create_Impl(iName); 
    }

    virtual Err Save(Resource* iRsc, Writer& iStreamer) const;
    virtual Resource* Load(Resource::Header const& iHeader, ResourceMetaData* iMetaData, Reader& iStreamer) const;

    ResourceLoaderName GetName() const { return m_Name; }
    int32_t GetVersion() const { return m_Version; }

    virtual bool NeedsBaking(Resource* iRsc) const { return false; }
    virtual Resource* CreateBakedResource(Resource* iRsc) const { return nullptr; }
  protected:

    virtual Resource* Create_Impl(String const& iName) const { return nullptr; }
    virtual Resource* Create_Impl(ResourceMetaData*) const = 0;

    ResourceLoader(ResourceLoaderName iName, uint32_t iVersion)
      : m_Name(iName)
      , m_Version(iVersion)
    {}

    ResourceMetaData* CreateNewMetaData(String const& iName) const;
    ResourceMetaData* CreateBakedMetaData(ResourceMetaData const& iMetaData) const;
    ResourceMetaData* CreateSystemMetaData(String const& iName, Resource::UUID const& iUUID) const;
  private:
    ResourceLoaderName const m_Name;
    uint32_t const m_Version;
  };

  template <typename T, typename BaseLoader>
  class TResourceLoader : public BaseLoader
  {
  public:

    static TResourceLoader& Get()
    {
      static TResourceLoader s_This;
      return s_This;
    }

    TResourceLoader()
      : BaseLoader(T::StaticLoaderName(), 1)
    {

    }

#ifdef EXL_RSC_HAS_FILESYSTEM
    T* Create(Path const& iDir, String const& iName) const
    {
      ResourceMetaData* metaData = BaseLoader::CreateNewMetaData(iName);

      Path rscPath = iDir / Path(iName.c_str());
      rscPath.replace_extension(ResourceManager::GetAssetExtension().c_str());

      T* newRsc = Create_Impl(metaData);
      if (ResourceManager::SetPath(newRsc, rscPath))
      {
        return newRsc;
      }

      return nullptr;
    }
#endif

    //Resource* Load(Resource::Header const& iHeader, ResourceMetaData* iMetaData, Unstreamer& iStreamer) const override
    //{
    // 
    //}
  protected:
    T* Create_Impl(ResourceMetaData* iMetaData) const override
    {
      return eXl_NEW T(*iMetaData);
    }
  };
}