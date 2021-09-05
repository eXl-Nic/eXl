#include <engine/map/mcmcmodelrsc.hpp>
#include <core/resource/resourceloader.hpp>
#include <core/resource/resourcemanager.hpp>
#include <gen/mcmcsynthesis.hpp>
#include <gen/mcmcdiskmodel.hpp>
#include <gen/mcmcsvmmodel.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(MCMCModelRsc);
  IMPLEMENT_SERIALIZE_METHODS(MCMCModelRsc::ElementRef);

  using MCMCModelLoader = TResourceLoader<MCMCModelRsc, ResourceLoader>;

  MCMCModelRsc::~MCMCModelRsc() = default;

  void MCMCModelRsc::Init()
  {
    ResourceManager::AddLoader(&MCMCModelLoader::Get(), MCMCModelRsc::StaticRtti());
  }

  ResourceLoaderName MCMCModelRsc::StaticLoaderName()
  {
    return ResourceLoaderName("MCMCModel");
  }

#ifdef EXL_RSC_HAS_FILESYSTEM
  MCMCModelRsc* MCMCModelRsc::Create(Path const& iDir, String const& iName)
  {
    MCMCModelRsc* newModel = MCMCModelLoader::Get().Create(iDir, iName);

    return newModel;
  }
#endif

  MCMCModelRsc::MCMCModelRsc(ResourceMetaData& iMetaData)
    : Resource(iMetaData)
  {

  }

  uint32_t MCMCModelRsc::ComputeHash()
  {
    return 0;
  }

  Err MCMCModelRsc::ElementRef::Serialize(Serializer iSerializer)
  {
    iSerializer.BeginStruct();
    iSerializer.PushKey("Resource");
    iSerializer &= m_Resource;
    iSerializer.PopKey();
    iSerializer.PushKey("Subobject");
    iSerializer &= m_Subobject;
    iSerializer.PopKey();
    iSerializer.EndStruct();

    return Err::Success;
  }

  Err MCMCModelRsc::Serialize_Prologue(Serializer iStreamer)
  {
    iStreamer.PushKey("Elements");
    iStreamer &= m_ElementsVector;
    iStreamer.PopKey();

    iStreamer.PushKey("Scaling");
    iStreamer &= m_DimScaling;
    iStreamer.PopKey();

    return Err::Success;
  }

  Err MCMCModelRsc::Stream_Data(Streamer & iStreamer) const
  {
      iStreamer.BeginStruct();
      const_cast<MCMCModelRsc*>(this)->Serialize_Prologue(Serializer(iStreamer));
      if (m_Model)
      {
        iStreamer.PushKey("Model");
        iStreamer.BeginStruct();
        iStreamer.PushKey("Name");
        iStreamer.Write(&m_Model->GetModelName());
        iStreamer.PopKey();
        iStreamer.PushKey("Data");
        m_Model->Stream(iStreamer);
        iStreamer.PopKey();
        iStreamer.EndStruct();
        iStreamer.PopKey();
      }
      iStreamer.EndStruct();

      return Err::Success;
  }

  Err MCMCModelRsc::Unstream_Data(Unstreamer & iStreamer)
  {
    iStreamer.BeginStruct();
    Serialize_Prologue(Serializer(iStreamer));
    if (iStreamer.PushKey("Model"))
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("Name");
      String modelName;
      iStreamer.Read(&modelName);
      iStreamer.PopKey();
      iStreamer.PushKey("Data");
      if(modelName == MCMC2D::SVMLearner::ModelName())
      {
        m_Model.reset(MCMC2D::SVMLearner::StaticUnstreamModel(iStreamer));
      }
      if (modelName == MCMC2D::DiskLearner::ModelName())
      {
        m_Model.reset(MCMC2D::DiskLearner::StaticUnstreamModel(iStreamer));
      }
      iStreamer.PopKey();
      iStreamer.EndStruct();
      iStreamer.PopKey();
    }
    
    iStreamer.EndStruct();

    return Err::Success;
  }
}