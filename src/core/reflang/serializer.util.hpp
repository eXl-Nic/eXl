#pragma once

#include <core/string.hpp>

namespace eXl
{
  namespace reflang
  {
    namespace serializer
    {
      String GetNameWithoutColons(String name);

      using FromToPair = std::pair<String, String>;
      String ReplaceAll(const String& text, std::initializer_list<FromToPair> from_to_pairs);
    }
  }
}


