#pragma once

#include <clang-c/Index.h>

#include "types.hpp"

namespace eXl
{
  namespace reflang
  {
    namespace parser
    {
      Class GetClass(CXCursor cursor);
    }
  }
}

