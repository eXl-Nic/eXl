#pragma once

#include <iostream>

#include "types.hpp"

namespace eXl
{
  namespace reflang
  {
    namespace serializer
    {
      struct Options
      {
        String include_path;
        String out_hpp_path;
        String out_cpp_path;
        String internalLibName;
      };

      void Serialize(
        const Vector<std::unique_ptr<TypeBase>>& types,
        const Options& options = Options());
    }
  }
}
