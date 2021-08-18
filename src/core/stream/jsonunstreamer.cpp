/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/stream/jsonunstreamer.hpp>

#include <core/log.hpp>

#define CHECK_FAIL_STATUS do { if(m_FailStatus) return Err::Error;} while(false)

namespace eXl
{
  JSONUnstreamer::JSONUnstreamer(IStream* iInStream)
    :m_InStream(iInStream)
    ,m_Root(nullptr)
  {
  }

  Err JSONUnstreamer::Begin()
  {
    m_FailStatus = false;
    Err err = Err::Failure;
    if(m_Root == nullptr)
    {
      ElementDesc* elem = GetNextElement();
      if(elem)
      {
        m_InStream->clear();
        m_Root = elem;
        m_Stack.push_back(BrowseStack(elem));
        err = Err::Success;
      }
      if(err)
      {
        err = Unstreamer::Begin();
      }
    }
    if (m_FailStatus)
    {
      return Err::Failure;
    }
    return err;
  }

  Err JSONUnstreamer::End()
  {
    if(m_Root)
    {
      m_Stack.clear();
      eXl_DELETE m_Root;
      m_Root = nullptr;
    }
    Unstreamer::End();
    if (m_FailStatus)
    {
      return Err::Failure;
    }
    return Err::Success;
  }

  Err JSONUnstreamer::PushKey(String const& iKey)
  {
    CHECK_FAIL_STATUS;
    Err err = Err::Failure;
    if(!m_Stack.empty() && m_Stack.back().elem->kind == StructKind)
    {
      ElemStruct* structure = reinterpret_cast<ElemStruct*>(m_Stack.back().elem);
      auto iter = structure->m_Fields.find(iKey);
      if(iter != structure->m_Fields.end())
      {
        m_Stack.push_back(BrowseStack(iter->second));
        err = Err::Success;
      }
    }
    else
    {
      LOG_ERROR<<"Not a valid element to look for a key"<<"\n";
    }
    return err;
  }

  Err JSONUnstreamer::ClearWhiteSpaces()
  {
    return m_InStream->ClearWhiteSpaces();
  }

  JSONUnstreamer::ElementDesc* JSONUnstreamer::GetNextElement()
  {
    if (m_FailStatus)
    {
      return nullptr;
    }
    Err err = ClearWhiteSpaces();
    StreamOffset posBegin = m_InStream->getPos();
    if(m_InStream->peek() == '[')
    {
      StreamOffset seqBegin = m_InStream->getPos();
      m_InStream->get();
      ClearWhiteSpaces();

      ElemSequence* sequence = eXl_NEW ElemSequence;
      sequence->elemBegin = seqBegin;
      sequence->kind = SequenceKind;
      ClearWhiteSpaces();

      while(m_InStream->peek() != ']')
      {
        if(m_InStream->good())
        {
          ElementDesc* curElem = GetNextElement();
          if (m_FailStatus)
          {
            return nullptr;
          }
          if(curElem != nullptr && m_InStream->good())
          {
            sequence->m_Elements.push_back(curElem);
            err = ClearWhiteSpaces();
            if(err)
            {
              bool gotComa = false;
              if(m_InStream->peek() == ',')
              {
                m_InStream->get();
                gotComa = true;
                err = ClearWhiteSpaces();
              }
              if(err && m_InStream->peek() == ']')
              {
                continue;
              }
              if(err && gotComa)
                continue;
            }
          }
        }
        m_FailStatus = true;
        eXl_ASSERT(false);
        eXl_DELETE sequence;
        return nullptr;
      }

      m_InStream->get();

      sequence->elemEnd = m_InStream->getPos();

      return sequence;
    }
    else if(m_InStream->peek() == '{')
    {
      StreamOffset seqBegin = m_InStream->getPos();
      m_InStream->get();
      ClearWhiteSpaces();
      
      ElemStruct* structure = eXl_NEW ElemStruct;
      structure->kind = StructKind;
      structure->elemBegin = seqBegin;

      while(m_InStream->peek() != '}')
      {
        err = ClearWhiteSpaces();
        if(err)
        {
          String curKey;
          while(m_InStream->good() && m_InStream->peek() != ':')
          {
            curKey.push_back(m_InStream->get());
          }
          if(!curKey.empty())
          {
            while(StringUtil::IsSpace(curKey.back()))
            {
              curKey.pop_back();
            }
            if(curKey.front() == '\"' && curKey.back() == '\"'
              && m_InStream->good())
            {
              curKey.pop_back();
              curKey = curKey.substr(1);
              //get :
              m_InStream->get();
              ElementDesc* curElem = GetNextElement();
              if (m_FailStatus)
              {
                return nullptr;
              }
              if(curElem)
              {
                if(curElem != nullptr && m_InStream->good())
                {
                  structure->m_Fields.emplace(std::move(curKey), curElem);
                  err = ClearWhiteSpaces();
                  if(err)
                  {
                    bool gotComa = false;
                    if(m_InStream->peek() == ',')
                    {
                      m_InStream->get();
                      gotComa = true;
                      err = ClearWhiteSpaces();
                    }
                    if(err && m_InStream->peek() == '}')
                    {
                      continue;
                    }
                    if(err && gotComa)
                      continue;
                  }
                }
              }
            }
          }
        }
        m_FailStatus = true;
        eXl_ASSERT(false);
        eXl_DELETE structure;
        return nullptr;
      }
      m_InStream->get();
      structure->elemEnd = m_InStream->getPos();
      return structure;
    }
    else
    {
      StreamOffset seqBegin = m_InStream->getPos();
      
      ElementDesc* value = new ElementDesc;
      value->kind = ValueKind;
      value->elemBegin = seqBegin;

      StreamOffset lastPosNoWS = m_InStream->getPos();
      
      Err err = GetValueEnd(lastPosNoWS);

      if(!err)
      {
        m_FailStatus = true;
        eXl_ASSERT(false);
        eXl_DELETE value;
        return nullptr;
      }
      value->elemEnd = lastPosNoWS;
      return value;
    }
    return nullptr;
  }

