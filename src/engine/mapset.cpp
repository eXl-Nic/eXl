#if 0

#include "mapset.hpp"

#include "dummysprites.hpp"

#include <core/image/imagestreamer.hpp>
#include <core/base/corelib.hpp>

namespace eXl
{
  namespace
  {
    float unitSize = 4.0;
  }

  MapSet& MapSet::Get()
  {
    static MapSet s_MapSet;
    return s_MapSet;
  }

  MapSet::MapSet() :tileFloor(Tile_OLD::RepeatableX | Tile_OLD::RepeatableY | Tile_OLD::Rotate90 | Tile_OLD::Rotate180, Vector2f(unitSize, unitSize), &locFloor)
    , tileFloorBorder(Tile_OLD::RepeatableY | Tile_OLD::Rotate90 | Tile_OLD::Rotate180, Vector2f(unitSize, 2 * unitSize), &locFloorBorder)
    , tileFloorIntCorner(Tile_OLD::RepeatableX | Tile_OLD::RepeatableY | Tile_OLD::Rotate90 | Tile_OLD::Rotate180, Vector2f(unitSize / 2, unitSize / 2), &locFloorIntCorner)
    , tileFloorExtCorner(Tile_OLD::RepeatableX | Tile_OLD::RepeatableY | Tile_OLD::Rotate90 | Tile_OLD::Rotate180, Vector2f(unitSize / 2, unitSize / 2), &locFloorExtCorner)
    , tileWall(Tile_OLD::RepeatableY | Tile_OLD::Rotate90 | Tile_OLD::Rotate180 | Tile_OLD::FaceLeft, Vector2f(2 * unitSize, 2 * unitSize), &locWall)
    , tileWallIntCorner(Tile_OLD::RepeatableX | Tile_OLD::RepeatableY | Tile_OLD::Rotate90 | Tile_OLD::Rotate180 | Tile_OLD::FaceLeft, Vector2f(2 * unitSize, 2 * unitSize), &locWallIntCorner)
    , tileWallExtCorner(Tile_OLD::RepeatableX | Tile_OLD::RepeatableY | Tile_OLD::Rotate90 | Tile_OLD::Rotate180 | Tile_OLD::FaceLeft, Vector2f(2 * unitSize, 2 * unitSize), &locWallExtCorner)
    , tileFill(Tile_OLD::RepeatableX | Tile_OLD::RepeatableY | Tile_OLD::Rotate90 | Tile_OLD::Rotate180, Vector2f(unitSize, unitSize), &locFill)

