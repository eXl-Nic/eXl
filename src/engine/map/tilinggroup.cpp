
#include <engine/map/tilinggroup.hpp>
#include <core/resource/resourceloader.hpp>
#include <core/type/type.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(TilingGroup);

  using TilingGroupLoader = TResourceLoader<TilingGroup, ResourceLoader>;

  void TilingGroup::Init()
  {
    ResourceManager::AddLoader(&TilingGroupLoader::Get(), TilingGroup::StaticRtti());
  }

  ResourceLoaderName TilingGroup::StaticLoaderName()
  {
    return ResourceLoaderName("TilingGroup");
  }

#ifdef EXL_RSC_HAS_FILESYSTEM
  TilingGroup* TilingGroup::Create(Path const& iDir, String const& iName)
  {
    TilingGroup* newGroup = TilingGroupLoader::Get().Create(iDir, iName);

    return newGroup;
  }
#endif

  TilingGroup::TilingGroup(ResourceMetaData& iMetaData)
    : Resource(iMetaData)
  {

  }

  void TilingGroup::SetTileset(Tileset const& iTileset)
  {
    m_Tileset.Set(&iTileset);
  }

  void TilingGroup::SetTilesetId(Tileset::UUID const& iId)
  {
    m_Tileset.SetUUID(iId);
  }

  Err TilingGroup::Stream_Data(Streamer& iStreamer) const
  {
    return const_cast<TilingGroup*>(this)->Serialize(Serializer(iStreamer));
  }

  Err TilingGroup::Unstream_Data(Unstreamer& iStreamer)
  {
    return Serialize(Serializer(iStreamer));
  }

  Err TilingGroup::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();

    auto patternType = TilingPattern::GetType();

    iStreamer.PushKey("Tileset");
    iStreamer &= m_Tileset;
    iStreamer.PopKey();
    iStreamer.PushKey("DefaultTile");
    iStreamer &= m_DefaultTile;
    iStreamer.PopKey();
    iStreamer.PushKey("Patterns");
    iStreamer.HandleSequence(m_Patterns, 
      [](UnorderedMap<PatternName, TilingPattern>& oMap, Unstreamer& iStreamer)
    {
      TilingPattern pattern;
      String tempStr;
      iStreamer.BeginStruct();
      iStreamer.PushKey("Name");
      iStreamer.ReadString(&tempStr);
      iStreamer.PopKey();
      iStreamer.PushKey("Data");
      iStreamer.Read(&pattern);
      iStreamer.PopKey();
      iStreamer.EndStruct();

      oMap.insert(std::make_pair(TileName(tempStr), std::move(pattern)));
    },
      [](UnorderedMap<PatternName, TilingPattern>::value_type const& iEntry, Streamer& iStreamer)
    {
      iStreamer.BeginStruct();
      iStreamer.PushKey("Name");
      iStreamer.Write(&iEntry.first);
      iStreamer.PopKey();
      iStreamer.PushKey("Data");
      iStreamer.Write(&iEntry.second);
      iStreamer.PopKey();
      iStreamer.EndStruct();
    });
    iStreamer.PopKey();
    iStreamer.EndStruct();

    return Err::Success;
  }

  uint32_t TilingGroup::ComputeHash()
  {
    return 0;
  }

}