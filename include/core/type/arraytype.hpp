/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#pragma once

#include <core/type/type.hpp>
#include <core/rtti.hpp>

namespace eXl
{
  class ClassType;

  class EXL_CORE_API ArrayType : public Type
  {
    DECLARE_RTTI(ArrayType, Type);
  public:

    virtual unsigned int GetArraySize(void const* iObj) const = 0;

    virtual void SetArraySize_Uninit(void* iObj, unsigned int iSize) const = 0;

    virtual void* GetElement(void* iObj, unsigned int i) const = 0;

    void Destruct(void* iObj)const override;

    Err SetArraySize(void* iObj, unsigned int iSize) const;

    inline void const* GetElement(void const* iObj, unsigned int i) const
    {
      return GetElement((void*)iObj, i);
    }

    inline Type const* GetElementType()const
    {
      return m_ElemType;
    }

    Err Compare(void const* iVal1, void const* iVal2, CompRes& oRes)const override;

    Err Copy_Uninit(void const* iData, void* oData) const override;

#ifdef EXL_LUA

    luabind::object ConvertToLua(void const* iObj, lua_State* iState)const override;

    Err ConvertFromLua_Uninit(lua_State* iState, unsigned int& ioIndex, void* oObj)const override;

    void RegisterLua(lua_State* iState) const override;

    //luabind::object MakePropertyAccessor(lua_State* iState, Type const* iHolder, uint32_t iOffset) const override;

#endif

    Err Unstream_Uninit(void* oData, Unstreamer* iUnstreamer) const override;

    Err Stream(void const* iData, Streamer* iStreamer) const override;

    bool CanAssignFrom(Type const* iType) const override;

    Err Assign_Uninit(Type const* inputType, void const* iData, void* oData) const override;

    TypeName GetDynamicName() const;

  protected:
    ArrayType(TypeName iName,
      size_t iTypeId,
      size_t iSize,
      unsigned int iFlags,
      Type const* iElemType);

    Type const* m_ElemType;
  };

  template <class T>
  class CoreArrayType : public ArrayType
  {
  public:

    CoreArrayType();
    CoreArrayType(Type const* iType);

    void* Alloc()const override;

    void Free(void* iObj)const override;

    void* Construct(void* iObj)const override;

    void Destruct(void* iObj)const override;

    unsigned int GetArraySize(void const* iObj) const override;

    void SetArraySize_Uninit(void* iObj, unsigned int iSize) const override;

    void* GetElement(void* iObj, unsigned int i) const override;
  };
}

#include <core/type/typemanager.hpp>

namespace eXl
{
  template <class T>
  CoreArrayType<T>::CoreArrayType()
    : ArrayType(TypeManager::GetType<T>()->GetName() + "_Array", 0, TypeTraits::GetSize<Vector<T>>(), Type_Is_CoreType, TypeManager::GetType<T>())
  {

  }

  template <class T>
  CoreArrayType<T>::CoreArrayType(Type const* iType)
    : ArrayType(iType->GetName() + "_Array", 0, TypeTraits::GetSize<Vector<T>>(), iType->IsCoreType() ? Type_Is_CoreType : 0, iType)
  {

  }

  template <class T>
  void* CoreArrayType<T>::Alloc()const
  {
    return TypeTraits::Alloc<Vector<T> >();
  }

  template <class T>
  void CoreArrayType<T>::Free(void* iObj)const
  {
    TypeTraits::Free<Vector<T> >(iObj);
  }

  template <class T>
  void* CoreArrayType<T>::Construct(void* iObj)const
  {
    return TypeTraits::DefaultCTor<Vector<T> >(iObj);
  }

  template <class T>
  void CoreArrayType<T>::Destruct(void* iObj)const
  {
    return TypeTraits::DTor<Vector<T> >(iObj);
  }

  template <class T>
  unsigned int CoreArrayType<T>::GetArraySize(void const* iObj) const
  {
    return ((Vector<T>*)iObj)->size();
  }

  template <class T>
  void CoreArrayType<T>::SetArraySize_Uninit(void* iObj,unsigned int iSize) const
  {
    ((Vector<T>*)iObj)->resize(iSize);
  }

  template <class T>
  void* CoreArrayType<T>::GetElement(void* iObj,unsigned int i) const
  {
    eXl_ASSERT_MSG(i<GetArraySize(iObj),"Bad Idx");
    return &((Vector<T>*)iObj)->at(i);
  }

  class EXL_CORE_API GnrArrayType : public ArrayType
  {
  public:
    GnrArrayType(TypeName iName,
                 Type const* iElemType);
    
    void* Alloc()const;

    void Free(void* iObj)const;

    void* Construct(void* iObj)const;

    void Destruct(void* iObj)const;

    //Overloaded for optim
    Err Copy_Uninit(void const* iData, void* oData) const;

    unsigned int GetArraySize(void const* iObj) const;

    void SetArraySize_Uninit(void* iObj,unsigned int iSize) const;

    void* GetElement(void* iObj,unsigned int i) const;
  };
}