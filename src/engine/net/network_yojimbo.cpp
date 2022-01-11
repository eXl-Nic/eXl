#include <engine/net/network.hpp>
#include <core/log.hpp>
#include <core/clock.hpp>

#include <yojimbo.h>
#include <netcode.io/netcode.h>

namespace eXl
{
  namespace Network
  {

    class TestNetDriver : public NetDriver
    {
    public:

      void TestCommand(uint32_t iParam1, float iParam2, String const& iParam3)
      {

      }

      void TestCommand2()
      {}

      TestNetDriver(NetCtx& iCtx)
        : NetDriver(iCtx)
      {
        DeclareCommand(NetRole::Server, CommandName("Fougni"), &TestNetDriver::TestCommand, false);
        DeclareCommand(NetRole::Server, CommandName("Fougna"), &TestNetDriver::TestCommand2, false);
      }
    };

    namespace
    {
      static const uint8_t DEFAULT_PRIVATE_KEY[yojimbo::KeyBytes] = { 0 };
      static const int MAX_PLAYERS = 64;
      static const uint64_t s_ProtocolId = 0xE816CB0010000001;

      enum GameMessageType
      {
        CLIENT_COMMAND,
        SERVER_COMMAND,
        OBJECT_CREATE,
        OBJECT_DELETE,
        OBJECT_UPDATE,
        ASSIGN_PLAYER,
        MESSAGES_COUNT
      };

      enum GameChannel
      {
        RELIABLE,
        UNRELIABLE,
        CHANNELS_COUNT
      };

      struct GameConnectionConfig : yojimbo::ClientServerConfig
      {
        GameConnectionConfig() 
        {
          numChannels = CHANNELS_COUNT;
          protocolId = s_ProtocolId;
          channel[GameChannel::RELIABLE].type = yojimbo::CHANNEL_TYPE_RELIABLE_ORDERED;
          channel[GameChannel::UNRELIABLE].type = yojimbo::CHANNEL_TYPE_UNRELIABLE_UNORDERED;
#ifdef _DEBUG
          timeout = -1;
#endif
        }
      };


      class CommandMessage : public yojimbo::Message
      {
      public:
        ClientInputData m_Data;

        template <typename Stream>
        bool Serialize(Stream& stream) 
        {
          serialize_bool(stream, m_Data.m_Moving);
          serialize_float(stream, m_Data.m_Dir.X());
          serialize_float(stream, m_Data.m_Dir.Y());
          serialize_float(stream, m_Data.m_Dir.Z());
          return true;
        }

        YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
      };

      class UpdateMessage : public yojimbo::Message
      {
      public:
        ObjectId m_Object;
        ClientData m_Data;

        template <typename Stream>
        bool Serialize(Stream& stream) 
        {
          serialize_uint64(stream, m_Object.id);
          serialize_bool(stream, m_Data.m_Moving);
          serialize_float(stream, m_Data.m_Pos.X());
          serialize_float(stream, m_Data.m_Pos.Y());
          serialize_float(stream, m_Data.m_Pos.Z());
          serialize_float(stream, m_Data.m_Dir.X());
          serialize_float(stream, m_Data.m_Dir.Y());
          serialize_float(stream, m_Data.m_Dir.Z());
          return true;
        }

        YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
      };

