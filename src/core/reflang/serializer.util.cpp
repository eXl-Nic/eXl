#include "serializer.util.hpp"

#include <algorithm>
#include <regex>

namespace eXl
{
  namespace reflang
  {
    String serializer::GetNameWithoutColons(String name)
    {
      replace(name.begin(), name.end(), ':', '_');
      return name;
    }

    String serializer::ReplaceAll(
      const String& text,
      std::initializer_list<FromToPair> pairs)
    {
      String result = text;
      for (const auto& pair : pairs)
      {
        std::regex replace(pair.first);
        result = regex_replace(result, replace, pair.second);
      }
      return result;
    }

  }
}