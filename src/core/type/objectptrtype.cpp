/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/type/objectptrtype.hpp>
#include <core/type/typemanager.hpp>
#include <core/type/classtype.hpp>
#include <core/lua/luaconverter.hpp>

namespace eXl
{

#if 0
  IMPLEMENT_RTTI(ObjectPtrType);

  ObjectPtrType::ObjectPtrType()
    :CoreType(TypeTraits::GetName<RttiOPtr>(),
          0,
          TypeTraits::GetSize<RttiOPtr>(),
          Type_Is_CoreType)
    ,m_Rtti(RttiObjectRefC::StaticRtti())
  {
    //m_Flags |= Dynamic;
  }

  ObjectPtrType::ObjectPtrType(Rtti const& iRtti)
    : CoreType(TypeName(TypeTraits::GetName<RttiOPtr>() + iRtti.GetName()),
          0,
          TypeTraits::GetSize<RttiOPtr>(),
          Type_Is_CoreType)
    ,m_Rtti(iRtti)
  {
    //m_Flags |= Dynamic;
  }
  
  void* ObjectPtrType::Alloc()const
  {
    return TypeTraits::Alloc<RttiOPtr>();
  }

  
  void ObjectPtrType::Free(void* iObj)const
  {
    TypeTraits::Free<RttiOPtr>(iObj);
  }

  
  void* ObjectPtrType::Construct(void* iObj)const
  {
    void* obj = TypeTraits::DefaultCTor<RttiOPtr>(iObj);
    ClassType const* classType = TypeManager::GetClassForRtti(m_Rtti);
    if(classType)
    {
      void* newObj = nullptr;
      classType->Build(newObj, nullptr);
      if(newObj != nullptr)
      {
        *reinterpret_cast<RttiOPtr*>(obj) = reinterpret_cast<RttiObjectRefC*>(newObj);
      }
    }
    return obj;
  }
  
  void ObjectPtrType::Destruct(void* iObj)const
  {
    TypeTraits::DTor<RttiOPtr>(iObj);
  }
  
  Err ObjectPtrType::Unstream_Uninit(void* oData, Unstreamer* iUnstreamer)const
  {
    Err err = iUnstreamer->BeginStruct();
    if(err)
      err = iUnstreamer->PushKey("ObjectPointer");
    String tempVal;
    if(err)
      err = iUnstreamer->ReadString(&tempVal);
    if(StringUtil::ToASCII(tempVal) != m_Rtti.GetName())
    {
      LOG_ERROR<<"Wrong pointer class : " << tempVal <<" expected "<<m_Rtti.GetName() <<"\n";
      err = Err::Error;
    }
    if(err)
      err = iUnstreamer->PopKey();
    if(err)
      err = iUnstreamer->PushKey("Value");

    TypeTraits::DefaultCTor<RttiOPtr>(oData);
    void* obj = nullptr;
    if(err)
    {
      err = ClassType::DynamicUnstream(obj,iUnstreamer);
      if(err)
      {
        if(obj != nullptr)
          *reinterpret_cast<RttiOPtr*>(oData) = reinterpret_cast<RttiObjectRefC*>(obj);
        else
          err = Err::Error;
        if(err)
          err = iUnstreamer->PopKey();
      }
    }
    if(err)
      err = iUnstreamer->EndStruct();

    return err;
  }
  
  Err ObjectPtrType::Stream(void const* iData, Streamer* iStreamer)const
  {
    
    RttiObject const* obj = reinterpret_cast<RttiOPtr const*>(iData)->get();
    ClassType const* classType;
    Err err = iStreamer->BeginStruct();
    if(err)
      err = iStreamer->PushKey("ObjectPointer");
    if(err)
      err = iStreamer->WriteString(StringUtil::FromASCII(m_Rtti.GetName()));
    if(err)
      err = iStreamer->PopKey();
    if(err)
    {
      if(obj != nullptr)
      {
        classType = nullptr;
        //classType = obj->GetClassType();
        if(classType == nullptr)
          err = Err::Error;
        if(err)
          err = iStreamer->PushKey("Value");
        if(err)
          err = classType->Stream(obj,iStreamer);
        if(err)
          err = iStreamer->PopKey();
      }
    }

    if(err)
      err = iStreamer->EndStruct();

    return err;
  }

