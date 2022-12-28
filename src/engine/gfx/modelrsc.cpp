#include <engine/gfx/modelrsc.hpp>
#include <core/resource/resourceloader.hpp>
#include <core/resource/resourcemanager.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(ModelResource);

  using ModelLoader = TResourceLoader<ModelResource>;

  void ModelResource::Init()
  {
    ResourceManager::AddLoader(&ModelLoader::Get(), ModelResource::StaticRtti(), GetType());
  }

  ResourceLoaderName ModelResource::StaticLoaderName()
  {
    return ResourceLoaderName("ModelResource");
  }

#ifdef EXL_RSC_HAS_FILESYSTEM
  ModelResource* ModelResource::Create(Path const& iPath, String const& iName)
  {
    return ModelLoader::Get().CreateAt(iPath, iName);
  }
#endif

  Err ModelResource::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer &= m_ModelName;
    iStreamer.EndStruct();

    return Err::Success;
  }

  Err ModelResource::Stream_Data(Streamer& iStreamer) const
  {
    return const_cast<ModelResource*>(this)->Serialize(iStreamer);
  }

  Err ModelResource::Unstream_Data(Unstreamer& iStreamer)
  {
    return Serialize(iStreamer);
  }

  ModelResource::ModelResource(ResourceMetaData& iMeta)
    : Resource(iMeta)
  {

  }

  uint32_t ModelResource::ComputeHash() 
  {
    return 0;
  }

  void ModelResource::SetModelName(String const& iStr)
  {
    m_ModelName = iStr;
  }
}