
#include <gtest/gtest.h>

#include <core/corelib.hpp>
#include <core/clock.hpp>
#include <engine/common/world.hpp>
#include <engine/common/transforms.hpp>
#include <engine/net/network.hpp>

struct WorldState;

namespace eXl
{
  namespace Network
  {
    class TestNetDriver : public NetDriver
    {
    public:

      std::function<void(uint32_t iParam1, float iParam2, String const& iParam3)> TestCommand;
      std::function<void(uint32_t iLocalClient, ObjectId iObject)> AssignPlayer;
      std::function<void(ClientId iClientId, ClientInputData const& iData)> SetPlayerInput;

      TestNetDriver(NetCtx& iCtx)
        : NetDriver(iCtx)
      {
        DECLARE_CLIENT_RELIABLE_COMMAND(TestCommand);
        DECLARE_CLIENT_RELIABLE_COMMAND(AssignPlayer);
        DECLARE_SERVER_RELIABLE_COMMAND(SetPlayerInput);
      }
    };
  }
}

using namespace eXl;

struct WorldState
{
  UnorderedMap<Network::ObjectId, Network::ClientData> m_Objects;
  UnorderedMap<Network::ClientId, Network::ClientInputData> m_Clients;
  UnorderedMap<Network::ClientId, Network::ObjectId> m_ClientToObject;

  void TickAuth(float iDelta, UnorderedSet<Network::ObjectId>& oToUpdate)
  {
    for (auto const& client : m_Clients)
    {
      auto iterObject = m_ClientToObject.find(client.first);
      ASSERT_TRUE(iterObject != m_ClientToObject.end());
      auto iter = m_Objects.find(iterObject->second);
      ASSERT_TRUE(iter != m_Objects.end());
      if (client.second.m_Moving)
      {
        iter->second.m_Moving = true;
        iter->second.m_Dir = client.second.m_Dir;
        iter->second.m_Pos += client.second.m_Dir * iDelta;
        oToUpdate.insert(iter->first);
      }
      else if(iter->second.m_Moving)
      {
        iter->second.m_Moving = false;
        oToUpdate.insert(iter->first);
      }
    }
  }
};

struct TestClientEvents
{
  static void OnNewObject(WorldState& iWorld, Network::ObjectId iObject, Network::ClientData const& iData)
  {
    auto insertRes = iWorld.m_Objects.insert(std::make_pair(iObject, iData));
    ASSERT_TRUE(insertRes.second);
  }

  static void OnObjectDeleted(WorldState& iWorld, Network::ObjectId iObject)
  {
    ASSERT_EQ(iWorld.m_Objects.erase(iObject), 1);
  }

  static void OnObjectUpdated(WorldState& iWorld, Network::ObjectId iObject, Network::ClientData const& iData)
  {
    auto iterObj = iWorld.m_Objects.find(iObject);
    ASSERT_TRUE(iterObj != iWorld.m_Objects.end());
    iterObj->second = iData;
  }
};

struct TestServerEvents
{
  void OnClientConnected(Network::ClientId iClient)
  {
    auto handle = m_Objects.Alloc();
    Network::ObjectId netId{ handle.GetId() };
    auto insertClientRes = m_World.m_Clients.insert(std::make_pair(iClient, Network::ClientInputData()));
    ASSERT_TRUE(insertClientRes.second);

    auto insertClientObj = m_World.m_Objects.insert(std::make_pair(netId, Network::ClientData()));
    ASSERT_TRUE(insertClientObj.second);

    m_World.m_ClientToObject.insert(std::make_pair(iClient, netId));

    m_NewObjects.insert(netId);
    m_NewClient.insert(iClient);
  }

  void OnClientDisconnected(Network::ClientId iClient)
  {

  }

  void OnClientCommand(Network::ClientId iClient, Network::ClientInputData const& iData)
  {
    auto clientData = m_World.m_Clients.find(iClient);
    ASSERT_TRUE(clientData != m_World.m_Clients.end());
    clientData->second = iData;
  }

  void TickAuth(float iDelta)
  {
    m_World.TickAuth(iDelta, m_ToUpdate);
  }

