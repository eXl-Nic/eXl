#include "streamer_yojimbo.hpp"

namespace eXl
{
  namespace Network
  {
    void ReadAlloc::PushElement(ElementKind iKind)
    {
      uint32_t storageNum;
      switch (iKind)
      {
      case ElementKind::Sequence:
        storageNum = seqs.size();
        break;
      case ElementKind::Struct:
        storageNum = structs.size();
        break;
      case ElementKind::Int32:
        storageNum = ints.size();
        break;
      case ElementKind::UInt32:
        storageNum = uints.size();
        break;
      case ElementKind::UInt64:
        storageNum = uints64.size();
        break;
      case ElementKind::String:
        storageNum = strings.size();
        break;
      case ElementKind::Float:
        storageNum = flts.size();
        break;
      case ElementKind::Double:
        storageNum = dbls.size();
        break;
      }
      elements.push_back(Element{ iKind, storageNum });
    }

    void ReadAlloc::Clear()
    {
      ints.clear();
      uints.clear();
      uints64.clear();
      flts.clear();
      dbls.clear();
      strings.clear();
      seqs.clear();
      seqElem.clear();
      structs.clear();
      structElem.clear();
      elements.clear();
    }

    void WriteAlloc::Clear()
    {
      m_CurrentStructLevel = 0;
      m_SequenceLevel.clear();
      m_PendingSeqElem.clear();
    }

    bool WriteAlloc::CurLevelInSequence()
    {
      return !m_SequenceLevel.empty() && m_SequenceLevel.back() == m_CurrentStructLevel;
    }

    Err WriteAlloc::ElementPrologue(yojimbo::WriteStream& iYoStream)
    {
      if (CurLevelInSequence() && m_PendingSeqElem.back())
      {
        if (!iYoStream.SerializeBits(Control::s_Sequence_Next, Control::s_ControlBitSize))
        {
          return Err::Error;
        }
      }
      RETURN_SUCCESS;
    }

    Err WriteAlloc::ElementPrologue(yojimbo::MeasureStream& iYoStream)
    {
      if (CurLevelInSequence() && m_PendingSeqElem.back())
      {
        if (!iYoStream.SerializeBits(Control::s_Sequence_Next, Control::s_ControlBitSize))
        {
          return Err::Error;
        }
      }
      RETURN_SUCCESS;
    }

    Err WriteAlloc::ElementEpilogue()
    {
      if (CurLevelInSequence())
      {
        m_PendingSeqElem.back() = true;
      }
      RETURN_SUCCESS;
    }

    Yo_Unstreamer::Yo_Unstreamer(yojimbo::ReadStream& iStream)
        : m_YoStream(iStream)
        , m_Ctx(*reinterpret_cast<SerializationContext*>(iStream.GetContext()))
        , m_KeySize(m_Ctx.m_CmdDictionary.m_CommandsHash.GetData().m_HashLen)
        , m_Alloc(m_Ctx.m_RAlloc)
      {

      }

    Err Yo_Unstreamer::Begin()
    {
      Unstreamer::Begin();
      m_Alloc.Clear();
      if (FetchNextElement() && !m_Alloc.elements.empty())
      {
        m_Alloc.m_Stack.push_back(&m_Alloc.elements.front());
        return Err::Success;
      }
      return Err::Error;
    }

    Err Yo_Unstreamer::FetchStructure(SmallVector<ReadAlloc::StructElem, 8>& ioElems)
    {
      do
      {
        uint32_t control = 15;
        m_Good = m_YoStream.SerializeBits(control, s_ControlBitSize);
        CHECK_STREAM_GOOD;
        if (control == s_StructureEnd)
        {
          return Err::Success;
        }
        if (control == s_Key)
        {
          uint32_t keyIdx = -1;
          m_Good = m_YoStream.SerializeBits(keyIdx, m_KeySize);
          CHECK_STREAM_GOOD;
          uint32_t nextElemIdx = m_Alloc.elements.size();
          if (!FetchNextElement())
          {
            return Err::Error;
          }
          ReadAlloc::StructElem newElem;
          newElem.elem = nextElemIdx;
          newElem.key = keyIdx;
          ioElems.push_back(newElem);
        }
        else
        {
          // Some core structs directly serialize themselves to streamers
          // without going through fields, leave them be and read them in order
          uint32_t nextElemIdx = m_Alloc.elements.size();
          if (!InterpretNextElement(control))
          {
            return Err::Error;
          }
          ReadAlloc::StructElem newElem;
          newElem.elem = nextElemIdx;
          newElem.key = -1;
          ioElems.push_back(newElem);
        }

      } while (true);
    }