  namespace
  {
    bool IsElementValueEnd(Char iChar)
    {
      return iChar == ',' || iChar == '}' || iChar == ']';
    }
  }

  Err JSONUnstreamer::GetValueEnd(StreamOffset& oPos)
  {
    Char curChar = 0;
    StreamOffset lastPosNoWS = m_InStream->getPos();
    Err err = Err::Success;
    while(err
      && m_InStream->good() 
      && !IsElementValueEnd((curChar = m_InStream->peek())))
    {
      if(m_InStream->eof() && m_InStream->good())
      {
        //Reached EOF, valid if value is root
        if(m_Root == nullptr)
        {
          m_InStream->clear();
        }
      }

      m_InStream->get();
      if(curChar == '\"')
      {
        err = EatString();
        if(err)
        {
          lastPosNoWS = m_InStream->getPos();
          err = ClearWhiteSpaces();
        }
      }
      else 
      {
        if(!StringUtil::IsSpace(curChar))
        {
          lastPosNoWS = m_InStream->getPos();
        }
      }
    }

    if(m_InStream->eof() && m_InStream->good())
    {
      //Reached EOF, valid if stack is empty
      if(m_Root == nullptr)
      {
        m_InStream->clear();
      }
      else
      {
        err = Err::Error;
      }
    }
    oPos = lastPosNoWS;
    return err;
  }

  Err JSONUnstreamer::EatString()
  {
    bool escapeChar = false;
    
    while(m_InStream->good())
    {
      Char curChar = m_InStream->get();
      if(curChar == '\\' && !escapeChar)
      {
        escapeChar = true;
      }
      else if(curChar == '\"' && !escapeChar)
      {
        break;
      }
      else
      {
        escapeChar = false;
      }
    }
    if(m_InStream->good())
    {
      RETURN_SUCCESS;
    }
    return Err::Error;
  }

  Err JSONUnstreamer::PopKey()
  {
    CHECK_FAIL_STATUS;
    Err err = Err::Error;
    if(!m_Stack.empty() )
    {
      m_Stack.pop_back();
      if(!m_Stack.empty() && m_Stack.back().elem->kind == StructKind)
      {
        err = Err::Success;
      }
    }
    return err;
  }

  Err JSONUnstreamer::BeginSequence()
  {
    CHECK_FAIL_STATUS;
    Err err = Err::Error;
    if(!m_Stack.empty() && m_Stack.back().elem->kind == SequenceKind)
    {
      BrowseStack& seqElem = m_Stack.back();
      if(seqElem.seqIdx == -1)
      {
        ElemSequence* sequence = reinterpret_cast<ElemSequence*>(m_Stack.back().elem);
        if(sequence->m_Elements.size() > 0)
        {
          m_Stack.back().seqIdx = 0;
          m_Stack.push_back(sequence->m_Elements[0]);
          err = Err::Success;
        }
        else
        {
          err = Err::Failure;
        }
      }
    }
    return err;
  }