  void FlushAuth(Network::Server& iServer)
  {
    for (auto const& newObj : m_NewObjects)
    {
      for (auto const& client : m_World.m_Clients)
      {
        if (m_NewClient.count(client.first) != 0)
        {
          continue;
        }
        iServer.CreateObject(client.first, newObj, m_World.m_Objects[newObj]);
      }
    }
    m_NewObjects.clear();

    for (Network::ClientId newClient : m_NewClient)
    {
      for (auto const& object : m_World.m_Objects)
      {
        iServer.CreateObject(newClient, object.first, object.second);
      }
    }
    m_NewClient.clear();

    for (Network::ObjectId object : m_ToUpdate)
    {
      for (auto const& client : m_World.m_Clients)
      {
        iServer.UpdateObject(client.first, object, m_World.m_Objects[object]);
      }
    }
    m_ToUpdate.clear();
  }

  ObjectTable<int> m_Objects;
  UnorderedSet<Network::ObjectId> m_NewObjects;
  UnorderedSet<Network::ClientId> m_NewClient;
  UnorderedSet<Network::ObjectId> m_ToUpdate;
  WorldState m_World;
};

struct NetworkSim
{
  Network::NetCtx ctx;
  Network::TestNetDriver driver;
  Vector<uint8_t> key;
  Clock timer;

  NetworkSim()
    : driver(ctx)
  {
    key.resize(Network::Server::s_PrivateKeySize, 0);
  }

  void Tick(float iTimeout, std::function<bool()> iWaitCondition = [] { return false; })
  {
    float elapsedTime = 0.0;
    float lastDelta = 0.0;
    while (elapsedTime < iTimeout)
    {
      lastDelta = timer.GetTime();
      elapsedTime += lastDelta;
      if (ctx.m_Server)
      {
        ctx.m_Server->Tick();
        ctx.m_Server->Flush();
      }

      for (auto const& client : ctx.m_Clients)
      {
        client->Tick();
        client->Flush();
      }

      if (iWaitCondition && iWaitCondition())
      {
        break;
      }
    }
  }
};

TEST(Network, BasicConnect)
{
  String const serverAddress("127.0.0.1:21616");

  TestServerEvents serverEvents;

  NetworkSim net;
  net.ctx.m_ServerEvents.OnClientConnected = [&](Network::ClientId iClient)
  { serverEvents.OnClientConnected(iClient); };
  net.ctx.m_ServerEvents.OnClientDisconnected = [&](Network::ClientId iClient)
  { serverEvents.OnClientDisconnected(iClient); };
  

  Network::Server::Start(net.ctx, serverAddress, net.key);

  ASSERT_TRUE(net.ctx.m_Server != nullptr);

  Vector<uint8_t> connectToken = Network::Client::CreateConnectToken("FEDCBA987654321", serverAddress, net.key, Vector<uint8_t>());

  auto localIdx = Network::Client::Connect(net.ctx, "FEDCBA987654321", connectToken);
  ASSERT_TRUE(localIdx);

  auto client1 = net.ctx.m_Clients[*localIdx].get();
  ASSERT_TRUE(client1 != nullptr);

  net.Tick(1.0,
    [client1]
    {
      return client1->GetState() == Network::ClientState::Connected;
    });

  ASSERT_TRUE(client1->GetState() == Network::ClientState::Connected);

  connectToken = Network::Client::CreateConnectToken("2", serverAddress, net.key, Vector<uint8_t>());
  localIdx = Network::Client::Connect(net.ctx, "2", connectToken);
  ASSERT_TRUE(localIdx);

  auto client2 = net.ctx.m_Clients[*localIdx].get();
  ASSERT_TRUE(client2 != nullptr);

  net.Tick(1.0,
    [client2]
    {
      return client2->GetState() == Network::ClientState::Connected;
    });

  ASSERT_TRUE(client1->GetState() == Network::ClientState::Connected);
  ASSERT_TRUE(client2->GetState() == Network::ClientState::Connected);
}