      YOJIMBO_MESSAGE_FACTORY_START(GameMessageFactory, GameMessageType::MESSAGES_COUNT);
      YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::CLIENT_COMMAND, CommandMessage);
      YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::SERVER_COMMAND, CommandMessage);
      YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::OBJECT_CREATE, UpdateMessage);
      YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::OBJECT_UPDATE, UpdateMessage);
      YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::OBJECT_DELETE, UpdateMessage);
      YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::ASSIGN_PLAYER, UpdateMessage);
      YOJIMBO_MESSAGE_FACTORY_FINISH();

      class NetAlloc : public yojimbo::Allocator
      {
        void* Allocate(size_t size, const char* file, int line)
        {
          return eXl::MemoryManager::Allocate(size, file, line, nullptr);
        }

        void Free(void* p, const char* file, int line)
        {
          eXl::MemoryManager::Free(p, file, line, nullptr, false);
        }
      };

      NetAlloc& GetNetAllocator()
      {
        static NetAlloc s_Alloc;
        return s_Alloc;
      }

      ClientId AllocateClientId(uint32_t iClientIndex, uint32_t (&iClientGen)[MAX_PLAYERS])
      {
        uint64_t curGen = ++iClientGen[iClientIndex];
        return ClientId{ curGen << 32 | iClientIndex };
      }

      ClientId GetClientId(uint32_t iClientIndex, uint32_t const(&iClientGen)[MAX_PLAYERS])
      {
        uint64_t curGen = iClientGen[iClientIndex];
        return ClientId{ curGen << 32 | iClientIndex };
      }

      void ReleaseClientId(uint32_t iClientIndex, uint32_t (&iClientGen)[MAX_PLAYERS])
      {
        iClientGen[iClientIndex]++;
      }

      uint32_t GetClientIndexFromId(ClientId iClientId)
      {
        return iClientId.id & 0xFFFFFFFF;
      }

      bool IsValidClientId(ClientId iClientId, uint32_t const(&iClientGen)[MAX_PLAYERS])
      {
        uint32_t clientGen = iClientId.id >> 32;
        return iClientGen[GetClientIndexFromId(iClientId)] == clientGen;
      }
    }

    class GameAdapter : public yojimbo::Adapter
    {
    public:

      virtual yojimbo::Allocator* CreateAllocator(yojimbo::Allocator& allocator, void* memory, size_t bytes)
      {
        // Uses the TLSF allocator.
        return Adapter::CreateAllocator(allocator, memory, bytes);
      }

      yojimbo::MessageFactory* CreateMessageFactory(yojimbo::Allocator& allocator) override
      {
        return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
      }
    };

    class Client_Impl : public GameAdapter
    {
    public:
      Client_Impl(uint32_t iLocalIndex)
        : m_Client(GetNetAllocator(), yojimbo::Address("0.0.0.0"), GameConnectionConfig(), *this, 0.0)
        , m_LocalIndex(iLocalIndex)
      {
        m_TimeStart = Clock::GetTimestamp();
      }

      Client_Impl(uint32_t iLocalIndex, yojimbo::Address iAddr)
        : m_Client(GetNetAllocator(), yojimbo::Address("0.0.0.0"), GameConnectionConfig(), *this, 0.0)
        , m_LocalIndex(iLocalIndex)
      {
        m_TimeStart = Clock::GetTimestamp();
        uint64_t clientId;
        yojimbo::random_bytes((uint8_t*)&clientId, 8);
        m_Client.InsecureConnect(DEFAULT_PRIVATE_KEY, clientId, iAddr);
      }

      ~Client_Impl()
      {
      }

      void OnServerClientConnected(int clientIndex) override
      {
        eXl_ASSERT(false);
      }

      void OnServerClientDisconnected(int clientIndex) override
      {
        eXl_ASSERT(false);
      }

      void ClientSendLoopbackPacket(int clientIndex, const uint8_t* packetData, int packetBytes, uint64_t packetSequence) override;

      void Tick();
      void ProcessMessage(yojimbo::Message const& iMessage);

      Client* m_eXlClient;
      uint32_t m_LocalIndex;
      uint64_t m_TimeStart;
      yojimbo::Client m_Client;
    };

    class Server_Impl : public GameAdapter
    {
    public:
      Server_Impl(yojimbo::Address iAddr)
        : m_Server(GetNetAllocator(), DEFAULT_PRIVATE_KEY, std::move(iAddr), GameConnectionConfig(), *this, 0.0)
      {
        m_TimeStart = Clock::GetTimestamp();
        for (auto& gen : m_ClientSlotGeneration)
        {
          gen = 1;
        }
        m_Server.Start(MAX_PLAYERS);
      }

      ~Server_Impl()
      {
        m_Server.Stop();
      }

      void OnServerClientConnected(int clientIndex) override
      {
        m_eXlServer->m_Ctx.m_ServerEvents->OnClientConnected(AllocateClientId(clientIndex, m_ClientSlotGeneration));
      }

      void OnServerClientDisconnected(int clientIndex) override
      {
        m_eXlServer->m_Ctx.m_ServerEvents->OnClientDisconnected(GetClientId(clientIndex, m_ClientSlotGeneration));
        ReleaseClientId(clientIndex, m_ClientSlotGeneration);
      }

      virtual void ServerSendLoopbackPacket(int clientIndex, const uint8_t* packetData, int packetBytes, uint64_t packetSequence)
      {
        uint32_t* userData = reinterpret_cast<uint32_t*>(netcode_server_client_user_data(m_Server.GetNetcodeServer(), clientIndex));
        m_eXlServer->m_Ctx.m_Clients[*userData]->GetImpl().m_Client.ProcessLoopbackPacket(packetData, packetBytes, packetSequence);
      }

      void Tick();
      void ProcessMessage(uint32_t iClientIndex, yojimbo::Message const& iMessage);
      bool IsValidClientId(ClientId);

      void CreateObject(ClientId, ObjectId, ClientData const& iData);
      void UpdateObject(ClientId, ObjectId, ClientData const& iData);
      void AssignPlayer(ClientId iClient, ObjectId iObject);

      uint32_t m_ClientSlotGeneration[MAX_PLAYERS];
      Server* m_eXlServer;
      uint64_t m_TimeStart;
      yojimbo::Server m_Server;
    };

    void Client_Impl::ClientSendLoopbackPacket(int clientIndex, const uint8_t* packetData, int packetBytes, uint64_t packetSequence)
    {
      m_eXlClient->m_Ctx.m_Server->GetImpl().m_Server.ProcessLoopbackPacket(clientIndex, packetData, packetBytes, packetSequence);
    }

    bool Server_Impl::IsValidClientId(ClientId iClientId)
    {
      if (!Network::IsValidClientId(iClientId, m_ClientSlotGeneration))
      {
        return false;
      }
      if (!m_Server.IsClientConnected(GetClientIndexFromId(iClientId)))
      {
        eXl_ASSERT((iClientId.id >> 32) % 2 == 1);
        return false;
      }
      eXl_ASSERT((iClientId.id >> 32) % 2 == 0);
      return true;
    }

    Client::~Client()
    {
      if (m_Impl->m_Client.IsLoopback())
      {
        if (m_Ctx.m_Server)
        {
          yojimbo::Server& server = m_Ctx.m_Server->GetImpl().m_Server;
          for (uint32_t i = 0; i < MAX_PLAYERS; ++i)
          {
            if (server.IsClientConnected(i))
            {
              uint32_t* user_data = reinterpret_cast<uint32_t*>(netcode_server_client_user_data(server.GetNetcodeServer(), i));
              if (*user_data == m_Impl->m_LocalIndex)
              {
                server.DisconnectLoopbackClient(i);
                break;
              }
            }
          }
        }
        m_Impl->m_Client.DisconnectLoopback();
      }
      else
      {
        m_Impl->m_Client.Disconnect();
      }
    };

    Server::~Server()
    {}

    Client::Client(NetCtx& iCtx, std::unique_ptr<Client_Impl> iImpl)
      : m_Ctx(iCtx)
      , m_Impl(std::move(iImpl))
      
    {
      m_Impl->m_eXlClient = this;
    }

    Server::Server(NetCtx& iCtx, std::unique_ptr<Server_Impl> iImpl)
      : m_Ctx(iCtx)
      , m_Impl(std::move(iImpl))
    {
      m_Impl->m_eXlServer = this;
    }

    void Client::Tick()
    {
      m_Impl->Tick();
    }

    void Client_Impl::Tick()
    {
      double totalTime = double(Clock::GetTimestamp() - m_TimeStart) / Clock::GetTicksPerSecond();
      m_Client.AdvanceTime(totalTime);
      m_Client.ReceivePackets();

      if (m_Client.IsConnected()) 
      {
        for (int i = 0; i < GameConnectionConfig().numChannels; i++) 
        {
          yojimbo::Message* message = m_Client.ReceiveMessage(i);
          while (message != NULL) 
          {
            ProcessMessage(*message);
            m_Client.ReleaseMessage(message);
            message = m_Client.ReceiveMessage(i);
          }
        }
      }
    }

    void Client_Impl::ProcessMessage(yojimbo::Message const& iMessage)
    {
      switch (iMessage.GetType())
      {
      case GameMessageType::CLIENT_COMMAND:
      {
        eXl_ASSERT(false);
        break;
      }
      case GameMessageType::SERVER_COMMAND:
      {
        
        break;
      }
      case GameMessageType::OBJECT_CREATE:
      {
        UpdateMessage const& update = static_cast<UpdateMessage const&>(iMessage);
        m_eXlClient->m_Ctx.m_ClientEvents->OnNewObject(m_LocalIndex, update.m_Object, update.m_Data);
      }
      break;
      case GameMessageType::OBJECT_UPDATE:
      {
        UpdateMessage const& update = static_cast<UpdateMessage const&>(iMessage);
        m_eXlClient->m_Ctx.m_ClientEvents->OnObjectUpdated(m_LocalIndex, update.m_Object, update.m_Data);
      }
      break;
      case GameMessageType::OBJECT_DELETE:
      {
        UpdateMessage const& update = static_cast<UpdateMessage const&>(iMessage);
        m_eXlClient->m_Ctx.m_ClientEvents->OnObjectDeleted(m_LocalIndex, update.m_Object);
      }
      break;
      case GameMessageType::ASSIGN_PLAYER:      
      {
        UpdateMessage const& update = static_cast<UpdateMessage const&>(iMessage);
        m_eXlClient->m_Ctx.m_ClientEvents->OnAssignPlayer(m_LocalIndex, update.m_Object);
      }
      break;
      default:
        eXl_ASSERT_MSG(false, "Unrecognized message");
        break;
      }
    }

    void Client::Flush()
    {
      m_Impl->m_Client.SendPackets();
    }

    uint32_t Client::GetLocalIndex() const
    {
      return m_Impl->m_LocalIndex;
    }

    ClientState Client::GetState()
    {
      if (m_Impl->m_Client.IsDisconnected())
      {
        return ClientState::Disconnected;
      }

      if (m_Impl->m_Client.IsConnected())
      {
        return ClientState::Connected;
      }

      if (m_Impl->m_Client.IsConnecting())
      {
        return ClientState::Connecting;
      }

      return ClientState::Unknown;
    }

    void Client::SetClientInput(ClientInputData const& iInput)
    {
      eXl_ASSERT_REPAIR_RET(m_Impl->m_Client.IsConnected(), void());
      CommandMessage* message = (CommandMessage*)m_Impl->m_Client.CreateMessage(GameMessageType::CLIENT_COMMAND);
      message->m_Data = iInput;
      m_Impl->m_Client.SendMessage(GameChannel::RELIABLE, message);
    }

    void Server::Tick()
    {
      m_Impl->Tick();
    }

    void Server_Impl::Tick()
    {
      double totalTime = double(Clock::GetTimestamp() - m_TimeStart) / Clock::GetTicksPerSecond();
      m_Server.AdvanceTime(totalTime);
      m_Server.ReceivePackets();

      for (int i = 0; i < MAX_PLAYERS; i++) 
      {
        if (m_Server.IsClientConnected(i))
        {
          for (int j = 0; j < GameConnectionConfig().numChannels; j++)
          {
            yojimbo::Message* message = m_Server.ReceiveMessage(i, j);
            while (message != NULL) 
            {
              ProcessMessage(i, *message);
              m_Server.ReleaseMessage(i, message);
              message = m_Server.ReceiveMessage(i, j);
            }
          }
        }
      }
    }

    void Server_Impl::ProcessMessage(uint32_t iClientIndex, yojimbo::Message const& iMessage)
    {
      switch (iMessage.GetType())
      {
      case GameMessageType::CLIENT_COMMAND:
      {
        CommandMessage const& cmd = static_cast<CommandMessage const&>(iMessage);
        m_eXlServer->m_Ctx.m_ServerEvents->OnClientCommand(GetClientId(iClientIndex, m_ClientSlotGeneration), cmd.m_Data);
      }
      break;
      default:
        eXl_ASSERT_MSG(false, "Unrecognized message");
        break;
      }
    }

    void Server_Impl::CreateObject(ClientId iClient, ObjectId iObject, ClientData const& iData)
    {
      eXl_ASSERT_REPAIR_RET(IsValidClientId(iClient), void());

      uint32_t clientIndex = GetClientIndexFromId(iClient);
      UpdateMessage* message = (UpdateMessage*)m_Server.CreateMessage(clientIndex, GameMessageType::OBJECT_CREATE);
      message->m_Object = iObject;
      message->m_Data = iData;
      m_Server.SendMessage(clientIndex, GameChannel::RELIABLE, message);
    }

    void Server_Impl::UpdateObject(ClientId iClient, ObjectId iObject, ClientData const& iData)
    {
      eXl_ASSERT_REPAIR_RET(IsValidClientId(iClient), void());

      uint32_t clientIndex = GetClientIndexFromId(iClient);
      UpdateMessage* message = (UpdateMessage*)m_Server.CreateMessage(clientIndex, GameMessageType::OBJECT_UPDATE);
      message->m_Object = iObject;
      message->m_Data = iData;
      m_Server.SendMessage(clientIndex, GameChannel::RELIABLE, message);
    }

    void Server_Impl::AssignPlayer(ClientId iClient, ObjectId iObject)
    {
      eXl_ASSERT_REPAIR_RET(IsValidClientId(iClient), void());

      uint32_t clientIndex = GetClientIndexFromId(iClient);
      UpdateMessage* message = (UpdateMessage*)m_Server.CreateMessage(clientIndex, GameMessageType::ASSIGN_PLAYER);
      message->m_Object = iObject;
      m_Server.SendMessage(clientIndex, GameChannel::RELIABLE, message);
    }

    void Server::Flush()
    {
      m_Dispatcher.Flush(*this);
      m_Impl->m_Server.SendPackets();
    }

    void Server::CreateObject(ClientId iClient, ObjectId iObject, ClientData const& iData)
    {
      m_Impl->CreateObject(iClient, iObject, iData);
    }

    void Server::UpdateObject(ClientId iClient, ObjectId iObject, ClientData const& iData)
    {
      m_Impl->UpdateObject(iClient, iObject, iData);
    }

    void Server::AssignPlayer(ClientId iClient, ObjectId iObject)
    {
      m_Impl->AssignPlayer(iClient, iObject);
    }

    boost::optional<uint32_t> Client::Connect(NetCtx& iCtx, String const& iURL)
    {
      eXl_ASSERT_REPAIR_RET(iCtx.m_ClientEvents != nullptr, {});

      yojimbo::Address serverAddress(iURL.c_str(), iCtx.m_ServerPort);

      std::unique_ptr<Client_Impl> client = std::make_unique<Client_Impl>(iCtx.m_Clients.size(), serverAddress);
      uint32_t const localIndex = client->m_LocalIndex;

      if (!client->m_Client.IsConnecting())
      {
        char buffer[256];
        client->m_Client.GetAddress().ToString(buffer, sizeof(buffer));
        LOG_ERROR << "Could not create client" << buffer;
        return {};
      }

      iCtx.m_Clients.emplace_back(std::unique_ptr<Client>(eXl_NEW Client(iCtx, std::move(client))));

      return localIndex;
    }

    boost::optional<uint32_t> Client::ConnectLoopback(NetCtx& iCtx)
    {
      eXl_ASSERT_REPAIR_RET(iCtx.m_Server != nullptr, {});
      eXl_ASSERT_REPAIR_RET(iCtx.m_ClientEvents != nullptr, {});
      std::unique_ptr<Client_Impl> client = std::make_unique<Client_Impl>(iCtx.m_Clients.size());
      uint32_t const localIndex = client->m_LocalIndex;

      int32_t freeClientIndex = 0;
      while (freeClientIndex < MAX_PLAYERS && iCtx.m_Server->GetImpl().m_Server.IsClientConnected(freeClientIndex))
      {
        ++freeClientIndex;
      }

      if (freeClientIndex >= MAX_PLAYERS)
      {
        LOG_ERROR << "Maximum number of clients connected";
        return {};
      }
      
      uint64_t clientId;
      yojimbo::random_bytes((uint8_t*)&clientId, 8);
      client->m_Client.ConnectLoopback(freeClientIndex, clientId, MAX_PLAYERS);
      uint8_t userData[NETCODE_USER_DATA_BYTES] = { 0 };
      memcpy(userData, &localIndex, sizeof(localIndex));
      iCtx.m_Server->GetImpl().m_Server.ConnectLoopbackClient(freeClientIndex, clientId, userData);

      if (!client->m_Client.IsConnected())
      {
        char buffer[256];
        client->m_Client.GetAddress().ToString(buffer, sizeof(buffer));
        LOG_ERROR << "Could not create client" << buffer;
        return {};
      }

      iCtx.m_Clients.emplace_back(std::unique_ptr<Client>(eXl_NEW Client(iCtx, std::move(client))));

      return localIndex;
    }

    Server* Server::Start(NetCtx& iCtx, String const& iURL)
    {
      eXl_ASSERT_REPAIR_RET(iCtx.m_ServerEvents != nullptr, nullptr);
      eXl_ASSERT_REPAIR_RET(iCtx.m_Server == nullptr, nullptr);

      yojimbo::Address serverAddress(iURL.c_str(), iCtx.m_ServerPort);

      std::unique_ptr<Server_Impl> server = std::make_unique<Server_Impl>(serverAddress);

      if (!server->m_Server.IsRunning())
      {
        LOG_ERROR << "Could not start server at port " << std::to_string(serverAddress.GetPort());
        return nullptr;
      }

      char buffer[256];
      server->m_Server.GetAddress().ToString(buffer, sizeof(buffer));
      LOG_INFO << "Server address is " << buffer;

      iCtx.m_Server.reset(eXl_NEW Server(iCtx, std::move(server)));

      return iCtx.m_Server.get(); 
    }
  }
}