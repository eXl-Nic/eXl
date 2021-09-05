#pragma once

#include <engine/map/map.hpp>

namespace eXl
{
  struct TileItemData
  {
    EXL_REFLECT;

    Vector2i m_Position;
    ResourceHandle<Tileset> m_Tileset;
    TileName m_Tile;
    TerrainTypeName m_Type;
    uint32_t m_Layer;
  };

  struct TerrainIslandItemData
  {
    EXL_REFLECT;

    ResourceHandle<TilingGroup> m_TilingGroup;
    TerrainTypeName m_Terrain;
    uint8_t m_Layer;
    AABB2DPolygoni m_IslandPoly;
  };
}