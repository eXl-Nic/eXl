/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/stream/unstreamer.hpp>
#include <core/heapobject.hpp>

#include <core/stream/textreader.hpp>

#include <vector>

namespace eXl
{

  class EXL_CORE_API JSONUnstreamer : public Unstreamer
  {
  public:

    typedef size_t StreamOffset;
    typedef TextReader IStream;

    JSONUnstreamer(IStream* iInStream);

    Err Begin();
    Err End();

    Err PushKey(KString iKey);
    Err PopKey();

    Err BeginStruct();
    Err EndStruct();

    Err BeginSequence();
    Err NextSequenceElement();

    Err ReadBool(bool* oBoolean);
    Err ReadInt(int * oInt);
    Err ReadUInt(unsigned int * oUInt);
    Err ReadUInt64(uint64_t * oUInt);
    Err ReadFloat(float * oFloat);
    Err ReadDouble(double * oFloat);
    Err ReadString(String* oStr);
    Err ReadBinary(Vector<uint8_t>* oData);

  protected:

    enum ElementKind
    {
      ValueKind    = 1,
      StructKind   = 2,
      SequenceKind = 3
    };

    Err ClearWhiteSpaces();

    Err EatString();

    Err GetValueEnd(StreamOffset& oPos);

    //Err NextSequence();

    struct ElementDesc : public HeapObject
    {
      virtual ~ElementDesc(){}

      ElementKind  kind;
      StreamOffset elemBegin;
      StreamOffset elemEnd;
    };

    struct ElemStruct : ElementDesc
    {
      ~ElemStruct()
      {
        for(auto elem : m_Fields)
        {
          eXl_DELETE elem.second;
        }
      }
      UnorderedMap<KString, ElementDesc*> m_Fields;
    };

    struct ElemSequence : ElementDesc
    {
      ~ElemSequence()
      {
        for(auto elem : m_Elements)
        {
          eXl_DELETE elem;
        }
      }
      Vector<ElementDesc*> m_Elements;
    };

    JSONUnstreamer::ElementDesc* GetNextElement();

    struct BrowseStack
    {
      BrowseStack(ElementDesc* iElem) : elem(iElem), seqIdx(-1) {}
      ElementDesc* elem;
      int          seqIdx;
    };

    Vector<BrowseStack>      m_Stack;
    Vector<Char>             m_Cache;
    NameAllocHolder          m_KeysAlloc;
    ElementDesc*             m_Root;
    int                      m_CurrentSeqIdx;
    IStream*                 m_InStream;

    bool m_FailStatus = false;
  };
}