/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/corelib.hpp>
#include <core/vlog.hpp>

#include <core/plugin.hpp>

#ifdef EXL_TYPE_ENABLED
#include <core/type/typemanager.hpp>
#include <core/type/coretype.hpp>
#include <core/type/tagtype.hpp>
#include <core/type/typetraits.hpp>
#include <core/type/arraytype.hpp>
#endif

#ifdef EXL_LUA
#include <core/lua/luamanager.hpp>
#include <core/lua/luabindcore.hpp>
#endif

#include <core/stream/reader.hpp>
#include <core/stream/writer.hpp>

#if WIN32
#include <Windows.h>
#endif

#ifndef WIN32
//void* operator new(size_t iSize)
//{
//  return eXl::MemoryManager::Allocate(iSize);
//}
//
//void operator delete(void* iPtr) noexcept
//{
//  return eXl::MemoryManager::Free(iPtr, false);
//}
#endif

namespace eXl
{
  IMPLEMENT_RTTI(Reader);
  IMPLEMENT_RTTI(Writer);
  IMPLEMENT_RTTI(StdOutWriter);

  namespace
  {
#ifdef WIN32
    char exePath[MAX_PATH + 1] = { 0 };
#else
    char exePath[512] = { 0 };
#endif
    std::string_view exePathView;
  }


  IMPLEMENT_TYPE(uint64_t)
  IMPLEMENT_TYPE(uint32_t)
  IMPLEMENT_TYPE(int32_t)
  IMPLEMENT_TYPE(float)
  IMPLEMENT_TYPE(bool)
  IMPLEMENT_TYPE(uint8_t)
  IMPLEMENT_TYPE(uint16_t)

#ifndef __ANDROID__
  //IMPLEMENT_TYPE(WString)
#endif
  IMPLEMENT_TYPE(AString)
  IMPLEMENT_TYPE(KString)
  IMPLEMENT_TYPE_EX(Name, NameBase_T)

  IMPLEMENT_TAG_TYPE(ConstDynObject)
  IMPLEMENT_TAG_TYPE(DynObject)
  IMPLEMENT_TAG_TYPE(Rtti)
  IMPLEMENT_TAG_TYPE(Err)

  std::string_view GetAppPath()
  {
    if (exePath[0] == 0)
    {
#ifdef WIN32
      HMODULE exeModule = GetModuleHandleA(nullptr);
      uint32_t length = GetModuleFileNameA(exeModule, exePath, MAX_PATH);

      for (uint32_t i = 0; i < length; ++i)
      {
        if (exePath[i] == '\\')
        {
          exePath[i] = '/';
        }
      }
      exePathView = std::string_view(exePath, length);
#else
      ssize_t len = ::readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
      if (len != -1)
      {
        exePath[len] = '\0';
        char* posBack = strrchr(exePath, '/');
        exePathView = std::string_view(exePath, len);
        chdir(exePathView.begin());
      }
#endif
    }

    return exePathView;
  }

  void Name_Init();
  void Name_Destroy();

  namespace LuaManager
  {
    void Init();
    void Destroy();
  }

  static bool s_CorelibStarted = false;

  void StartCoreLib(IntrusivePtr<Log_Manager::LogOutput> iInitialisationLog)
  {
    if (s_CorelibStarted)
    {
      return;
    }
    s_CorelibStarted = true;

    Log_Manager::AddStream(INFO_STREAM, "Info", "Info : ");
    Log_Manager::AddStream(WARNING_STREAM, "Warning", "Warning : ");
    Log_Manager::AddStream(ERROR_STREAM, "Err", "Err : ");
    Log_Manager::AddStream(LUA_OUT_STREAM, "LuaOut", "Lua output : ");
    Log_Manager::AddStream(LUA_ERR_STREAM, "LuaErr", "Lua error : ");

    Log_Manager::EnableStream(INFO_STREAM);
    Log_Manager::EnableStream(WARNING_STREAM);
    Log_Manager::EnableStream(ERROR_STREAM);
    Log_Manager::EnableStream(LUA_OUT_STREAM);
    Log_Manager::EnableStream(LUA_ERR_STREAM);

    if (iInitialisationLog)
    {
      Log_Manager::AddOutput(iInitialisationLog.get(), -1);
    }

    LOG_INFO << "Start eXl initialization";

#ifdef EXL_TYPE_ENABLED

    LOG_INFO << "Name initialization";
    Name_Init();
    //TypeName::init();
    //TypeFieldName::init();
    //TypeEnumName::init();

    LOG_INFO << "Type initialization";
    TypeManager::RegisterCoreType<uint64_t>();
    TypeManager::RegisterCoreType<uint32_t>();
    TypeManager::RegisterCoreType<int32_t>();
    TypeManager::RegisterCoreType<float>();
    TypeManager::RegisterCoreType<bool>();
    TypeManager::RegisterCoreType<uint8_t>();
    TypeManager::RegisterCoreType<uint16_t>();
    TypeManager::RegisterCoreType<AString>();
    TypeManager::RegisterCoreType<Name>();
    TypeManager::RegisterCoreType<KString>();

#endif

#ifdef EXL_LUA
    LOG_INFO << "Lua initialization";
    LuaManager::Init();
    LuaManager::AddRegFun(&BindCore);
#endif

    LOG_INFO << "Plugins initialization";
    detail::_PLInit();

    LOG_INFO << "eXl initialization done";
    if (iInitialisationLog)
    {
      Log_Manager::RemoveOutput(iInitialisationLog.get());
    }
  }
  
  void StopCoreLib()
  {
    if (!s_CorelibStarted)
    {
      return;
    }
    s_CorelibStarted = false;

#ifdef EXL_LUA
    LuaManager::Destroy();
#endif
    //TypeManager::Destroy();
    detail::_PLClose();
#ifdef _DEBUG
    //MemoryManager::ReportLeaks();
#endif
    Name_Destroy();
    detail::_LogShutdown();
    //MemoryManager::SetFreeFn(&free);
    
  }
}
