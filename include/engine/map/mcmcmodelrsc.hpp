#pragma once

#include <engine/enginelib.hpp>
#include <core/path.hpp>
#include <core/resource/resource.hpp>
#include <core/stream/serializer.hpp>

namespace eXl
{
  namespace MCMC2D
  {
    class LearnedModel;
  }

  class EXL_ENGINE_API MCMCModelRsc : public Resource
  {
    DECLARE_RTTI(MCMCModelRsc, Resource)
  public:

    static void Init();

    ~MCMCModelRsc();

#ifdef EXL_RSC_HAS_FILESYSTEM
    static MCMCModelRsc* Create(Path const& iDir, String const& iName);
#endif

    static ResourceLoaderName StaticLoaderName();

    Err Stream_Data(Streamer& iStreamer) const override;
    Err Unstream_Data(Unstreamer& iStreamer) override;

    uint32_t ComputeHash() override;
    
    struct ElementRef
    {
      Resource::UUID m_Resource;
      Name m_Subobject;

      SERIALIZE_METHODS;
    };

    float m_DimScaling = 1.0;
    Vector<ElementRef> m_ElementsVector;
    std::unique_ptr<MCMC2D::LearnedModel> m_Model;
    
  private:

    Err Serialize_Prologue(Serializer iStreamer);

    friend TResourceLoader<MCMCModelRsc, ResourceLoader>;
    MCMCModelRsc(ResourceMetaData&);
  };
}