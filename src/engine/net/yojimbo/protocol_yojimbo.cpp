
#include "network_yojimbo.hpp"
#include "streamer_yojimbo.hpp"

namespace yojimbo
{
  const uint8_t DEFAULT_PRIVATE_KEY[KeyBytes] = { 0 };
  const uint64_t s_ProtocolId = 0xE816CB0010000001;

  using namespace eXl::Network;

  bool CommandMessage::SerializeInternal(ReadStream& stream)
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
    m_Args.SetType(&(*cmd.m_Args), cmd.m_Args->Alloc(), true);

    Yo_Unstreamer unstreamer(stream);
    if (!unstreamer.Begin())
    {
      return false;
    }
    cmd.m_Args->Unstream_Uninit(m_Args.GetBuffer(), &unstreamer);
    if (!unstreamer.m_Good)
    {
      return false;
    }
    unstreamer.End();
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
    cmd.m_Args->Stream(m_Args.GetBuffer(), &streamer);
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
    cmd.m_Args->Stream(m_Args.GetBuffer(), &streamer);
    if (!streamer.m_Good)
    {
      return false;
    }
    streamer.End();

    return true;
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

    Optional<uint64_t> HexToUint64(KString iStr)
    {
      uint64_t outId = 0;
      uint32_t counter = 0;
      for (auto digit : iStr)
      {
        outId <<= 4;
        if (counter >= 16)
        {
          return {};
        }
        if (digit >= '0' && digit <= '9')
        {
          outId += (digit - '0');
        }
        else if (digit >= 'a' && digit <= 'f')
        {
          outId += 10 + (digit - 'a');
        }
        else if (digit >= 'A' && digit <= 'F')
        {
          outId += 10 + (digit - 'A');
        }
        else
        {
          return {};
        }
        ++counter;
      }
      return outId;
    }

    String Uint64ToHex(uint64_t iId)
    {
      if (iId == 0)
      {
        return "0";
      }
      String str;
      while (iId != 0)
      {
        char digit = iId % 16;
        if (digit >= 0 && digit <= 9)
        {
          str += ('0' + digit);
        }
        else
        {
          str += ('A' + digit);
        }
        iId >>= 4;
      }
      return str;
    }
  }
}