  Err ObjectPtrType::Copy_Uninit(void const* iData, void* oData) const
  {
    if(iData != nullptr)
    {
      RttiObject const* origObj = reinterpret_cast<RttiOPtr const*>(iData)->get();
      if(origObj != nullptr)
      {
        //ClassType const* classType = origObj->GetClassType();
        ClassType const* classType = nullptr;
        if(classType)
        {
          void* obj = classType->Copy(origObj);
          if(obj != nullptr)
          {
            TypeTraits::DefaultCTor<RttiOPtr>(oData);
            *reinterpret_cast<RttiOPtr*>(oData) = reinterpret_cast<RttiObjectRefC*>(obj);
            RETURN_SUCCESS;
          }
        }
      }
    }
    RETURN_FAILURE;
  }
#ifdef EXL_LUA
  
  luabind::object ObjectPtrType::ConvertToLua(void const* iObj,lua_State* iState)const
  {
    if(iState==nullptr)
      return luabind::object();
    return eXl::LuaConverter<RttiOPtr>::ConvertToLua(iObj,this,iState);
  }

  
  Err ObjectPtrType::ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const
  {
    if(iState==nullptr)
      RETURN_FAILURE;
    if(oObj==nullptr)
      RETURN_FAILURE;

    eXl::LuaConverter<RttiOPtr>::ConvertFromLua(this,oObj,iState,ioIndex);
  
    RETURN_SUCCESS;
  }
#endif
  Err ObjectPtrType::Compare(void const* iVal1, void const* iVal2, CompRes& oRes)const
  {
    if(iVal1 == nullptr || iVal2 == nullptr)
      RETURN_FAILURE;

    oRes = TypeTraits::Compare<RttiOPtr>(iVal1,iVal2);

    RETURN_SUCCESS;
  }

  bool ObjectPtrType::CanAssignFrom(Type const* iOtherType) const
  {
    if(iOtherType == this)
      return true;
    else
    {
      ObjectPtrType const* otherObject = ObjectPtrType::DynamicCast(iOtherType);
      if(otherObject)
      {
        if(otherObject->GetMinimalRtti().IsKindOf(m_Rtti))
        {
          return true;
        }
      }
      else
      {
        return false;
      }
    }
    return false;
  }

  Err ObjectPtrType::Assign_Uninit(Type const* inputType, void const* iData, void* oData) const
  {
    if(iData != nullptr && oData != nullptr && CanAssignFrom(inputType))
    {
      TypeTraits::Copy<RttiOPtr>(oData,iData);
      RETURN_SUCCESS;
    }
    RETURN_FAILURE;
  }

  bool ObjectPtrType::Isnullptr(void const* iData) const
  {
    if(iData)
    {
      return reinterpret_cast<RttiOPtr const*>(iData)->get() == nullptr;
    }
    return true;
  }

  ClassType const* ObjectPtrType::GetObjectType(void const* iData) const
  {
    if(iData)
    {
      RttiOPtr const* ptr = reinterpret_cast<RttiOPtr const*>(iData);
      if(ptr != nullptr && ptr->get() != nullptr)
      {
        //return ptr->get()->GetClassType();
        return nullptr;
      }
    }
    return nullptr;
  }

  Rtti const* ObjectPtrType::GetObjectRtti(void const* iData) const
  {
    if(iData)
    {
      RttiOPtr const& ptr = *reinterpret_cast<RttiOPtr const*>(iData);
      if(ptr != nullptr)
      {
        return &ptr->GetRtti();
      }
    }
    return nullptr;
  }
#endif
}