TEST(Network, BasicRepl)
{
  String const serverAddress("127.0.0.1");

  NetworkSim net;
  TestServerEvents serverEvents;
  net.ctx.m_ServerEvents.OnClientConnected = [&](Network::ClientId iClient)
  { serverEvents.OnClientConnected(iClient); };
  net.ctx.m_ServerEvents.OnClientDisconnected = [&](Network::ClientId iClient)
  { serverEvents.OnClientDisconnected(iClient); };

  WorldState clientWorlds[2];
  net.ctx.m_ClientEvents.OnNewObject = [&](uint32_t iLocalClient, Network::ObjectId iObject, Network::ClientData const& iData)
  {
    TestClientEvents::OnNewObject(clientWorlds[iLocalClient], iObject, iData);
  };
  net.ctx.m_ClientEvents.OnObjectUpdated = [&](uint32_t iLocalClient, Network::ObjectId iObject, Network::ClientData const& iData)
  {
    TestClientEvents::OnObjectUpdated(clientWorlds[iLocalClient], iObject, iData);
  };
  net.ctx.m_ClientEvents.OnObjectDeleted = [&](uint32_t iLocalClient, Network::ObjectId iObject)
  {
    TestClientEvents::OnObjectDeleted(clientWorlds[iLocalClient], iObject);
  };

  net.driver.SetPlayerInput = [&](Network::ClientId iClientId, Network::ClientInputData const& iData)
  {
    serverEvents.OnClientCommand(iClientId, iData);
  };

  Network::Server::Start(net.ctx, serverAddress, net.key);

  ASSERT_TRUE(net.ctx.m_Server != nullptr);

  auto localIdx = Network::Client::ConnectLoopback(net.ctx, "1");
  ASSERT_TRUE(localIdx);

  auto client1 = net.ctx.m_Clients[*localIdx].get();
  ASSERT_TRUE(client1 != nullptr);

  uint32_t const client1Idx = *localIdx;

  net.Tick(1.0,
    [client1, &serverEvents]
    {
      return client1->GetState() == Network::ClientState::Connected
        && serverEvents.m_World.m_Clients.size() > 0;
    });

  serverEvents.TickAuth(1.0);
  serverEvents.FlushAuth(*net.ctx.m_Server);
  net.Tick(0.01);
  ASSERT_TRUE(clientWorlds[*localIdx].m_Objects.size() > 0);

  Network::ClientInputData input;
  input.m_Moving = true;
  input.m_Dir = Vector3f::UNIT_X;
  net.driver.CallServerCommand(0, net.driver.SetPlayerInput).WithArgs(input).Send();
  //client1->SetClientInput(input);

  serverEvents.TickAuth(1.0);
  serverEvents.FlushAuth(*net.ctx.m_Server);
  net.Tick(0.01);
  // Server just received client's packet.
  ASSERT_TRUE(clientWorlds[*localIdx].m_Objects.begin()->second.m_Pos == Vector3f::ZERO);

  serverEvents.TickAuth(1.0);
  serverEvents.FlushAuth(*net.ctx.m_Server);
  net.Tick(0.01);

  // Server ticked once.
  ASSERT_TRUE(clientWorlds[*localIdx].m_Objects.begin()->second.m_Pos == Vector3f::UNIT_X);

  serverEvents.TickAuth(1.0);
  serverEvents.FlushAuth(*net.ctx.m_Server);
  net.Tick(0.01);

  // Server ticked twice.
  ASSERT_TRUE(clientWorlds[*localIdx].m_Objects.begin()->second.m_Pos == Vector3f::UNIT_X * 2);

  localIdx = Network::Client::ConnectLoopback(net.ctx, "2");
  ASSERT_TRUE(localIdx);
  auto client2 = net.ctx.m_Clients[*localIdx].get();
  ASSERT_TRUE(client2 != nullptr);

  uint32_t client2Idx = *localIdx;

  net.Tick(1.0,
    [client2, &serverEvents]
    {
      return client2->GetState() == Network::ClientState::Connected
        && serverEvents.m_World.m_Clients.size() == 2;
    });

  serverEvents.TickAuth(1.0);
  serverEvents.FlushAuth(*net.ctx.m_Server);
  net.Tick(0.01);
  ASSERT_EQ(clientWorlds[client1Idx].m_Objects.size(), 2);

  input.m_Moving = false;
  
  //client1->SetClientInput(input);

  //serverEvents.TickAuth(1.0);
  //serverEvents.FlushAuth(*net.ctx.m_Server);
  //net.Tick(0.01);
  //serverEvents.TickAuth(1.0);
  //serverEvents.FlushAuth(*net.ctx.m_Server);
  //net.Tick(0.01);
}