  Err JSONUnstreamer::NextSequenceElement()
  {
    CHECK_FAIL_STATUS;
    Err err = Err::Error;
    if(m_Stack.size() >= 2)
    {
      BrowseStack& seqElem = *(m_Stack.rbegin() + 1);
      if(seqElem.elem->kind == SequenceKind)
      {
        m_Stack.pop_back();
        int& curIdx = seqElem.seqIdx;
        if(curIdx >= 0)
        {
          ElemSequence* sequence = reinterpret_cast<ElemSequence*>(seqElem.elem);
          if(curIdx == sequence->m_Elements.size() - 1)
          {
            curIdx = -1;
            err = Err::Failure;
          }
          else
          {
            m_Stack.push_back(BrowseStack(sequence->m_Elements[++curIdx]));
            err = Err::Success;
          }
        }
      }
    }
    return err;
  }

  //Err JSONUnstreamer::EndSequence()
  //{
  //  return NextSequence();
  //}

  Err JSONUnstreamer::BeginStruct()
  {
    CHECK_FAIL_STATUS;
    if(!m_Stack.empty() && m_Stack.back().elem->kind == StructKind)
      RETURN_SUCCESS;
    return Err::Error;
  }

  Err JSONUnstreamer::EndStruct()
  {
    CHECK_FAIL_STATUS;
    if(!m_Stack.empty() && m_Stack.back().elem->kind == StructKind)
      RETURN_SUCCESS;
    return Err::Error;
  }

  Err JSONUnstreamer::ReadInt(int * oInt)
  {
    CHECK_FAIL_STATUS;
    if(!m_Stack.empty() && m_Stack.back().elem->kind == ValueKind)
    {
      ElementDesc* elem = m_Stack.back().elem;
      m_InStream->setPos(elem->elemBegin);

      return m_InStream->ExtractInt(*oInt);
    }
    return Err::Error;
  }

  Err JSONUnstreamer::ReadUInt(unsigned int * oUInt)
  {
    CHECK_FAIL_STATUS;
    if(!m_Stack.empty() && m_Stack.back().elem->kind == ValueKind)
    {
      ElementDesc* elem = m_Stack.back().elem;
      m_InStream->setPos(elem->elemBegin);
      return m_InStream->ExtractUInt(*oUInt);
    }
    return Err::Error;
  }

  Err JSONUnstreamer::ReadUInt64(uint64_t * oUInt)
  {
    CHECK_FAIL_STATUS;
    if (!m_Stack.empty() && m_Stack.back().elem->kind == ValueKind)
    {
      ElementDesc* elem = m_Stack.back().elem;
      m_InStream->setPos(elem->elemBegin);
      return m_InStream->ExtractUInt64(*oUInt);
    }
    return Err::Error;
  }

  Err JSONUnstreamer::ReadFloat(float * oFloat)
  {
    CHECK_FAIL_STATUS;
    if(!m_Stack.empty() && m_Stack.back().elem->kind == ValueKind)
    {
      ElementDesc* elem = m_Stack.back().elem;
      m_InStream->setPos(elem->elemBegin);
      return m_InStream->ExtractFloat(*oFloat);
    }
    return Err::Error;
  }

  Err JSONUnstreamer::ReadDouble(double * oDouble)
  {
    CHECK_FAIL_STATUS;
    if(!m_Stack.empty() && m_Stack.back().elem->kind == ValueKind)
    {
      ElementDesc* elem = m_Stack.back().elem;
      m_InStream->setPos(elem->elemBegin);
      return m_InStream->ExtractDouble(*oDouble);
    }
    return Err::Error;
  }

  Err JSONUnstreamer::ReadString(String* oStr)
  {
    static Char charEscaped[8] = {'\"','\\','/','b','f','n','r','t'};
    static Char charToAppend[8] = {'\"','\\','/','\b','\f','\n','\r','\t'};

    Err err = Err::Success;
    if(!m_Stack.empty() && m_Stack.back().elem->kind == ValueKind)
    {
      ElementDesc* elem = m_Stack.back().elem;
      m_InStream->setPos(elem->elemBegin);
      if(m_InStream->get() == '\"')
      {
        m_Cache.clear();
        bool escapeChar = false;
        while(m_InStream->good() && err)
        {
          Char curChar = m_InStream->get();
          if(curChar == '\\' && !escapeChar)
          {
            escapeChar = true;
          }
          else if(curChar == '\"' && !escapeChar)
          {
            break;
          }
          else if(escapeChar)
          {
            unsigned int i;
            for(i = 0; i< 8; ++i)
            {
              if(charEscaped[i] == curChar)
              {
                m_Cache.push_back(charToAppend[i]);
                escapeChar = false;
                break;
              }
            }
            if (i == 8)
            {
              m_FailStatus = true;
              err = Err::Error;
            }
          }
          else
          {
            m_Cache.push_back(curChar);
          }
        }
        if(err)
        {
          new(oStr) String(m_Cache.begin(), m_Cache.end());
          //err = NextSequence();
          err = Err::Success;
        }
      }
    }
    return err;
  }
}