/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/type/tupletype.hpp>
#include <string.h>

#include <core/stream/streamer.hpp>
#include <core/stream/unstreamer.hpp>

namespace eXl
{

  IMPLEMENT_RTTI(TupleType)

  TupleType::TupleType(TypeName iName,size_t iId,size_t iSize,unsigned int iFlags)
    : Type(iName,iId,iSize,iFlags)
  {
    
  }

#ifdef EXL_LUA
  Err TupleType::ConvertFromLuaRaw(lua_State* iState,unsigned int& ioIndex,void*& oObj)const
  {
    if(iState != nullptr)
    {
      if(oObj != nullptr)
      {
        Destruct(oObj);
      }
      else
      {
        oObj = Alloc();
      }
      if(oObj != nullptr)
      {
        return ConvertFromLuaRaw_Uninit(iState,ioIndex,oObj);
      }
    }
    RETURN_FAILURE;
  }
#endif
  Err TupleType::Compare(void const* iVal1, void const* iVal2, CompRes& oRes)const
  {
    if(iVal1 == nullptr || iVal2 == nullptr)
      RETURN_FAILURE;

    oRes = CompEqual;

    for(unsigned int i = 0 ; i<GetNumField() ; ++i)
    {
      Type const* fieldType;
      void const* field1 = GetField(iVal1,i,fieldType);
      void const* field2 = GetField(iVal2,i,fieldType);

      CompRes tempRes;
      Err err = fieldType->Compare(field1,field2,tempRes);



      if(tempRes != CompEqual)
      {
        oRes = CompDifferent;
        RETURN_SUCCESS;
      }
    }
    RETURN_SUCCESS;
  }

  Err TupleType::Unstream_Uninit(void* oData, Unstreamer* iUnstreamer) const
  {
    if (!IsPOD())
    {
      Construct(oData);
    }

    Err err = iUnstreamer->BeginStruct();

    unsigned int numFields = GetNumField();
    for(unsigned int i = 0; i<numFields; ++i)
    {
      Type const* fieldType = nullptr;
      void* fieldData = GetField(oData,i,fieldType);

      if(fieldData && fieldType)
      {
        TypeFieldName fieldName;
        if(GetFieldDetails(i,fieldName))
        {
          err = iUnstreamer->PushKey(StringUtil::FromASCII(fieldName));
          if(err)
          {
            if (IsPOD())
            {
              err = fieldType->Unstream_Uninit(fieldData, iUnstreamer);
            }
            else
            {
              err = fieldType->Unstream(fieldData, iUnstreamer);
            }
            if(err)
            {
              err = iUnstreamer->PopKey();
            }
          }
          else if(IsPOD())
          {
            fieldType->Construct(fieldData);
          }
        }
        else
          err = Err::Error;

        if(err != Err::Error && !err)
        {
          LOG_WARNING << "Field " << fieldName << " not found during unstream of "<< GetName() <<"\n";

          //LOG_ERROR << "Err while unstreaming field ") << fieldName << EXL_TEXT("\n";
          //break;
        }
      }
    }
    if(err != Err::Error)
    {
      err = iUnstreamer->EndStruct();
    }
    if(err == Err::Error)
    {
      LOG_ERROR << "Err while unstreaming " << GetName() << "\n";
    }
    else 
    {
      err = Err::Success;
    }
    
    return err;
  }

  Err TupleType::Stream(void const* iData, Streamer* iStreamer) const
  {
    if(iData == nullptr || iStreamer == nullptr)
      return Err::Error;

    Err err = Err::Failure;

    err = iStreamer->BeginStruct();
    if(err)
    {
      unsigned int numFields = GetNumField();
      for(unsigned int i = 0; i<numFields; ++i)
      {
        Type const* fieldType = nullptr;
        void const* fieldData = GetField(iData,i,fieldType);

        if(fieldData && fieldType)
        {
          TypeFieldName fieldName;
          if(GetFieldDetails(i,fieldName))
          {
            err = iStreamer->PushKey(StringUtil::FromASCII(fieldName));
            if(err)
            {
              err = fieldType->Stream(fieldData,iStreamer);
              if(err)
              {
                err = iStreamer->PopKey();
              }
            }
          }
          else
            err = Err::Error;

          if(!err)
          {
            LOG_ERROR << "Err while streaming field " << fieldName << "\n";
            break;
          }
        }
      }
    }
    if(err)
    {
      err = iStreamer->EndStruct();
    }
    if(!err)
    {
      LOG_ERROR << "Err while streaming " << GetName() << "\n";
    }
    return err;
  }

