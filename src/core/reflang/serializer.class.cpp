#include "serializer.class.hpp"
#include <sstream>

#include "serializer.function.hpp"
#include "serializer.util.hpp"
#include "types.hpp"

namespace eXl
{
  namespace reflang
  {
    void serializer::SerializeClassHeader(std::ostream& o, const Class& c, String const& iDefineDirective)
    {
      //o << iDefineDirective << "(" << c.GetShortName() <<")" "\n";
    }

    //String ConvertFullNameToFriendly(String const& iFullName)
    //{
    //  String friendlyName = iFullName;
    //  size_t pos;
    //  
    //  while ((pos = friendlyName.find("::")) != String::npos)
    //  {
    //    friendlyName.replace(pos, 2, "_");
    //  }
    //
    //  return friendlyName;
    //}

    void serializer::SerializeClassSources(std::ostream& o, const Class& c)
    {
      String className = c.GetShortName();
      String fullName = c.GetFullName();
      String friendlyName = GetNameWithoutColons(fullName);

      o << "namespace { Type const* s_" << friendlyName << "_TypeStorage = nullptr; }\n";
      o << "\n";
      //o << "Type const* Get_" << className << "_NativeType()\n";
      //o << "{ return s_" << className << "_TypeStorage; }\n";
      o << "void Register_"<< friendlyName <<"_Type()\n";
      o << "{\n";
      o << "s_" << friendlyName << "_TypeStorage = \n";
      o << "TypeManager::BeginNativeTypeRegistration<" << fullName << ">(\"" << friendlyName << "\")\n";
      for (auto const& field : c.m_Fields)
      {
        o << ".AddField(\"" << field.name << "\", &" << fullName << "::" << field.name << ")\n";
      }
      o << ".EndRegistration();\n";
      o << "}\n";
      o << "\n";
      o << "Type const* " << fullName << "::GetType() { return s_" << friendlyName << "_TypeStorage; }\n";
      o << "Err " << fullName << "::Stream(Streamer& iStreamer) const { return GetType()->Stream(this, &iStreamer); }\n";
      o << "Err " << fullName << "::Unstream(Unstreamer& iStreamer) { void* readBuffer = this; return GetType()->Unstream(readBuffer, &iStreamer); }\n";
      o << "\n";

#ifdef EXL_LUA
      o << "void Register_" << friendlyName << "_Lua(lua_State* iState)\n";
      o << "{\n";
      //o << "luabind::module(iState, \"eXl\")[\n";
      //o << "luabind::class_<" << fullName << ">(\"" << className << "\")\n";
      //o << ".def(luabind::constructor<>())\n";
      //for (auto const& field : c.m_Fields)
      //{
      //  o << ".def_readwrite(\"" << field.name << "\", &" << fullName << "::" << field.name << ")\n";
      //}
      //o << "];\n";
      //o << "LuaManager::RegisterType<" << fullName << ">(LuaManager::GetCurrentState());\n";
      o << "TypeManager::GetType<" << fullName << ">()->RegisterLua(iState);";
      o << "}\n";
      o << "\n";
#endif


    }
  }
}