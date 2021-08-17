/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/type/classtype.hpp>
#include <core/type/typemanager.hpp>
#include <core/type/tupletypestruct.hpp>
#include <core/type/dynobject.hpp>
#include <core/rtti.hpp>

namespace eXl
{
  Rtti const& ClassType::StaticRtti()
  {
    static Rtti s_Rtti("ClassType", &Type::StaticRtti());
    return s_Rtti;
  }

  Type const* ClassType::GetType()
  {
    static ClassType s_Class("ClassType", StaticRtti(), ClassType::DynamicCast(Type::GetType()));

    return &s_Class;
  }

  const Rtti& ClassType::GetRtti() const
  {
    return StaticRtti();
  }

  ClassType* ClassType::DynamicCast(RttiObject* ptr)
  {
    if (ptr == nullptr)
    {
      return nullptr;
    }
    if (ptr->GetRtti() == ptr->StaticRtti())
    {
      return static_cast<ClassType*>(ptr);
    }
    return nullptr;
  }

  const ClassType* ClassType::DynamicCast(const RttiObject* ptr)
  {
    if (ptr == nullptr)
    {
      return nullptr;
    }
    if (ptr->GetRtti() == ptr->StaticRtti())
    {
      return static_cast<ClassType const*>(ptr);
    }
    return nullptr;
  }

  ClassType::ClassType(TypeName const& iName, Rtti const& iClassRtti, ClassType const* iParentRttiClass)
    : Type(iName, 0, 0, Type_Is_TagType)
    , m_ParentRttiClass(iParentRttiClass)
    , m_ClassRtti(iClassRtti)
  {
    if (iParentRttiClass)
    {
      eXl_ASSERT_MSG(iClassRtti.IsKindOf(iParentRttiClass->GetClassRtti()), "Error in inheritance");
    }
  }

  ClassType::~ClassType()
  {

  }
#if 0
  bool ClassType::IsAbstract() const
  {
    return false;
  }

  Err ClassType::RetrieveDesc(void const* iObj, void*& oDesc) const
  {
    if(m_StreamingStruct != nullptr && iObj)
    {
      if(oDesc == nullptr)
      {
        oDesc = m_StreamingStruct->Alloc();
      }
      else
      {
        m_StreamingStruct->Destroy(oDesc);
      }
      if(oDesc != nullptr)
      {
        return RetrieveDesc_Uninit(iObj,oDesc);
      }
    }
    RETURN_FAILURE;
  }

  Err ClassType::DynamicUnstream(void*& oData, Unstreamer* iUnstreamer)
  {
    Err err = iUnstreamer->BeginStruct();
    if(err)
      err = iUnstreamer->PushKey("ClassName");
    String tempVal;
    if(err)
      err = iUnstreamer->ReadString(&tempVal);
    if(err)
      err = iUnstreamer->PopKey();
    if(err)
      err = iUnstreamer->EndStruct();
    if(err)
    {
      ClassType const* classType = TypeManager::GetClassForName(TypeName(StringUtil::ToASCII(tempVal)));
      if(classType)
      {
        void* obj = nullptr;
        classType->Unstream(obj,iUnstreamer);

        oData = obj;
        if(obj == nullptr)
          err = Err::Error;
      }
      else
        err = Err::Error;
    }
    return err;
  }

  Err ClassType::Unstream(void*& oData, Unstreamer* iUnstreamer) const
  {
    oData = nullptr;
    Err err = iUnstreamer->BeginStruct();
    if(err)
      err = iUnstreamer->PushKey("ClassName");
    String tempVal;
    if(err)
      err = iUnstreamer->ReadString(&tempVal);
    if(err)
      err = iUnstreamer->PopKey();
    if(StringUtil::ToASCII(tempVal) != m_ClassRtti.GetName())
    {
      LOG_ERROR<<"Unexpected class data : "<<tempVal <<" expected "<<m_ClassRtti.GetName();
      err = Err::Error;
    }

    if(err)
      err = Build(oData,nullptr);

    if(err)
    {
      err = iUnstreamer->PushKey("Value");
      if(err)
        //err = reinterpret_cast<RttiObject*>(oData)->Unstream(this,iUnstreamer);
      if(err)
        err = iUnstreamer->PopKey();
    }
    else if(m_StreamingStruct)
    {
      err = iUnstreamer->PushKey("Value");

      DynObject temp;
      temp.SetType(m_StreamingStruct,m_StreamingStruct->Alloc(),true);

      if(err)
        err = m_StreamingStruct->Unstream_Uninit(temp.GetBuffer(),iUnstreamer);
      if(err)
        err = Build(oData,temp.GetBuffer());
      if(err)
        err = iUnstreamer->PopKey();
    }
    if(err)
      err = iUnstreamer->EndStruct();

    return err;
  }

  Err ClassType::Stream(void const* iData, Streamer* iStreamer) const  
  {
    Err err = Err::Failure;
    if(iData != nullptr)
    {
      err = iStreamer->BeginStruct();
      if(err)
      {
        err = iStreamer->PushKey("ClassName");
      }
      if(err)
      {
        err = iStreamer->WriteString(StringUtil::FromASCII(m_ClassRtti.GetName()));
      }
      if(err)
      {
        err = iStreamer->PopKey();
      }
      if(err)
      {
        RttiObject const* obj = reinterpret_cast<RttiObject const*>(iData);
        err = iStreamer->PushKey("Value");
        if(err)
          //err = obj->Stream(this,iStreamer);
        if(err)
          err = iStreamer->PopKey();
      }
      if(err)
      {
        err = iStreamer->EndStruct();
      }
    }
    return err;
  }
#endif
}
