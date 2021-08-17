/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include <core/type/arraytype.hpp>
#include <core/type/typemanager.hpp>
#include <core/stream/unstreamer.hpp>
#include <core/lua/luabind_eXl.hpp>

extern void stackDump (lua_State *L);

namespace eXl
{
  IMPLEMENT_RTTI(ArrayType);


  ArrayType::ArrayType(TypeName iName,
              size_t iTypeId,
              size_t iSize,
              unsigned int iFlags,
              Type const* iElemType) 
    : Type(iName,iTypeId,iSize,iFlags | (iElemType->IsPOD() ? Type_Is_POD : 0)),m_ElemType(iElemType)
  {
  
  }

  void ArrayType::Destruct(void* iObj)const
  {
    unsigned int curSize = GetArraySize(iObj);
    if(curSize > 0)
    {
      for(unsigned int i = 0; i< curSize; ++i)
      {
        void* ptr = GetElement(iObj,i);
        m_ElemType->Destruct(ptr);
      }
    }
  }

  Err ArrayType::SetArraySize(void* iObj,unsigned int iSize) const
  {
    if(iObj == nullptr)
      RETURN_FAILURE;

    unsigned int arraySize = GetArraySize(iObj);

    if(iSize != arraySize)
    {
      if(iSize>arraySize)
      {
        SetArraySize_Uninit(iObj,iSize);
        for(unsigned int i = arraySize; i<iSize; ++i)
        {
          void* elem = GetElement(iObj,i);
          m_ElemType->Construct(elem);
        }
      }
      else
      {
        for(unsigned int i = iSize; i<arraySize; ++i)
        {
          void* elem = GetElement(iObj,i);
          m_ElemType->Destruct(elem);
        }
        SetArraySize_Uninit(iObj,iSize);
      }
    }
    RETURN_SUCCESS;
  }

  Err ArrayType::Copy_Uninit(void const* iData, void* oData) const
  {
    unsigned int arraySize = GetArraySize(iData);
    Construct(oData);
    SetArraySize_Uninit(oData,arraySize);
    for(unsigned int i = 0 ; i<arraySize;++i)
    {
      void const* origElem = GetElement(iData,i);
      void *      newElem  = GetElement(oData,i);
      Err err = m_ElemType->Copy_Uninit(origElem,newElem);
      if(!err)
        return err;
    }
    RETURN_SUCCESS;
  }
#ifdef EXL_LUA
  luabind::object ArrayType::ConvertToLua(void const* iObj,lua_State* iState)const
  {
    unsigned int arraySize = GetArraySize(iObj);

    luabind::object res = luabind::newtable(iState);

    int index = 1;

    for(unsigned int i = 0; i< arraySize;++i)
    {
      void const* origElem = GetElement(iObj,i);
      luabind::object temp = m_ElemType->ConvertToLua(origElem,iState);
      res[index]=temp;
      index++;
    }
    return res;
  }

  Err ArrayType::ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const
  {
    if(!lua_istable(iState,ioIndex)){
      LOG_WARNING<<"Need a table"<<"\n";
      stackDump(iState);
      RETURN_FAILURE;
    }

    if(oObj == nullptr)
      RETURN_FAILURE;

    Construct(oObj);

    unsigned int arraySize=0;
    //Parse table, from lua.org
    lua_pushnil(iState);  // first key, idx 5 //
    while (lua_next(iState, ioIndex) != 0) 
    {
      // uses 'key') (at index -2) and EXL_TEXT('value' (at index -1) 
      //printf("%s - %s\n",
      //  lua_typename(L, lua_type(L, -2)),
      //  lua_typename(L, lua_type(L, -1)));
      //value at index ioIndex+2
      eXl_ASSERT_MSG(lua_isnumber(iState,-2),"Must index table with numbers");
      lua_pop(iState, 1);
      arraySize++;
    }

    SetArraySize_Uninit(oObj,arraySize);

    lua_pushnil(iState);  // first key, idx 5 //
    while (lua_next(iState, ioIndex) != 0) {
      
      //String key(lua_tonumber(iState,-2));
      //Commence à 1 en lua
      unsigned int idx = lua_tointeger(iState,-2) - 1;
      void* elem = GetElement(oObj,idx);
      if(elem!=nullptr){
        unsigned int ioIdx2=ioIndex+2;
        Err err = m_ElemType->ConvertFromLua_Uninit(iState,ioIdx2,elem);
        eXl_ASSERT_MSG(err == Err::Success,"Conversion failed");
        //numFieldsSet++;
      }
      // removes 'value'); keeps EXL_TEXT('key' for next iteration
      lua_pop(iState, 1);
    }
    /*if(numFieldsSet<GetNumField()){
      LOG_WARNING<<"Not enough fields in lua table"<<"\n";
      //if(alloced)eXl_DELETE oObj;
        //return nullptr;
    }*/
    ioIndex++;
    RETURN_SUCCESS;
  }

