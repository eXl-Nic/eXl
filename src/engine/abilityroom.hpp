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

  class AbilityRoom : public Scenario, public Network::ClientEvents, public Network::ServerEvents
  {
  public:
    AbilityRoom();

    void Init(World& iWorld) override;

    void Step(float iDelta);

    void StartServer(String const& iURL);

    void StartClient(String const& iURL);

    void StartLocal();

  protected:

    void OnNewObject(uint32_t, Network::ObjectId, Network::ClientData const&) override;
    void OnObjectDeleted(uint32_t, Network::ObjectId) override;
    void OnObjectUpdated(uint32_t, Network::ObjectId, Network::ClientData const&) override;
    void OnAssignPlayer(uint32_t, Network::ObjectId) override;

    void OnClientConnected(Network::ClientId) override;
    void OnClientDisconnected(Network::ClientId) override;
    void OnClientCommand(Network::ClientId, Network::ClientInputData const&) override;

    static NavMesh BuildDefaultMap(World& iWorld, ObjectHandle iMapHandle);

    void ProcessInputs(World& iWorld);

    ObjectHandle m_MapHandle;
    ObjectHandle m_MainChar;
    ObjectHandle SpawnCharacter(World& iWorld, Network::NetRole iRole, Vector3f const& iPos, uint64_t iId = ObjectCreationInfo::s_AutoNamedFlag);
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

    struct PlateTrigger;
    PlateTrigger* m_PlateBehaviour;
    TriggerCallbackHandle m_PlateCallback;

    LuaScriptBehaviour* m_TriggerB;

    Network::NetCtx m_Net;
    Network::NetRole m_CurMode = Network::NetRole::None;

    uint32_t dirMask = 0;
    bool keyChanged = false;
  };
}