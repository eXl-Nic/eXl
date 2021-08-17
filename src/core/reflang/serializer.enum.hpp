#pragma once

#include <iostream>

#include "serializer.hpp"
#include "types.hpp"

namespace eXl
{
  namespace reflang
  {
    namespace serializer
    {
      void SerializeEnumHeader(std::ostream& o, const Enum& e);
      void SerializeEnumSources(std::ostream& o, const Enum& e);
    }
  }
}