  void ArrayType::RegisterLua(lua_State* iState) const
  {
    luabind::detail::class_base newClass(GetName().c_str());
    newClass.init(this, luabind::detail::allocate_class_id(this), nullptr, luabind::detail::allocate_class_id(nullptr));
    newClass.add_member(new type_constructor_registration(this));
    newClass.add_default_member(new type_constructor_registration(this));

    newClass.add_member(new access_element_registration(this));

    luabind::module(iState, "eXl")
      [
        newClass
      ];
  }

  //luabind::object ArrayType::MakePropertyAccessor(lua_State* iState, Type const* iHolder, uint32_t iField) const
  //{
  //  
  //}

#endif
  Err ArrayType::Unstream_Uninit(void* oData, Unstreamer* iUnstreamer) const
  {
    Err err(Err::Failure);
    unsigned int arraySize = 0;

    err = iUnstreamer->BeginSequence();
    if(err)
    {
      do
      {
        arraySize++;
      }
      while((err = iUnstreamer->NextSequenceElement()));
    }
    else
    {
      //Empty array.
      if(err != Err::Error)
      {
        Construct(oData);
        err = Err::Success;
      }
    }
    //end of the array.
    if(err == Err::Failure)
    {
      Construct(oData);
      SetArraySize_Uninit(oData,arraySize);
      iUnstreamer->BeginSequence();
      for(unsigned int i = 0; i< arraySize;++i)
      {
        void* elem = GetElement(oData,i);
        err = m_ElemType->Unstream(elem,iUnstreamer);
        if(!err)
        {
          LOG_ERROR<<"Err while unstreaming element "<< i << "\n";
          break;
        }
        iUnstreamer->NextSequenceElement();
      }
    }
    
    if(!err)
    {
      LOG_ERROR<<"Err while unstreaming "<< GetName() << "\n";
    }
    return err;
  }

  Err ArrayType::Stream(void const* iData, Streamer* iStreamer) const
  {
    if(iData == nullptr || iStreamer == nullptr)
      return Err::Failure;

    unsigned int arraySize = GetArraySize(iData);

    Err err = Err::Success;
    iStreamer->BeginSequence(/*arraySize*/);
    for(unsigned int i = 0; i< arraySize;++i)
    {
      void const* origElem = GetElement(iData,i);
      err = m_ElemType->Stream(origElem,iStreamer);
      if(!err)
      {
        LOG_ERROR<<"Err while streaming element "<< i << "\n";
        break;
      }
    }
    if(err)
    {
      iStreamer->EndSequence();
    }
    else
    {
      LOG_ERROR<<"Err while streaming "<< GetName() << "\n";
    }
    return err;
  }

  Err ArrayType::Compare(void const* iVal1, void const* iVal2, CompRes& oRes)const
  {
    if(iVal1 == nullptr || iVal2 == nullptr)
      RETURN_FAILURE;

    unsigned int arraySize1 = GetArraySize(iVal1);
    unsigned int arraySize2 = GetArraySize(iVal2);

    if(arraySize1 != arraySize2)
    {
      oRes = CompDifferent;
      RETURN_SUCCESS;
    }

    oRes = CompEqual;

    for(unsigned int i = 0 ; i<arraySize1 ; ++i)
    {
      
      void const* field1 = GetElement(iVal1,i);
      void const* field2 = GetElement(iVal2,i);

      CompRes tempRes;
      Err err = m_ElemType->Compare(field1,field2,tempRes);

      if(tempRes != CompEqual)
      {
        oRes = CompDifferent;
        RETURN_SUCCESS;
      }
    }
    RETURN_SUCCESS;
  }