  bool TupleType::CanAssignFrom(Type const* iType) const
  {
    if(IsPOD())
    {
      return Type::CanAssignFrom(iType);
    }
    else if(iType != nullptr)
    {
      if(iType == this)
      {
        return true;
      }
      else
      {
        if(/*(m_TypeId & StorageFlag) &&*/ (iType->GetTypeId() == m_TypeId))
        {
          return true;
        }

        TupleType const* inputTuple = iType->IsTuple();
        if(inputTuple)
        {
          unsigned int numFields = GetNumField();
          unsigned int numFieldsOther = inputTuple->GetNumField();
          if(numFields == numFieldsOther)
          {
            for(unsigned int i = 0; i<numFields; ++i)
            {
              Type const* fieldType = GetFieldDetails(i);
              Type const* inputFieldType = inputTuple->GetFieldDetails(i);

              if(!fieldType->CanAssignFrom(inputFieldType))
                return false;
            }
            return true;
          }
        }
      }
    }
    return false;
  }

  Err TupleType::Assign_Uninit(Type const* iInputType, void const* iData, void* oData) const
  {
    if(IsPOD())
    {
      return Type::Assign_Uninit(iInputType,iData,oData);
    }
    else
    {
      Err err = Err::Failure;
      if(iData != nullptr && oData != nullptr && CanAssignFrom(iInputType))
      {
        TupleType const* inputTuple = iInputType->IsTuple();
        
        unsigned int numFields = GetNumField();
          
        for(unsigned int i = 0; i<numFields; ++i)
        {
          Type const* inFieldType;
          void const* inFieldData = inputTuple->GetField(iData,i,inFieldType);

          Type const* outFieldType;
          void* outFieldData = GetField(oData,i,outFieldType);

          err = outFieldType->Assign_Uninit(inFieldType,inFieldData,outFieldData);
          if(!err)
          {
            break;
          }
        }
      }
      return err;
    }
  }

  void* TupleType::Construct(void* iObj) const
  {
    eXl_ASSERT(IsPOD());
    if (iObj != nullptr)
    {
      for (uint32_t i = 0; i < GetNumField(); ++i)
      {
        Type const* fieldType;
        void* fieldPtr = GetField(iObj, i, fieldType);
        fieldType->Construct(fieldPtr);
      }
    }
    return iObj;
  }

  void TupleType::Destruct(void* iObj) const
  {
    eXl_ASSERT(IsPOD());
    if (iObj != nullptr)
    {
      for (uint32_t i = 0; i < GetNumField(); ++i)
      {
        Type const* fieldType;
        void* fieldPtr = GetField(iObj, i, fieldType);
        fieldType->Destruct(fieldPtr);
      }
    }
  }

  Err TupleType::ResolveFieldPath(AString const& iPath, unsigned int& oOffset, Type const*& oType) const
  {
    String::size_type pos = iPath.find(".");
    if (pos != String::npos)
    {
      AString suffix = iPath.substr(pos + 1);
      AString prefix = iPath.substr(0, pos);
      if (suffix.size() == 0)
      {
        LOG_WARNING << "Ill-formed path" << "\n";
        RETURN_FAILURE;
      }
      Type const* fieldType = nullptr;
      TypeFieldName fieldName;
      unsigned int i = 0;
      for (; i < GetNumField(); ++i)
      {
        fieldType = GetFieldDetails(i, fieldName);
        if (fieldType && fieldName == prefix)
          break;
      }
      if (i == GetNumField())
      {
        LOG_WARNING << "Prefix " << prefix << " not found" << "\n";
        RETURN_FAILURE;
      }
      TupleType const* tupleType = fieldType->IsTuple();
      if (tupleType == nullptr)
      {
        LOG_WARNING << "Field " << fieldName << "is not a tuple" << "\n";
        RETURN_FAILURE;
      }
      Err err = tupleType->ResolveFieldPath(suffix, oOffset, oType);
      if (err)
      {
        ptrdiff_t curOffset = (ptrdiff_t)GetField((void const*)(1), i, fieldType) - 1;
        oOffset += curOffset;
        RETURN_SUCCESS;
      }
      else
        return err;
    }
    else
    {
      Type const* fieldType = nullptr;
      TypeFieldName fieldName;
      unsigned int i = 0;
      for (; i < GetNumField(); ++i)
      {
        fieldType = GetFieldDetails(i, fieldName);
        if (fieldType && fieldName == iPath)
          break;
      }
      if (i == GetNumField())
      {
        LOG_WARNING << "Field " << iPath << " not found" << "\n";
        RETURN_FAILURE;
      }
      oOffset = (ptrdiff_t)GetField((void const*)(1), i, fieldType) - 1;
      oType = fieldType;
      RETURN_SUCCESS;
    }
  }
}
