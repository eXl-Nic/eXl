#pragma once

#include <engine/net/network.hpp>
#include <core/log.hpp>
#include <core/clock.hpp>

#include <yojimbo.h>
#include <netcode.io/netcode.h>

#include "../network_internal.hpp"

namespace yojimbo
{
  extern const uint8_t DEFAULT_PRIVATE_KEY[KeyBytes];
  constexpr int MAX_PLAYERS = 64;
  extern const uint64_t s_ProtocolId;

  enum GameMessageType
  {
    SEND_MANIFEST,
    CLIENT_COMMAND,
    SERVER_COMMAND,
    CLIENT_REPLY,
    SERVER_REPLY,
    OBJECT_CREATE,
    OBJECT_DELETE,
    OBJECT_UPDATE,
    MESSAGES_COUNT
  };

  enum GameChannel
  {
    RELIABLE,
    UNRELIABLE,
    CHANNELS_COUNT
  };

  struct GameConnectionConfig : ClientServerConfig
  {
    GameConnectionConfig()
    {
      numChannels = CHANNELS_COUNT;
      protocolId = s_ProtocolId;
      channel[GameChannel::RELIABLE].type = CHANNEL_TYPE_RELIABLE_ORDERED;
      channel[GameChannel::UNRELIABLE].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
#ifdef _DEBUG
      timeout = -1;
#endif
    }
  };

  class ManifestMessage : public BlockMessage
  {
  public:
    uint32_t seeds[3];
    uint32_t arraySize;
    uint32_t hashLen;
    uint32_t hashMask;

    template <typename Stream>
    bool Serialize(Stream& stream)
    {
      if (Stream::IsWriting)
      {
        eXl::Network::SerializationContext const& ctx = *reinterpret_cast<eXl::Network::SerializationContext const*>(stream.GetContext());
        ctx.m_CmdDictionary.m_CommandsHash.GetSeeds(seeds);
        arraySize = ctx.m_CmdDictionary.m_CommandsHash.GetData().m_AssignmentTable.size();
        hashLen = ctx.m_CmdDictionary.m_CommandsHash.GetData().m_HashLen;
        hashMask = ctx.m_CmdDictionary.m_CommandsHash.GetData().m_Mask;
      }
      bool good = true;
      good &= stream.SerializeBits(seeds[0], 32);
      good &= stream.SerializeBits(seeds[1], 32);
      good &= stream.SerializeBits(seeds[2], 32);
      good &= stream.SerializeBits(arraySize, 32);
      good &= stream.SerializeBits(hashLen, 32);
      good &= stream.SerializeBits(hashMask, 32);
      if (Stream::IsReading)
      {
        eXl::Network::SerializationContext& ctx = *reinterpret_cast<eXl::Network::SerializationContext*>(stream.GetContext());
        ctx.m_CmdDictionary.m_CommandsHash.SetSeeds(seeds);
        ctx.m_CmdDictionary.m_CommandsHash.GetData().m_AssignmentTable.resize(arraySize);
        ctx.m_CmdDictionary.m_CommandsHash.GetData().m_RankTable.resize(arraySize);
        ctx.m_CmdDictionary.m_CommandsHash.GetData().m_HashLen = hashLen;
        ctx.m_CmdDictionary.m_CommandsHash.GetData().m_Mask = hashMask;
      }
      return good;
    }

    YOJIMBO_VIRTUAL_SERIALIZE_FUNCTIONS();
  };

  class CommandMessage : public yojimbo::Message
  {
  public:
    uint32_t m_CommandId;
    uint64_t m_QueryId = 0;
    eXl::DynObject m_Args;

    bool SerializeInternal(yojimbo::ReadStream& stream);
    bool SerializeInternal(yojimbo::WriteStream& stream);
    bool SerializeInternal(yojimbo::MeasureStream& stream);
  };

  class UpdateMessage : public yojimbo::Message
  {
  public:
    eXl::Network::ObjectId m_Object;
    eXl::Network::ClientData m_Data;

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
  YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::SEND_MANIFEST, ManifestMessage);
  YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::CLIENT_COMMAND, CommandMessage);
  YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::SERVER_COMMAND, CommandMessage);
  YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::CLIENT_REPLY, CommandMessage);
  YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::SERVER_REPLY, CommandMessage);
  YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::OBJECT_CREATE, UpdateMessage);
  YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::OBJECT_UPDATE, UpdateMessage);
  YOJIMBO_DECLARE_MESSAGE_TYPE(GameMessageType::OBJECT_DELETE, UpdateMessage);
  YOJIMBO_MESSAGE_FACTORY_FINISH();

  class NetAlloc : public Allocator
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

  class GameAdapter : public Adapter
  {
  public:

    GameAdapter();

    virtual Allocator* CreateAllocator(Allocator& allocator, void* memory, size_t bytes)
    {
      // Uses the TLSF allocator.
      return Adapter::CreateAllocator(allocator, memory, bytes);
    }

    MessageFactory* CreateMessageFactory(Allocator& allocator) override
    {
      return YOJIMBO_NEW(allocator, GameMessageFactory, allocator);
    }
  };
}

namespace eXl
{
  namespace Network
  {
    yojimbo::NetAlloc& GetNetAllocator();

    inline ClientId AllocateClientId(uint32_t iClientIndex, uint32_t(&iClientGen)[yojimbo::MAX_PLAYERS])
    {
      uint64_t curGen = ++iClientGen[iClientIndex];
      return ClientId{ curGen << 32 | iClientIndex };
    }

