/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>
#include <core/heapobject.hpp>
#include <core/refcobject.hpp>

#include <core/type/typedefs.hpp>
#include <core/type/typemanager_get.hpp>

#ifdef EXL_LUA
extern "C"
{
  struct lua_State;
}

namespace luabind
{
  namespace adl
  {
    class object;
  }
}
#endif
namespace eXl
{
  class Type;

  class DynObject;

  /**
     Class used to manipulate objects allocated through the reflexion mechanism. This object has a const semantic.
  **********************************************************************/
  class EXL_CORE_API ConstDynObject : public HeapObject
  {
  public:
    ConstDynObject();

    ConstDynObject(const Type* iType,void const* iBuffer);
    ~ConstDynObject();
    ConstDynObject(ConstDynObject&&);

    ConstDynObject ConstRef() const;

    /**
       Check whether the object is valid, that is to say, has a type and an allocated buffer.
    **********************************************************************/
    inline bool IsValid()const{return m_Type!=0 && m_Mem!=0;}

    void SetTypeConst(const Type* iType,void const* iMem);
    
    /**
       Cast the content of the buffer to a particular type. Can only be used with types which declared a TypeId(see typetraits.hpp)
    **********************************************************************/
    template <class T>
    const T* CastBuffer() const;

    //void ToLua(luabind::adl::object& iObj) const;
#ifdef EXL_LUA
    luabind::adl::object ToLua(lua_State* iState) const;
#endif

    Err GetField(unsigned int i,ConstDynObject& oObj)const;

    Err GetField(TypeFieldName iName,ConstDynObject& oObj)const;

    Err GetField(unsigned int i,DynObject& oObj)const;

    Err GetField(TypeFieldName iName,DynObject& oObj)const;

    Err GetElement(unsigned int i,ConstDynObject& oObj)const;

    Err GetElement(unsigned int i,DynObject& oObj)const;

    template <class T>
    inline T const* GetFieldCheckPtr(unsigned int i)const{return I_GetFieldCheckPtr<T>(i);}

    template <class T>
    inline T const* GetField(unsigned int i)const{return I_GetField<T>(i);}

    template <class T>
    inline T const* GetFieldCheckPtr(TypeFieldName iName)const{return I_GetFieldCheckPtr<T>(iName);}

    template <class T>
    inline T const* GetField(TypeFieldName iName)const{return I_GetField<T>(iName);}
    
    const Type* GetType()const;
    
    inline const void* GetBuffer()const{return m_Mem;}

  protected:

    ConstDynObject(ConstDynObject const&) = delete;

    ConstDynObject& operator=(ConstDynObject const&) = delete;

    template <class T>
    inline T* I_GetFieldCheckPtr(unsigned int i)const;
    
    template <class T>
    inline T* I_GetField(unsigned int i)const;

    template <class T>
    inline T* I_GetFieldCheckPtr(TypeFieldName iName)const;
    
    template <class T>
    inline T* I_GetField(TypeFieldName iName)const;

    void* m_Mem;
    const Type* m_Type;
  };

  /**
     Class used to manipulate objects allocated through the reflexion mechanism.
  **********************************************************************/
  class EXL_CORE_API DynObject : public ConstDynObject
  {
  public:

    template <class T>
    static Err BuildFrom(DynObject const& oObj, T& iVal);

    /**
       The object is not valid.
    **********************************************************************/
    DynObject();
    /**
      Copy iObj's data
    **********************************************************************/
    /*explicit*/ DynObject(ConstDynObject const* iObj);
    /*explicit*/ DynObject(DynObject const& iObj);

    /**
      Move the other object's data
    **********************************************************************/
    DynObject(DynObject&&);
    DynObject& operator=(DynObject&&);

    DynObject& operator=(DynObject const&);
    /**
       The object does not own the buffer.
    **********************************************************************/
    DynObject(const Type* iType,void* iBuffer);
#ifdef EXL_LUA
    DynObject(const Type* iType, luabind::adl::object const& iObj);
#endif

    DynObject Ref() const;

    /**
       If the object owns its buffer, it's freed.
    **********************************************************************/
    ~DynObject();

    void Swap(DynObject& iOther);

    void Release();

    /**
       Cast the content of the buffer to a particular type. Can only be used with types which declared a TypeId(see typetraits.hpp)
    **********************************************************************/
    template <class T>
    T* CastBuffer();

