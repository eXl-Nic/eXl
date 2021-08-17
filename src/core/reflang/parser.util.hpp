#pragma once

#include <core/string.hpp>
#include <core/path.hpp>

#include <clang-c/Index.h>

namespace eXl
{
  namespace reflang
  {
    namespace parser
    {
      String Convert(const CXString& s);
      Path ToPath(const CXString& s);

      String GetFullName(CXCursor cursor);
      String GetName(const CXType& type);
      Path GetFile(const CXCursor& cursor);

      bool IsRecursivelyPublic(CXCursor cursor);
    }
  }
}

