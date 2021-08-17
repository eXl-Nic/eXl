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
      String GetFunctionSignature(const Function& f);
      void SerializeFunctionHeader(std::ostream& o, const Function& c);
      void SerializeFunctionSources(std::ostream& o, const Function& c);
    }
  }
}

