#include "maptest.hpp"

#include <math/mathtools.hpp>

#include <engine/common/world.hpp>

#include <core/input.hpp>

#include <engine/map/dungeongraph_res.hpp>
#include <engine/map/dungeongraph_z.hpp>
#include <engine/map/dungeonlayout.hpp>

#include <engine/physics/physicsys.hpp>
#include <engine/physics/physiccomponent.hpp>
#include <engine/pathfinding/navmesh.hpp>

#include <engine/map/tilinggroup.hpp>
#include <engine/game/commondef.hpp>

#include <engine/map/maptiler.hpp>

namespace eXl
{
  NavMesh CreateMapFromRooms(World& iWorld, ObjectHandle iHandle, DungeonGraph const& iGraph, LayoutCollection iRooms)
  {
    ResourceHandle<TilingGroup> tilingGroupHandle;
    {
      Resource::UUID id({ 1205751177, 2135190898, 2707884201, 2854434754 });
      tilingGroupHandle.SetUUID(id);
      tilingGroupHandle.Load();
    }
    TilingGroup const* floorGroup = tilingGroupHandle.Get();

    {
      Resource::UUID id({ 2449374641, 4148984866, 84817822, 2662215188 });
      tilingGroupHandle.SetUUID(id);
      tilingGroupHandle.Load();
    }
    TilingGroup const* wallGroup = tilingGroupHandle.Get();

    Vector<AABB2Di> boxes;
    {
      uint32_t scaleFactor = 4;
      Map<DungeonGraph::Graph::vertex_descriptor, uint32_t> roomMap;
      Vector<AABB2DPolygoni> polys;
      for (auto const& room : iRooms[0])
      {
        if (iRooms[0].size() != 1)
        {
          AABB2Di box;
          box.m_Data[0] = (room.m_Box.m_Data[0] * scaleFactor) + Vector2i::ONE * scaleFactor;
          box.m_Data[1] = (room.m_Box.m_Data[1] * scaleFactor) - Vector2i::ONE * scaleFactor;
          roomMap.insert(std::make_pair(room.m_Node, polys.size()));
          polys.push_back(AABB2DPolygoni(box));
          boxes.push_back(box);
        }
        else
        {
          polys.push_back(room.m_Box);
          polys[0].Scale(scaleFactor);
          polys[0].GetBoxes(boxes);

          roomMap.insert(std::make_pair(room.m_Node, polys.size()));
        }
      }

      for (auto edge = boost::edges(iGraph.GetGraph()); edge.first != edge.second; ++edge.first)
      {
        auto vtx1 = edge.first->m_source;
        auto vtx2 = edge.first->m_target;

        AABB2Di doorPlace;
        doorPlace.SetCommonBox(iRooms[0][roomMap[vtx1]].m_Box, iRooms[0][roomMap[vtx2]].m_Box);
        doorPlace.m_Data[0] *= scaleFactor;
        doorPlace.m_Data[1] *= scaleFactor;

        int32_t doorDir = doorPlace.m_Data[0].X() == doorPlace.m_Data[1].X() ? 0 : 1;

        Vector2i doorOrig = doorPlace.GetCenter() - Vector2i::ONE * scaleFactor / 2;
        Vector2i doorSize = Vector2i::ONE * scaleFactor;

        doorOrig.m_Data[doorDir] -= scaleFactor / 2;
        doorSize.m_Data[doorDir] += scaleFactor;

        doorPlace = AABB2Di(doorOrig, doorSize);

        polys.push_back(AABB2DPolygoni(doorPlace));
        boxes.push_back(doorPlace);
      }

      AABB2Di enclosingBox = polys[0].GetAABB();
      for (uint32_t i = 1; i < polys.size(); ++i)
      {
        enclosingBox.Absorb(polys[i].GetAABB());
      }

      AABB2Di fullSpace = enclosingBox;
      fullSpace.m_Data[0] -= Vector2i::ONE * 5;
      fullSpace.m_Data[1] += Vector2i::ONE * 5;

      AABB2DPolygoni::Merge(polys);

      Vector<AABB2DPolygoni> walls(1, AABB2DPolygoni(fullSpace));
      Vector<AABB2DPolygoni> wallsTemp;

      for (auto& poly : polys)
      {
        for (auto& wall : walls)
        {
          Vector<AABB2DPolygoni> wallsOut;
          wall.Difference(poly, wallsOut);
          wallsTemp.insert(wallsTemp.end(), wallsOut.begin(), wallsOut.end());
        }
        AABB2DPolygoni::Merge(wallsTemp);
        walls = std::move(wallsTemp);
      }

      auto& gfxSys = *iWorld.GetSystem<GfxSystem>();
      auto& phSys = *iWorld.GetSystem<PhysicsSystem>();

      MapTiler::Batcher batcher;

      MapTiler::Blocks floorBlock;
      floorBlock.group = floorGroup;
      floorBlock.islands = polys;

      MapTiler::Blocks wallBlock;
      wallBlock.group = wallGroup;
      wallBlock.islands = walls;

      MapTiler::ComputeGfxForBlock(batcher, fullSpace, floorBlock);
      MapTiler::ComputeGfxForBlock(batcher, fullSpace, wallBlock);

      ObjectHandle mapObject = iWorld.CreateObject();

      batcher.Finalize(gfxSys, mapObject, 0);

      PhysicInitData wallPh;
      wallPh.SetFlags(PhysicFlags::Static);
      wallPh.SetCategory(DunAtk::s_WallCategory, DunAtk::s_WallMask);

      for (auto const& wall : walls)
      {
        Vector<AABB2Di> wallBoxes;
        wall.GetBoxes(wallBoxes);

        for (unsigned int j = 0; j < wallBoxes.size(); ++j)
        {
          Vector2i origi(wallBoxes[j].m_Data[0] + wallBoxes[j].m_Data[1]);
          Vector2i sizei(wallBoxes[j].m_Data[1] - wallBoxes[j].m_Data[0]);
          Vector3f orig(origi.X() / 2.0, origi.Y() / 2.0, 0.0);
          //orig *= DunAtk::s_WorldToPixel;
          orig *= 2;
          Vector3f size(sizei.X(), sizei.Y(), 20.0);
          //size *= DunAtk::s_WorldToPixel;
          size *= 2;
          wallPh.AddBox(size, orig);
        }
      }

      phSys.CreateComponent(iHandle, wallPh);
    }

    for (auto& box : boxes)
    {
      box.m_Data[0] *= 2;
      box.m_Data[1] *= 2;
    }

    return NavMesh::MakeFromBoxes(boxes);
  }

