/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef EXL_LUA

#include <core/lua/luabindcore.hpp>

#include <core/lua/luamanager.hpp>

#include <core/type/typemanager.hpp>
#include <core/type/tupletypestruct.hpp>
#include <core/type/enumtype.hpp>
#include <core/type/arraytype.hpp>
#include <core/type/dynobject.hpp>

#include <core/rtti.hpp>


#include <luabind/luabind.hpp>
#include <luabind/raw_policy.hpp>
#include <luabind/adopt_policy.hpp>
#include <luabind/dependency_policy.hpp>
#include <luabind/return_reference_to_policy.hpp>
#include <luabind/out_value_policy.hpp>
#include <luabind/operator.hpp>
#include <core/vlog.hpp>

void stackDump (lua_State *L) {
  int i;
  int top = lua_gettop(L);
  for (i = 1; i <= top; i++) {  /* repeat for each level */
    int t = lua_type(L, i);
    switch (t) {
    
    case LUA_TSTRING:  /* strings */
      printf("`%s'", lua_tostring(L, i));
      break;
    
    case LUA_TBOOLEAN:  /* booleans */
      printf(lua_toboolean(L, i) ? "true" : "false");
      break;
    
    case LUA_TNUMBER:  /* numbers */
      printf("%g", lua_tonumber(L, i));
      break;
    
    default:  /* other values */
      printf("%s", lua_typename(L, t));
      break;
    
    }
    printf("  ");  /* put a separator */
  }
  printf("\n");  /* end the listing */
}

namespace eXl
{
  class LuaKeys{

  };

  LUA_REG_FUN(BindCore)
  {
    char const* strPtr = nullptr;

    luabind::module(iState, "eXl")[
      
      luabind::class_<Err>("Err").enum_("Const")[
        luabind::value("Err::Success",Err::Success),
          luabind::value("Err::Failure",Err::Failure)
      ],
      
      luabind::class_<RttiObject>("RttiObject")
      .def("GetRtti",&RttiObject::GetRtti),

      luabind::class_<String>("String")
          .def(luabind::constructor<>())
          .def(luabind::constructor<char const*>())
          //.def("size", &String::size)
          .def("data", (char const*(String::*)()const)&String::data)
          .def("c_str", (char const*(String::*)()const)&String::c_str),

      luabind::class_<ConstDynObject>("ConstDynObject")
      .def(luabind::constructor<>())
      .def("GetType",&ConstDynObject::GetType)
      .def("IsValid",&ConstDynObject::IsValid)
      .def("GetFieldConst",(Err (ConstDynObject::*)(TypeFieldName, ConstDynObject&) const)&ConstDynObject::GetField)
      .def("GetElementConst",(Err (ConstDynObject::*)(unsigned int, ConstDynObject&)const)&ConstDynObject::GetElement)
      .def("ToLua",&ConstDynObject::ToLua/*,luabind::out_value(_2)*/),

      luabind::class_<DynObject,ConstDynObject>("DynObject")
      .def(luabind::constructor<>())
      .def(luabind::constructor<Type const*, luabind::object const&>())
      .def(luabind::constructor<ConstDynObject const*>())
      .def("FromLua",&DynObject::FromLua)
      .def("GetField",(Err (DynObject::*)(TypeFieldName, DynObject&) )&DynObject::GetField)
      .def("GetElement",&DynObject::GetElement),

      luabind::class_<Type>("Type")
      //.def("ConvertToLua",&Type::ConvertToLua)
      //.def("ConvertFromLua",TypeConvertFromLua,luabind::adopt(luabind::result))
      .def("IsTuple",&Type::IsTuple,luabind::dependency_policy<0, 1>()),
        
      luabind::class_<TupleType,Type>("TupleType")
      /*.def("GetFieldIdx",(DynObject*(TupleType::*)(DynObject*,unsigned int,DynObject*)const)&TupleType::GetField)
      .def("GetFieldStr",(DynObject*(TupleType::*)(DynObject*,const std::string&,DynObject*)const)&TupleType::GetField)
      .def("GetFieldConstIdx",(ConstDynObject*(TupleType::*)(const ConstDynObject*,unsigned int,ConstDynObject*)const)&TupleType::GetField)
      .def("GetFieldConstStr",(ConstDynObject*(TupleType::*)(const ConstDynObject*,const std::string&,ConstDynObject*)const)&TupleType::GetField)*/,

      luabind::class_<TupleTypeStruct,TupleType>("TupleTypeStruct"),

      luabind::class_<EnumType,Type>("EnumType")
      .def("GetNumEnum",&EnumType::GetNumEnum)
      //.def("GetEnumName",&EnumType::GetEnumName)
      .def("GetEnumValue",&EnumType::GetEnumValue,luabind::pure_out_value<3>()),

      luabind::class_<ArrayType,Type>("ArrayType")
      .def("GetElementType",&ArrayType::GetElementType),

      luabind::namespace_("TypeManager")[
        luabind::def("GetArrayType",static_cast<ArrayType const*(*)(Type const*)>(&TypeManager::GetArrayType))
        ],

        luabind::class_<Name>("Name")
          .def(luabind::constructor<>())
          .def(luabind::constructor<const char*>())
          .def(luabind::constructor<String const&>())
          .def("get", &Name::get)
      ];

    TypeManager::GetArrayType<uint64_t>()->RegisterLua(iState);
    TypeManager::GetArrayType<uint32_t>()->RegisterLua(iState);
    TypeManager::GetArrayType<int32_t>()->RegisterLua(iState);
    TypeManager::GetArrayType<float>()->RegisterLua(iState);
    TypeManager::GetArrayType<uint8_t>()->RegisterLua(iState);
    TypeManager::GetArrayType<AString>()->RegisterLua(iState);
    TypeManager::GetArrayType<Name>()->RegisterLua(iState);

    return 0;
  }
}

#endif
