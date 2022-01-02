#pragma once

#include <engine/common/world.hpp>
#include <engine/pathfinding/navmesh.hpp>

#include <engine/common/app.hpp>
#include <engine/common/transformanim.hpp>

#include <engine/net/network.hpp>
#include <engine/physics/physicsdef.hpp>

#include <math/quaternion.hpp>

namespace eXl
{
  class World;
  class Random;
  class Tileset;
  class LuaScriptBehaviour;
  class GfxGUIRenderNode;

  class AbilityRoom : public Scenario
  {
  public:
    AbilityRoom();

    void PreInit(World& iWorld) override;
    void Init(World& iWorld) override;

    void Step(World& iWorld, float iDelta);

    void StartServer(World& iWorld);

    void StartClient(World& iWorld, String const& iURL);

    void StartLocal(World& iWorld);

  protected:

    static NavMesh BuildDefaultMap(World& iWorld, ObjectHandle iMapHandle);

    void ProcessInputs(World& iWorld);

    ObjectHandle m_MapHandle;
    ObjectHandle m_MainChar;
    ObjectHandle SpawnCharacter(World& iWorld, Network::NetRole iRole);
    ObjectHandle CreateFireball(World& iWorld, Vector3f const& iPos, Vector3f const& iDir);
    void CreateTrigger(World& iWorld, Vector3f const& iPos, Vector3f const& iEmitterPos, Vector3f const& iDir);
    ObjectHandle CreateCrate(World& iWorld, Vector3f const& iPos);
    ObjectHandle CreateVase(World& iWorld, Vector3f const& iPos);

    Quaternionf m_RotatingBase = Quaternionf::IDENTITY;

    NavMesh m_NavMesh;
    Random* m_RandGen;

    Vector2f m_RoomCenter;
    Vector2i m_MousePos;
    Tileset const* m_CharacterTileset;
    Tileset const* m_FireballTileset;
    Tileset const* m_CrateTileset;
    Tileset const* m_VaseTileset;
    Tileset const* m_DungeonTileset;

    GfxGUIRenderNode* m_GUIRenderNode;

    struct PlateTrigger;
    PlateTrigger* m_PlateBehaviour;
    TriggerCallbackHandle m_PlateCallback;

    LuaScriptBehaviour* m_TriggerB;

    Network::NetRole m_CurMode = Network::NetRole::None;

    uint32_t dirMask = 0;
    bool keyChanged = false;
  };
}