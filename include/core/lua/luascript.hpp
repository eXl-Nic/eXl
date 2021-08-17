#pragma once

#include <core/resource/resource.hpp>
#include <core/resource/resourceloader.hpp>

namespace eXl
{
  class Serializer;
  class EXL_CORE_API LuaScript : public Resource
  {
    DECLARE_RTTI(LuaScript, Resource);
  public:
    AString m_Script;

    AString const& GetModuleName() const
    {
      return m_ModuleName;
    }

    Err Stream_Data(Streamer& iStreamer) const override;
    Err Unstream_Data(Unstreamer& iStreamer) override;

  protected:
    uint32_t ComputeHash() override;
    Err Serialize(Serializer& iSerialize);
    LuaScript(ResourceMetaData&);

    AString m_ModuleName;

  };

  class EXL_CORE_API LuaScriptLoader : public ResourceLoader
  {
  public:

    static char const* const s_HeaderSection;
    static char const* const s_DataSection;

    Err Save(Resource* iRsc, Writer& iWriter) const override;
    Resource* Load(Resource::Header const& iHeader, ResourceMetaData* iMetaData, Reader& iStreamer) const override;
  protected:
    LuaScriptLoader(ResourceLoaderName iName, uint32_t iVersion)
      : ResourceLoader(iName, iVersion)
    {}
  };

  template <typename T>
  using LuaScriptLoader_T = TResourceLoader<T, LuaScriptLoader>;

#if 0

  template <typename T>
  class LuaScriptLoader_T : public LuaScriptLoader
  {
  public:

    static LuaScriptLoader_T<T>& Get()
    {
      static LuaScriptLoader_T<T> s_This;
      return s_This;
    }

    LuaScriptLoader_T()
      : LuaScriptLoader(T::StaticLoaderName(), 1)
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

  protected:

    T* Create_Impl(ResourceMetaData* iMetaData) const override
    {
      return eXl_NEW T(*iMetaData);
    }
  };
#endif
}