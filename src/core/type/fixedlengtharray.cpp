/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/type/fixedlengtharray.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(FixedLengthArray);

  FixedLengthArray const* FixedLengthArray::Create(Type const* iType, unsigned int iNumFields)
  {
    if(iType == nullptr || iNumFields == 0)
      return nullptr;

    return eXl_NEW FixedLengthArray(iType,iNumFields);
  }

  FixedLengthArray::FixedLengthArray(Type const* iType, unsigned int iNum)
    : TupleType(TypeName(iType->GetName()+"["+StringUtil::AFromInt(iNum)+"]"),0,iType->GetSize() * iNum, 0)
    , m_Type(iType)
    , m_NumFields(iNum)
  {

  }

  void* FixedLengthArray::Construct(void* iObj)const
  {
    unsigned char* iter = reinterpret_cast<unsigned char*>(iObj);

    for(unsigned int i = 0; i< m_NumFields; ++i)
    {
      m_Type->Construct(iter);
      iter += m_Type->GetSize();
    }

    return iObj;
  }

  void FixedLengthArray::Destruct(void* iObj)const
  {
    unsigned char* iter = reinterpret_cast<unsigned char*>(iObj);

    for(unsigned int i = 0; i< m_NumFields; ++i)
    {
      m_Type->Destruct(iter);
      iter += m_Type->GetSize();
    }
  }

  Err FixedLengthArray::Copy_Uninit(void const* iData, void* oData) const
  {
    unsigned char const* iterOrig = reinterpret_cast<unsigned char const*>(iData);
    unsigned char* iterDest = reinterpret_cast<unsigned char*>(oData);
    for(unsigned int i = 0; i< m_NumFields; ++i)
    {
      m_Type->Copy_Uninit(iterOrig,iterDest);
      iterOrig += m_Type->GetSize();
      iterDest += m_Type->GetSize();
    }

    RETURN_SUCCESS;
  }
