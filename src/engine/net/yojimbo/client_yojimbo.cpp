#include "network_yojimbo.hpp"
#include <thread>

namespace eXl
{
  namespace Network
  {
    Client_Impl::Client_Impl(uint32_t iLocalIndex, uint64_t iClientId)
      : m_Client(GetNetAllocator(), yojimbo::Address("0.0.0.0"), yojimbo::GameConnectionConfig(), *this, 0.0)
      , m_LocalIndex(iLocalIndex)
      , m_Commands(m_SerializationCtx.m_CmdDictionary)
      , m_ClientId(iClientId)
    {
      m_TimeStart = Clock::GetTimestamp();
      m_Client.SetContext(&m_SerializationCtx);
    }

    Client_Impl::Client_Impl(uint32_t iLocalIndex, uint64_t iClientId, Vector<uint8_t> iConnectToken)
      : m_Client(GetNetAllocator(), yojimbo::Address("0.0.0.0"), yojimbo::GameConnectionConfig(), *this, 0.0)
      , m_LocalIndex(iLocalIndex)
      , m_Commands(m_SerializationCtx.m_CmdDictionary)
      , m_ClientId(iClientId)
    {
      m_TimeStart = Clock::GetTimestamp();
      m_Client.SetContext(&m_SerializationCtx);
      m_Client.Connect(m_ClientId, iConnectToken.data());
    }

    Client_Impl::~Client_Impl() = default;

    void Client_Impl::ClientSendLoopbackPacket(int clientIndex, const uint8_t* packetData, int packetBytes, uint64_t packetSequence)
    {
      m_eXlClient->m_Ctx.m_Server->GetImpl().m_Server.ProcessLoopbackPacket(clientIndex, packetData, packetBytes, packetSequence);
    }