  {
#if defined(EXL_IMAGESTREAMER_ENABLED)


    String AppPath = GetAppPath();
    String DataPath = AppPath.substr(0, AppPath.rfind("/"));
    String SpritePath = DataPath;

    texFloor = eXl_NEW SpriteDesc(SpritePath + "/../../data/Floor.png");
    texFloorBorder = eXl_NEW SpriteDesc(SpritePath + "/../../data/FloorBorder.png");
    texFloorIntCorner = eXl_NEW SpriteDesc(SpritePath + "/../../data/FloorIntCorner.png");
    texFloorExtCorner = eXl_NEW SpriteDesc(SpritePath + "/../../data/FloorExtCorner.png");
    texWall = eXl_NEW SpriteDesc(SpritePath + "/../../data/WallLeft.png");
    texFill = eXl_NEW SpriteDesc(SpritePath + "/../../data/Wall.png");
    texIntCorner = eXl_NEW SpriteDesc(SpritePath + "/../../data/IntCorners.png");
    texExtCorner = eXl_NEW SpriteDesc(SpritePath + "/../../data/ExtCorners.png");

    locFloor.m_Tex = texFloor.get();
    locFloor.m_TexLoc = AABB2Di(8, 8, 16, 16);
    locFloorBorder.m_Tex = texFloorBorder.get();
    locFloorBorder.m_TexLoc = AABB2Di(0, 8, 16, 40);
    locFloorIntCorner.m_Tex = texFloorIntCorner.get();
    locFloorIntCorner.m_TexLoc = AABB2Di(0, 0, 16, 16);
    locFloorExtCorner.m_Tex = texFloorExtCorner.get();
    locFloorExtCorner.m_TexLoc = AABB2Di(0, 0, 16, 16);
    locWall.m_Tex = texWall.get();
    locWall.m_TexLoc = AABB2Di(0, 0, 32, 32);
    locWallIntCorner.m_Tex = texIntCorner.get();
    locWallIntCorner.m_TexLoc = AABB2Di(0, 0, 32, 32);
    locWallExtCorner.m_Tex = texExtCorner.get();
    locWallExtCorner.m_TexLoc = AABB2Di(8, 8, 40, 40);
    locFill.m_Tex = texFill.get();
    locFill.m_TexLoc = AABB2Di(0, 0, 8, 8);


#else
    Image dummyImage(nullptr, Image::Size(1, 1), Image::RGBA, Image::Char, 1);

    dummyImage = DummySprites::BitmapToImage(DummySprites::floorTile, Image::Size(8, 8));
    texFloor = eXl_NEW SpriteDesc(&dummyImage, Image::Adopt);

    dummyImage = DummySprites::BitmapToImage(DummySprites::floorBorder, Image::Size(8, 8));
    texFloorBorder = eXl_NEW SpriteDesc(&dummyImage, Image::Adopt);

    dummyImage = DummySprites::BitmapToImage(DummySprites::floorIntCorner, Image::Size(8, 8));
    texFloorIntCorner = eXl_NEW SpriteDesc(&dummyImage, Image::Adopt);

    dummyImage = DummySprites::BitmapToImage(DummySprites::floorExtCorner, Image::Size(8, 8));
    texFloorExtCorner = eXl_NEW SpriteDesc(&dummyImage, Image::Adopt);

    dummyImage = DummySprites::BitmapToImage(DummySprites::wall, Image::Size(8, 8));
    texWall = eXl_NEW SpriteDesc(&dummyImage, Image::Adopt);

    dummyImage = DummySprites::BitmapToImage(DummySprites::fill, Image::Size(8, 8));
    texFill = eXl_NEW SpriteDesc(&dummyImage, Image::Adopt);

    dummyImage = DummySprites::BitmapToImage(DummySprites::intCorner, Image::Size(8, 8));
    texIntCorner = eXl_NEW SpriteDesc(&dummyImage, Image::Adopt);

    dummyImage = DummySprites::BitmapToImage(DummySprites::extCorner, Image::Size(8, 8));
    texExtCorner = eXl_NEW SpriteDesc(&dummyImage, Image::Adopt);

    locFloor.m_Tex = texFloor.get();
    locFloor.m_TexLoc = AABB2Di(0, 0, 8, 8);
    locFloorBorder.m_Tex = texFloorBorder.get();
    locFloorBorder.m_TexLoc = AABB2Di(0, 0, 8, 8);
    locFloorIntCorner.m_Tex = texFloorIntCorner.get();
    locFloorIntCorner.m_TexLoc = AABB2Di(0, 0, 8, 8);
    locFloorExtCorner.m_Tex = texFloorExtCorner.get();
    locFloorExtCorner.m_TexLoc = AABB2Di(0, 0, 8, 8);
    locWall.m_Tex = texWall.get();
    locWall.m_TexLoc = AABB2Di(0, 0, 8, 8);
    locWallIntCorner.m_Tex = texIntCorner.get();
    locWallIntCorner.m_TexLoc = AABB2Di(0, 0, 8, 8);
    locWallExtCorner.m_Tex = texExtCorner.get();
    locWallExtCorner.m_TexLoc = AABB2Di(0, 0, 8, 8);
    locFill.m_Tex = texFill.get();
    locFill.m_TexLoc = AABB2Di(0, 0, 8, 8);

#endif
    mapSet.unitFloor = unitSize;
    mapSet.wall_Thickness = 2 * unitSize;
    mapSet.minWallSize = 2 * unitSize - 1;
    //mapSet.wall_FloorBand = unitSize / 2.0f;
    mapSet.Fill = &tileFill;
    mapSet.Wall = &tileWall;
    mapSet.WallIntCorner = &tileWallIntCorner;
    mapSet.WallExtCorner = &tileWallExtCorner;
    mapSet.Floor = &tileFloor;
    mapSet.FloorExtCorner = &tileFloorExtCorner;
    mapSet.FloorIntCorner = &tileFloorIntCorner;
    mapSet.FloorBorder = &tileFloorBorder;
  }
}
#endif