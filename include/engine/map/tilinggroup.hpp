#pragma once

#include <engine/enginelib.hpp>
#include <engine/gfx/tileset.hpp>

namespace eXl
{
  enum class TilingGroupLocConstraint
  {
    Undefined,
    SameGroup,
    OtherGroup
  };

  EXL_REFLECT_ENUM(TilingGroupLocConstraint, eXl__TilingGroupLocConstraint, EXL_ENGINE_API);

  struct EXL_ENGINE_API TilingDrawElement
  {
    EXL_REFLECT;

    TileName m_Name;
    Vector2i m_Position;
  };

  struct EXL_ENGINE_API TilingPattern
  {
    EXL_REFLECT;

    TilingPattern()
    {
      pattern.push_back(TilingGroupLocConstraint::Undefined);
    }

    Vector2i patternSize = Vector2i::ONE;
    Vector2i anchor;
    Vector<TilingGroupLocConstraint> pattern;
    Vector<TilingDrawElement> drawElement;
  };

  class EXL_ENGINE_API TilingGroup : public Resource
  {
    DECLARE_RTTI(TilingGroup, Resource)
  public:

    //struct PatternNameTag {};
    //using PatternName = Name;

    MAKE_NAME_DECL(PatternName);

    static void Init();

#ifdef EXL_RSC_HAS_FILESYSTEM
    static TilingGroup* Create(Path const& iDir, String const& iName);
#endif

    static ResourceLoaderName StaticLoaderName();

    Err Stream_Data(Streamer& iStreamer) const override;
    Err Unstream_Data(Unstreamer& iStreamer) override;

    uint32_t ComputeHash() override;

    TileName m_DefaultTile; // <== gives default size
    UnorderedMap<PatternName, TilingPattern> m_Patterns;

    void SetTileset(Tileset const& iTileset);
    void SetTilesetId(Tileset::UUID const& iId);
    ResourceHandle<Tileset> const& GetTileset() const { return m_Tileset; }

  private:

    Err Serialize(Serializer iStreamer);

    friend TResourceLoader<TilingGroup, ResourceLoader>;
    TilingGroup(ResourceMetaData&);

    ResourceHandle<Tileset> m_Tileset;
  };

  MAKE_NAME_TYPE(TilingGroup::PatternName);
}