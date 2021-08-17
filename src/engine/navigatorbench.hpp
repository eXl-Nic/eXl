#pragma once

#include <dunatk/common/world.hpp>
#include <dunatk/pathfinding/navmesh.hpp>
#include <dunatk/game/character.hpp>

#include <dunatk/common/app.hpp>

namespace eXl
{
  class World;
  class Random;

  class NavigatorBench : public Scenario
  {
  public:
    using ProbaTable = Vector<std::pair<float, uint32_t>>;

    NavigatorBench(Random* iRand);

    void Init(World& iWorld) override;

    void Step(World& iWorld, float iDelta);

    void SetNavDest(World& iWorld, Vector3f const& iDest);

    static NavMesh BuildMap(World& iWorld, ObjectHandle iMapHandle, Random& iRand);

    static Vector<ObjectHandle> BuildCrossingTest(World& iWorld, CharacterSystem::Desc& ioBaseDesc, NavMesh const& iNavMesh, uint32_t iComponent);
    static Vector<ObjectHandle> BuildFullScaleTest(World& iWorld, CharacterSystem::Desc& iBaseDesc, uint32_t iNumNavAgents, NavMesh const& iNavMesh, uint32_t iComponent, Random& iRand, ProbaTable& ioProbaTable);
    static void StepFullScaleTest(World& world, float iDelta, NavMesh const& iNavMesh, uint32_t iComponent, Random& iRand, ProbaTable const& iProbaTable);

  protected:

    void ProcessInputs(World& iWorld);

    ObjectHandle CreateProjectile(World& iWorld, Vector3f const& iPos, Vector3f const& iDir);

    static Vector2f const (&GetMovingObstaclesPos())[4];
    static Vector3f GetNextDestination(Vector3f const& iCurPos);

    Tileset const* m_FireballTileset;

    //Vector2f PickRandomPosInBox(AABB2Df const& iBox) const;
    //Vector3f PickRandomPos() const;
    //Vector3f PickRandomDest(Vector3f const& iCurPos) const;

    ObjectHandle m_MapHandle;
    ObjectHandle m_NavHandle;

    Vector2i m_MousePos;

    NavMesh m_NavMesh;
    ProbaTable m_ProbaTable;
    uint32_t m_Component;
    Random* m_RandGen;
  };
}