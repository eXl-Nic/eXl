
#include <gtest/gtest.h>

#include <core/corelib.hpp>
#include <core/clock.hpp>
#include <engine/common/world.hpp>
#include <engine/common/transforms.hpp>
#include <engine/net/network.hpp>

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

struct TestClientEvents : Network::ClientEvents
{
  void OnNewObject(uint32_t iLocalClient, Network::ObjectId iObject, Network::ClientData const& iData) override
  {
    auto insertRes = m_World[iLocalClient].m_Objects.insert(std::make_pair(iObject, iData));
    ASSERT_TRUE(insertRes.second);
  }

  void OnObjectDeleted(uint32_t iLocalClient, Network::ObjectId iObject) override
  {
    ASSERT_EQ(m_World[iLocalClient].m_Objects.erase(iObject), 1);
  }

  void OnObjectUpdated(uint32_t iLocalClient, Network::ObjectId iObject, Network::ClientData const& iData) override
  {
    auto iterObj = m_World[iLocalClient].m_Objects.find(iObject);
    ASSERT_TRUE(iterObj != m_World[iLocalClient].m_Objects.end());
    iterObj->second = iData;
  }

  void OnAssignPlayer(uint32_t, Network::ObjectId)
  {

  }

  Vector<WorldState> m_World;
};

struct TestServerEvents : Network::ServerEvents
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
  Clock timer;

  static uint16_t const serverPort = 16987;
  NetworkSim() : ctx(serverPort){}

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
  String const serverAddress("127.0.0.1");

  TestClientEvents clientEvents;
  TestServerEvents serverEvents;

  NetworkSim net;
  net.ctx.m_ServerEvents = &serverEvents;
  net.ctx.m_ClientEvents = &clientEvents;

  Network::Server::Start(net.ctx, serverAddress);

  ASSERT_TRUE(net.ctx.m_Server != nullptr);

  auto localIdx = Network::Client::Connect(net.ctx, serverAddress);
  ASSERT_TRUE(localIdx);

  auto client1 = net.ctx.m_Clients[*localIdx].get();
  ASSERT_TRUE(client1 != nullptr);

  net.Tick(1.0,
    [client1]
    {
      return client1->GetState() == Network::ClientState::Connected;
    });

  ASSERT_TRUE(client1->GetState() == Network::ClientState::Connected);

  localIdx = Network::Client::Connect(net.ctx, serverAddress);
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

  TestClientEvents clientEvents;
  TestServerEvents serverEvents;

  NetworkSim net;
  net.ctx.m_ServerEvents = &serverEvents;
  net.ctx.m_ClientEvents = &clientEvents;

  Network::Server::Start(net.ctx, serverAddress);

  ASSERT_TRUE(net.ctx.m_Server != nullptr);

  clientEvents.m_World.push_back(WorldState());
  auto localIdx = Network::Client::ConnectLoopback(net.ctx/*, serverAddress*/);
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
  ASSERT_TRUE(clientEvents.m_World[*localIdx].m_Objects.size() > 0);

  Network::ClientInputData input;
  input.m_Moving = true;
  input.m_Dir = Vector3f::UNIT_X;
  client1->SetClientInput(input);

  serverEvents.TickAuth(1.0);
  serverEvents.FlushAuth(*net.ctx.m_Server);
  net.Tick(0.01);
  // Server just received client's packet.
  ASSERT_TRUE(clientEvents.m_World[*localIdx].m_Objects.begin()->second.m_Pos == Vector3f::ZERO);

  serverEvents.TickAuth(1.0);
  serverEvents.FlushAuth(*net.ctx.m_Server);
  net.Tick(0.01);

  // Server ticked once.
  ASSERT_TRUE(clientEvents.m_World[*localIdx].m_Objects.begin()->second.m_Pos == Vector3f::UNIT_X);

  serverEvents.TickAuth(1.0);
  serverEvents.FlushAuth(*net.ctx.m_Server);
  net.Tick(0.01);

  // Server ticked twice.
  ASSERT_TRUE(clientEvents.m_World[*localIdx].m_Objects.begin()->second.m_Pos == Vector3f::UNIT_X * 2);

  clientEvents.m_World.push_back(WorldState());
  localIdx = Network::Client::ConnectLoopback(net.ctx/*, serverAddress*/);
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
  ASSERT_EQ(clientEvents.m_World[client1Idx].m_Objects.size(), 2);

  input.m_Moving = false;
  
  //client1->SetClientInput(input);

  //serverEvents.TickAuth(1.0);
  //serverEvents.FlushAuth(*net.ctx.m_Server);
  //net.Tick(0.01);
  //serverEvents.TickAuth(1.0);
  //serverEvents.FlushAuth(*net.ctx.m_Server);
  //net.Tick(0.01);
}