  MapTest::MapTest(Random* iRand)
    : m_RandGen(iRand)
  {

  }

  void MapTest::Init(World& world)
  {
    auto& transforms = *world.GetSystem<Transforms>();
    auto& gfxSys = *world.GetSystem<GfxSystem>();
    auto& phSys = *world.GetSystem<PhysicsSystem>();

    ResourceHandle<TilingGroup> tilingGroupHandle;
    {
      Resource::UUID id({ 1205751177, 2135190898, 2707884201, 2854434754 });
      tilingGroupHandle.SetUUID(id);
      tilingGroupHandle.Load();
    }
    TilingGroup const* floorGroup = tilingGroupHandle.Get();

    {
      Resource::UUID id({ 2449374641, 4148984866, 84817822, 2662215188 });
      tilingGroupHandle.SetUUID(id);
      tilingGroupHandle.Load();
    }
    TilingGroup const* wallGroup = tilingGroupHandle.Get();

    Vector2i fullSize = Vector2i::ONE * 32;

    AABB2DPolygoni fullSpace(AABB2Di(Vector2i::ZERO, fullSize));
    AABB2DPolygoni floor(AABB2Di(Vector2i::ONE * 8, Vector2i::ONE * 16));
    AABB2DPolygoni temp;
    floor.Union(AABB2DPolygoni(AABB2Di(Vector2i(4, 8), Vector2i::ONE * 4)), temp);
    temp.Union(AABB2DPolygoni(AABB2Di(Vector2i(24, 8), Vector2i::ONE)), floor);

    Vector<AABB2DPolygoni> walls;
    fullSpace.Difference(floor, walls);

    MapTiler::Blocks floorBlock;
    floorBlock.group = floorGroup;
    floorBlock.islands.push_back(floor);

    MapTiler::Blocks wallBlock;
    wallBlock.group = wallGroup;
    wallBlock.islands = walls;

    {
      MapTiler::Batcher batcher;
      MapTiler::ComputeGfxForBlock(batcher, fullSpace.GetAABB(), floorBlock);

      ObjectHandle mapHandle = world.CreateObject();

      Matrix4f mapTrans;
      mapTrans.MakeIdentity();
      MathTools::GetPosition(mapTrans) = MathTools::To3DVec(Vector2f(fullSize.X(), fullSize.Y()) * -0.5);
      transforms.AddTransform(mapHandle, &mapTrans);

      batcher.Finalize(gfxSys, mapHandle, 0);
    }
    {
      MapTiler::Batcher batcher;
      MapTiler::ComputeGfxForBlock(batcher, fullSpace.GetAABB(), wallBlock);

      ObjectHandle mapHandle = world.CreateObject();

      Matrix4f mapTrans;
      mapTrans.MakeIdentity();
      MathTools::GetPosition(mapTrans) = MathTools::To3DVec(Vector2f(fullSize.X(), fullSize.Y()) * -0.5);
      transforms.AddTransform(mapHandle, &mapTrans);

      batcher.Finalize(gfxSys, mapHandle, 0);
    }

    

    world.AddTick(World::FrameStart, [this](World& iWorld, float iDelta)
    {
      ProcessInputs(iWorld);
      Step(iWorld, iDelta);
    });

  }

  void MapTest::ProcessInputs(World& iWorld)
  {
    DunAtk_Application& app = DunAtk_Application::GetAppl();

    InputSystem& iInputs = app.GetInputSystem();

    for (auto move : iInputs.m_MouseMoveEvts)
    {
      m_MousePos = Vector2i(move.absX, move.absY);
    }

    for (auto evt : iInputs.m_MouseEvts)
    {
      if (evt.button == MouseButton::Left && evt.pressed == false)
      {
        GfxSystem& gfxSys = *iWorld.GetSystem<GfxSystem>();

        Vector3f worldPos;
        Vector3f viewDir;
        gfxSys.ScreenToWorld(m_MousePos, worldPos, viewDir);

        worldPos.Z() = 0;
        //SetNavDest(iWorld, worldPos);
      }
    }

    GetCamera().ProcessInputs(iWorld, iInputs);
  }

  

  void MapTest::Step(World& world, float iDelta)
  {
    auto& transforms = *world.GetSystem<Transforms>();
    
  }
  
}