    Err Yo_Unstreamer::FetchSequence(SmallVector<uint32_t, 8>& ioElems)
    {
      uint32_t nextElemIdx = m_Alloc.elements.size();
      if (!FetchNextElement())
      {
        return Err::Error;
      }
      ioElems.push_back(nextElemIdx);

      do
      {
        uint32_t control = 15;
        m_Good = m_YoStream.SerializeBits(control, s_ControlBitSize);
        CHECK_STREAM_GOOD;
        if (control == s_Sequence_End)
        {
          return Err::Success;
        }
        else if (control == s_Sequence_Next)
        {
          uint32_t nextElemIdx = m_Alloc.elements.size();
          if (!FetchNextElement())
          {
            return Err::Error;
          }
          ioElems.push_back(nextElemIdx);
        }

      } while (true);
    }

    Err Yo_Unstreamer::FetchNextElement()
    {
      CHECK_STREAM_GOOD;
      uint32_t control = 15;
      m_Good = m_YoStream.SerializeBits(control, s_ControlBitSize);
      CHECK_STREAM_GOOD;
      m_Good = control >= 3 && control <= 12;
      CHECK_STREAM_GOOD;
      return InterpretNextElement(control);
    }

    Err Yo_Unstreamer::InterpretNextElement(uint32_t control)
    {
      switch (control)
      {
      case s_String:
      {
        uint32_t stringLen;
        m_Good = m_YoStream.SerializeBits(stringLen, 8);
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBytes(m_Alloc.strReadBuffer, stringLen);
        CHECK_STREAM_GOOD;
        m_Alloc.PushElement(ElementKind::String);
        m_Alloc.strings.push_back(String((char*)m_Alloc.strReadBuffer, stringLen));
        return Err::Success;
      }
        break;
      case s_Double:
      {
        double dbl;
        m_Good = m_YoStream.SerializeBytes((uint8_t*)&dbl, sizeof(dbl));
        CHECK_STREAM_GOOD;
        m_Alloc.PushElement(ElementKind::Double);
        m_Alloc.dbls.push_back(dbl);
        return Err::Success;
      }
      break;
      case s_Float:
      {
        float flt;
        m_Good = m_YoStream.SerializeBytes((uint8_t*)&flt, sizeof(flt));
        CHECK_STREAM_GOOD;
        m_Alloc.PushElement(ElementKind::Float);
        m_Alloc.flts.push_back(flt);
        return Err::Success;
      }
      break;
      case s_UInt64:
      {
        uint64_t uint;
        m_Good = m_YoStream.SerializeBytes((uint8_t*)&uint, sizeof(uint));
        CHECK_STREAM_GOOD;
        m_Alloc.PushElement(ElementKind::UInt64);
        m_Alloc.uints64.push_back(uint);
        return Err::Success;
      }
      break;
      case s_UInteger:
      {
        uint32_t uint;
        m_Good = m_YoStream.SerializeBytes((uint8_t*)&uint, sizeof(uint));
        CHECK_STREAM_GOOD;
        m_Alloc.PushElement(ElementKind::UInt32);
        m_Alloc.uints.push_back(uint);
        return Err::Success;
      }
      break;
      case s_Integer:
      {
        int32_t integer;
        m_Good = m_YoStream.SerializeBytes((uint8_t*)&integer, sizeof(integer));
        CHECK_STREAM_GOOD;
        m_Alloc.PushElement(ElementKind::Int32);
        m_Alloc.ints.push_back(integer);
        return Err::Success;
      }
      case s_Structure:
      {
        uint32_t curStructIdx = m_Alloc.structs.size();
        m_Alloc.PushElement(ElementKind::Struct);
        m_Alloc.structs.push_back(ReadAlloc::Struct());
          
        SmallVector<ReadAlloc::StructElem, 8> keyElems;
        if (FetchStructure(keyElems))
        {
          ReadAlloc::Struct& curStruct = m_Alloc.structs[curStructIdx];
          curStruct.begin = m_Alloc.structElem.size();
          curStruct.end = curStruct.begin + keyElems.size();
          for (auto const& elem : keyElems)
          {
            m_Alloc.structElem.push_back(elem);
          }

          return Err::Success;
        }
        else
        {
          return Err::Error;
        }
      }
      case s_Sequence:
      {
        uint32_t curSeqIdx = m_Alloc.seqs.size();
        m_Alloc.PushElement(ElementKind::Sequence);
        m_Alloc.seqs.push_back(ReadAlloc::Seq());

        SmallVector<uint32_t, 8> elems;
        if (FetchSequence(elems))
        {
          ReadAlloc::Seq& curSeq = m_Alloc.seqs[curSeqIdx];
          curSeq.begin = m_Alloc.seqElem.size();
          curSeq.end = curSeq.begin + elems.size();
          for (auto const& elem : elems)
          {
            m_Alloc.seqElem.push_back(elem);
          }

          return Err::Success;
        }
        else
        {
          return Err::Error;
        }
      }
      case s_EmptySequence:
        m_Alloc.PushElement(ElementKind::Sequence);
        m_Alloc.seqs.push_back(ReadAlloc::Seq());
        break;
      }

      return Err::Failure;
    }

