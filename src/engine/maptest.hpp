#pragma once

#include <dunatk/common/world.hpp>
#include <dunatk/pathfinding/navmesh.hpp>
#include <dunatk/game/character.hpp>
#include <dunatk/map/dungeonlayout.hpp>

#include <dunatk/common/app.hpp>

namespace eXl
{
  class World;
  class Random;

  NavMesh CreateMapFromRooms(World& iWorld, ObjectHandle iHandle, DungeonGraph const& iGraph, LayoutCollection iRooms);

  class MapTest : public Scenario
  {
  public:

    MapTest(Random* iRand);

    void Init(World& iWorld) override;

    void Step(World& iWorld, float iDelta);

  protected:

    void ProcessInputs(World& iWorld);

    ObjectHandle m_MapHandle;

    Vector2i m_MousePos;

    NavMesh m_NavMesh;
    uint32_t m_Component;
    Random* m_RandGen;
  };
}