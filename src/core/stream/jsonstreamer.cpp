/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/stream/jsonstreamer.hpp>
#include <core/log.hpp>

#define FAIL_STREAM do {m_StreamFailed = true; return Err::Error;} while(false)
#define CHECK_FAIL_STATUS do { if(m_StreamFailed) return Err::Error;} while(false)

namespace eXl
{
  namespace
  {
    template <unsigned int Size>
    inline void WriteStaticString(JSONStreamer::OStream* iStr,Char const (&string)[Size])
    {
      iStr->write(string,Size - 1);
    }
  }
  JSONStreamer::JSONStreamer(OStream* iOutStream)
    :m_OutStream(iOutStream)
    ,m_CurrentStructLevel(0)
    ,m_CurrentKeyLevel(0)
    ,m_PendingPopKey(false)
    ,m_PendingPushKey(false)
  {

  }

  Err JSONStreamer::Begin()
  {
    //Push a virtual 'Root' key.
    m_PendingPushKey = true;
    m_CurrentKeyLevel = 1;
    m_StreamFailed = false;
    return Streamer::Begin();
  }

  Err JSONStreamer::End()
  {
    Err err = Streamer::End();
    if(err)
    {
      m_PendingPopKey = false;
      if(m_CurrentKeyLevel != 1)
      {
        LOG_ERROR<<"Keys are still pushed"<<"\n";
        RETURN_FAILURE;
      }
      if(m_CurrentStructLevel != 0)
      {
        LOG_ERROR<<"Struct is still opened"<<"\n";
        RETURN_FAILURE;
      }
      if(!m_SequenceLevel.empty())
      {
        LOG_ERROR<<"Sequence is still opened"<<"\n";
        RETURN_FAILURE;
      }
      CHECK_FAIL_STATUS;
    }
    return err;
  }

  Err JSONStreamer::PushKey(String const& iKey)
  {
    CHECK_FAIL_STATUS;

    if(m_PendingPushKey)
    {
      LOG_ERROR<<"Cannot push several keys in a row"<<"\n";
      FAIL_STREAM;
    }

    if(m_PendingPopKey)
    {
      WriteStaticString(m_OutStream,", \n");
      m_PendingPopKey = false;
    }

    m_CurrentKeyLevel++;

    if(m_CurrentKeyLevel > (m_CurrentStructLevel + 1))
    {
      LOG_ERROR<<"Cannot push keys without opening structs"<<"\n";
      FAIL_STREAM;
    }
    WriteIndent();
    WriteStaticString(m_OutStream,"\"");
    m_OutStream->write(iKey.c_str(), iKey.size());
    WriteStaticString(m_OutStream,"\"");
    WriteStaticString(m_OutStream," : ");
    m_PendingPushKey = true;
    
    RETURN_SUCCESS;
  }

  Err JSONStreamer::BeginStruct()
  {
    CHECK_FAIL_STATUS;
    Err err = Err::Success;
    err = ElementPrologue();
    if(err)
    {
      WriteStaticString(m_OutStream,"{\n");
      m_CurrentStructLevel ++;
    }
    return err;
  }

  Err JSONStreamer::EndStruct()
  { 
    CHECK_FAIL_STATUS;
    eXl_ASSERT(m_CurrentStructLevel > 0);
    m_CurrentStructLevel--;
    WriteStaticString(m_OutStream,"\n");
    WriteIndent();
    WriteStaticString(m_OutStream,"}");
    m_PendingPopKey = false;
    return ElementEpilogue();
  }

  Err JSONStreamer::PopKey()
  {
    CHECK_FAIL_STATUS;
    eXl_ASSERT(m_CurrentKeyLevel > 0);
    m_CurrentKeyLevel--;
    if(m_CurrentStructLevel > m_CurrentKeyLevel)
    {
      LOG_ERROR<<"Cannot pop keys without ending structs"<<"\n";
      FAIL_STREAM;
    }
    m_PendingPopKey = true;
    
    RETURN_SUCCESS;
  }

  Err JSONStreamer::BeginSequence(/*unsigned int iSize*/)
  {
    CHECK_FAIL_STATUS;
    Err err = ElementPrologue();
    if(err)
    {
      WriteStaticString(m_OutStream," [ ");
      m_SequenceLevel.push_back(m_CurrentStructLevel);
      m_PendingSeqElem.push_back(false);
    }
    return err;
  }

