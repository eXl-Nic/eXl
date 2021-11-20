
#include "gtest/gtest.h"
#include <core/corelib.hpp>
#include <core/coretest.hpp>
#include <core/vlog.hpp>

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