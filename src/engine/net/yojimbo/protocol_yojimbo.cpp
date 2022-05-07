
#include "network_yojimbo.hpp"
#include "streamer_yojimbo.hpp"

#include <core/stream/jsonstreamer.hpp>

#include <cstdarg>

namespace yojimbo
{
  const uint8_t DEFAULT_PRIVATE_KEY[KeyBytes] = { 0 };
  const uint64_t s_ProtocolId = 0xE816CB0010000001;

  GameConnectionConfig::GameConnectionConfig()
  {
    numChannels = CHANNELS_COUNT;
    protocolId = s_ProtocolId;
    channel[GameChannel::RELIABLE].type = CHANNEL_TYPE_RELIABLE_ORDERED;
    channel[GameChannel::UNRELIABLE].type = CHANNEL_TYPE_UNRELIABLE_UNORDERED;
#ifdef _DEBUG
    timeout = -1;
#else
    timeout = 15;
#endif
  }

  thread_local char s_YoPrintBuffer[4096];

  int YojimboLog(const char* iFmt, ...)
  {
    va_list args;
    va_start(args, iFmt);
    int res = vsnprintf(s_YoPrintBuffer, eXl::ArrayLength(s_YoPrintBuffer), iFmt, args);
    va_end(args);

    if (res > 0)
    {
      eXl::Log_Manager::Log(eXl::CoreLog::ERROR_STREAM).write(s_YoPrintBuffer);
    }
    return res;
  }

  void YojimboAssert(const char* iTest, const char* iMsg, const char* file, int line)
  {
    eXl::AssertionError(iTest, iMsg, file, line, false);
  }

  GameAdapter::GameAdapter()
  {
    static bool s_StaticInit = false;
    if (!s_StaticInit)
    {
      yojimbo_set_printf_function(&YojimboLog);
      yojimbo_set_assert_function(&YojimboAssert);
      yojimbo_log_level(YOJIMBO_LOG_LEVEL_ERROR);
      s_StaticInit = true;
    }
  }

  using namespace eXl::Network;

  bool CommandMessage::SerializeInternal(ReadStream& stream)
  { 
#ifdef YO_BIN_DEBUG
    base64::base64_encodestate state;
    base64::base64_init_encodestate(&state);
    
    size_t const inputBufferSize = 128;
    size_t const encodeBufferSize = 256;
    char encodeBuffer[encodeBufferSize]; 
    
    eXl::Vector<char> tempStr;
    uint32_t numWrites = eXl::Mathi::Max(1, stream.m_reader.m_numBytes / inputBufferSize);
    for (uint32_t block = 0; block < numWrites; ++block)
    {
      uint32_t readSize = block == numWrites - 1 && stream.m_reader.m_numBytes % inputBufferSize != 0
        ? stream.m_reader.m_numBytes % inputBufferSize
        : inputBufferSize;
    
      size_t encodeSize = base64::base64_encode_block((char*)stream.m_reader.m_data + block * inputBufferSize, readSize, encodeBuffer, &state);
      tempStr.insert(tempStr.end(), encodeBuffer, encodeBuffer + encodeSize);
    }
    size_t encodeSize = base64::base64_encode_blockend(encodeBuffer, &state);
    
    LOG_INFO << "Received message binary" << eXl::KString(tempStr.data(), tempStr.size());
#endif

    if (!stream.SerializeBits(m_CommandId, 32))
      return false;
    if (!yojimbo::serialize_uint64_internal(stream, m_QueryId))
      return false;

    SerializationContext const& ctx = *reinterpret_cast<SerializationContext const*>(stream.GetContext());
    auto iter = ctx.m_CmdDictionary.m_Commands.find(m_CommandId);
    if (iter == ctx.m_CmdDictionary.m_Commands.end())
    {
      return false;
    }
    CommandDesc const& cmd = *iter->second;
    m_Args.SetType(&cmd.m_FunDesc.GetType(), cmd.m_FunDesc.GetType().Alloc(), true);

    Yo_Unstreamer unstreamer(stream);
    if (!unstreamer.Begin())
    {
      return false;
    }
    cmd.m_FunDesc.GetType().Unstream_Uninit(m_Args.GetBuffer(), &unstreamer);
    if (!unstreamer.m_Good)
    {
      return false;
    }
    unstreamer.End();
#ifdef YO_ARGS_DEBUG
    std::stringstream sstream;
    eXl::JSONStreamer dbgStreamer(&sstream);
    
    dbgStreamer.Begin();
    cmd.m_Args->Stream(m_Args.GetBuffer(), &dbgStreamer);
    dbgStreamer.End();
    LOG_INFO << "Received message " << cmd.m_Name.c_str() << " with payload " << sstream.str();
#endif
    return true;
  }

  bool CommandMessage::SerializeInternal(WriteStream& stream)
  { 
    if (!stream.SerializeBits(m_CommandId, 32))
      return false;
    if (!yojimbo::serialize_uint64_internal(stream, m_QueryId))
      return false;

    SerializationContext const& ctx = *reinterpret_cast<SerializationContext const*>(stream.GetContext());
    auto iter = ctx.m_CmdDictionary.m_Commands.find(m_CommandId);
    if (iter == ctx.m_CmdDictionary.m_Commands.end())
    {
      return false;
    }
    CommandDesc const& cmd = *iter->second;
    Yo_Streamer<WriteStream> streamer(stream);
    if (!streamer.Begin())
    {
      return false;
    }
    cmd.m_FunDesc.GetType().Stream(m_Args.GetBuffer(), &streamer);
    if (!streamer.m_Good)
    {
      return false;
    }
    streamer.End();
    return true;
  }

  bool CommandMessage::SerializeInternal(MeasureStream& stream)
  {
    if (!stream.SerializeBits(m_CommandId, 32))
      return false;
    if (!yojimbo::serialize_uint64_internal(stream, m_QueryId))
      return false;

    SerializationContext const& ctx = *reinterpret_cast<SerializationContext const*>(stream.GetContext());
    auto iter = ctx.m_CmdDictionary.m_Commands.find(m_CommandId);
    if (iter == ctx.m_CmdDictionary.m_Commands.end())
    {
      return false;
    }
    CommandDesc const& cmd = *iter->second;
    Yo_Streamer<MeasureStream> streamer(stream);
    if (!streamer.Begin())
    {
      return false;
    }
    cmd.m_FunDesc.GetType().Stream(m_Args.GetBuffer(), &streamer);
    if (!streamer.m_Good)
    {
      return false;
    }
    streamer.End();

    return true;
  }

  template <typename Stream>
  bool ManifestMessage::Serialize(Stream& stream)
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

  bool ManifestMessage::SerializeInternal(yojimbo::ReadStream& stream)
  {
    return Serialize(stream);
  }

  bool ManifestMessage::SerializeInternal(yojimbo::WriteStream& stream)
  {
    return Serialize(stream);
  }

  bool ManifestMessage::SerializeInternal(yojimbo::MeasureStream& stream)
  {
    return Serialize(stream);
  }
}

namespace eXl
{
  namespace Network
  {
    yojimbo::NetAlloc& GetNetAllocator()
    {
      static yojimbo::NetAlloc s_Alloc;
      return s_Alloc;
    }
  }
}