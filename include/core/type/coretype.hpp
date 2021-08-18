/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/type/type.hpp>
#include <core/rtti.hpp>
#ifdef EXL_LUA
#include <core/lua/luaconverter.hpp>
#include <luabind/class.hpp>
#include <core/lua/luabind_eXl.hpp>
#endif

namespace eXl
{
  class EXL_CORE_API CoreType : public Type
  {
    DECLARE_RTTI(CoreType,Type);
  
  protected:
    CoreType(TypeName iName,
         size_t iTypeId,
         size_t iSize,
         unsigned int iFlags);
  };

  template <class T>
  class T_CoreType : public CoreType
  {
  public:
    T_CoreType();
  
    void* Alloc() const override;

    void Free(void* iObj) const override;

    void* Construct(void* iObj) const override;

    void Destruct(void* iObj) const override;

    Err Copy_Uninit(void const* iData, void* oData) const override;

    Err Stream(void const* iData, Streamer* iStreamer) const override;

    Err Unstream_Uninit(void* oData, Unstreamer* iStreamer) const override;

#ifdef EXL_LUA
    Err ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj) const override;

    luabind::object ConvertToLua(void const* iObj,lua_State* iState) const override;

    void RegisterLua(lua_State* iState) const override;

    luabind::object MakePropertyAccessor(lua_State* iState, Type const* iHolder, uint32_t iOffset) const override;
    luabind::object MakeElementAccessor(lua_State* iState, ArrayType const* iHolder) const override;
#endif

    Err Compare(void const* iVal1, void const* iVal2, CompRes& oRes) const override;
  };
}

namespace eXl
{
  #include "coretype.inl"
}