    Err Yo_Unstreamer::BeginSequence()
    {
      CHECK_STREAM_GOOD;
      if (m_Alloc.m_Stack.empty())
      {
        return Err::Failure;
      }
      auto& curStackElem = m_Alloc.m_Stack.back();
      auto const& curElem = *curStackElem.elem;
      if (curElem.kind != ElementKind::Sequence)
      {
        return Err::Error;
      }
      ReadAlloc::Seq const& curSeq = m_Alloc.seqs[curElem.idx];
      if (curSeq.begin >= curSeq.end)
      {
        return Err::Failure;
      }
      if (curStackElem.seqIdx == -1)
      {
        curStackElem.seqIdx = curSeq.begin;
        auto const* nextElem = &m_Alloc.elements[curSeq.begin];
        m_Alloc.m_Stack.emplace_back(nextElem);
        return Err::Success;
      }
      return Err::Failure;
    }

    Err Yo_Unstreamer::NextSequenceElement()
    {
      CHECK_STREAM_GOOD;
      if (m_Alloc.m_Stack.size() < 2
        || std::prev(std::prev(m_Alloc.m_Stack.end()))->elem->kind != ElementKind::Sequence)
      {
        return Err::Failure;
      }
      m_Alloc.m_Stack.pop_back();
      auto& curStackElem = m_Alloc.m_Stack.back();
      auto const& curElem = *curStackElem.elem;
      ReadAlloc::Seq const& curSeq = m_Alloc.seqs[curElem.idx];
      curStackElem.seqIdx++;

      if (curStackElem.seqIdx >= curSeq.end)
      {
        curStackElem.seqIdx = -1;
        return Err::Failure;
      }

      auto const* nextElem = &m_Alloc.elements[curStackElem.seqIdx];
      m_Alloc.m_Stack.emplace_back(nextElem);
      return Err::Success;
    }

    Err Yo_Unstreamer::BeginStruct()
    {
      CHECK_STREAM_GOOD;
      if (m_Alloc.m_Stack.empty())
      {
        return Err::Failure;
      }
      auto& curStackElem = m_Alloc.m_Stack.back();
      auto const& curElem = *curStackElem.elem;
      if (curElem.kind != ElementKind::Struct)
      {
        return Err::Error;
      }
      return Err::Success;
    }

    Err Yo_Unstreamer::EndStruct()
    {
      CHECK_STREAM_GOOD;
      if (m_Alloc.m_Stack.empty())
      {
        return Err::Failure;
      }
      auto& curStackElem = m_Alloc.m_Stack.back();
      auto const& curElem = *curStackElem.elem;
      if (curElem.kind != ElementKind::Struct)
      {
        return Err::Error;
      }
      return Err::Success;
    }