#ifdef EXL_LUA
  luabind::object FixedLengthArray::ConvertToLua(void const* iObj,lua_State* iState)const
  {
    luabind::object res = luabind::newtable(iState);

    int index = 1;

    for(unsigned int i = 0; i< m_NumFields;++i){
      void const* origElem = reinterpret_cast<unsigned char const*>(iObj) + m_Type->GetSize() * i;
      luabind::object temp = m_Type->ConvertToLua(origElem,iState);
      res[index]=temp;
      index++;
    }
    return res;
  }

  Err FixedLengthArray::ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const
  {
    if(!lua_istable(iState,ioIndex)){
      LOG_WARNING<<"Need a table"<<"\n";
      RETURN_FAILURE;
    }

    if(oObj == nullptr)
      RETURN_FAILURE;

    Construct(oObj);

    unsigned int arraySize=0;
    //Parse table, from lua.org
    lua_pushnil(iState);  // first key, idx 5 //
    while (lua_next(iState, ioIndex) != 0) {
      // uses 'key') (at index -2) and EXL_TEXT('value' (at index -1) 
      //printf("%s - %s\n",
      //  lua_typename(L, lua_type(L, -2)),
      //  lua_typename(L, lua_type(L, -1)));
      //value at index ioIndex+2
      eXl_ASSERT_MSG(lua_isnumber(iState,-2),"Must index table with numbers");
      lua_pop(iState, 1);
      arraySize++;
    }

    if(arraySize != m_NumFields)
      RETURN_FAILURE;

    lua_pushnil(iState);  // first key, idx 5 //
    while (lua_next(iState, ioIndex) != 0) {
      
      //String key(lua_tonumber(iState,-2));
      //Commence à 1 en lua
      unsigned int idx = lua_tonumber(iState,-2) - 1;
      void* elem = reinterpret_cast<unsigned char*>(oObj) + m_Type->GetSize() * idx;
      if(elem!=nullptr){
        unsigned int ioIdx2=ioIndex+2;
        Err err = m_Type->ConvertFromLua_Uninit(iState,ioIdx2,elem);
        eXl_ASSERT_MSG(err == Err::Success ,"Conversion failed");
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

  Err FixedLengthArray::ConvertFromLuaRaw_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const
  {
    if(oObj==nullptr || iState == nullptr){
      RETURN_FAILURE;
    }
    
    unsigned char* iter = reinterpret_cast<unsigned char*>(oObj);
    for(unsigned int i=0;i<GetNumField();i++){
      m_Type->ConvertFromLua_Uninit(iState,ioIndex,iter);
      iter += m_Type->GetSize();
    }

    RETURN_SUCCESS;
  }
#endif
  Err FixedLengthArray::ResolveFieldPath(AString const& iPath, unsigned int& oOffset, Type const*& oType)const
  {
# if 0
    String::size_type pos = iPath.find(".");
    if(pos != String::npos)
    {
      String suffix = iPath.substr(pos+1);
      String prefix = iPath.substr(0,pos);
      if(suffix.size() == 0)
      {
        LOG_WARNING << "Ill-formed path" << "\n";
        RETURN_FAILURE;
      }
      String indexToScan;
      unsigned int field = m_NumFields;
      if(iPath.front() == '[')
      {
        size_t pos = iPath.find(']');
        if(pos != String::npos)
        {
          indexToScan = iPath.substr(1, pos - 1);
        }
      }
      if()
      {
        if(field < m_NumFields)
        {
          TupleType const* tupleType = m_Type->IsTuple();
          if(tupleType == nullptr)
          {
            LOG_WARNING << "Field "<< m_Type->GetName() <<"is not a tuple" << "\n";
            RETURN_FAILURE;
          }
          Err err = tupleType->ResolveFieldPath(suffix,oOffset,oType);
          if(EC_SUCCESS(err))
          {
            oOffset += m_Type->GetSize()*field;
            RETURN_SUCCESS;
          }
          else
            return err;
        }
      }
      
      LOG_WARNING << "Prefix "<< prefix <<" not found in type "<< GetName() << "\n";
      RETURN_FAILURE;
     
    }
    else
    {
      unsigned int field = m_NumFields;
      if( 1 == sscanf(iPath.c_str(),"[%i]"),field)
      {
        if(field < m_NumFields)
        {
          oOffset = m_Type->GetSize()*field;
          oType = m_Type;
          RETURN_SUCCESS;
        }
      }

      LOG_WARNING << "Field "<< iPath << " not found in type "<< GetName() << "\n";
      RETURN_FAILURE; 
    }
#endif
    RETURN_FAILURE; 
  }

  bool FixedLengthArray::UserType()const
  {
    return true;
  }
    
  //Make sure iObj outlives the returned DynObject :), like holding a ptr to a struct
  void* FixedLengthArray::GetField (void* iObj,unsigned int iNum ,Type const*& oType)const
  {
    if(iNum < m_NumFields)
    {
      unsigned char* iter = reinterpret_cast<unsigned char*>(iObj);
      oType = m_Type;
      return iter + iNum*m_Type->GetSize();
    }
    return nullptr;
  }

  void* FixedLengthArray::GetField (void* iObj,TypeFieldName iName,Type const*& oType)const
  {
    unsigned int field = m_NumFields;
#if 0
    {
    if( 1 == sscanf(iName.c_str(),"[%i]"),field)
    {
      if(field < m_NumFields)
      {
        unsigned char* iter = reinterpret_cast<unsigned char*>(iObj);
        oType = m_Type;
        return iter += m_Type->GetSize();
      }
    }
#endif
    return nullptr;
  }
    
  void const* FixedLengthArray::GetField (void const* iObj,unsigned int iNum, Type const*& oType)const
  {
    if(iNum < m_NumFields)
    {
      unsigned char const* iter = reinterpret_cast<unsigned char const*>(iObj);
      oType = m_Type;
      return iter + iNum*m_Type->GetSize();
    }
    return nullptr;
  }

  void const* FixedLengthArray::GetField (void const* iObj,TypeFieldName iName,Type const*& oType)const
  {
#if 0
    unsigned int field = m_NumFields;
    if( 1 == sscanf(iName.c_str(),"[%i]"),field)
    {
      if(field < m_NumFields)
      {
        unsigned char const* iter = reinterpret_cast<unsigned char const*>(iObj);
        oType = m_Type;
        return iter += m_Type->GetSize();
      }
    }
#endif
    return nullptr;
  }

  const Type* FixedLengthArray::GetFieldDetails(unsigned int iNum,TypeFieldName& oFieldName)const
  {
    if(iNum < m_NumFields)
    {
      oFieldName = TypeFieldName("[" + StringUtil::AFromInt(iNum) + "]");
      return m_Type;
    }
    return nullptr;
  }

  const Type* FixedLengthArray::GetFieldDetails(TypeFieldName iFieldName,unsigned int& oNumField)const
  {
    unsigned int field = m_NumFields;
#if 0
    if( 1 == sscanf(iFieldName.c_str(),"[%i]"),field)
    {
      if(field < m_NumFields)
      {
        oNumField = field;
        return m_Type;
      }
    }
#endif
    return nullptr;
  }

  const Type* FixedLengthArray::GetFieldDetails(unsigned int iNum)const
  {
    if(iNum < m_NumFields)
    {
      return m_Type;
    }
    return nullptr;
  }

  const Type* FixedLengthArray::GetFieldDetails(TypeFieldName iFieldName)const
  {
    unsigned int field = m_NumFields;
#if 0
    if( 1 == sscanf(iFieldName.c_str(),"[%i]"),field)
    {
      if(field < m_NumFields)
      {
        return m_Type;
      }
    }
#endif
    return nullptr;
  }
}