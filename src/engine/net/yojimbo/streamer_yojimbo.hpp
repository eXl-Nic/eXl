#pragma once

#include "network_yojimbo.hpp"

#include <yojimbo.h>

#include <core/stream/streamer.hpp>
#include <core/stream/unstreamer.hpp>

#define CHECK_STREAM_GOOD do { if(!m_Good) return Err::Error;} while(false)

namespace eXl
{
  namespace Network
  {
    class Yo_Unstreamer : public Unstreamer, Control
    {
    public:

      yojimbo::ReadStream& m_YoStream;
      SerializationContext& m_Ctx;
      uint32_t const m_KeySize;
      bool m_Good = true;
      ReadAlloc& m_Alloc;

      Yo_Unstreamer(yojimbo::ReadStream& iStream);

      Err Begin() override;
      Err FetchStructure(SmallVector<ReadAlloc::StructElem, 8>& ioElems);
      Err FetchSequence(SmallVector<uint32_t, 8>& ioElems);
      Err FetchNextElement();
      Err InterpretNextElement(uint32_t control);

      Err BeginSequence() override;
      Err NextSequenceElement() override;

      Err BeginStruct() override;
      Err EndStruct() override;
      Err PushKey(KString iKey) override;
      Err PopKey() override;

      Err ReadInt(int* iInt) override;
      Err ReadUInt(unsigned int* iUInt) override;
      Err ReadUInt64(uint64_t* iUInt) override;
      Err ReadFloat(float* iFloat) override;
      Err ReadDouble(double* iDouble) override;
      Err ReadString(String* iStr) override;
      Err ReadBool(bool* iBool) override;
      Err ReadBinary(Vector<uint8_t>* iStr) override;
    };

    template <typename T>
    class Yo_Streamer : public Streamer , Control
    {
    public:

      T& m_YoStream;
      SerializationContext& m_Ctx;
      uint32_t const m_KeySize;
      bool m_Good = true;

      WriteAlloc& m_Alloc;

      Yo_Streamer(T& iStream)
        : m_YoStream(iStream)
        , m_Ctx(*reinterpret_cast<SerializationContext*>(iStream.GetContext()))
        , m_KeySize(m_Ctx.m_CmdDictionary.m_CommandsHash.GetData().m_HashLen)
        , m_Alloc(m_Ctx.m_WAlloc)
      {

      }

      Err Begin() override
      {
        Streamer::Begin();
        m_Alloc.Clear();

        return Err::Success;
      }

      Err BeginSequence() override
      {
        CHECK_STREAM_GOOD;
        m_Good = !(!m_Alloc.ElementPrologue(m_YoStream));
        CHECK_STREAM_GOOD;
        m_Alloc.m_SequenceLevel.push_back(m_Alloc.m_CurrentStructLevel);
        m_Alloc.m_PendingSeqElem.push_back(false);
        return m_Good ? Err::Success : Err::Failure;
      }

      Err EndSequence() override
      {
        CHECK_STREAM_GOOD;
        if (!m_Alloc.m_PendingSeqElem.back())
        {
          m_Good = m_YoStream.SerializeBits(s_EmptySequence, s_ControlBitSize);
        }
        else
        {
          m_Good = m_YoStream.SerializeBits(s_Sequence_End, s_ControlBitSize);
        }
        m_Alloc.m_SequenceLevel.pop_back();
        m_Alloc.m_PendingSeqElem.pop_back();
        return m_Good ? m_Alloc.ElementEpilogue() : Err::Failure;
      }

      Err BeginStruct() override
      {
        CHECK_STREAM_GOOD;
        m_Alloc.m_CurrentStructLevel++;
        m_Good = m_YoStream.SerializeBits(s_Structure, s_ControlBitSize);

        return m_Good ? Err::Success : Err::Failure;
      }

      Err EndStruct() override
      {
        CHECK_STREAM_GOOD;

        m_Good = m_YoStream.SerializeBits(s_StructureEnd, s_ControlBitSize);
        m_Alloc.m_CurrentStructLevel--;
        return m_Good ? m_Alloc.ElementEpilogue() : Err::Failure;
      }

      Err PushKey(KString iKey) override
      {
        CHECK_STREAM_GOOD;

        uint32_t keyIdx = m_Ctx.m_CmdDictionary.m_CommandsHash.Compute(iKey);
        m_Good = m_YoStream.SerializeBits(s_Key, s_ControlBitSize);
        if (!m_Good)
        {
          return Err::Error;
        }
        m_Good = m_YoStream.SerializeBits(keyIdx, m_KeySize);

        return m_Good ? Err::Success : Err::Failure;
      }
      Err PopKey() override
      {
        CHECK_STREAM_GOOD;

        return m_Good ? Err::Success : Err::Failure;
      }

