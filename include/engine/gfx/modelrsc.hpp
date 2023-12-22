#pragma once

#include <engine/enginelib.hpp>
#include <core/resource/resource.hpp>
#include <core/path.hpp>
#include <core/stream/serializer.hpp>

namespace eXl
{
  class ResourceLoader;
  template <typename T, typename BaseLoader>
  class TResourceLoader;

  class EXL_ENGINE_API ModelResource : public Resource
  {
    DECLARE_RTTI(ModelResource, Resource)
  public:

    static void Init();

#ifndef EXL_IS_BAKED_PLATFORM
    static ModelResource* Create(Path const& iDir, String const& iName);
#endif

    static ResourceLoaderName StaticLoaderName();

    Err Stream_Data(Streamer& iStreamer) const override;
    Err Unstream_Data(Unstreamer& iStreamer) override;

    uint32_t ComputeHash() override;

    String const& GetModelName() const { return m_ModelName; }
    void SetModelName(String const& iStr);

  private:
    Err Serialize(Serializer iStreamer);

    friend TResourceLoader<ModelResource, ResourceLoader>;
    ModelResource(ResourceMetaData&);

    String m_ModelName;
  };
}