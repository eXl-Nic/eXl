/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/type/dynobject.hpp>
#include <core/type/type.hpp>
#include <core/type/arraytype.hpp>

#ifdef EXL_LUA
#include <core/lua/luamanager.hpp>
#endif

#define MAKE_OWNER(x) (Type const*)((size_t)(x) | 1)
#define GET_PTR(x) (Type const*)((size_t)(x) & ~3)
#define IS_OWNER(x) (((size_t)(x) & 3)==1)


namespace eXl
{
  ConstDynObject::ConstDynObject()
    : m_Mem(nullptr)
    , m_Type(nullptr)
  {
  }

  ConstDynObject::ConstDynObject(const Type* iType,void const* iBuffer)
    : m_Mem((void*)iBuffer)
    , m_Type(iType)
  {
  }

  ConstDynObject::~ConstDynObject()
  {
    if(IS_OWNER(m_Type) && m_Type!=nullptr && m_Mem)
    {
      GetType()->Destroy(m_Mem);
    }
  }

  ConstDynObject ConstDynObject::ConstRef() const
  {
    ConstDynObject ref;
    ref.m_Type = GET_PTR(m_Type);
    ref.m_Mem = m_Mem;

    return std::move(ref);
  }

  void ConstDynObject::SetTypeConst(const Type* iType,void const* iMem)
  {
    if(IS_OWNER(m_Type) && m_Type!=nullptr && m_Mem)
    {
      GetType()->Destroy(m_Mem);
    }

    m_Mem = (void*)iMem;
    m_Type = iType;
  }

  const Type* ConstDynObject::GetType()const{return GET_PTR(m_Type);}

  Err ConstDynObject::GetField(unsigned int i,ConstDynObject& oObj)const
  {
    if(!IsValid())
      RETURN_FAILURE;

    const TupleType* tupleType = GetType()->IsTuple();
    if(tupleType!=nullptr)
    {
      Type const* fieldType;
      void const* field=tupleType->GetField(GetBuffer(),i,fieldType);
      if(field != nullptr)
      {
        oObj.SetTypeConst(fieldType,field);
        RETURN_SUCCESS;
      }
    }
    RETURN_FAILURE;
  }

  Err ConstDynObject::GetField(TypeFieldName iName,ConstDynObject& oObj)const
  {
    if(!IsValid())
      RETURN_FAILURE;

    const TupleType* tupleType = GetType()->IsTuple();
    if(tupleType!=nullptr)
    {
      Type const* fieldType;
      void const* field=tupleType->GetField(GetBuffer(), iName, fieldType);
      if(field != nullptr)
      {
        oObj.SetTypeConst(fieldType,field);
        RETURN_SUCCESS;
      }
    }
    RETURN_FAILURE;
  }

  Err ConstDynObject::GetField(unsigned int i,DynObject& oObj)const
  {
    RETURN_FAILURE;
  }

  Err ConstDynObject::GetField(TypeFieldName iName,DynObject& oObj)const
  {
    RETURN_FAILURE;
  }

  Err ConstDynObject::GetElement(unsigned int i,ConstDynObject& oObj)const
  {
    if(!IsValid())
      RETURN_FAILURE;

    ArrayType const* arrayT = ArrayType::DynamicCast(GetType());
    if(arrayT != nullptr)
    {
      void const* elem = arrayT->GetElement(GetBuffer(),i);
      if(elem != nullptr)
      {
        oObj.SetTypeConst(arrayT->GetElementType(),elem);
        RETURN_SUCCESS;
      }
    }
    RETURN_FAILURE;
  }

  Err ConstDynObject::GetElement(unsigned int i,DynObject& oObj)const
  {
    RETURN_FAILURE;
  }

  ConstDynObject::ConstDynObject(ConstDynObject&& iOther)
  {
    m_Type = iOther.m_Type;
    m_Mem = iOther.m_Mem;
    iOther.m_Mem = nullptr;
    iOther.m_Type = nullptr;
  }

  DynObject::DynObject():ConstDynObject(nullptr,nullptr){}
  DynObject::DynObject(const Type* iType,void* iBuffer)
    : ConstDynObject(iType,iBuffer)
  {
  }

  DynObject::~DynObject()
  {
  }

  DynObject::DynObject(DynObject&& iOther)
    : ConstDynObject(std::move(iOther))
  {
  }

  DynObject& DynObject::operator=(DynObject&& iOther)
  {
    this->~DynObject();
    new(this) DynObject(std::move(iOther));

    return *this;
  }

  DynObject DynObject::Ref() const
  {
    DynObject ref;
    ref.m_Type = GET_PTR(m_Type);
    ref.m_Mem = m_Mem;

    return std::move(ref);
  }

  void DynObject::Release()
  {
    m_Type = nullptr;
    m_Mem = nullptr;
  }

#ifdef EXL_LUA
  DynObject::DynObject(const Type* iType, luabind::object const& iObj):ConstDynObject(iType,nullptr)
  {
    FromLua(iType,iObj);
  }
#endif
  void DynObject::Swap(DynObject& iOther)
  {
    void* tempMem = iOther.m_Mem;
    Type const* tempType = iOther.m_Type;
    iOther.m_Mem = m_Mem;
    iOther.m_Type = m_Type;
    m_Mem = tempMem;
    m_Type = tempType;
  }

