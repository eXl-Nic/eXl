/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/plugin.hpp>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include<windows.h>
#define LIB_HANDLE HMODULE

#ifdef EXL_CHAR_TYPE_IS_CHAR
#define OPEN_LIB(x) LoadLibraryA( (x+".dll").c_str() )
#endif

#ifdef EXL_CHAR_TYPE_IS_WCHAR
#define OPEN_LIB(x) LoadLibraryW( (x+".dll").c_str() )
#endif

#define GET_PROC(x,y) GetProcAddress(x,y)
#define CLOSE_LIB(x) FreeLibrary(x) 
#else
#include <dlfcn.h>
#define LIB_HANDLE void*
#define OPEN_LIB(x) dlopen((x+".so").c_str(),RTLD_NOW)
#define GET_PROC(x,y) dlsym(x,y)
#define CLOSE_LIB(x) dlclose(x)
#endif

#include <boost/container/flat_map.hpp>
#include <boost/container/static_vector.hpp>

#include <core/log.hpp>


namespace eXl
{

#ifndef EXL_SHARED_LIBRARY
  extern PluginLoadMap s_StaticPluginMap;
#endif

  typedef Map<String, Plugin*> PluginMap;
  using PendingPluginVector = boost::container::static_vector<Plugin*, 128>;
  namespace
  {

    PluginMap& GetPluginMap()
    {
      static PluginMap s_PluginMap;
      return s_PluginMap;
    }
    PendingPluginVector& GetPendingLoadPlugins()
    {
      static PendingPluginVector s_PendingLoadPlugins;
      return s_PendingLoadPlugins;
    }
    unsigned int s_LoadNum = 0;

    //class RootPlugin : public Plugin
    //{
    //public:
    //  RootPlugin() : Plugin(PLUGIN_NAME_STR)
    //  {
    //    m_LoadOrder = 0;
    //    s_LoadNum ++ ;
    //  }
    //
    //  void _Load(){}
    //};
    //
    //RootPlugin s_RootPlugin;
    
  }
//#ifdef EXL_SHARED_LIBRARY
//  Plugin& GetLocalPlugin()
//#else
//  Plugin& _GetLocalPlugin(PLUGIN_NAME)()
//#endif
//  {
//    return s_RootPlugin;
//  }

  Plugin::Plugin(String const& iName) :
    m_Name(iName),
    m_Library(nullptr)
  {
    GetPluginMap().insert(std::make_pair(iName,this));
    m_LoadOrder = s_LoadNum++;
    GetPendingLoadPlugins().push_back(this);
  }

  Plugin::~Plugin()
  {
    PluginMap::iterator iter = GetPluginMap().find(m_Name);
    if(iter != GetPluginMap().end())
    {
      GetPluginMap().erase(iter);
    }

    if (m_Library)
      CLOSE_LIB((LIB_HANDLE)m_Library);
  }

  void Plugin::LoadDependencies()
  {
    Vector<String>::iterator iter = m_Dependencies.begin();
    Vector<String>::iterator iterEnd = m_Dependencies.end();
    for(;iter != iterEnd; ++iter)
    {
      Plugin* dep = LoadLib(*iter);
      eXl_ASSERT_MSG_REPAIR_BEGIN(dep != nullptr, iter->c_str())
      {
        continue;
      }
      else
      {
        if (dep->m_Depth + 1 > m_Depth)
        {
          m_Depth = dep->m_Depth + 1;
        }
      }
    }
  }

  //typedef Plugin* (*PtrFunc)();

  Plugin* Plugin::LoadLib(const String& iFilename)
  {
    PluginMap::iterator iter = GetPluginMap().find(iFilename);

    if(iter == GetPluginMap().end())
    {
#ifndef EXL_SHARED_LIBRARY
      PluginLoadMap::iterator iter = s_StaticPluginMap.find(iFilename);

      if(iter == s_StaticPluginMap.end())
      {
        return nullptr;
      }

      PluginFactory Function = iter->second;
      LIB_HANDLE libHandle = 0;
#else
      

#if _DEBUG
      LIB_HANDLE libHandle = OPEN_LIB((iFilename + "_d"));
#else
      LIB_HANDLE libHandle = OPEN_LIB(iFilename);
#endif

      if (!libHandle)
        return nullptr;

      PluginFactory Function = reinterpret_cast<PluginFactory>(GET_PROC(libHandle, "GetPlugin"));
#endif
      if (!Function)
      {
        LOG_ERROR << "Missing plugin " << iFilename;
        return nullptr;
      }
    
      Plugin& plugin = Function();
      plugin.m_Library = libHandle;
      plugin.LoadDependencies();
      return &plugin;
    }
    return iter->second;
  }

  namespace detail
  {
    void _PLInit()
    {
      Plugin::FlushLoadedPluginInit();
    }

    struct CompareLoadOrder
    {
      bool operator()(Plugin* const& iPl1, Plugin* const& iPl2) const
      {
        return iPl1->GetLoadOrder() < iPl2->GetLoadOrder();
      }
    };

    void _PLClose()
    {
      Set<Plugin*,CompareLoadOrder> destSet;
      PluginMap::iterator iter = GetPluginMap().begin();
      PluginMap::iterator iterEnd = GetPluginMap().end();
      for(;iter != iterEnd;++iter)
      {
        destSet.insert(iter->second);
      }
      Set<Plugin*,CompareLoadOrder>::reverse_iterator destIter = destSet.rbegin();
      Set<Plugin*,CompareLoadOrder>::reverse_iterator destIterEnd = destSet.rend();
      for(;destIter != destIterEnd; ++destIter)
      {
        (*destIter)->_Unload();
      }
    }
  }

  void Plugin::FlushLoadedPluginInit()
  {
    PendingPluginVector newlyLoadedPlugins = GetPendingLoadPlugins();
    PendingPluginVector pluginsToProcess = GetPendingLoadPlugins();
    GetPendingLoadPlugins().clear();
    while (!pluginsToProcess.empty())
    {
      for (auto plugin : pluginsToProcess)
      {
        plugin->LoadDependencies();
      }
      for (auto newPlugin : GetPendingLoadPlugins())
      {
        newlyLoadedPlugins.push_back(newPlugin);
      }
      pluginsToProcess = GetPendingLoadPlugins();
      GetPendingLoadPlugins().clear();
    }

    std::sort(newlyLoadedPlugins.begin(), newlyLoadedPlugins.end(), [](Plugin* const& iPl1, Plugin* const& iPl2)
    {
      return iPl1->m_Depth < iPl2->m_Depth;
    });

    for (auto plugin : newlyLoadedPlugins)
    {
      LOG_INFO << "Load plugin " << plugin->GetName();
      plugin->_Load();
    }
  }
}