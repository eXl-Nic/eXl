#pragma once

#include <engine/common/app.hpp>
#include <engine/net/network.hpp>
#include <engine/map/map.hpp>

namespace eXl
{
  class EXL_ENGINE_API Scenario_Base : public Scenario
  {
  public:

    void Init(World& iWorld) override;

    void StartServer(World& iWorld);

    void StartClient(World& iWorld, String const& iURL);

    void StartLocal(World& iWorld);

    void SetMap(ResourceHandle<MapResource> iMap)
    {
      m_Map = iMap;
    }

    ObjectHandle GetMainChar()
    {
      return m_MainChar;
    }

    MapResource::InstanceData const& GetMapData() { return m_InstatiatedMap; }

  protected:

    ResourceHandle<MapResource> m_Map;
    MapResource::InstanceData m_InstatiatedMap;

    ObjectHandle m_MainChar;
    Network::NetRole m_CurMode = Network::NetRole::None;

    ObjectHandle SpawnCharacter(World& iWorld, Network::NetRole iRole);
    Vector2f m_SpawnPos;

    void ProcessInputs(World& iWorld);
    uint32_t dirMask = 0;
    bool keyChanged = false;
  };
}