  bool ArrayType::CanAssignFrom(Type const* iType) const
  {
    if(IsPOD())
    {
      return Type::CanAssignFrom(iType);
    }
    else
    {
      if(iType == this)
        return true;

      ArrayType const* inputArrayType = ArrayType::DynamicCast(iType);
      if(inputArrayType)
      {
        return m_ElemType->CanAssignFrom(inputArrayType->GetElementType());
      }
      return false;
    }
  }

  Err ArrayType::Assign_Uninit(Type const* inputType, void const* iData, void* oData) const
  {
    if(IsPOD())
    {
      return Type::Assign_Uninit(inputType, iData,oData);
    }
    else
    {
      Err err = Err::Failure;
      if(iData != nullptr && oData != nullptr && CanAssignFrom(inputType))
      {
        err = Err::Success;
        ArrayType const* inputArrayType = ArrayType::DynamicCast(inputType);
        Type const* inputElementType = inputArrayType->GetElementType();
        unsigned int arraySize = inputArrayType->GetArraySize(iData);
        Construct(oData);
        SetArraySize(oData,arraySize);

        for(unsigned int i = 0 ; i<arraySize;++i)
        {
          void const* origElem = inputArrayType->GetElement(iData,i);
          void *      newElem  = GetElement(oData,i);
          Err err = m_ElemType->Assign_Uninit(inputElementType,origElem,newElem);
          if(!err)
          {
            break;
          }
        }
      }
      return err;
    }
  }

  typedef std::vector<char> GnrArray;

  GnrArrayType::GnrArrayType(TypeName iName,
                 Type const* iElemType)
    : ArrayType(iName, 0, sizeof(GnrArray), 0, iElemType)
  {

  }
    
  void* GnrArrayType::Alloc()const
  {
    return eXl_ALLOC(sizeof(GnrArray));
  }

  void GnrArrayType::Free(void* iObj)const
  {
    eXl_FREE(iObj);
  }

  void* GnrArrayType::Construct(void* iObj)const
  {
    return new(iObj) GnrArray;
  }

  void GnrArrayType::Destruct(void* iObj)const
  {
    ArrayType::Destruct(iObj);
    GnrArray* localArray = (GnrArray*)iObj;
    ((GnrArray*)iObj)->~GnrArray();
  }

  unsigned int GnrArrayType::GetArraySize(void const* iObj) const
  {
    return ((GnrArray*)iObj)->size()/m_ElemType->GetSize();
  }

  void GnrArrayType::SetArraySize_Uninit(void* iObj,unsigned int iSize) const
  {
    GnrArray* localArray = (GnrArray*)iObj;
    unsigned int curSize = localArray->size();
    const unsigned int elemSize = m_ElemType->GetSize();
    if(iSize > curSize)
    {
      if(iSize > localArray->capacity())
      {
        GnrArray newArray(iSize*elemSize);
        if(curSize > 0)
        {
          char* ptrFrom = &localArray->at(0);
          char* ptrTo = &newArray.at(0);
          for(unsigned int i = 0; i< curSize; ++i)
          {
            m_ElemType->Copy_Uninit(ptrFrom,ptrTo);
            m_ElemType->Destruct(ptrFrom);
            ptrFrom += elemSize;
            ptrTo += elemSize;
          }
        }
        localArray->swap(newArray);
      }
      else
      {
        localArray->resize(iSize * elemSize);
      }
    }
    else if(iSize < curSize)
    {
      localArray->resize(iSize * elemSize);
    }
  }

  void* GnrArrayType::GetElement(void* iObj,unsigned int i) const
  {
    GnrArray* localArray = (GnrArray*)iObj;
    const unsigned int offset = i*m_ElemType->GetSize();
    if( offset >= localArray->size())
      return nullptr;
    return &localArray->at(offset);
  }
  
  Err GnrArrayType::Copy_Uninit(void const* iData, void* oData) const
  {
    if(IsPOD())
    {
      GnrArray const* iArray = reinterpret_cast<GnrArray const*>(iData);
      unsigned int curSize = iArray->size();
      GnrArray* oArray = reinterpret_cast<GnrArray*>(oData);
      oArray->resize(curSize);
      if(curSize > 0)
      {
        memcpy(&oArray[0],&iArray[0],curSize);
      }
      RETURN_SUCCESS;
    }
    else
    {
      return ArrayType::Copy_Uninit(iData,oData);
    }
  }

}

