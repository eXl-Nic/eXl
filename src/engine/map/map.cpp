/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/map/map.hpp>

#include <core/resource/resourceloader.hpp>
#include <core/resource/resourcemanager.hpp>
#include <math/mathtools.hpp>
#include <boost/uuid/random_generator.hpp>
#include <engine/game/commondef.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(MapResource);
  IMPLEMENT_SERIALIZE_METHODS(MapResource::Object);
  IMPLEMENT_SERIALIZE_METHODS(MapResource::Terrain::Block);
  IMPLEMENT_SERIALIZE_METHODS(MapResource::Terrain);

  static TerrainType WallType()
  {
    static TerrainType s_WallType = { 0.0, 1.0, EngineCommon::s_WallCategory, EngineCommon::s_WallMask, TerrainTypeName("Wall") };
    return s_WallType;
  }

  static TerrainType FloorType()
  {
    static TerrainType s_FloorType = { 0.0, 0.0, 0, 0, TerrainTypeName("Floor") };
    return s_FloorType;
  }

  TerrainType TerrainType::GetTerrainTypeFromName(TerrainTypeName iName)
  {
    if (iName == WallType().m_TerrainType)
    {
      return WallType();
    }

    if (iName == FloorType().m_TerrainType)
    {
      return FloorType();
    }
    eXl_ASSERT(false);
    TerrainType dummyType = { 0.0, 0.0, 0, 0, TerrainTypeName("") };
    return dummyType;
  }

  Vector<TerrainType> TerrainType::GetTerrainTypes()
  {
    Vector<TerrainType> terrains = {WallType(), FloorType()};

    return terrains;
  }

  uint64_t MapResource::ObjectHeader::AllocObjectID()
  {
    static boost::uuids::random_generator s_UUIDGen;
    boost::uuids::uuid newuuid = s_UUIDGen();
    uint32_t* dwords = reinterpret_cast<uint32_t*>(newuuid.data);

    static_assert (sizeof(size_t) == sizeof(uint64_t), "");
    {
      uint64_t objId[2] = { dwords[0], dwords[2] };
      objId[0] <<= 32;
      objId[0] |= dwords[1];
      objId[1] <<= 32;
      objId[1] |= dwords[3];
      boost::hash_combine(objId[0], objId[1]);
      objId[0] &= ~World::s_AnonymousFlag;
      return objId[0];
    }
    //else
    //{
    //  size_t seed = dwords[0];
    //  boost::hash_combine(seed, dwords[1]);
    //  boost::hash_combine(seed, dwords[2]);
    //  boost::hash_combine(seed, dwords[3]);
    //
    //  return seed;
    //}
  }

  Err MapResource::Object::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();

    iStreamer.PushKey("Header");
    iStreamer &= m_Header;
    iStreamer.PopKey();

    iStreamer.PushKey("Overrides");
    iStreamer &= m_Data;
    iStreamer.PopKey();

    iStreamer.EndStruct();

    return Err::Success;
  }

  Err MapResource::Terrain::Block::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("Shape");
    iStreamer &= m_Shape;
    iStreamer.PopKey();
    iStreamer.PushKey("Layer");
    iStreamer &= m_Layer;
    iStreamer.PopKey();
    iStreamer.EndStruct();

    return Err::Success;
  }

  Err MapResource::Terrain::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("Terrain");
    iStreamer &= m_Type;
    iStreamer.PopKey();
    iStreamer.PushKey("TilingGroup");
    iStreamer &= m_TilingGroup;
    iStreamer.PopKey();
    iStreamer.PushKey("Blocks");
    iStreamer &= m_Blocks;
    iStreamer.PopKey();
    iStreamer.EndStruct();

    return Err::Success;
  }

#if 0

  class MapLoader : public ResourceLoader
  {
  public:

    static MapLoader& Get()
    {
      static MapLoader s_This;
      return s_This;
    }

    MapLoader()
      : ResourceLoader(MapResource::StaticLoaderName(), 1)
    {

    }
#ifndef EXL_IS_BAKED_PLATFORM
    MapResource* Create(Path const& iDir, String const& iName) const
    {
      ResourceMetaData* metaData = CreateNewMetaData(iName);

      Path rscPath = iDir / Path(iName.c_str());
      rscPath.replace_extension(ResourceManager::GetAssetExtension().c_str());

      MapResource* newMap = eXl_NEW MapResource(*metaData);
      if (ResourceManager::SetPath(newMap, rscPath))
      {
        return newMap;
      }

      return nullptr;
    }
#endif

    Resource* Load(Resource::Header const& iHeader, ResourceMetaData* iMetaData, Unstreamer& iStreamer) const override
    {
      MapResource* loadedMap = eXl_NEW MapResource(*iMetaData);

      loadedMap->Unstream(iStreamer);

      return loadedMap;
    }
  };
#endif

  using MapLoader = TResourceLoader<MapResource, ResourceLoader>;

  void MapResource::Init()
  {
    ResourceManager::AddLoader(&MapLoader::Get(), MapResource::StaticRtti());
  }

#ifndef EXL_IS_BAKED_PLATFORM
  MapResource* MapResource::Create(Path const& iDir, String const& iName)
  {
    return MapLoader::Get().Create(iDir, iName);
  }
#endif

  MapResource::MapResource(ResourceMetaData& iMeta)
    : Resource(iMeta)
  {

  }

  ResourceLoaderName MapResource::StaticLoaderName()
  {
    return ResourceLoaderName("Map");
  }

  Err MapResource::Stream_Data(Streamer& iStreamer) const
  {
    return const_cast<MapResource*>(this)->Serialize(Serializer(iStreamer));
  }

  Err MapResource::Unstream_Data(Unstreamer& iStreamer)
  {
    return Serialize(Serializer(iStreamer));
  }

  Err MapResource::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();

    iStreamer.PushKey("Tiles");
    iStreamer &= m_Tiles;
    iStreamer.PopKey();

    iStreamer.PushKey("Terrain");
    iStreamer &= m_Terrains;
    iStreamer.PopKey();

    iStreamer.PushKey("Objects");
    iStreamer &= m_Objects;
    iStreamer.PopKey();

    iStreamer.EndStruct();

    return Err::Success;
  }

  uint32_t MapResource::ComputeHash()
  {
    return 0;
  }
}