    Err Yo_Unstreamer::PushKey(String const& iKey)
    {
      CHECK_STREAM_GOOD;
      if (m_Alloc.m_Stack.empty())
      {
        return Err::Failure;
      }
      auto& curStackElem = m_Alloc.m_Stack.back();
      auto const& curElem = *curStackElem.elem;
      if (curElem.kind != ElementKind::Struct)
      {
        return Err::Error;
      }
      uint32_t selectedElem = -1;
      uint32_t keyIdx = m_Ctx.m_CmdDictionary.m_CommandsHash.Compute(iKey);
      auto const& curStruct = m_Alloc.structs[curElem.idx];
      if (curStackElem.seqIdx != -1 
        && curStackElem.seqIdx < curStruct.end - 1)
      {
        auto const& keyElem = m_Alloc.structElem[curStackElem.seqIdx + 1];
        if (keyElem.key == keyIdx)
        {
          ++curStackElem.seqIdx;
          selectedElem = keyElem.elem;
        }
      }
      for (int32_t key = curStruct.begin ; key < curStruct.end; ++key)
      {
        auto const& keyElem = m_Alloc.structElem[key];
        if (keyElem.key == keyIdx)
        {
          curStackElem.seqIdx = key;
          selectedElem = keyElem.elem;
          break;
        }
      }

      if (selectedElem == -1)
      {
        // Key not found. Assume custom serialization in order
        if (curStackElem.seqIdx == -1)
        {
          curStackElem.seqIdx = curStruct.begin;
          auto const& keyElem = m_Alloc.structElem[curStackElem.seqIdx];
          selectedElem = keyElem.elem;
        }
        else if (curStackElem.seqIdx < curStruct.end - 1)
        {
          curStackElem.seqIdx++;
          auto const& keyElem = m_Alloc.structElem[curStackElem.seqIdx];
          selectedElem = keyElem.elem;
        }
        else
        {
          curStackElem.seqIdx = -1;
          return Err::Failure;
        }
      }

      auto const* nextElem = &m_Alloc.elements[selectedElem];
      m_Alloc.m_Stack.emplace_back(nextElem);
      return Err::Success;
    }

    Err Yo_Unstreamer::PopKey()
    {
      CHECK_STREAM_GOOD;
      if (m_Alloc.m_Stack.size() < 2
        || std::prev(std::prev(m_Alloc.m_Stack.end()))->elem->kind != ElementKind::Struct)
      {
        return Err::Failure;
      }

      m_Alloc.m_Stack.pop_back();
      return Err::Success;
    }

    Err Yo_Unstreamer::ReadInt(int* iInt)
    {
      CHECK_STREAM_GOOD;
      if (!m_Alloc.m_Stack.empty() 
        && m_Alloc.m_Stack.back().elem->kind == ElementKind::Int32)
      {
        *iInt = m_Alloc.ints[m_Alloc.m_Stack.back().elem->idx];
        return Err::Success;
      }
      return Err::Failure;
    }

    Err Yo_Unstreamer::ReadUInt(unsigned int* iUInt)
    {
      CHECK_STREAM_GOOD;
      if (!m_Alloc.m_Stack.empty()
        && m_Alloc.m_Stack.back().elem->kind == ElementKind::UInt32)
      {
        *iUInt = m_Alloc.uints[m_Alloc.m_Stack.back().elem->idx];
        return Err::Success;
      }
      return Err::Failure;
    }

    Err Yo_Unstreamer::ReadUInt64(uint64_t* iUInt)
    {
      CHECK_STREAM_GOOD;
      if (!m_Alloc.m_Stack.empty()
        && m_Alloc.m_Stack.back().elem->kind == ElementKind::UInt64)
      {
        *iUInt = m_Alloc.uints64[m_Alloc.m_Stack.back().elem->idx];
        return Err::Success;
      }
      return Err::Failure;
    }

    Err Yo_Unstreamer::ReadFloat(float* iFloat)
    {
      CHECK_STREAM_GOOD;
      if (!m_Alloc.m_Stack.empty()
        && m_Alloc.m_Stack.back().elem->kind == ElementKind::Float)
      {
        *iFloat = m_Alloc.flts[m_Alloc.m_Stack.back().elem->idx];
        return Err::Success;
      }
      return Err::Failure;
    }

    Err Yo_Unstreamer::ReadDouble(double* iDouble)
    {
      CHECK_STREAM_GOOD;
      if (!m_Alloc.m_Stack.empty()
        && m_Alloc.m_Stack.back().elem->kind == ElementKind::Double)
      {
        *iDouble = m_Alloc.dbls[m_Alloc.m_Stack.back().elem->idx];
        return Err::Success;
      }
      return Err::Failure;
    }

    Err Yo_Unstreamer::ReadString(String* iStr)
    {
      CHECK_STREAM_GOOD;
      if (!m_Alloc.m_Stack.empty()
        && m_Alloc.m_Stack.back().elem->kind == ElementKind::String)
      {
        *iStr = m_Alloc.strings[m_Alloc.m_Stack.back().elem->idx];
        return Err::Success;
      }
      return Err::Failure;
    }
  }
}