#pragma once

#include <dunatk/map/map.hpp>
#include <dunatk/map/tile.hpp>
#include <dunatk/map/tileblock.hpp>

namespace eXl
{
  class MapSet : public HeapObject
  {
    MapSet();
  public:
    IntrusivePtr<SpriteDesc const> texFloor;
    IntrusivePtr<SpriteDesc const> texFloorBorder;
    IntrusivePtr<SpriteDesc const> texFloorIntCorner;
    IntrusivePtr<SpriteDesc const> texFloorExtCorner;
    IntrusivePtr<SpriteDesc const> texWall;
    IntrusivePtr<SpriteDesc const> texFill;
    IntrusivePtr<SpriteDesc const> texIntCorner;
    IntrusivePtr<SpriteDesc const> texExtCorner;
    TileLoc locFloor;
    TileLoc locFloorBorder;
    TileLoc locFloorIntCorner;
    TileLoc locFloorExtCorner;
    TileLoc locWall;
    TileLoc locWallIntCorner;
    TileLoc locWallExtCorner;
    TileLoc locFill;

    Tile_OLD tileFloor;
    Tile_OLD tileFloorBorder;
    Tile_OLD tileFloorIntCorner;
    Tile_OLD tileFloorExtCorner;

    Tile_OLD tileWall;
    Tile_OLD tileWallIntCorner;
    Tile_OLD tileWallExtCorner;
    Tile_OLD tileFill;
  public:
    Old::TileSet mapSet;

    static MapSet& Get();
  };
}