/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <iostream>
#include <fstream>

#include <sstream>
#include <cstdio>
#include <ctime>
#include <core/vlog.hpp>
#include <boost/container/static_vector.hpp>

#ifdef EXL_THREADAWARE
#include <boost/thread/thread.hpp>
#include <boost/thread/tss.hpp>
#endif

using namespace std;

namespace eXl
{
  struct LocStrStorage
  {
    Vector<char> m_Storage;
    bool m_InUse = false;
  };

#ifdef EXL_THREADAWARE
  boost::thread_specific_ptr<LocStrStorage> s_tlsLogStorage;
#else
  LocStrStorage s_Storage;
#endif

  struct Log_Manager::Impl
  {
    boost::container::static_vector<Log_Manager::LogStream, 32> m_Streams;
    boost::container::static_vector<Set<IntrusivePtr<Log_Manager::LogOutput> >, 32> m_Outputs;

    void Shutdown()
    {
      m_Streams.clear();
      m_Outputs.clear();
    }

    Err AddOutput(LogOutput* iOut, unsigned int iStreamFlags)
    {
      eXl_ASSERT_MSG_REPAIR_RET(iOut != nullptr, "nullptr pointer", Err::Failure)
      {
        eXl_ASSERT_MSG_REPAIR_RET(iStreamFlags != 0, "No streams", Err::Failure)
        {

        }
      }

      for (unsigned int i = 0; i < m_Streams.size(); ++i)
      {
        if (iStreamFlags & 1 << i && (m_Streams[i].streamNum < 32))
        {
          m_Outputs[i].insert(iOut);
        }
      }

      RETURN_SUCCESS;
    }

    Err RemoveOutput(LogOutput* iOut, unsigned int iStreamFlags = -1)
    {
      eXl_ASSERT_MSG_REPAIR_RET(iOut != nullptr, "nullptr pointer", Err::Failure)
      {
        eXl_ASSERT_MSG_REPAIR_RET(iStreamFlags != 0, "No streams", Err::Failure)
        {

        }
      }

      for (unsigned int i = 0; i < m_Streams.size(); ++i)
      {
        if (iStreamFlags & 1 << i && (m_Streams[i].streamNum < 32))
        {
          m_Outputs[i].erase(iOut);
        }
      }

      RETURN_SUCCESS;
    }

    Err GetStream(KString iName, unsigned int& oStreamNum)
    {
      for (unsigned int i = 0; i < m_Streams.size(); ++i)
      {
        if (m_Streams[i].GetName() == iName)
        {
          oStreamNum = i;
          RETURN_SUCCESS;
        }
      }
      RETURN_FAILURE;
    }

    Err AddStream(unsigned int iStream, KString iName, KString iPrefix)
    {
      if (iStream >= 32 || (iStream < m_Streams.size() && (m_Streams[iStream].streamNum < 32)))
        RETURN_FAILURE;

      for (unsigned int i = 0; i < m_Streams.size(); ++i)
      {
        if (m_Streams[i].GetName() == iName)
          RETURN_FAILURE;
      }

      for (unsigned int i = m_Streams.size(); i < iStream; ++i)
      {
        m_Streams.push_back(LogStream());
        m_Outputs.push_back(Set<IntrusivePtr<LogOutput> >());
      }

      if (iStream >= m_Streams.size())
      {
        m_Streams.push_back(LogStream(iStream, iName, iPrefix));
        m_Outputs.push_back(Set<IntrusivePtr<LogOutput> >());
      }
      else
      {
        m_Streams[iStream] = LogStream(iStream, iName, iPrefix);
      }
      RETURN_SUCCESS;
    }

    void EnableStream(unsigned int iStream)
    {
      if (iStream < m_Streams.size())
      {
        if (m_Streams[iStream].streamNum < 32)
        {
          m_Streams[iStream].enabled = true;
        }
      }
    }

    void DisableStream(unsigned int iStream)
    {
      if (iStream < m_Streams.size())
      {
        if (m_Streams[iStream].streamNum < 32)
        {
          m_Streams[iStream].enabled = false;
        }
      }
    }

    LogObject Log(unsigned int iStream)
    {

#ifdef EXL_THREADAWARE
      auto locStorage = s_tlsLogStorage.get();
      if (locStorage == nullptr)
      {
        locStorage = new LocStrStorage;
        s_tlsLogStorage.reset(locStorage);
      }
      eXl_ASSERT(locStorage->m_InUse == false);
      locStorage->m_InUse = true;
#endif
      static LogStream m_Default;
      if (iStream < m_Streams.size())
      {
#ifdef EXL_THREADAWARE
        return LogObject(m_Streams[iStream], locStorage->m_Storage);
#else
        return LogObject(m_Streams[iStream], s_Storage.m_Storage);
#endif
      }
#ifdef EXL_THREADAWARE
      return LogObject(m_Default, locStorage->m_Storage);
#else
      return LogObject(m_Default, s_Storage.m_Storage);
#endif
    }


    void ClearOutput()
    {
      for (unsigned int i = 0; i < m_Outputs.size(); i++)
      {
        m_Outputs[i].clear();
      }
    }

    void write(AString const& msg, unsigned int stream)
    {
      Set<IntrusivePtr<LogOutput> >::iterator iter = m_Outputs[stream].begin();
      Set<IntrusivePtr<LogOutput> >::iterator iterEnd = m_Outputs[stream].end();
      while (iter != iterEnd)
      {
        (*iter)->write(msg.c_str());
        iter++;
      }
    }