    template <class T>
    const T* CastBuffer() const
    {
      return ConstDynObject::CastBuffer<T>();
    }

#ifdef EXL_LUA
    void FromLua(const Type* iType, luabind::adl::object const& iObj);
#endif

    Err GetField(unsigned int i,DynObject& oObj);

    Err GetField(TypeFieldName iName,DynObject& oObj);

    Err GetElement(unsigned int i,DynObject& oObj);

    Err SetArraySize(unsigned int iSize);

    template <class T>
    inline T* GetFieldCheckPtr(unsigned int i){return ConstDynObject::I_GetFieldCheckPtr<T>(i);}
    
    template <class T>
    inline T* GetField(unsigned int i){return ConstDynObject::I_GetField<T>(i);}

    template <class T>
    inline T* GetFieldCheckPtr(TypeFieldName iName){return ConstDynObject::I_GetFieldCheckPtr<T>(iName);}
    
    template <class T>
    inline T* GetField(TypeFieldName iName){return ConstDynObject::I_GetField<T>(iName);}

    inline void* GetBuffer(){return m_Mem;}

    inline void const* GetBuffer()const{return m_Mem;}

    /**
       Checks whether the object owns its buffer.
    **********************************************************************/
    bool IsOwner()const;

    /**
       Set the content of the object.
       @param iType The type of the object.
       @param iMem The object memory location
       @param makeOwner Whether the DynObject should own the storage.
    **********************************************************************/
    void SetType(const Type* iType,void * iMem,bool makeOwner = false);
  };
}
#include <core/type/tupletype.hpp>
namespace eXl
{  
  template <class T>
  const T* ConstDynObject::CastBuffer()const
  {
    if (GetType() == nullptr || GetBuffer() == nullptr)
    {
      return nullptr;
    }
    if (GetType() != TypeManager::GetType<T>())
    {
      return nullptr;
    }
    return (const T*)GetBuffer();
  }

  template <class T>
  T* DynObject::CastBuffer()
  {
    if (GetType() == nullptr || GetBuffer() == nullptr)
    {
      return nullptr;
    }
    if (GetType() != TypeManager::GetType<T>())
    {
      return nullptr;
    }
    return (T*)GetBuffer();
  }

  template <class T>
  inline T* ConstDynObject::I_GetFieldCheckPtr(unsigned int i)const
  {
    const TupleType* tupleType = GetType()->IsTuple();
    eXl_ASSERT_MSG(tupleType!=nullptr,"Not a tupletype");
    Type const* fieldType;
    void* res=tupleType->GetField(m_Mem,i,fieldType);
    eXl_ASSERT_MSG(res!=nullptr,"Could not get field");
    eXl_ASSERT_MSG(fieldType == TypeManager::GetType<T>(),"Invalid cast for field");
    return (T*)res;
  }
    
  template <class T>
  inline T* ConstDynObject::I_GetField(unsigned int i)const
  {
    T* res = nullptr;
    const TupleType* tupleType = GetType()->IsTuple();
    if(tupleType!=nullptr)
    {
      Type const* fieldType;
      res=(T*)tupleType->GetField(m_Mem,i,fieldType);
      if(res != nullptr)
      {
        if(fieldType != TypeManager::GetType<T>()){
          res = nullptr;
        }
      }
    }
    return res;
  }

  template <class T>
  inline T* ConstDynObject::I_GetFieldCheckPtr(TypeFieldName iName)const
  {
    const TupleType* tupleType = GetType()->IsTuple();
    eXl_ASSERT_MSG(tupleType!=nullptr,"Not a tupletype");
    Type const* fieldType;
    void* res=tupleType->GetField(m_Mem,iName,fieldType);
    eXl_ASSERT_MSG(res!=nullptr,"Could not get field");
    eXl_ASSERT_MSG(fieldType == TypeManager::GetType<T>(),"Invalid cast for field");
    return (T*)res;
  }
    
  template <class T>
  inline T* ConstDynObject::I_GetField(TypeFieldName iName)const
  {
    T* res = nullptr;
    const TupleType* tupleType = GetType()->IsTuple();
    if(tupleType!=nullptr)
    {
      Type const* fieldType;
      res=(T*)tupleType->GetField(m_Mem,iName,fieldType);
      if(res != nullptr)
      {
        if(fieldType != TypeManager::GetType<T>())
        {
          res = nullptr;
        }
      }
    }
    return res;
  }
}
//#include <core/typemanager.hpp>
namespace eXl
{
  template <class T>
  static Err BuildFrom(DynObject const& oObj, T& iVal);
}
