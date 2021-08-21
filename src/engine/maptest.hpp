#pragma once

#include <engine/common/world.hpp>
#include <engine/pathfinding/navmesh.hpp>
#include <engine/game/character.hpp>
#include <engine/map/dungeonlayout.hpp>

#include <engine/common/app.hpp>

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