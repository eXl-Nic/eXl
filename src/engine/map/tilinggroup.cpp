/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/


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

  void TilingGroup::AddWangPatterns(UnorderedMap<PatternName, TilingPattern>& oMap)
  {
    oMap = UnorderedMap<PatternName, TilingPattern>({
      { PatternName("TR_InnerCorner"),
        TilingPattern(Vector2i(2,2), Vector2i(1,1),
        Vector<TilingGroupLocConstraint>(
          {TilingGroupLocConstraint::OtherGroup, TilingGroupLocConstraint::SameGroup
          ,TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::SameGroup}),
          Vector<TilingDrawElement>({TilingDrawElement{TileName(), Vector2i(1,1)}}))
      },
      { PatternName("TL_InnerCorner"),
        TilingPattern(Vector2i(2,2), Vector2i(1,1),
        Vector<TilingGroupLocConstraint>(
          {TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::OtherGroup
          ,TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::SameGroup}),
          Vector<TilingDrawElement>({TilingDrawElement{TileName(), Vector2i(0,1)}}))
      },
      { PatternName("BL_InnerCorner"),
        TilingPattern(Vector2i(2,2), Vector2i(1,1),
        Vector<TilingGroupLocConstraint>(
          {TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::SameGroup
          ,TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::OtherGroup}),
          Vector<TilingDrawElement>({TilingDrawElement{TileName(), Vector2i(0,0)}}))
      },
      { PatternName("BR_InnerCorner"),
        TilingPattern(Vector2i(2,2), Vector2i(1,1),
        Vector<TilingGroupLocConstraint>(
          {TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::SameGroup
          ,TilingGroupLocConstraint::OtherGroup, TilingGroupLocConstraint::SameGroup}),
          Vector<TilingDrawElement>({TilingDrawElement{TileName(), Vector2i(1,0)}}))
      },
      { PatternName("BL_Corner"),
        TilingPattern(Vector2i(2,2), Vector2i(1,1),
        Vector<TilingGroupLocConstraint>(
          {TilingGroupLocConstraint::Undefined, TilingGroupLocConstraint::OtherGroup
          ,TilingGroupLocConstraint::OtherGroup, TilingGroupLocConstraint::SameGroup}),
          Vector<TilingDrawElement>({TilingDrawElement{TileName(), Vector2i(1,1)}}))
      },
      { PatternName("TL_Corner"),
        TilingPattern(Vector2i(2,2), Vector2i(1,1),
        Vector<TilingGroupLocConstraint>(
          {TilingGroupLocConstraint::OtherGroup, TilingGroupLocConstraint::SameGroup
          ,TilingGroupLocConstraint::Undefined, TilingGroupLocConstraint::OtherGroup}),
          Vector<TilingDrawElement>({TilingDrawElement{TileName(), Vector2i(1,0)}}))
      },
      { PatternName("BR_Corner"),
        TilingPattern(Vector2i(2,2), Vector2i(1,1),
        Vector<TilingGroupLocConstraint>(
          {TilingGroupLocConstraint::OtherGroup, TilingGroupLocConstraint::Undefined
          ,TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::OtherGroup}),
          Vector<TilingDrawElement>({TilingDrawElement{TileName(), Vector2i(0,1)}}))
      },
      { PatternName("TR_Corner"),
        TilingPattern(Vector2i(2,2), Vector2i(1,1),
        Vector<TilingGroupLocConstraint>(
          {TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::OtherGroup
          ,TilingGroupLocConstraint::OtherGroup, TilingGroupLocConstraint::Undefined}),
          Vector<TilingDrawElement>({TilingDrawElement{TileName(), Vector2i(0,0)}}))
      },
      { PatternName("L_Border"),
        TilingPattern(Vector2i(2,3), Vector2i(1,-1),
        Vector<TilingGroupLocConstraint>(
          {TilingGroupLocConstraint::Undefined, TilingGroupLocConstraint::SameGroup
          ,TilingGroupLocConstraint::OtherGroup, TilingGroupLocConstraint::SameGroup
          ,TilingGroupLocConstraint::Undefined, TilingGroupLocConstraint::SameGroup}),
          Vector<TilingDrawElement>({TilingDrawElement{TileName(), Vector2i(1,1)}}))
      },
      { PatternName("R_Border"),
        TilingPattern(Vector2i(2,3), Vector2i(1,-1),
        Vector<TilingGroupLocConstraint>(
          {TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::Undefined
          ,TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::OtherGroup
          ,TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::Undefined}),
          Vector<TilingDrawElement>({TilingDrawElement{TileName(), Vector2i(0,1)}}))
      },
      { PatternName("T_Border"),
        TilingPattern(Vector2i(3,2), Vector2i(-1,1),
        Vector<TilingGroupLocConstraint>(
          {TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::SameGroup
          ,TilingGroupLocConstraint::Undefined,TilingGroupLocConstraint::OtherGroup, TilingGroupLocConstraint::Undefined}),
          Vector<TilingDrawElement>({TilingDrawElement{TileName(), Vector2i(1,0)}}))
      },
      { PatternName("B_Border"),
        TilingPattern(Vector2i(3,2), Vector2i(-1,1),
        Vector<TilingGroupLocConstraint>(
          {TilingGroupLocConstraint::Undefined,TilingGroupLocConstraint::OtherGroup, TilingGroupLocConstraint::Undefined,
          TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::SameGroup, TilingGroupLocConstraint::SameGroup}),
          Vector<TilingDrawElement>({TilingDrawElement{TileName(), Vector2i(1,1)}}))
      },
      });
  }
}