  DynObject::DynObject(ConstDynObject const* iObj):ConstDynObject(nullptr,nullptr)
  {
    if(iObj != nullptr)
    {
      Type const* type = iObj->GetType();
      void* data = nullptr;
      if(type != nullptr && iObj->IsValid())
      {
        data = type->Alloc();
        type->Assign_Uninit(type,iObj->GetBuffer(),data);
      }
      SetType(type,data,true);
    }
  }

  DynObject::DynObject(DynObject const& iObj)
  {
    Type const* type = iObj.GetType();
    void* data = nullptr;
    if(type != nullptr && iObj.IsValid())
    {
      data = type->Alloc();
      type->Assign_Uninit(type,static_cast<ConstDynObject const&>(iObj).GetBuffer(), data);
    }
    SetType(type,data,true);
  }

  DynObject& DynObject::operator=(DynObject const& iObj)
  {
    Type const* type = iObj.GetType();
    void* data = nullptr;
    if(type != nullptr && iObj.IsValid())
    {
      data = type->Alloc();
      type->Assign_Uninit(type,((ConstDynObject const&)iObj).GetBuffer(),data);
    }
    SetType(type,data,true);

    return *this;
  }

  bool DynObject::IsOwner()const{return IS_OWNER(m_Mem);}
  
  void DynObject::SetType(const Type* iType,void* iMem,bool makeOwner)
  {
    if(IS_OWNER(m_Type) && m_Type!=nullptr && m_Mem != nullptr)
    {
      GetType()->Destroy(m_Mem);
    }
    
    if(iType != nullptr && iMem != nullptr)
    {
      m_Mem = iMem;
      if(makeOwner && iMem!=nullptr)
      {
        m_Type = MAKE_OWNER(iType);
      }
      else
      {
        m_Type = iType;
      }
    }
    else
    {
      m_Type = iType;
      m_Mem = nullptr;
    }
  }

  Err DynObject::GetField(unsigned int i,DynObject& oObj)
  {
    if(!IsValid())
      RETURN_FAILURE;
    const TupleType* tupleType = GetType()->IsTuple();
    if(tupleType!=nullptr)
    {
      Type const* fieldType;
      void* field=tupleType->GetField(GetBuffer(),i,fieldType);
      if(field != nullptr)
      {
        oObj.SetType(fieldType,field,false);
        RETURN_SUCCESS;
      }
    }
    RETURN_FAILURE;
  }

  Err DynObject::GetField(TypeFieldName iName,DynObject& oObj)
  {
    if(!IsValid())
      RETURN_FAILURE;
    const TupleType* tupleType = GetType()->IsTuple();
    if(tupleType!=nullptr)
    {
      Type const* fieldType;
      void* field=tupleType->GetField(GetBuffer(),iName,fieldType);
      if(field != nullptr)
      {
        oObj.SetType(fieldType,field,false);
        RETURN_SUCCESS;
      }
    }
    RETURN_FAILURE;
  }

  Err DynObject::GetElement(unsigned int i,DynObject& oObj)
  {
    if(!IsValid())
      RETURN_FAILURE;

    ArrayType const* arrayT = ArrayType::DynamicCast(GetType());
    if(arrayT != nullptr)
    {
      void* elem = arrayT->GetElement(GetBuffer(),i);
      if(elem != nullptr)
      {
        oObj.SetType(arrayT->GetElementType(),elem,false);
        RETURN_SUCCESS;
      }
    }
    RETURN_FAILURE;
  }

#ifdef EXL_LUA
  void DynObject::FromLua(const Type* iType, luabind::object const& iObj)
  {
    if(iType != nullptr && iObj.is_valid())
    {
      if(!IsValid() || IsOwner())
      {
        void* newMem = nullptr;
        Err err = iType->ConvertFromLua(iObj,newMem);
        if(err)
        {
          SetType(iType,newMem,true);
        }
      }
      else
      {
        if(iType == GetType())
        {
          void* mem = GetBuffer();
          Err err = iType->ConvertFromLua(iObj,mem);
        }
        else
        {
          LOG_WARNING << "Err : conversion failed" << "\n";
        }
      }
    }
  }
#endif
  /*void ConstDynObject::ToLua(luabind::object& iObj)const
  {
    lua_State* interpreter = iObj.interpreter();
    if(interpreter == nullptr)
    {
      interpreter = LuaManager::GetLocalState();
    }

    eXl_ASSERT_MSG(interpreter != nullptr, "Could not get interpreter");

    if(interpreter != nullptr)
    {
      if(IsValid())
      {
        iObj = GetType()->ConvertToLua(GetBuffer(),interpreter);
      }
      else
      {
        iObj = luabind::object();
      }
    }
  }*/
#ifdef EXL_LUA
  luabind::object ConstDynObject::ToLua(lua_State* interpreter)const
  {
    /*lua_State* interpreter = iObj.interpreter();
    if(interpreter == nullptr)
    {
      interpreter = LuaManager::GetLocalState();
    }*/

    eXl_ASSERT_MSG(interpreter != nullptr, "Could not get interpreter");

    if(interpreter != nullptr)
    {
      if(IsValid())
      {
        return GetType()->ConvertToLua(GetBuffer(),interpreter);
      }
    }
    return luabind::object();
  }
#endif
  Err DynObject::SetArraySize(unsigned int iSize)
  {
    ArrayType const* arrayType = ArrayType::DynamicCast(GetType());
    if(arrayType == nullptr)
      RETURN_FAILURE;

    return arrayType->SetArraySize(GetBuffer(),iSize);
  }
}
