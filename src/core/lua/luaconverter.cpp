/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef EXL_LUA

#include <core/lua/luaconverter.hpp>

void stackDump (lua_State *L);

namespace eXl
{
  luabind::object LuaConverter<unsigned int>::ConvertToLua(const void* iObj, Type const* iType,lua_State* iState)
  {
    if (iObj == nullptr || iType == nullptr)
    {
      return luabind::object();
    }
    unsigned int val = *(unsigned int*)iObj;
    lua_pushinteger(iState,val);
    luabind::object res(luabind::from_stack(iState,-1));
    lua_pop(iState,1);
    return res;
  }

  Err LuaConverter<unsigned int>::ConvertFromLua(const Type* iType,void* oObj,lua_State* iState,unsigned int& ioIndex)
  {
    if(!lua_isnumber(iState,ioIndex))
    {
      stackDump(iState);
      eXl_ASSERT_MSG(false,"Conversion failed");
    }
    unsigned int tempVal = lua_tonumber(iState,ioIndex);
    iType->Copy_Uninit(&tempVal,oObj);
    ioIndex++;
    RETURN_SUCCESS;
  }
  
  luabind::object LuaConverter<float>::ConvertToLua(const void* iObj, Type const* iType,lua_State* iState)
  {
    if (iObj == nullptr || iType == nullptr)
    {
      return luabind::object();
    }
    float val = *(float*)iObj;
    lua_pushnumber(iState,val);
    luabind::object res(luabind::from_stack(iState,-1));
    lua_pop(iState,1);
    return res;
  }

  Err LuaConverter<float>::ConvertFromLua(const Type* iType,void* oObj,lua_State* iState,unsigned int& ioIndex)
  {
    if(!lua_isnumber(iState,ioIndex))
    {
      stackDump(iState);
      eXl_ASSERT_MSG(false,"Conversion failed");
    }
    float tempVal = lua_tonumber(iState,ioIndex);
    iType->Copy_Uninit(&tempVal,oObj);
    ioIndex++;
    RETURN_SUCCESS;
  }
  
  luabind::object LuaConverter<int>::ConvertToLua(const void* iObj, Type const* iType,lua_State* iState)
  {
    if (iObj == nullptr || iType == nullptr)
    {
      return luabind::object();
    }
    int val = *(int*)iObj;
    lua_pushinteger(iState,val);
    luabind::object res(luabind::from_stack(iState,-1));
    lua_pop(iState,1);
    return res;
  }

  Err LuaConverter<int>::ConvertFromLua(const Type* iType,void* oObj,lua_State* iState,unsigned int& ioIndex)
  {
    if(!lua_isnumber(iState,ioIndex))
    {
      stackDump(iState);
      eXl_ASSERT_MSG(false,"Conversion failed");
    }
    int tempVal = lua_tonumber(iState,ioIndex);
    iType->Copy_Uninit(&tempVal,oObj);
    ioIndex++;
    RETURN_SUCCESS;
  }
  
  luabind::object LuaConverter<bool>::ConvertToLua(const void* iObj, Type const* iType,lua_State* iState)
  {
    if (iObj == nullptr || iType == nullptr)
    {
      return luabind::object();
    }
    bool val = *(bool*)iObj;
    lua_pushboolean(iState,val);
    luabind::object res(luabind::from_stack(iState,-1));
    lua_pop(iState,1);
    return res;
  }

  Err LuaConverter<bool>::ConvertFromLua(const Type* iType,void* oObj,lua_State* iState,unsigned int& ioIndex){
    if(!lua_isboolean(iState,ioIndex))
    {
      stackDump(iState);
      eXl_ASSERT_MSG(false,"Conversion failed");
    }
    unsigned int tempVal = lua_toboolean(iState,ioIndex);
    iType->Copy_Uninit(&tempVal,oObj);
    ioIndex++;
    RETURN_SUCCESS;
  }
  
  luabind::object LuaConverter<unsigned char>::ConvertToLua(const void* iObj, Type const* iType,lua_State* iState)
  {
    if (iObj == nullptr || iType == nullptr)
    {
      return luabind::object();
    }
    unsigned char val = *(unsigned char*)iObj;
    lua_pushinteger(iState,val);
    luabind::object res(luabind::from_stack(iState,-1));
    lua_pop(iState,1);
    return res;
  }

  Err LuaConverter<unsigned char>::ConvertFromLua(const Type* iType,void* oObj,lua_State* iState,unsigned int& ioIndex)
  {
    if(!lua_isnumber(iState,ioIndex))
    {
      stackDump(iState);
      eXl_ASSERT_MSG(false,"Conversion failed");
    }
    unsigned char tempVal = lua_tonumber(iState,ioIndex);
    iType->Copy_Uninit(&tempVal,oObj);
    ioIndex++;
    RETURN_SUCCESS;
  }

  luabind::object LuaConverter<std::string>::ConvertToLua(const void* iObj, Type const* iType,lua_State* iState)
  {
    if (iObj == nullptr || iType == nullptr)
    {
      return luabind::object();
    }
    std::string* val = (std::string*)iObj;
    lua_pushlstring(iState,val->c_str(),val->size());
    luabind::object res(luabind::from_stack(iState,-1));
    lua_pop(iState,1);
    return res;

  }
  Err LuaConverter<std::string>::ConvertFromLua(const Type* iType,void* oObj,lua_State* iState,unsigned int& ioIndex)
  {
    if(!lua_isstring(iState,ioIndex))
    {
      stackDump(iState);
      eXl_ASSERT_MSG(false,"Conversion failed");
    }
    const char* tempStr = lua_tostring(iState,ioIndex);
    std::string tempVal(tempStr);
    iType->Copy_Uninit(&tempVal,oObj);
    ioIndex++;
    RETURN_SUCCESS;
  }
}

#endif