    void Client_Impl::Tick()
    {
      double totalTime = double(Clock::GetTimestamp() - m_TimeStart) / Clock::GetTicksPerSecond();
      m_Client.AdvanceTime(totalTime);
      bool hadManifest;
      do
      {
        hadManifest = m_HasManifest;
        if (!m_HasManifest)
        {
          if (!m_Client.PollPacket())
          {
            return;
          }
        }
        else
        {
          m_Client.ReceivePackets();
        }

        if (m_Client.IsConnected())
        {
          for (int i = 0; i < yojimbo::GameConnectionConfig().numChannels; i++)
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
      } while (!hadManifest);
    }

    void Client_Impl::ProcessMessage(yojimbo::Message const& iMessage)
    {
      using namespace yojimbo;

      switch (iMessage.GetType())
      {
      case GameMessageType::CLIENT_COMMAND:
      {
        eXl_ASSERT(false);
        break;
      }
      case GameMessageType::SEND_MANIFEST:
      {
        ManifestMessage const& message = static_cast<ManifestMessage const&>(iMessage);
        uint8_t const* data = message.GetBlockData();
        {
          uint64_t* assignmentData = m_SerializationCtx.m_CmdDictionary.m_CommandsHash.GetData().m_AssignmentTable.data();
          memcpy(assignmentData, data, sizeof(uint64_t) * message.arraySize);
          data += sizeof(uint64_t) * message.arraySize;
        }
        {
          uint32_t* rankData = m_SerializationCtx.m_CmdDictionary.m_CommandsHash.GetData().m_RankTable.data();
          memcpy(rankData, data, sizeof(uint32_t) * message.arraySize);
        }

        m_SerializationCtx.m_CmdDictionary.Build_Client(*m_eXlClient->m_Ctx.m_NetDriver);

        m_HasManifest = true;

        break;
      }
      case GameMessageType::SERVER_COMMAND:
      {
        CommandMessage const& cmd = static_cast<CommandMessage const&>(iMessage);
        eXl::DynObject result;
        m_SerializationCtx.m_CmdDictionary.ReceiveCommand(m_LocalIndex, cmd.m_CommandId, cmd.m_Args, result);

        if (cmd.m_QueryId != 0)
        {
          CommandMessage* message = (CommandMessage*)m_Client.CreateMessage(GameMessageType::CLIENT_REPLY);
          message->m_CommandId = cmd.m_CommandId;
          message->m_QueryId = cmd.m_QueryId;
          message->m_Args = std::move(result);
          m_Client.SendMessage(GameChannel::RELIABLE, message);
        }
        break;
      }
      case GameMessageType::SERVER_REPLY:
      {
        CommandMessage const& cmd = static_cast<CommandMessage const&>(iMessage);
        m_Commands.ReceiveResponse(cmd.m_QueryId, cmd.m_Args);
        break;
      }
      case GameMessageType::OBJECT_CREATE:
      {
        UpdateMessage const& update = static_cast<UpdateMessage const&>(iMessage);
        auto& cb = m_eXlClient->m_Ctx.m_ClientEvents.OnNewObject;
        if (cb)
        {
          cb(m_LocalIndex, update.m_Object, update.m_Data);
        }
      }
      break;
      case GameMessageType::OBJECT_UPDATE:
      {
        UpdateMessage const& update = static_cast<UpdateMessage const&>(iMessage);
        auto& cb = m_eXlClient->m_Ctx.m_ClientEvents.OnObjectUpdated;
        if (cb)
        {
          cb(m_LocalIndex, update.m_Object, update.m_Data);
        }
      }
      break;
      case GameMessageType::OBJECT_DELETE:
      {
        UpdateMessage const& update = static_cast<UpdateMessage const&>(iMessage);
        auto& cb = m_eXlClient->m_Ctx.m_ClientEvents.OnObjectDeleted;
        if (cb)
        {
          cb(m_LocalIndex, update.m_Object);
        }
      }
      break;

      default:
        eXl_ASSERT_MSG(false, "Unrecognized message");
        break;
      }
    }

    Err Client_Impl::SendServerCommand(CommandCallData&& iCall)
    {
      using namespace yojimbo;
      m_Commands.Enqueue(std::move(iCall));
      m_Commands.ProcessQueue([&](uint32_t iCmdId, eXl::DynObject iArgs, uint64_t iQueryId, bool iIsReliable)
        {
          CommandMessage* message = (CommandMessage*)m_Client.CreateMessage(GameMessageType::CLIENT_COMMAND);
          message->m_CommandId = iCmdId;
          message->m_QueryId = iQueryId;
          message->m_Args = std::move(iArgs);
          m_Client.SendMessage(iIsReliable ? GameChannel::RELIABLE : GameChannel::UNRELIABLE, message);
        });

      return eXl::Err::Success;
    }
    

    Client::~Client()
    {
      if (!m_Impl->m_Client.IsConnected())
      {
        return;
      }
      if (m_Impl->m_Client.IsLoopback())
      {
        if (m_Ctx.m_Server)
        {
          yojimbo::Server& server = m_Ctx.m_Server->GetImpl().m_Server;
          for (uint32_t i = 0; i < yojimbo::MAX_PLAYERS; ++i)
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

    Client::Client(NetCtx& iCtx, std::unique_ptr<Client_Impl> iImpl)
      : m_Ctx(iCtx)
      , m_Impl(std::move(iImpl))
      
    {
      m_Impl->m_eXlClient = this;
    }

    void Client::Tick()
    {
      m_Impl->Tick();
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

    Err Client::SendServerCommand(CommandCallData&& iCall)
    {
      return m_Impl->SendServerCommand(std::move(iCall));
    }

    const size_t Client::s_PrivateKeySize = yojimbo::KeyBytes;
    const size_t Client::s_UserDataSize = yojimbo::ConnectTokenBytes;
    const size_t Client::s_ConnectTokenSize = NETCODE_CONNECT_TOKEN_BYTES;

    Vector<uint8_t> Client::CreateConnectToken(String const& iClientId, String const& iIPAddr, Vector<uint8_t> const& iPrivateKey, Vector<uint8_t> const& iUserData)
    {
      Vector<uint8_t> outToken;

      eXl_ASSERT_REPAIR_RET(iPrivateKey.size() == s_PrivateKeySize, outToken);
      eXl_ASSERT_REPAIR_RET(iUserData.size() <= s_UserDataSize, outToken);

      Optional<uint64_t> clientId = HexToUint64(iClientId);
      eXl_ASSERT_REPAIR_RET(clientId, outToken);

      outToken.resize(NETCODE_CONNECT_TOKEN_BYTES);

      const char* publicServerAddrArr = iIPAddr.c_str();
      const char* serverAddrArr = iIPAddr.c_str();

      Vector<uint8_t> userDataBuffer = iUserData;
      if (userDataBuffer.size() < s_UserDataSize)
      {
        userDataBuffer.resize(s_UserDataSize, 0);
      }

      bool success = netcode_generate_connect_token(1,
        &publicServerAddrArr,
        &serverAddrArr, 3600, 15,
        *clientId,
        yojimbo::s_ProtocolId,
        iPrivateKey.data(),
        userDataBuffer.data(),
        outToken.data()) == NETCODE_OK;

      eXl_ASSERT_REPAIR_BEGIN(success)
      {
        outToken.clear();
      }

      return outToken;
    }

    Optional<uint32_t> Client::Connect(NetCtx& iCtx, String const& iClientId, Vector<uint8_t> iConnectToken)
    {
      eXl_ASSERT_REPAIR_RET(iCtx.m_NetDriver != nullptr, {});
      eXl_ASSERT_REPAIR_RET(iConnectToken.size() == s_ConnectTokenSize, {});

      Optional<uint64_t> clientId = HexToUint64(iClientId);

      eXl_ASSERT_REPAIR_RET(!(!clientId), {});

      std::unique_ptr<Client_Impl> client = std::make_unique<Client_Impl>(iCtx.m_Clients.size(), *clientId, iConnectToken);
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

    Optional<uint32_t> Client::ConnectLoopback(NetCtx& iCtx, String const& iClientId)
    {
      eXl_ASSERT_REPAIR_RET(iCtx.m_Server != nullptr, {});
      eXl_ASSERT_REPAIR_RET(iCtx.m_NetDriver != nullptr, {});

      Optional<uint64_t> clientId = HexToUint64(iClientId);

      eXl_ASSERT_REPAIR_RET(!(!clientId), {});

      std::unique_ptr<Client_Impl> client = std::make_unique<Client_Impl>(iCtx.m_Clients.size(), *clientId);
      uint32_t const localIndex = client->m_LocalIndex;

      int32_t freeClientIndex = 0;
      while (freeClientIndex < yojimbo::MAX_PLAYERS && iCtx.m_Server->GetImpl().m_Server.IsClientConnected(freeClientIndex))
      {
        ++freeClientIndex;
      }

      if (freeClientIndex >= yojimbo::MAX_PLAYERS)
      {
        LOG_ERROR << "Maximum number of clients connected";
        return {};
      }
      
      client->m_Client.ConnectLoopback(freeClientIndex, *clientId, yojimbo::MAX_PLAYERS);
      uint8_t userData[NETCODE_USER_DATA_BYTES] = { 0 };
      memcpy(userData, &localIndex, sizeof(localIndex));
      iCtx.m_Server->GetImpl().m_Server.ConnectLoopbackClient(freeClientIndex, *clientId, userData);

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
  }
}