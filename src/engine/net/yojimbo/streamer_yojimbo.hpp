#pragma once

#include "../network_internal.hpp"

#include <yojimbo.h>

#include <core/stream/streamer.hpp>
#include <core/stream/unstreamer.hpp>

#define CHECK_STREAM_GOOD do { if(!m_Good) return Err::Error;} while(false)

namespace eXl
{
  namespace Network
  {
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

      void PushElement(ElementKind iKind)
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

      void Clear()
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
    };

    struct WriteAlloc
    {
      Vector<unsigned int> m_SequenceLevel;
      Vector<bool>         m_PendingSeqElem;

      unsigned int m_CurrentStructLevel = 0;

      void Clear()
      {
        m_CurrentStructLevel = 0;
        m_SequenceLevel.clear();
        m_PendingSeqElem.clear();
      }

      bool CurLevelInSequence()
      {
        return !m_SequenceLevel.empty() && m_SequenceLevel.back() == m_CurrentStructLevel;
      }

      Err ElementPrologue(yojimbo::WriteStream& iYoStream)
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

      Err ElementPrologue(yojimbo::MeasureStream& iYoStream)
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

      Err ElementEpilogue()
      {
        if (CurLevelInSequence())
        {
          m_PendingSeqElem.back() = true;
        }
        RETURN_SUCCESS;
      }
    };

    struct SerializationContext
    {
      CommandDictionary m_CmdDictionary;
      ReadAlloc m_RAlloc;
      WriteAlloc m_WAlloc;
    };

    class Yo_Unstreamer : public Unstreamer, Control
    {
    public:

      yojimbo::ReadStream& m_YoStream;
      SerializationContext& m_Ctx;
      uint32_t const m_KeySize;
      bool m_Good = true;
      ReadAlloc& m_Alloc;

      Yo_Unstreamer(yojimbo::ReadStream& iStream)
        : m_YoStream(iStream)
        , m_Ctx(*reinterpret_cast<SerializationContext*>(iStream.GetContext()))
        , m_KeySize(m_Ctx.m_CmdDictionary.m_CommandsHash.GetData().m_HashLen)
        , m_Alloc(m_Ctx.m_RAlloc)
      {

      }

      Err Begin() override
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

      Err FetchStructure(SmallVector<ReadAlloc::StructElem, 8>& ioElems)
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

      Err FetchSequence(SmallVector<uint32_t, 8>& ioElems)
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

      Err FetchNextElement()
      {
        CHECK_STREAM_GOOD;
        uint32_t control = 15;
        m_Good = m_YoStream.SerializeBits(control, s_ControlBitSize);
        CHECK_STREAM_GOOD;
        m_Good = control >= 3 && control <= 12;
        CHECK_STREAM_GOOD;
        return InterpretNextElement(control);
      }

      Err InterpretNextElement(uint32_t control)
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

      Err BeginSequence() override
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

      Err NextSequenceElement() override
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

      Err BeginStruct() override
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

      Err EndStruct() override
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

      Err PushKey(String const& iKey) override
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

      Err PopKey() override
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

      Err ReadInt(int* iInt) override
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

      Err ReadUInt(unsigned int* iUInt) override
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

      Err ReadUInt64(uint64_t* iUInt) override
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

      Err ReadFloat(float* iFloat) override
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

      Err ReadDouble(double* iDouble) override
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

      Err ReadString(String* iStr) override
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

      Err PushKey(String const& iKey) override
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
        m_Good = m_YoStream.SerializeBytes((uint8_t*)iInt, sizeof(int32_t));

        return m_Good ? m_Alloc.ElementEpilogue() : Err::Failure;
      }

      Err WriteUInt(unsigned int const* iUInt) override
      {
        CHECK_STREAM_GOOD;
        m_Good = !(!m_Alloc.ElementPrologue(m_YoStream));
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(s_UInteger, s_ControlBitSize);
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBytes((uint8_t*)iUInt, sizeof(uint32_t));

        return m_Good ? m_Alloc.ElementEpilogue() : Err::Failure;
      }

      Err WriteUInt64(uint64_t const* iUInt) override
      {
        CHECK_STREAM_GOOD;
        m_Good = !(!m_Alloc.ElementPrologue(m_YoStream));
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBits(s_UInt64, s_ControlBitSize);
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBytes((uint8_t*)iUInt, sizeof(uint64_t));

        return m_Good ? m_Alloc.ElementEpilogue() : Err::Failure;
      }

      Err WriteFloat(float const* iFloat) override
      {
        CHECK_STREAM_GOOD;
        m_Good = !(!m_Alloc.ElementPrologue(m_YoStream));
        CHECK_STREAM_GOOD; 
        m_Good = m_YoStream.SerializeBits(s_Float, s_ControlBitSize);
        CHECK_STREAM_GOOD;
        m_Good = m_YoStream.SerializeBytes((uint8_t*)iFloat, sizeof(float));

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
      
    };
  }
}