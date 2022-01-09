
#include "gtest/gtest.h"
#include <core/corelib.hpp>
#include <core/coretest.hpp>
#include <core/vlog.hpp>

#ifndef EXL_SHARED_LIBRARY

#include <core/plugin.hpp>

namespace eXl
{
  Plugin& MathPlugin_GetPlugin();
  Plugin& OGLPlugin_GetPlugin();
  Plugin& EnginePlugin_GetPlugin();

  PluginLoadMap s_StaticPluginMap =
  {
    {"eXl_OGL", &OGLPlugin_GetPlugin},
    {"eXl_Math", &MathPlugin_GetPlugin},
    {"eXl_Engine", &EnginePlugin_GetPlugin}
  };
}
#endif

struct CoreLibCtx
{
  CoreLibCtx()
  {
    eXl::StartCoreLib(nullptr);
  }

  ~CoreLibCtx()
  {
    eXl::StopCoreLib();
  }
};

GTEST_API_ int main(int argc, char** argv) 
{
  testing::InitGoogleTest(&argc, argv);

  CoreLibCtx eXlCtx;

  eXl::SetErrorHandling(eXl::DEBUG_STAGE);

  return RUN_ALL_TESTS();
}