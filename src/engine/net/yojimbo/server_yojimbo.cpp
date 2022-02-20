
#include "network_yojimbo.hpp"

namespace eXl
{
  namespace Network
  {
    Server_Impl::Server_Impl(yojimbo::Address iAddr, Vector<uint8_t> iPrivateKey)
      : m_Server(GetNetAllocator(), iPrivateKey.data(), std::move(iAddr), yojimbo::GameConnectionConfig(), *this, 0.0)
    {
      m_TimeStart = Clock::GetTimestamp();
      m_Server.SetContext(&m_SerializationCtx);
      for (auto& gen : m_ClientSlotGeneration)
      {
        gen = 1;
      }
      m_Server.Start(yojimbo::MAX_PLAYERS);
    }

    void Server_Impl::OnServerClientConnected(int clientIndex)
    {
      using namespace yojimbo;
      while (m_ClientCmdQueues.size() <= clientIndex)
      {
        m_ClientCmdQueues.emplace_back(m_SerializationCtx.m_CmdDictionary);
      }

      ManifestMessage* message = (ManifestMessage*)m_Server.CreateMessage(clientIndex, GameMessageType::SEND_MANIFEST);
      uint32_t const arraySize = m_SerializationCtx.m_CmdDictionary.m_CommandsHash.GetData().m_AssignmentTable.size();
      size_t const blockSize = arraySize * (sizeof(uint32_t) + sizeof(uint64_t));
      uint8_t* data = m_Server.AllocateBlock(clientIndex, blockSize);
      m_Server.AttachBlockToMessage(clientIndex, message, data, blockSize);

      memcpy(data, m_SerializationCtx.m_CmdDictionary.m_CommandsHash.GetData().m_AssignmentTable.data(), arraySize * sizeof(uint64_t));
      data += arraySize * sizeof(uint64_t);
      memcpy(data, m_SerializationCtx.m_CmdDictionary.m_CommandsHash.GetData().m_RankTable.data(), arraySize * sizeof(uint32_t));

      m_Server.SendMessage(clientIndex, GameChannel::RELIABLE, message);

      auto& cb = m_eXlServer->m_Ctx.m_ServerEvents.OnClientConnected;
      if (cb)
      {
        cb(AllocateClientId(clientIndex, m_ClientSlotGeneration));
      }
    }

    void Server_Impl::OnServerClientDisconnected(int clientIndex)
    {
      auto& cb = m_eXlServer->m_Ctx.m_ServerEvents.OnClientDisconnected;
      if (cb)
      {
        cb(GetClientId(clientIndex, m_ClientSlotGeneration));
      }
      m_ClientCmdQueues[clientIndex].Clear();
      ReleaseClientId(clientIndex, m_ClientSlotGeneration);
    }

