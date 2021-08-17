/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/type/tupletype.hpp>

#ifdef EXL_LUA
#include <core/lua/luaconverter.hpp>
#include <core/lua/luabind_eXl.hpp>
#endif

namespace eXl
{
  class ClassType;
  class TupleTypeStructClassType;

  namespace TypeManager
  {
    EXL_CORE_API const TupleType* MakeTuple(TypeName ,const List<FieldDesc>&,size_t*);

    template <class T>
    class NativeTypeReg;

    class SignTypeReg;

    namespace detail
    {
      template <class T>
      const TupleType* RegisterSignature(size_t iId,const List<FieldDesc>& iFields);
    }
  }

  class EXL_CORE_API TupleTypeAdapter : public TupleType
  {
  public:
    inline TupleTypeAdapter(TypeName iName,size_t iId,size_t iSize,unsigned int iFlags)
      : TupleType(iName,iId,iSize,iFlags)
    {}
  protected:

    typedef Vector<FieldDesc> FieldVector;

    static size_t S_GetNumField(FieldVector const& iVect);

    static Err S_ResolveFieldPath(FieldVector const& iVect, AString const& iPath, unsigned int& oOffset, Type const*& oType);
    
    //Make sure iObj outlives the returned DynObject :), like holding a ptr to a struct
    static void* S_GetField (FieldVector const& iVect, void* iObj,unsigned int,Type const*& oType);

    static void* S_GetField (FieldVector const& iVect, void* iObj, TypeFieldName iName,Type const*& oType);
    
    static void const* S_GetField (FieldVector const& iVect, void const* iObj,unsigned int,Type const*& oType);

    static void const* S_GetField (FieldVector const& iVect, void const* iObj,TypeFieldName iName,Type const*& oType);

    static const Type* S_GetFieldDetails(FieldVector const& iVect, unsigned int iNum,TypeFieldName& oFieldName);

    static const Type* S_GetFieldDetails(FieldVector const& iVect, TypeFieldName iFieldName, unsigned int& oNumField);

    static const Type* S_GetFieldDetails(FieldVector const& iVect, unsigned int iNum);

    static const Type* S_GetFieldDetails(FieldVector const& iVect, TypeFieldName iFieldName);
  };

  /**
     Specialization of TupleType to hold classic POD types.
  **********************************************************************/
  class EXL_CORE_API TupleTypeStruct : public TupleTypeAdapter
  {
    DECLARE_RTTI(TupleTypeStruct,TupleType);
    friend class TupleTypeStructClassType;
    friend class TypeManager::SignTypeReg;
    friend const TupleType* TypeManager::MakeTuple(TypeName ,const List<FieldDesc>&,size_t*);
    template <class T>
    friend const TupleType* TypeManager::detail::RegisterSignature(size_t iId,const List<FieldDesc>& iFields);
  public:

    static TupleType* Create(const List<FieldDesc>& iList, TypeName name = TypeName(""));

    size_t GetNumField()const override;

    void* Construct(void* iObj) const override;

    void Destruct(void* iObj) const override;

    Err Copy_Uninit(void const* iData, void* oData) const override;
#ifdef EXL_LUA
    luabind::object ConvertToLua(void const* iObj,lua_State* iState) const override;

    Err ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj) const override;

    Err ConvertFromLuaRaw_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj) const override;

    void RegisterLua(lua_State* iState) const override;

    //luabind::object MakePropertyAccessor(lua_State* iState, Type const* iHolder, uint32_t iOffset) const override;
#endif
    Err ResolveFieldPath(AString const& iPath, unsigned int& oOffset, Type const*& oType) const override;
    
    //Make sure iObj outlives the returned DynObject :), like holding a ptr to a struct
    void* GetField (void* iObj,unsigned int,Type const*& oType) const override;

    void* GetField (void* iObj, TypeFieldName iName, Type const*& oType) const override;
    
    void const* GetField (void const* iObj, unsigned int, Type const*& oType) const override;

    void const* GetField (void const* iObj, TypeFieldName iName, Type const*& oType) const override;

    const Type* GetFieldDetails(unsigned int iNum, TypeFieldName& oFieldName) const override;

    const Type* GetFieldDetails(TypeFieldName iFieldName, unsigned int& oNumField) const override;

    const Type* GetFieldDetails(unsigned int iNum) const override;

    const Type* GetFieldDetails(TypeFieldName iFieldName) const override;

    //void DoEnable();

    //void DoDisable();
    
  protected:

    void FillFields(const List<FieldDesc>& iList);

    static TupleType* MakeTuple(TypeName iName,const List<FieldDesc>& iList,size_t iId);

    TupleTypeStruct(TypeName iName, size_t iId, size_t iSize, unsigned int iFlags);

    FieldVector m_Data;
  };
}

namespace eXl
{

  template <class T>
  class CoreTupleType : public TupleTypeAdapter
  {
    template <class U>
    friend const TupleType* TypeManager::detail::RegisterSignature(size_t iId,const List<FieldDesc>& iFields);
    template <class U>
    friend class TypeManager::NativeTypeReg;
  protected:
    
    static TupleType* MakeTuple(TypeName iName,const List<FieldDesc>& iList,size_t iId);

    CoreTupleType(TypeName iName,size_t iId,const List<FieldDesc>& iFields);
    
    size_t GetNumField() const override;

    void* Alloc() const override;

    void Free(void* iObj) const override;
    
    void* Construct(void* iObj) const override;

    void Destruct(void* iObj) const override;

    Err Copy_Uninit(void const* iData, void* oData) const override;
#ifdef EXL_LUA
    luabind::object ConvertToLua(void const* iObj,lua_State* iState) const override;

    Err ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj) const override;

    Err ConvertFromLuaRaw_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj) const override;

    void RegisterLua(lua_State* iState) const override;
#endif
    Err ResolveFieldPath(AString const& iPath, unsigned int& oOffset, Type const*& oType) const override;

    void* GetField (void* iObj,unsigned int,Type const*& oType) const override;

    void* GetField (void* iObj, TypeFieldName iName,Type const*& oType) const override;
    
    void const* GetField (void const* iObj, unsigned int,Type const*& oType) const override;

    void const* GetField (void const* iObj, TypeFieldName iName,Type const*& oType) const override;

    const Type* GetFieldDetails(unsigned int iNum, TypeFieldName& oFieldName) const override;

    const Type* GetFieldDetails(TypeFieldName iFieldName,unsigned int& oNumField) const override;

    const Type* GetFieldDetails(unsigned int iNum) const override;

    const Type* GetFieldDetails(TypeFieldName iFieldName) const override;

    typedef Vector<FieldDesc> FieldVector;
    FieldVector m_Fields;
  };

#include "tupletypestruct.inl"

}
