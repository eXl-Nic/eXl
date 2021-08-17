/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#ifndef CORELIBEXP_INCLUDED
#include <core/corelibexp.hpp>
#endif
#ifndef VLOG_INCLUDED
#include <core/vlog.hpp>
#endif

#if defined(__ANDROID__) && 0

#include <android/log.h>
#include <sstream>
namespace eXl{
  class AndroidLogging :public std::stringstream{
  public:
    AndroidLogging(android_LogPriority reason);
    ~AndroidLogging();
    AndroidLogging& putStr (const char* str );
  private:
    android_LogPriority m_Reas;
  };
}

inline eXl::AndroidLogging& operator<< (eXl::AndroidLogging& iLog,const char* str ){return iLog.putStr(str);}

#define INIT_LOG(x)
#define LOAD_LOG
#define CHECK_LOG_INIT

#define NEW_ENTRY eXl::AndroidLogging(ANDROID_LOG_INFO)<<__FILE__<<" : "<<__LINE__<<" : "
#define LOG eXl::AndroidLogging(ANDROID_LOG_INFO)
#define LOG_INFO eXl::AndroidLogging(ANDROID_LOG_INFO)<<__FILE__<<" : "<<__LINE__<<" : "
#define LOG_WARNING eXl::AndroidLogging(ANDROID_LOG_WARN)<<__FILE__<<" : "<<__LINE__<<" : "
#define LOG_ERROR eXl::AndroidLogging(ANDROID_LOG_ERROR)<<__FILE__<<" : "<<__LINE__<<" : "
#define RAW_LOG_WARNING eXl::AndroidLogging(ANDROID_LOG_WARN)
#define RAW_LOG_ERROR eXl::AndroidLogging(ANDROID_LOG_ERROR)

#else

//typedef eXl::Log_Manager Log;
namespace eXl
{
  void EXL_CORE_API InitConsoleLog();
  void EXL_CORE_API InitFileLog(const AString& iFile);
}

#define LOAD_LOG
#define CHECK_LOG_INIT

#define NEW_ENTRY eXl::Log_Manager::Log(eXl::INFO_STREAM)<<__FILE__<<" : "<<__LINE__<<" : "
#define LOG eXl::Log_Manager::Log(eXl::INFO_STREAM)
#define LOG_INFO eXl::Log_Manager::Log(eXl::INFO_STREAM)<<__FUNCTION__<<" : "<<__LINE__<<" : "
#define LOG_WARNING eXl::Log_Manager::Log(eXl::WARNING_STREAM)<<__FUNCTION__<<" : "<<__LINE__<<" : "
#define LOG_ERROR eXl::Log_Manager::Log(eXl::ERROR_STREAM)<<__FUNCTION__<<" : "<<__LINE__<<" : "
#define RAW_LOG_INFO eXl::Log_Manager::Log(eXl::INFO_STREAM)
#define RAW_LOG_WARNING eXl::Log_Manager::Log(eXl::WARNING_STREAM)
#define RAW_LOG_ERROR eXl::Log_Manager::Log(eXl::ERROR_STREAM)

#endif