    void Server_Impl::ServerSendLoopbackPacket(int clientIndex, const uint8_t* packetData, int packetBytes, uint64_t packetSequence)
    {
      uint32_t* userData = reinterpret_cast<uint32_t*>(netcode_server_client_user_data(m_Server.GetNetcodeServer(), clientIndex));
      m_eXlServer->m_Ctx.m_Clients[*userData]->GetImpl().m_Client.ProcessLoopbackPacket(packetData, packetBytes, packetSequence);
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

    void Server_Impl::Tick()
    {
      double totalTime = double(Clock::GetTimestamp() - m_TimeStart) / Clock::GetTicksPerSecond();
      m_Server.AdvanceTime(totalTime);
      m_Server.ReceivePackets();

      for (int i = 0; i < yojimbo::MAX_PLAYERS; i++)
      {
        if (m_Server.IsClientConnected(i))
        {
          for (int j = 0; j < yojimbo::GameConnectionConfig().numChannels; j++)
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
      using namespace yojimbo;
      switch (iMessage.GetType())
      {
      case GameMessageType::CLIENT_COMMAND:
      {
        CommandMessage const& cmd = static_cast<CommandMessage const&>(iMessage);
        DynObject result;
        m_SerializationCtx.m_CmdDictionary.ReceiveCommand(GetClientId(iClientIndex, m_ClientSlotGeneration).id, cmd.m_CommandId, cmd.m_Args, result);

        if (cmd.m_QueryId != 0)
        {
          CommandMessage* message = (CommandMessage*)m_Server.CreateMessage(iClientIndex, GameMessageType::SERVER_REPLY);
          message->m_CommandId = cmd.m_CommandId;
          message->m_QueryId = cmd.m_QueryId;
          message->m_Args = std::move(result);
          m_Server.SendMessage(iClientIndex, GameChannel::RELIABLE, message);
        }
        break;
      }
      case GameMessageType::CLIENT_REPLY:
      {
        CommandMessage const& cmd = static_cast<CommandMessage const&>(iMessage);
        m_ClientCmdQueues[iClientIndex].ReceiveResponse(cmd.m_QueryId, cmd.m_Args);
      }
      break;
      default:
        eXl_ASSERT_MSG(false, "Unrecognized message");
        break;
      }
    }

    void Server_Impl::CreateObject(ClientId iClient, ObjectId iObject, ClientData const& iData)
    {
      using namespace yojimbo;
      eXl_ASSERT_REPAIR_RET(IsValidClientId(iClient), void());

      uint32_t clientIndex = GetClientIndexFromId(iClient);
      UpdateMessage* message = (UpdateMessage*)m_Server.CreateMessage(clientIndex, GameMessageType::OBJECT_CREATE);
      message->m_Object = iObject;
      message->m_Data = iData;
      m_Server.SendMessage(clientIndex, GameChannel::RELIABLE, message);
    }

    void Server_Impl::UpdateObject(ClientId iClient, ObjectId iObject, ClientData const& iData)
    {
      using namespace yojimbo;
      eXl_ASSERT_REPAIR_RET(IsValidClientId(iClient), void());

      uint32_t clientIndex = GetClientIndexFromId(iClient);
      UpdateMessage* message = (UpdateMessage*)m_Server.CreateMessage(clientIndex, GameMessageType::OBJECT_UPDATE);
      message->m_Object = iObject;
      message->m_Data = iData;
      m_Server.SendMessage(clientIndex, GameChannel::RELIABLE, message);
    }

    void Server_Impl::DeleteObject(ClientId iClient, ObjectId iObject)
    {
      using namespace yojimbo;
      eXl_ASSERT_REPAIR_RET(IsValidClientId(iClient), void());

      uint32_t clientIndex = GetClientIndexFromId(iClient);
      UpdateMessage* message = (UpdateMessage*)m_Server.CreateMessage(clientIndex, GameMessageType::OBJECT_DELETE);
      message->m_Object = iObject;
      m_Server.SendMessage(clientIndex, GameChannel::RELIABLE, message);
    }

    Err Server_Impl::SendClientCommand(CommandCallData&& iCall, ClientId iClient)
    {
      if (!IsValidClientId(iClient))
      {
        return Err::Failure;
      }

      m_ClientCmdQueues[GetClientIndexFromId(iClient)].Enqueue(std::move(iCall));
      return Err::Success;
    }

    void Server_Impl::Flush()
    {
      using namespace yojimbo;
      for (int i = 0; i < MAX_PLAYERS; i++)
      {
        if (m_Server.IsClientConnected(i))
        {
          m_ClientCmdQueues[i].ProcessQueue([&](uint32_t iCmdId, DynObject iArgs, uint64_t iQueryId, bool iIsReliable)
            {
              CommandMessage* message = (CommandMessage*)m_Server.CreateMessage(i, GameMessageType::SERVER_COMMAND);
              message->m_CommandId = iCmdId;
              message->m_QueryId = iQueryId;
              message->m_Args = std::move(iArgs);
              m_Server.SendMessage(i, iIsReliable ? GameChannel::RELIABLE : GameChannel::UNRELIABLE, message);
            });
        }
      }

      m_Server.SendPackets();
    }

    Server::~Server()
    {}

    Server::Server(NetCtx& iCtx, std::unique_ptr<Server_Impl> iImpl)
      : m_Ctx(iCtx)
      , m_Impl(std::move(iImpl))
    {
      m_Impl->m_eXlServer = this;
      m_Impl->m_SerializationCtx.m_CmdDictionary.Build(*m_Ctx.m_NetDriver);
    }

    void Server::Tick()
    {
      m_Impl->Tick();
    }

    Err Server::SendClientCommand(CommandCallData&& iCall, ClientId iClient)
    {
      return m_Impl->SendClientCommand(std::move(iCall), iClient);
    }

    void Server::Flush()
    {
      m_Dispatcher.Flush(*this);
      m_Impl->Flush();
    }

    void Server::CreateObject(ClientId iClient, ObjectId iObject, ClientData const& iData)
    {
      m_Impl->CreateObject(iClient, iObject, iData);
    }

    void Server::UpdateObject(ClientId iClient, ObjectId iObject, ClientData const& iData)
    {
      m_Impl->UpdateObject(iClient, iObject, iData);
    }

    void Server::DeleteObject(ClientId iClient, ObjectId iObject)
    {
      m_Impl->DeleteObject(iClient, iObject);
    }

    String Server::GetExternalClientId(ClientId iClient)
    {
      if (!IsValidClientId(iClient, m_Impl->m_ClientSlotGeneration))
      {
        return String();
      }
      uint64_t id = m_Impl->m_Server.GetClientId(GetClientIndexFromId(iClient));
      return Uint64ToHex(id);
    }

    const size_t Server::s_PrivateKeySize = yojimbo::KeyBytes;

    Server* Server::Start(NetCtx& iCtx, String const& iIPAddr, Vector<uint8_t> const& iPrivateKey)
    {
      eXl_ASSERT_REPAIR_RET(iCtx.m_Server == nullptr, nullptr);
      eXl_ASSERT_REPAIR_RET(iCtx.m_NetDriver != nullptr, nullptr);
      eXl_ASSERT_MSG_REPAIR_RET(iPrivateKey.size() == s_PrivateKeySize, eXl_FORMAT("Size is %i", int(iPrivateKey.size())), nullptr);

      yojimbo::Address serverAddress(iIPAddr.c_str());

      std::unique_ptr<Server_Impl> server = std::make_unique<Server_Impl>(serverAddress, iPrivateKey);

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