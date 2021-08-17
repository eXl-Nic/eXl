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
      ResourceMetaData* metaData = CreateNewMetaData(iName);

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