    inline ClientId GetClientId(uint32_t iClientIndex, uint32_t const(&iClientGen)[yojimbo::MAX_PLAYERS])
    {
      uint64_t curGen = iClientGen[iClientIndex];
      return ClientId{ curGen << 32 | iClientIndex };
    }

    inline void ReleaseClientId(uint32_t iClientIndex, uint32_t(&iClientGen)[yojimbo::MAX_PLAYERS])
    {
      iClientGen[iClientIndex]++;
    }

    inline uint32_t GetClientIndexFromId(ClientId iClientId)
    {
      return iClientId.id & 0xFFFFFFFF;
    }

    inline bool IsValidClientId(ClientId iClientId, uint32_t const(&iClientGen)[yojimbo::MAX_PLAYERS])
    {
      uint32_t clientGen = iClientId.id >> 32;
      return iClientGen[GetClientIndexFromId(iClientId)] == clientGen;
    }

    Optional<uint64_t> HexToUint64(KString iStr);
    String Uint64ToHex(uint64_t iId);

    struct Control
    {
      static uint32_t constexpr s_StructureEnd = 2;
      static uint32_t constexpr s_Sequence_Next = 1;
      static uint32_t constexpr s_Sequence_End = 0;


      static uint32_t constexpr s_EmptySequence = 12;
      static uint32_t constexpr s_Sequence = 11;
      static uint32_t constexpr s_Structure = 10;
      static uint32_t constexpr s_Key = 9;
      static uint32_t constexpr s_Integer = 8;
      static uint32_t constexpr s_UInteger = 7;
      static uint32_t constexpr s_UInt64 = 6;
      static uint32_t constexpr s_Float = 5;
      static uint32_t constexpr s_Double = 4;
      static uint32_t constexpr s_String = 3;

      static uint32_t constexpr s_ControlBitSize = 4;
    };

    enum class ElementKind
    {
      Sequence,
      Struct,
      Int32,
      UInt32,
      UInt64,
      String,
      Float,
      Double
    };

    struct ReadAlloc
    {
      struct Element
      {
        ElementKind kind;
        uint32_t idx;
      };

      struct Seq
      {
        int32_t begin = 0;
        int32_t end = 0;
      };

      struct Struct
      {
        int32_t begin = 0;
        int32_t end = 0;
      };

      struct StructElem
      {
        uint32_t key;
        uint32_t elem;
      };
      uint8_t strReadBuffer[1028];
      Vector<int32_t> ints;
      Vector<uint32_t> uints;
      Vector<uint64_t> uints64;
      Vector<float> flts;
      Vector<double> dbls;
      Vector<String> strings;
      Vector<Seq> seqs;
      Vector<uint32_t> seqElem;
      Vector<Struct> structs;
      Vector<StructElem> structElem;
      Vector<Element> elements;

      struct BrowseStack
      {
        BrowseStack(Element const* iElem) : elem(iElem), seqIdx(-1) {}
        Element const* elem;
        int      seqIdx;
      };

      Vector<BrowseStack> m_Stack;

      void PushElement(ElementKind iKind);
      void Clear();
    };

    struct WriteAlloc
    {
      Vector<unsigned int> m_SequenceLevel;
      Vector<bool>         m_PendingSeqElem;

      unsigned int m_CurrentStructLevel = 0;

      void Clear();
      bool CurLevelInSequence();
      Err ElementPrologue(yojimbo::WriteStream& iYoStream);
      Err ElementPrologue(yojimbo::MeasureStream& iYoStream);
      Err ElementEpilogue();
    };

    struct SerializationContext
    {
      CommandDictionary m_CmdDictionary;
      ReadAlloc m_RAlloc;
      WriteAlloc m_WAlloc;
    };

    class Client_Impl : public yojimbo::GameAdapter
    {
    public:
      Client_Impl(uint32_t iLocalIndex, uint64_t iClientId);
      Client_Impl(uint32_t iLocalIndex, yojimbo::Address iAddr, uint64_t iClientId);

      ~Client_Impl();

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

      eXl::Err SendServerCommand(eXl::Network::CommandCallData&& iCall);

      Client* m_eXlClient;
      uint32_t m_LocalIndex;
      uint64_t m_TimeStart;
      uint64_t m_ClientId;
      yojimbo::Client m_Client;

      SerializationContext m_SerializationCtx;
      CommandHandler m_Commands;
    };

    class Server_Impl : public yojimbo::GameAdapter
    {
    public:
      Server_Impl(yojimbo::Address iAddr);

      ~Server_Impl()
      {
        m_Server.Stop();
      }

      void OnServerClientConnected(int clientIndex) override;
      void OnServerClientDisconnected(int clientIndex) override;
      virtual void ServerSendLoopbackPacket(int clientIndex, const uint8_t* packetData, int packetBytes, uint64_t packetSequence);

      void Tick();
      void Flush();
      void ProcessMessage(uint32_t iClientIndex, yojimbo::Message const& iMessage);
      bool IsValidClientId(ClientId);

      void CreateObject(ClientId, ObjectId, ClientData const& iData);
      void UpdateObject(ClientId, ObjectId, ClientData const& iData);
      void DeleteObject(ClientId, ObjectId);

      Err SendClientCommand(CommandCallData&& iCall, ClientId iClient);

      uint32_t m_ClientSlotGeneration[yojimbo::MAX_PLAYERS];
      Server* m_eXlServer;
      uint64_t m_TimeStart;
      yojimbo::Server m_Server;

      SerializationContext m_SerializationCtx;
      Vector<CommandHandler> m_ClientCmdQueues;
    };
  }
}
  