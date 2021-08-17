/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <vector>
#include <set>
#include <core/corelibexp.hpp>
#include <core/coredef.hpp>
#include <core/refcobject.hpp>
#include <core/heapobject.hpp>

namespace eXl
{
  enum CoreLog
  {
    INFO_STREAM    = 0,
    WARNING_STREAM = 1,
    ERROR_STREAM   = 2,
    LUA_OUT_STREAM = 3,
    LUA_ERR_STREAM = 4,

    INFO_STREAM_FLAG    = 1<<INFO_STREAM,
    WARNING_STREAM_FLAG = 1<<WARNING_STREAM,
    ERROR_STREAM_FLAG   = 1<<ERROR_STREAM,
    LUA_OUT_STREAM_FLAG = 1<<LUA_OUT_STREAM,
    LUA_ERR_STREAM_FLAG = 1<<LUA_ERR_STREAM
  };

  namespace Log_Manager
  {
    struct Impl;

    class EXL_CORE_API LogOutput : public HeapObject
    {
      DECLARE_RefC;
    public:
      LogOutput();
      virtual ~LogOutput(){}
      virtual void write(const char* tolog)=0;
    };

    class LogStream
    {
    public:

      inline void write(WString const&  tolog)const;
      inline void write(AString const&  tolog)const;
      inline void write(const Char* tolog)const;
      inline LogStream const& Prefix() const;
      KString const& GetName() const{return m_Name;}
      KString const& GetPrefix() const{return m_Prefix;}
    private:
      friend struct Impl;
      LogStream();
      LogStream(unsigned int iStreamNum, KString iName, KString iPrefix);

      KString m_Name;
      KString m_Prefix;
      unsigned int streamNum;
      bool enabled;
    };

    class EXL_CORE_API LogObject
    {
    public:
      ~LogObject();
      LogObject(LogObject const&) = delete;
      LogObject& operator=(LogObject const&) = delete;
      LogObject& operator=(LogObject&& iOther) = delete;

      LogObject(LogObject&& iOther)
        : m_Stream(iOther.m_Stream)
        , m_Out(iOther.m_Out)
      {}
      
      //inline void write(WString const&  tolog)const 
      //{
      //  m_Out.insert(m_Out.end(), tolog.begin(), tolog.end());
      //}

      inline void write(AString const&  tolog)const 
      {
        m_Out.insert(m_Out.end(), tolog.begin(), tolog.end());
      }

      inline void write(KString const&  tolog)const
      {
        m_Out.insert(m_Out.end(), tolog.begin(), tolog.end());
      }

      inline void write(const Char* tolog)const 
      {
        m_Out.insert(m_Out.end(), tolog, tolog + strlen(tolog));
      }
      
    protected:
      friend struct Impl;
      LogObject(LogStream& iStream, Vector<Char>& iOutStr)
        : m_Stream(iStream)
        , m_Out(iOutStr)
      {}

      Vector<Char>& m_Out;

      LogStream& m_Stream;
    };

    String Maintenant();

    void ClearOutput();

    EXL_CORE_API Err AddOutput(LogOutput* iOut,unsigned int iStreamFlags);
    EXL_CORE_API Err RemoveOutput(LogOutput* iOut, unsigned int iStreamFlags = -1);

    EXL_CORE_API Err GetStream(KString iName, unsigned int& oStreamNum);

    EXL_CORE_API Err AddStream(unsigned int iStream, KString iName, KString iPrefix);

    EXL_CORE_API void EnableStream(unsigned int iStream);

    EXL_CORE_API void DisableStream(unsigned int iStream);

    EXL_CORE_API LogObject Log(unsigned int iStream);

    //inline const LogStream& LogError(){write("Err : ",2);return m_Streams[2];}
    //
    //inline const LogStream& LogInfo(){write("Info : ",0);return m_Streams[0];}
    //
    //inline const LogStream& LogWarning(){write("Warning : ",1);return m_Streams[1];}

    void write(AString const& tolog,unsigned int stream);
    void write(WString const& tolog,unsigned int stream);
    void write(const char* tolog,unsigned int stream);
    void write(const wchar_t* tolog,unsigned int stream);
  };

  namespace detail
  {
    void _LogShutdown();
  }
 
#include "vlog.inl"
}
