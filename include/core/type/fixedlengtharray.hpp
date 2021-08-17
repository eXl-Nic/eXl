/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/type/tupletype.hpp>

namespace eXl
{
  class EXL_CORE_API FixedLengthArray : public TupleType
  {
    DECLARE_RTTI(FixedLengthArray,TupleType)
  public:

    static FixedLengthArray const* Create(Type const*, unsigned int iNumFields);

    size_t GetNumField() const override {return m_NumFields;}

    inline Type const* GetArrayType() const {return m_Type;}

    void* Construct(void* iObj) const override;

    void Destruct(void* iObj) const override;

    Err Copy_Uninit(void const* iData, void* oData) const override;
#ifdef EXL_LUA
    luabind::object ConvertToLua(void const* iObj,lua_State* iState)const override;

    Err ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const override;

    Err ConvertFromLuaRaw_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const override;
#endif
    Err ResolveFieldPath(AString const& iPath, unsigned int& oOffset, Type const*& oType) const override;

    bool UserType()const;
    
    //Make sure iObj outlives the returned DynObject :), like holding a ptr to a struct
    void* GetField (void* iObj,unsigned int,Type const*& oType) const override;

    void* GetField (void* iObj, TypeFieldName iName,Type const*& oType) const override;
    
    void const* GetField (void const* iObj,unsigned int,Type const*& oType) const override;

    void const* GetField (void const* iObj,TypeFieldName iName,Type const*& oType) const override;

    const Type* GetFieldDetails(unsigned int iNum,TypeFieldName& oFieldName)const override;

    const Type* GetFieldDetails(TypeFieldName iFieldName,unsigned int& oNumField)const override;

    const Type* GetFieldDetails(unsigned int iNum)const override;

    const Type* GetFieldDetails(TypeFieldName iFieldName)const override;

  protected:

    FixedLengthArray(Type const* iType, unsigned int iNum);

    Type const*  m_Type;
    unsigned int m_NumFields;
  };
}
