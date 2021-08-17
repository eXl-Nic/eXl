/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/type/type.hpp>
#ifdef EXL_LUA
namespace eXl
{
  template <class T>
  struct LuaConverter{
    static luabind::object ConvertToLua(const void* iObj, Type const* iType,lua_State* iState)
    {
      if(iObj==nullptr || iType==nullptr)
        return luabind::object();
      void * buffer = iType->Alloc();
      iType->Assign_Uninit(iType,iObj,buffer);
      return luabind::object(iState,(T*)buffer,luabind::adopt_policy<0>());
    }

    static Err ConvertFromLua(const Type* iType,void* oObj,lua_State* iState,unsigned int& ioIndex)
    {
      if(oObj == nullptr)
      {
        LOG_WARNING<<"Prob with iObj"<<"\n";
        RETURN_FAILURE;
      }
      else
      {
        luabind::object ref(luabind::from_stack(iState,ioIndex));
        if(ref.is_valid())
        {
          T const* refPtr = luabind::object_cast<T const*>(ref);
          eXl_ASSERT_MSG(refPtr!=nullptr,"Conversion failed");
          iType->Assign_Uninit(iType,refPtr,oObj);
        }
        else
        {
          LOG_WARNING<<"Invalid reference"<<"\n";
        }
      }
      ioIndex++;
      RETURN_SUCCESS;
    }
  };

  template <class T>
  struct LuaConverter<T*>{
    static luabind::object ConvertToLua(const void* iObj, Type const* iType,lua_State* iState)
    {
      if(iObj==nullptr || iType==nullptr)
        return luabind::object();
      return luabind::object(iState,((T*)iObj));
    }

    static Err ConvertFromLua(const Type* iType,void* oObj,lua_State* iState,unsigned int& ioIndex)
    {
      if(oObj == nullptr)
      {
        LOG_WARNING<<"Prob with iObj"<<"\n";
        RETURN_FAILURE;
      }
      else{
        luabind::object ref(luabind::from_stack(iState,ioIndex));
        if(ref.is_valid())
        {
          T const* refPtr = luabind::object_cast<T const*>(ref);
          eXl_ASSERT_MSG(refPtr!=nullptr,"Conversion failed");
          iType->Assign_Uninit(iType,refPtr,oObj);
        }
        else
        {
          LOG_WARNING<<"Invalid reference"<<"\n";
        }
      }
      ioIndex++;
      RETURN_SUCCESS;
    }
  };
  
  template <>
  struct LuaConverter<unsigned int>{
    EXL_CORE_API static luabind::object ConvertToLua(const void* iObj, Type const* iType,lua_State* iState);
    EXL_CORE_API static Err ConvertFromLua(const Type* iType,void* oObj,lua_State* iState,unsigned int& ioIndex);
  };
  template <>
  struct LuaConverter<float>{
    EXL_CORE_API static luabind::object ConvertToLua(const void* iObj, Type const* iType,lua_State* iState);
    EXL_CORE_API static Err ConvertFromLua(const Type* iType,void* oObj,lua_State* iState,unsigned int& ioIndex);
  };
  template <>
  struct LuaConverter<int>{
    EXL_CORE_API static luabind::object ConvertToLua(const void* iObj, Type const* iType,lua_State* iState);
    EXL_CORE_API static Err ConvertFromLua(const Type* iType,void* oObj,lua_State* iState,unsigned int& ioIndex);
  };
  template <>
  struct LuaConverter<bool>{
    EXL_CORE_API static luabind::object ConvertToLua(const void* iObj, Type const* iType,lua_State* iState);
    EXL_CORE_API static Err ConvertFromLua(const Type* iType,void* oObj,lua_State* iState,unsigned int& ioIndex);
  };
  template <>
  struct LuaConverter<unsigned char>{
    EXL_CORE_API static luabind::object ConvertToLua(const void* iObj, Type const* iType,lua_State* iState);
    EXL_CORE_API static Err ConvertFromLua(const Type* iType,void* oObj,lua_State* iState,unsigned int& ioIndex);
  };

  template <>
  struct LuaConverter<std::string>{
    EXL_CORE_API static luabind::object ConvertToLua(const void* iObj, Type const* iType,lua_State* iState);
    EXL_CORE_API static Err ConvertFromLua(const Type* iType,void* oObj,lua_State* iState,unsigned int& ioIndex);
  };

}
#endif