      Err WriteInt(int const* iInt) override
      {
        CHECK_STREAM_GOOD;
        m_Good = !(!m_Alloc.ElementPrologue(m_YoStream));
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(s_Integer, s_ControlBitSize);
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(*reinterpret_cast<uint32_t const*>(iInt), 32);

        return m_Good ? m_Alloc.ElementEpilogue() : Err::Failure;
      }

      Err WriteUInt(unsigned int const* iUInt) override
      {
        CHECK_STREAM_GOOD;
        m_Good = !(!m_Alloc.ElementPrologue(m_YoStream));
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(s_UInteger, s_ControlBitSize);
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(*iUInt, 32);

        return m_Good ? m_Alloc.ElementEpilogue() : Err::Failure;
      }

      Err WriteUInt64(uint64_t const* iUInt) override
      {
        CHECK_STREAM_GOOD;
        m_Good = !(!m_Alloc.ElementPrologue(m_YoStream));
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(s_UInt64, s_ControlBitSize);
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(static_cast<uint32_t>(*iUInt), 32);
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(static_cast<uint32_t>(*iUInt >> 32), 32);

        return m_Good ? m_Alloc.ElementEpilogue() : Err::Failure;
      }

      Err WriteFloat(float const* iFloat) override
      {
        CHECK_STREAM_GOOD;
        m_Good = !(!m_Alloc.ElementPrologue(m_YoStream));
        CHECK_STREAM_GOOD; 
        m_Good = m_YoStream.SerializeBits(s_Float, s_ControlBitSize);
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(*reinterpret_cast<uint32_t const*>(iFloat), 32);

        return m_Good ? m_Alloc.ElementEpilogue() : Err::Failure;
      }

      Err WriteDouble(double const* iDouble) override
      {
        CHECK_STREAM_GOOD;
        m_Good = !(!m_Alloc.ElementPrologue(m_YoStream));
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(s_Double, s_ControlBitSize);
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBytes((uint8_t*)iDouble, sizeof(double));

        return m_Good ? m_Alloc.ElementEpilogue() : Err::Failure;
      }

      Err WriteString(Char const* iStr) override
      {
        CHECK_STREAM_GOOD;
        size_t len = strlen(iStr);
        return WriteString(KString(iStr, len));
      }


      Err WriteString(KString const& iStr) override
      {
        CHECK_STREAM_GOOD;
        m_Good = !(!m_Alloc.ElementPrologue(m_YoStream));
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(s_String, s_ControlBitSize);
        CHECK_STREAM_GOOD;
        uint32_t strLen = iStr.size();
        m_Good = m_YoStream.SerializeBits(strLen, 8);
        if (!m_Good)
        {
          return Err::Error;
        }
        m_Good = m_YoStream.SerializeBytes((uint8_t*)iStr.data(), iStr.size());

        return m_Good ? m_Alloc.ElementEpilogue() : Err::Failure;
      }

      Err WriteBool(bool const* iBool) override
      {
        CHECK_STREAM_GOOD;
        m_Good = !(!m_Alloc.ElementPrologue(m_YoStream));
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(s_Boolean, s_ControlBitSize);
        CHECK_STREAM_GOOD;
        uint32_t boolVal = *iBool ? 1 : 0;
        m_Good = m_YoStream.SerializeBits(boolVal, 1);
        
        return m_Good ? m_Alloc.ElementEpilogue() : Err::Failure;
      }

      Err WriteBinary(uint8_t const* iData, size_t iSize) override
      {
        eXl_ASSERT_REPAIR_RET(iSize < (1 << 12) - 1, Err::Error);

        CHECK_STREAM_GOOD;
        m_Good = !(!m_Alloc.ElementPrologue(m_YoStream));
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(s_Binary, s_ControlBitSize);
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(iSize, 12);
        if (!m_Good)
        {
          return Err::Error;
        }
        m_Good = m_YoStream.SerializeBytes(iData, iSize);

        return m_Good ? m_Alloc.ElementEpilogue() : Err::Failure;
      }

      Err WriteBinary(std::istream* iStream, Optional<size_t> iLen) override
      {
        CHECK_STREAM_GOOD;

        char readBuff[256];
        Vector<char> data;

        data.reserve(iLen ? *iLen : sizeof(readBuff));

        iStream->read(readBuff, sizeof(readBuff));
        size_t readSize = iStream->gcount();
        while (readSize > 0)
        {
          data.insert(data.end(), readBuff, readBuff + readSize);

          iStream->read(readBuff, sizeof(readBuff));
          readSize = iStream->gcount();
        }

        return WriteBinary((uint8_t const*)data.data(), data.size());
      }
      
    };
  }
}