/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>
#include <string>
#include <vector>

#define PLUGIN_NAME_STR eXl_TO_STR(PLUGIN_NAME)

namespace eXl
{
  class Plugin;
  typedef Plugin& (*PluginFactory)();
}

#ifdef EXL_SHARED_LIBRARY

namespace eXl
{
  Plugin& GetLocalPlugin();
}

#ifdef WIN32
#define PLUGIN_EXPORT __declspec(dllexport)
#elif defined __GNUC__
#define PLUGIN_EXPORT __attribute__ ((visibility("default")))
#else
#define PLUGIN_EXPORT
#endif

#define PLUGIN_FACTORY(PluginClassName)   \
namespace                            \
{                                    \
  PluginClassName s_PluginInstance;  \
}                                    \
                                     \
Plugin& GetLocalPlugin()             \
{                                    \
  return s_PluginInstance;           \
}                                    \
                                     \
extern "C"                           \
{                                    \
  PLUGIN_EXPORT                      \
  eXl::Plugin& GetPlugin()           \
  {                                  \
    return s_PluginInstance;         \
  }                                  \
}

#else

#include <map>

#define _GetLocalPlugin(name) eXl_CONCAT(name,_GetLocalPlugin)

namespace eXl
{
  //Plugin* _GetLocalPlugin(PLUGIN_NAME)();
  typedef std::map<String,PluginFactory> PluginLoadMap;
}

#define PLUGIN_FACTORY(PluginClassName)   \
                                          \
namespace                                 \
{                                         \
  PluginClassName s_PluginInstance;       \
}                                         \
                                          \
Plugin& _GetLocalPlugin(PluginClassName)()\
{                                         \
  return s_PluginInstance;                \
}                                         \
                                          \
Plugin& PluginClassName##_GetPlugin()     \
{                                         \
  return s_PluginInstance;                \
}

#define GetLocalPlugin() _GetLocalPlugin(PLUGIN_NAME)()

#endif



namespace eXl
{

  namespace detail
  {
    void _PLInit();
    void _PLClose();
  }

  class EXL_CORE_API Plugin
  {
  public :
    friend void detail::_PLClose();
    static Plugin* LoadLib(const String& Filename);

    static void FlushLoadedPluginInit();

    inline String const& GetName() const {return m_Name;}

    inline unsigned int GetLoadOrder()const{return m_LoadOrder;}

  protected:

    Plugin(String const& );

    virtual ~Plugin();

    void LoadDependencies();

    virtual void _Load() = 0;
    virtual void _Unload() {};

    //
    //virtual void Unload() = 0;

    String m_Name;
    void* m_Library;
    unsigned int m_LoadOrder;
    unsigned int m_Depth = 0;

    Vector<String> m_Dependencies;
  };
}