    void write(WString const& msg, unsigned int stream)
    {
      Set<IntrusivePtr<LogOutput> >::iterator iter = m_Outputs[stream].begin();
      Set<IntrusivePtr<LogOutput> >::iterator iterEnd = m_Outputs[stream].end();
      while (iter != iterEnd)
      {
        (*iter)->write(StringUtil::ToASCII(msg).c_str());
        iter++;
      }
    }

    void write(const char* tolog, unsigned int stream)
    {
      Set<IntrusivePtr<LogOutput> >::iterator iter = m_Outputs[stream].begin();
      Set<IntrusivePtr<LogOutput> >::iterator iterEnd = m_Outputs[stream].end();
      while (iter != iterEnd)
      {
        (*iter)->write(tolog);
        iter++;
      }
    }

    void write(const wchar_t* tolog, unsigned int stream)
    {
      Set<IntrusivePtr<LogOutput> >::iterator iter = m_Outputs[stream].begin();
      Set<IntrusivePtr<LogOutput> >::iterator iterEnd = m_Outputs[stream].end();
      while (iter != iterEnd)
      {
        WString temp(tolog);
        (*iter)->write(StringUtil::ToASCII(temp).c_str());
        iter++;
      }
    }
  };
  namespace Log_Manager
  {
    namespace
    {
      Impl& GetImpl()
      {
        static Impl s_Impl;
        return s_Impl;
      }
    }
  }

  namespace detail
  {
    void _LogShutdown()
    {
      Log_Manager::GetImpl().Shutdown();
    }
  }

  IMPLEMENT_RefC(Log_Manager::LogOutput)
 
  Log_Manager::LogOutput::LogOutput():m_RefCount(0)
  {}

  String Log_Manager::Maintenant() 
  {
    time_t rawtime;
    time(&rawtime);
#ifdef EXL_CHAR_TYPE_IS_CHAR
    return String(ctime(&rawtime));
#endif
#ifdef EXL_CHAR_TYPE_IS_WCHAR
    return String(_wctime(&rawtime));
#endif
  }

  Err Log_Manager::AddOutput(LogOutput* iOut,unsigned int iStreamFlags)
  {
    return GetImpl().AddOutput(iOut, iStreamFlags);
  }

  Err Log_Manager::RemoveOutput(LogOutput* iOut, unsigned int iStreamFlags)
  {
    return GetImpl().RemoveOutput(iOut, iStreamFlags);
  }

  void Log_Manager::ClearOutput()
  {
    GetImpl().ClearOutput();
  }

  void Log_Manager::write(AString const& msg,unsigned int stream)
  {
    GetImpl().write(msg, stream);
  }

  void Log_Manager::write(WString const& msg,unsigned int stream)
  {
    GetImpl().write(msg, stream);
  }

  void Log_Manager::write(const char* tolog,unsigned int stream)
  {
    GetImpl().write(tolog, stream);
  }

  void Log_Manager::write(const wchar_t* tolog,unsigned int stream)
  {
    GetImpl().write(tolog, stream);
  }

  Err Log_Manager::GetStream(KString iName, unsigned int& oStreamNum)
  {
    return GetImpl().GetStream(iName, oStreamNum);
  }

  
  Log_Manager::LogStream::LogStream()
    :enabled(false)
    ,streamNum(32)
  {

  }

  Log_Manager::LogStream::LogStream(unsigned int iStreamNum, KString iName, KString iPrefix)
    :m_Name(iName)
    ,m_Prefix(iPrefix)
    ,enabled(false)
    ,streamNum(iStreamNum)
  {

  }

  void Log_Manager::EnableStream(unsigned int iStream)
  {
    GetImpl().EnableStream(iStream);
  }

  void Log_Manager::DisableStream(unsigned int iStream)
  {
    GetImpl().DisableStream(iStream);
  }

  Err Log_Manager::AddStream(unsigned int iStream, KString iName, KString iPrefix)
  {
    return GetImpl().AddStream(iStream, iName, iPrefix);
  }

  Log_Manager::LogObject::~LogObject()
  {
    if (m_Out.back() != '\n')
    {
      m_Out.push_back('\n');
    }
    m_Out.push_back(0);
    m_Stream.Prefix().write(m_Out.data());
#ifdef EXL_THREADAWARE
    auto locStorage = s_tlsLogStorage.get();
    locStorage->m_InUse = false;
#endif
    m_Out.clear();
  }

  Log_Manager::LogObject Log_Manager::Log(unsigned int iStream)
  {
    return GetImpl().Log(iStream);
  }

  /*
    unsigned int Log_Manager::AddSection(const String& iStr)
    {
    sectionPrefix.push_back(iStr);
    section.push_back(String());
    return section.size()-1;
    }

    void Log_Manager::PushSection(unsigned int iSec,const String& iMsg)
    {
    section[iSec]=section[iSec]+iMsg;
    }

    void Log_Manager::SetSection(unsigned int iSec,const String& iMsg)
    {
    section[iSec]=iMsg;
    }

    void Log_Manager::CollectSection(unsigned int iSec,String& oMsg)
    {
    oMsg = sectionPrefix[iSec] + " : " + section[iSec];
    section[iSec]="";
    }
  
    void Log_Manager::ClearSection(unsigned int iSec)
    {
    section[iSec]="";
    }
  
    void Log_Manager::CollectAllSections(String& oMsg)
    {
    for(unsigned int i =0 ;i<section.size();i++)
    {
    oMsg.append(sectionPrefix[i]);
    oMsg.append(" : ");
    oMsg.append(section[i]);
    oMsg.append("\n");
    section[i]="";
    }
    }
  
    void Log_Manager::ClearAllSections()
    {
    for(unsigned int i =0 ;i<section.size();i++)
    {
    section[i]="";
    }
    }*/
}
