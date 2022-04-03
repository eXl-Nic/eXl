#pragma once

#include <engine/common/world.hpp>
#include <engine/pathfinding/navmesh.hpp>
#include <engine/game/character.hpp>

#include <engine/common/app.hpp>

namespace eXl
{
  class World;
  class Random;
  class MenuManager;

  namespace NavigatorBench
  {
    using ProbaTable = Vector<std::pair<float, uint32_t>>;

    struct Data;

    void BuildCrossingTest(World& iWorld, Archetype const& iArch, CharacterSystem::Desc& ioBaseDesc, NavMesh const& iNavMesh, uint32_t iComponent, Data& ioData);
    void BuildFullScaleTest(World& iWorld, Archetype const& iArch, CharacterSystem::Desc& iBaseDesc, uint32_t iNumNavAgents, NavMesh const& iNavMesh, uint32_t iComponent, Data& ioData);
    void StepFullScaleTest(World& world, float iDelta, NavMesh const& iNavMesh, Random& iRand, Data& iData);
    ObjectHandle CreateProjectile(World& iWorld, Archetype const& iArch, Vec3 const& iPos, Vec3 const& iDir);
    void AddNavigatorBenchMenu(MenuManager& iMenus, World& iWorld);

  };
}