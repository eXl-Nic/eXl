/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/resource/resource.hpp>
#include <math/aabb2dpolygon.hpp>
#include <engine/map/tilinggroup.hpp>
#include <engine/game/archetype.hpp>
#include <engine/gfx/tileset.hpp>
#include <engine/pathfinding/navmesh.hpp>
#include <core/stream/serializer.hpp>

namespace eXl
{
  MAKE_NAME(TerrainTypeName)

  struct EXL_ENGINE_API TerrainType
  {
    EXL_REFLECT;

    float m_Altitude = 0.0;
    float m_Height = 0.0;
    uint16_t m_PhysicCategory;
    uint16_t m_PhysicFilter;
    TerrainTypeName m_TerrainType;

    bool operator==(TerrainType const& iOther) const
    {
      return m_Height == iOther.m_Height
        && m_Altitude == iOther.m_Altitude
        && m_TerrainType == iOther.m_TerrainType;
    }

    static TerrainType GetTerrainTypeFromName(TerrainTypeName);
    static Vector<TerrainType> GetTerrainTypes();
  };

  class EXL_ENGINE_API MapResource : public Resource
  {
    DECLARE_RTTI(MapResource, Resource);
  public:

    struct EXL_ENGINE_API PlacedTiles
    {
      struct EXL_ENGINE_API Tile
      {
        EXL_REFLECT;
        Vec2i m_Position;
        uint32_t m_Layer = 0;
        TileName m_Name;
      };
      EXL_REFLECT;

      TerrainTypeName m_Type;
      ResourceHandle<Tileset> m_Tileset;
      Vector<Tile> m_Tiles;
    };

    struct EXL_ENGINE_API Terrain
    {
      struct EXL_ENGINE_API Block
      {
        SERIALIZE_METHODS;
      public:
        AABB2DPolygoni m_Shape;
        uint32_t m_Layer = 0;
      };
      SERIALIZE_METHODS;
    public:
      TerrainTypeName m_Type;
      ResourceHandle<TilingGroup> m_TilingGroup;
      Vector<Block> m_Blocks;
    };

    struct EXL_ENGINE_API ObjectHeader
    {
      EXL_REFLECT;

      uint64_t m_ObjectId;
      String m_DisplayName;

      static uint64_t AllocObjectID();

      Vec3 m_Position;
      ResourceHandle<Archetype> m_Archetype;
    };

    struct Object
    {
      SERIALIZE_METHODS;
    public:
      ObjectHeader m_Header;
      CustomizationData m_Data;
    };

    static void Init();

#ifndef EXL_IS_BAKED_PLATFORM
    static MapResource* Create(Path const& iDir, String const& iName);
#endif

    static ResourceLoaderName StaticLoaderName();

    MapResource(ResourceMetaData& iMeta);

    struct InstanceData
    {
      Vector<ObjectHandle> tiles;
      Vector<ObjectHandle> terrain;
      Vector<ObjectHandle> objects;
      std::unique_ptr<NavMesh> navMesh;
    };

    InstanceData Instantiate(World& iWorld, const Mat4& iPos = Identity<Mat4>()) const;

    Err Stream_Data(Streamer& iStreamer) const override;
    Err Unstream_Data(Unstreamer& iStreamer) override;

    uint32_t ComputeHash() override;

    Vector<PlacedTiles> m_Tiles;
    Vector<Terrain> m_Terrains;
    Vector<Object> m_Objects;
  private:
    Err Serialize(Serializer iSerializer);
  };

  inline size_t hash_value(TerrainType const& iVal)
  {
    size_t hash = 0;
    boost::hash_combine(hash, iVal.m_Height);
    boost::hash_combine(hash, iVal.m_TerrainType);
    return hash;
  }
} 