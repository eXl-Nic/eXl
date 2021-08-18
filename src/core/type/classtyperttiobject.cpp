/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/type/classtyperttiobject.hpp>
#include <core/type/typemanager.hpp>
#include <core/type/tupletypestruct.hpp>

namespace eXl
{
#if 0
  IMPLEMENT_RTTI(ClassTypeRttiObject);

  ClassTypeRttiObject::ClassTypeRttiObject(TypeName const& iName, Rtti const& iClassRtti, ClassType const* iParentRttiClass, ObjectFactory iFactory)
      : ClassType(iName,iClassRtti,iParentRttiClass)
      , m_Factory(iFactory)
      , m_Offset(0)
  {
    ComputeStreamingStruct();
    if(iParentRttiClass != nullptr)
    {
      //m_Offset = iParentRttiClass->GetNumProps();
    }
  }

  bool ClassTypeRttiObject::IsAbstract() const
  {
    return m_Factory == nullptr;
  }

  Err ClassTypeRttiObject::Build(void*& ioObj, void const* iDesc) const
  {
    Err err = Err::Failure;

    if(ioObj == nullptr)
    {
      if(m_Factory)
      {
        ioObj = m_Factory();
        if(ioObj != nullptr)
        {
          err = Err::Success;
        }
      }
    }
    else
    {
      err = Err::Success;
    }
    if(err)
    {
      if(iDesc != nullptr)
      {
        if(m_StreamingStruct)
        {
          unsigned int fieldOffset = 0;
          if(m_ParentRttiClass)
          {
            if(m_ParentRttiClass->GetStreamingStruct())
              fieldOffset = m_ParentRttiClass->GetStreamingStruct()->GetNumField();
            err = GetParentRttiClass()->Build(ioObj, iDesc);
          }
          if(err)
          {
            //unsigned int numProps = m_Properties.size();
            //
            //for(unsigned int i = 0; i<numProps; ++i)
            //{
            //  Type const* fieldType;
            //  void const* fieldData = m_StreamingStruct->GetField(iDesc,i+fieldOffset,fieldType);
            //  if(fieldData != nullptr)
            //  {
            //    err = SetProperty(i,ioObj,fieldData);
            //    if(!EC_SUCCESS(err))
            //      break;
            //  }
            //  else
            //  {
            //    err = EC_Error;
            //    break;
            //  }
            //}
        
          }
        }
        else
        {
          LOG_WARNING << "Description provided for a class that does not have a streaming structure"<<"\n";
        }
        //else
        //  err = EC_Undefined;
      }
    }
    return err;
  }

  Err ClassTypeRttiObject::RetrieveDesc_Uninit(void const* iObj, void* oDesc) const
  {
    Err err = Err::Error;
    if(m_StreamingStruct)
    {
      unsigned int fieldOffset = 0;
      if(m_ParentRttiClass)
      {
        if(m_ParentRttiClass->GetStreamingStruct())
          fieldOffset = m_ParentRttiClass->GetStreamingStruct()->GetNumField();
        err = GetParentRttiClass()->RetrieveDesc_Uninit(iObj,oDesc);
      }
      else
        err = Err::Success;
      if(err)
      {
        //unsigned int numProps = GetNumProps();
        //
        //for(unsigned int i = 0; i<numProps; ++i)
        //{
        //  Type const* fieldType;
        //  void* fieldData = m_StreamingStruct->GetField(oDesc,i + fieldOffset,fieldType);
        //  if(fieldData != nullptr)
        //  {
        //    fieldType->Construct(fieldData);
        //    err = GetProperty(i,iObj,fieldData);
        //    if(!EC_SUCCESS(err))
        //      break;
        //  }
        //  else
        //  {
        //    err = EC_Error;
        //    break;
        //  }
        //}
      }
    }
    return err;
  }

  void* ClassTypeRttiObject::Copy(void const* iObject) const
  {
    if(iObject)
    {
      return reinterpret_cast<RttiObject const*>(iObject)->Duplicate();
    }
    return nullptr;
  }

  void ClassTypeRttiObject::Destroy(void* iObj) const
  {
    if(iObj)
    {
      RttiObject* object = reinterpret_cast<RttiObject*>(iObj);
      if(object->GetRtti().IsKindOf(RttiObjectRefC::StaticRtti()))
      {
        eXl_ASSERT_MSG(false, "Cannot destroy a refcounted object by this mean.");
      }
      else
      {
        eXl_DELETE object;
      }
    }
  }

  void ClassTypeRttiObject::ComputeStreamingStruct()
  {
    //unsigned int numProps = GetNumProps();
    //
    //std::list<FieldDesc> fieldList;
    //
    //unsigned int currentOffset = 0;
    //if(GetParentRttiClass() && GetParentRttiClass()->m_StreamingStruct != nullptr)
    //{
    //  TupleType const* parentStruct = GetParentRttiClass()->m_StreamingStruct;
    //  for(unsigned int i = 0; i<parentStruct->GetNumField(); ++i)
    //  {
    //    std::string fieldName;
    //    Type const* fieldType = parentStruct->GetFieldDetails(i,fieldName);
    //    fieldList.push_back(FieldDesc(fieldName,currentOffset,fieldType));
    //    currentOffset += fieldType->GetSize();
    //  }
    //}
    //
    //for(unsigned int i = 0; i<numProps; ++i)
    //{
    //  Type const* propType = GetPropertyType(i);
    //  std::string const* propName = GetPropertyName(i);
    //  fieldList.push_back(FieldDesc(*propName,currentOffset,propType));
    //  currentOffset += propType->GetSize();
    //}
    //
    //m_StreamingStruct = TupleTypeStruct::Create(fieldList,GetName()+"StreamingStruct");
  }
#endif
}
