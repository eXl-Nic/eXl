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
      void SerializeClassHeader(std::ostream& o, const Class& c, String const& iDefineDirective);
      void SerializeClassSources(std::ostream& o, const Class& c);
    }
  }
}
