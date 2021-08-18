/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/log.hpp>
#include <iostream>
#include <fstream>

#ifdef __ANDROID__
#include <android/log.h>
#include <sstream>
#endif

namespace eXl
{

#ifdef __ANDROID__

  class AndroidLogOut : public Log_Manager::LogOutput
  {
  public:
    AndroidLogOut(android_LogPriority reason) :m_Reas(reason) {}
    void write(const char* tolog)
    {
      __android_log_write(m_Reas, "eXl Log", tolog);
    }

  private:
    android_LogPriority m_Reas;
  };

#endif

  class coutOut : public Log_Manager::LogOutput
  {
    void write(const char* tolog);
  };

  class cerrOut : public Log_Manager::LogOutput
  {
    void write(const char* tolog);
  };

  class fileOut : public Log_Manager::LogOutput
  {
  public:
    fileOut(const AString& iFileName);
    void write(const char*);
    ~fileOut();
  private:
    std::fstream Output;
  };

  void coutOut::write(const char* tolog)
  {
    std::cout<<tolog<<std::flush;
  }

  void cerrOut::write(const char* tolog)
  {
    std::cerr<<tolog;
  }
  
  fileOut::fileOut(const AString& out)
  {
    Output.open(out.c_str(),std::fstream::out);
  }
  
  void fileOut::write(const char* tolog)
  {
    Output<<tolog<<std::flush;
    Output.flush();
  }

  fileOut::~fileOut()
  {
    Output.close();
  }

  void InitConsoleLog()
  {
#ifdef __ANDROID__

    Log_Manager::LogOutput* pOut = eXl_NEW AndroidLogOut(ANDROID_LOG_INFO);
    Log_Manager::AddOutput(pOut, INFO_STREAM_FLAG | LUA_OUT_STREAM_FLAG);

    pOut = eXl_NEW AndroidLogOut(ANDROID_LOG_WARN);
    Log_Manager::AddOutput(pOut, WARNING_STREAM_FLAG);

    pOut = eXl_NEW AndroidLogOut(ANDROID_LOG_ERROR);
    Log_Manager::AddOutput(pOut, ERROR_STREAM_FLAG | LUA_ERR_STREAM_FLAG);

#else
    Log_Manager::LogOutput* pOut = eXl_NEW coutOut;
    Log_Manager::AddOutput(pOut, INFO_STREAM_FLAG | LUA_OUT_STREAM_FLAG);

    pOut = eXl_NEW cerrOut;
    Log_Manager::AddOutput(pOut, ERROR_STREAM_FLAG | WARNING_STREAM_FLAG | LUA_ERR_STREAM_FLAG);
#endif
  }

  void InitFileLog(const AString& iFile)
  {
    Log_Manager::LogOutput* fOut=eXl_NEW fileOut(iFile);
    Log_Manager::AddOutput(fOut,INFO_STREAM_FLAG|ERROR_STREAM_FLAG|WARNING_STREAM_FLAG|LUA_OUT_STREAM_FLAG|LUA_ERR_STREAM_FLAG);
  }
}
