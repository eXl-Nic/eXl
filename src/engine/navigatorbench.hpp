#pragma once

#include <engine/common/world.hpp>
#include <engine/pathfinding/navmesh.hpp>
#include <engine/game/character.hpp>

#include <engine/common/app.hpp>

namespace eXl
{
  class World;
  class Random;

  namespace NavigatorBench
  {
    using ProbaTable = Vector<std::pair<float, uint32_t>>;

    struct Data
    {
      Vector<ObjectHandle> m_Agents;
      ProbaTable m_ProbaTable;
      uint32_t m_Component;
    };

    Data BuildCrossingTest(World& iWorld, CharacterSystem::Desc& ioBaseDesc, NavMesh const& iNavMesh, uint32_t iComponent);
    Data BuildFullScaleTest(World& iWorld, CharacterSystem::Desc& iBaseDesc, uint32_t iNumNavAgents, NavMesh const& iNavMesh, uint32_t iComponent, Random& iRand);
    void StepFullScaleTest(World& world, float iDelta, NavMesh const& iNavMesh, Random& iRand, Data& iData);
    ObjectHandle CreateProjectile(World& iWorld, Archetype const& iArch, Vector3f const& iPos, Vector3f const& iDir);


  };
}