  Err JSONStreamer::EndSequence()
  {
    CHECK_FAIL_STATUS;
    if(m_SequenceLevel.empty())
    {
      LOG_ERROR<<"Sequence stack underflow"<<"\n";
      FAIL_STREAM;
    }

    WriteStaticString(m_OutStream," ] ");
    m_SequenceLevel.pop_back();
    m_PendingSeqElem.pop_back();
    
    return ElementEpilogue();
  }

  bool JSONStreamer::ConsumeKey()
  {
    if(!CurLevelInSequence())
    {
      if(!m_PendingPushKey)
        return false;
      m_PendingPushKey = false;
    }
    return true;
  }

  bool JSONStreamer::CurLevelInSequence()
  {
    return !m_SequenceLevel.empty() && m_SequenceLevel.back() == m_CurrentStructLevel;
  }

  Err JSONStreamer::ElementPrologue()
  {
    if(!ConsumeKey())
      RETURN_FAILURE;
    if(CurLevelInSequence() && m_PendingSeqElem.back())
    {
      WriteStaticString(m_OutStream,", ");
    }
    RETURN_SUCCESS;
  }

  Err JSONStreamer::ElementEpilogue()
  {
    if(CurLevelInSequence())
    {
      m_PendingSeqElem.back() = true;
    }
    RETURN_SUCCESS;
  }

  void JSONStreamer::WriteIndent()
  {
    for(unsigned int i = 0; i<m_CurrentStructLevel; ++i)
    {
      WriteStaticString(m_OutStream,"  ");
    }
  }

  Err JSONStreamer::WriteInt(int const* iInt)
  {
    CHECK_FAIL_STATUS;
    Err err = ElementPrologue();
    if(err)
    {
      m_OutStream->operator<<(*iInt);
      err = ElementEpilogue();
    }
    return err;
  }

  Err JSONStreamer::WriteUInt(unsigned int const* iUInt)
  {
    CHECK_FAIL_STATUS;
    Err err = ElementPrologue();
    if(err)
    {
      m_OutStream->operator<<(*iUInt);
      err =ElementEpilogue();
    }
    return err;
  }

  Err JSONStreamer::WriteUInt64(uint64_t const* iUInt)
  {
    CHECK_FAIL_STATUS;
    Err err = ElementPrologue();
    if (err)
    {
      m_OutStream->operator<<(*iUInt);
      err = ElementEpilogue();
    }
    return err;
  }

  Err JSONStreamer::WriteFloat(float const* iFloat)
  {
    CHECK_FAIL_STATUS;
    Err err = ElementPrologue();
    if(err)
    {
      m_OutStream->operator<<(*iFloat);
      err =ElementEpilogue();
    }
    return err;
  }

  Err JSONStreamer::WriteDouble(double const* iDouble)
  {
    CHECK_FAIL_STATUS;
    Err err = ElementPrologue();
    if(err)
    {
      m_OutStream->operator<<(*iDouble);
      err =ElementEpilogue();
    }
    return err;
  }

  Err JSONStreamer::WriteString(KString const& iStr)
  {
    CHECK_FAIL_STATUS;
    Err err = ElementPrologue();
    if (err)
    {
      WriteStaticString(m_OutStream, "\"");

      for(auto iter = iStr.begin(); iter < iStr.end(); ++iter)
      {
        WriteChar(*iter);
      }
      WriteStaticString(m_OutStream, "\"");
      err = ElementEpilogue();
    }
    return err;
  }

  Err JSONStreamer::WriteString(Char const* iStr)
  {
    CHECK_FAIL_STATUS;
    Err err = ElementPrologue();
    if (err)
    {
      WriteStaticString(m_OutStream, "\"");
      //Need to escape "
      Char const* strIterator = iStr;
      while (*strIterator != '\0')
      {
        WriteChar(*strIterator);
        ++strIterator;
      }
      WriteStaticString(m_OutStream, "\"");
      err = ElementEpilogue();
    }
    return err;
  }

  void JSONStreamer::WriteChar(Char iChar)
  {
    static Char charToEscape[8] = {'\"','\\','/','\b','\f','\n','\r','\t'};
    static Char charToAppend[8] = {'\"','\\','/','b','f','n','r','t'};

    unsigned int i;
    for(i = 0; i<8; ++i)
    {
      if(charToEscape[i] == iChar)
      {
        WriteStaticString(m_OutStream,"\\");
        m_OutStream->write(charToAppend + i,1);
        break;
      }
    }
    if (i == 8)
    {
      m_OutStream->write(&iChar, 1);
    }
  }
}