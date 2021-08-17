#include "serializer.enum.hpp"

#include <memory>
#include <sstream>
#include <unordered_set>
#include <vector>

#include "serializer.util.hpp"

namespace eXl
{
  namespace reflang
  {

    namespace
    {
      Vector<String> GetEnumUniqueValues(const Enum& e)
      {
        Vector<String> results;
        UnorderedSet<int> used_values;

        for (const auto& it : e.m_Values)
        {
          if (used_values.find(it.second) == used_values.end())
          {
            used_values.insert(it.second);
            results.push_back(it.first);
          }
        }

        return results;
      }

      Vector<String> GetEnumValues(const Enum& e)
      {
        Vector<String> results;

        for (const auto& it : e.m_Values)
        {
          results.push_back(it.first);
        }

        return results;
      }
    }

    void serializer::SerializeEnumHeader(std::ostream& o, const Enum& e)
    {

    }

    void serializer::SerializeEnumSources(std::ostream& o, const Enum& e)
    {
      String enumName = e.GetShortName();
      String friendlyName = GetNameWithoutColons(e.GetFullName());

      
      o << "namespace { Type const* s_" << friendlyName << "_TypeStorage = nullptr; }\n";
      o << "\n";
      o << "void Register_" << friendlyName << "_Type()\n";
      o << "{\n";
      o << "s_" << friendlyName << "_TypeStorage = \n";
      o << "TypeManager::BeginEnumTypeRegistration(\"" << friendlyName << "\")\n";
      Map<uint32_t, String> values;
      for (auto const& value : e.m_Values)
      {
        values.insert(std::make_pair(value.second, value.first));
      }
      uint32_t valueCounter = 0;
      for (auto const& value : values)
      {
        assert(valueCounter == value.first);
        o << ".AddValue(\"" << value.second << "\")\n";
        ++valueCounter;
      }

      o << ".EndRegistration();\n";
      o << "}\n";
      o << "\n";
      o << "Type const* Get_" << friendlyName << "_EnumType() { return s_" << friendlyName << "_TypeStorage; }";
      o << "\n";

#ifdef EXL_LUA
      o << "void Register_" << friendlyName << "_Lua(lua_State* iState)\n";
      o << "{\n";
      o << "luabind::module(iState, \"eXl\")[\n";
      o << "luabind::class_<" << e.GetFullName() << ">(\"" << enumName << "\")\n";
      o << ".enum_(\"Const\")[\n";
      bool firstEnum = true;
      for (auto const& value : e.m_Values)
      {
        if (!firstEnum)
        {
          o << ",\n";
        }
        o << "luabind::value(\"" << value.first << "\", "<< value.second << ")";
        firstEnum = false;
      }
      o << "\n]\n";
      o << "];\n";
      o << "}\n";
      o << "\n";
#endif
    }
  }
}
