/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/type/tupletypestruct.hpp>
#include <core/log.hpp>
#include <core/type/dynobject.hpp>
#include <core/type/arraytype.hpp>
#include <core/type/typemanager.hpp>
#include <luabind/class.hpp>
#include <luabind/lua_proxy_interface.hpp>
#include <luabind/make_function.hpp>
#include <core/lua/luabind_eXl.hpp>

namespace eXl
{
  namespace
  {
    ClassType const* s_TupleTypeStructClassType = nullptr;
  }

  IMPLEMENT_RTTI(TupleTypeStruct);

  namespace
  {
    void* InternalGetField (void* iObj,const FieldDesc& iField,Type const*& oType)
    {
      if(iObj!=nullptr)
      {
        oType = iField.GetType();
        return (unsigned char*)iObj+iField.GetOffset();
      }
      return nullptr;
    }

    void const* InternalGetFieldConst (void const* iObj,const FieldDesc& iField,Type const*& oType)
    {
      if(iObj!=nullptr)
      {
        oType = iField.GetType();
        return (unsigned char*)iObj+iField.GetOffset();
      }
      return nullptr;
    }
  }

  size_t TupleTypeAdapter::S_GetNumField(FieldVector const& iVect)
  {
    return iVect.size();
  }

  Err TupleTypeAdapter::S_ResolveFieldPath(FieldVector const& iVect, AString const& iPath, unsigned int& oOffset, Type const*& oType)
  {
    String::size_type pos = iPath.find(".");
    if(pos != String::npos)
    {
      AString suffix = iPath.substr(pos+1);
      AString prefix = iPath.substr(0,pos);
      if(suffix.size() == 0)
      {
        LOG_WARNING << "Ill-formed path" << "\n";
        RETURN_FAILURE;
      }
      unsigned int i = 0;
      for(; i<iVect.size();++i)
      {
        if(iVect[i].GetName() == prefix)
          break;
      }
      if(i == iVect.size())
      {
        LOG_WARNING << "Prefix "<< prefix <<" not found" << "\n";
        RETURN_FAILURE;
      }
      TupleType const* tupleType = iVect[i].GetType()->IsTuple();
      if(tupleType == nullptr)
      {
        LOG_WARNING << "Field "<< iVect[i].GetName() <<"is not a tuple" << "\n";
        RETURN_FAILURE;
      }
      Err err = tupleType->ResolveFieldPath(suffix,oOffset,oType);
      if(err)
      {
        oOffset += iVect[i].GetOffset();
        RETURN_SUCCESS;
      }
      else
        return err;
    }
    else
    {
      unsigned int i = 0;
      for(; i<iVect.size();++i)
      {
        if(iVect[i].GetName() == iPath)
          break;
      }
      if(i == iVect.size())
      {
        LOG_WARNING << "Field "<< iPath << " not found"<< "\n";
        RETURN_FAILURE;
      }
      oOffset = iVect[i].GetOffset();
      oType = iVect[i].GetType();
      RETURN_SUCCESS;
    }
  }
    
  void* TupleTypeAdapter::S_GetField (FieldVector const& iVect, void* iObj,unsigned int iField,Type const*& oType)
  {
    if(iVect.size()>iField)
    {
      if(iObj != nullptr)
      {
        return InternalGetField(iObj,iVect[iField],oType);
      }
      else
      {
        LOG_WARNING<<"Wrong object for field selection"<<"\n";
        return nullptr;
      }
    }
    else
    {
      LOG_WARNING<<"Wrong field number"<<"\n";
    }
    return nullptr;
  }

  void* TupleTypeAdapter::S_GetField (FieldVector const& iVect, void* iObj,TypeFieldName iName,Type const*& oType)
  {
    FieldVector::const_iterator iter = iVect.begin();
    FieldVector::const_iterator iterEnd = iVect.end();
    for(;iter!=iterEnd;iter++)
    {
      if(iter->GetName()==iName)break;
    }
    if(iter==iterEnd)
    {
      LOG_WARNING<<"Wrong object for field selection"<<"\n";
    }
    else
      return InternalGetField(iObj,*iter,oType);

    return nullptr;
  }
    
  void const* TupleTypeAdapter::S_GetField (FieldVector const& iVect, void const* iObj,unsigned int iField,Type const*& oType)
  {
    if(iVect.size()>iField)
    {
      if(iObj != nullptr)
      {
        return InternalGetFieldConst(iObj,iVect[iField],oType);
      }
      else
      {
        LOG_WARNING<<"Wrong object for field selection"<<"\n";
      }
    }
    else
    {
      LOG_WARNING<<"Wrong field number"<<"\n";
    }
    return nullptr;
  }

  void const* TupleTypeAdapter::S_GetField (FieldVector const& iVect, void const* iObj,TypeFieldName iName,Type const*& oType)
  {
    FieldVector::const_iterator iter = iVect.begin();
    FieldVector::const_iterator iterEnd = iVect.end();
    for(;iter!=iterEnd;iter++)
    {
      if(iter->GetName()==iName)break;
    }
    if(iter==iterEnd)
    {
      LOG_WARNING<<"Wrong object for field selection"<<"\n";
        
    }
    else
      return InternalGetFieldConst(iObj,*iter,oType);

    return nullptr;
  }

  const Type* TupleTypeAdapter::S_GetFieldDetails(FieldVector const& iVect, unsigned int iNum,TypeFieldName& oFieldName)
  {
    if(iVect.size() > iNum)
    {
      oFieldName = iVect[iNum].GetName();
      return iVect[iNum].GetType();
    }
    return nullptr;
  }

  const Type* TupleTypeAdapter::S_GetFieldDetails(FieldVector const& iVect, TypeFieldName iFieldName,unsigned int& oNumField)
  {
    FieldVector::const_iterator iter = iVect.begin();
    FieldVector::const_iterator iterEnd = iVect.end();
    for(;iter!=iterEnd;iter++)
    {
      if(iter->GetName()==iFieldName)break;
    }

    if(iter!=iterEnd)
    {
      oNumField = iter-iVect.begin();
      return iter->GetType();
    }
    return nullptr;
  }

  const Type* TupleTypeAdapter::S_GetFieldDetails(FieldVector const& iVect, unsigned int iNum)
  {
    if(iVect.size()>iNum)
    {
      return iVect[iNum].GetType();
    }
    return nullptr;
  }

  const Type* TupleTypeAdapter::S_GetFieldDetails(FieldVector const& iVect, TypeFieldName iFieldName)
  {
    FieldVector::const_iterator iter = iVect.begin();
    FieldVector::const_iterator iterEnd = iVect.end();
    for(;iter!=iterEnd;iter++)
    {
      if(iter->GetName()==iFieldName)break;
    }
    if(iter==iterEnd)
      return nullptr;

    return iter->GetType();
  }

  TupleTypeStruct::TupleTypeStruct(TypeName iName,size_t iId,size_t iSize,unsigned int iFlags)
    : TupleTypeAdapter(iName,iId,iSize,iFlags)
  {
  }

  TupleType* TupleTypeStruct::MakeTuple(TypeName iName,const List<FieldDesc>& iList,size_t iId)
  {
    if(!iList.empty())
    {
      bool isPOD = false;
      size_t totSize = 0;
      List<FieldDesc>::const_iterator iter = iList.begin();
      List<FieldDesc>::const_iterator iterEnd = iList.end();
      List<FieldDesc>::const_iterator iterMax = iter;
      for(;iter!=iterEnd;iter++)
      {
        eXl_ASSERT_MSG(iter->GetOffset()>=totSize,"Err, wrong offset");
        if(iter->GetOffset()>iterMax->GetOffset())
          iterMax=iter;
        if(!iter->GetType()->IsPOD())
          isPOD = false;
      }

      totSize = iterMax->GetOffset() + iterMax->GetType()->GetSize();
      TupleTypeStruct* newType = eXl_NEW TupleTypeStruct(iName,iId,totSize,(isPOD ? Type_Is_POD : 0));
      newType->m_Data = FieldVector();
      FieldVector& fields = newType->m_Data;
      iter = iList.begin();
      for(;iter!=iterEnd;iter++)
      {
        fields.push_back(*iter);
      }
      return newType;
    }
    return nullptr;
  }

  TupleType* TupleTypeStruct::Create(const List<FieldDesc>& iList, TypeName iName)
  {
    TupleType* newType = MakeTuple(iName,iList,0);
    
    return newType;
  }

  void TupleTypeStruct::FillFields(const List<FieldDesc>& iList)
  {
    m_Data = Vector<FieldDesc>();
    FieldVector& fields = m_Data;
    std::list<FieldDesc>::const_iterator iter = iList.begin();
    std::list<FieldDesc>::const_iterator iterEnd = iList.end();
    for(;iter!=iterEnd;iter++)
    {
      fields.push_back(*iter);
    }
  }

  size_t TupleTypeStruct::GetNumField()const
  {
   
    {
      FieldVector const& fields = m_Data;
      return fields.size();
    }
    return 0;
  }

  const Type* TupleTypeStruct::GetFieldDetails(unsigned int iNum,TypeFieldName& oFieldName)const
  {
    
    {
      FieldVector const& fields = m_Data;
      return TupleTypeAdapter::S_GetFieldDetails(fields,iNum,oFieldName);
    }
    return nullptr;
  }

  const Type* TupleTypeStruct::GetFieldDetails(TypeFieldName iFieldName,unsigned int& oNumField)const
  {
    
    {
      FieldVector const& fields = m_Data;
      return TupleTypeAdapter::S_GetFieldDetails(fields,iFieldName,oNumField);
    }
    return nullptr;
  }

  const Type* TupleTypeStruct::GetFieldDetails(unsigned int iNum)const
  {
    
    {
      FieldVector const& fields = m_Data;
      return TupleTypeAdapter::S_GetFieldDetails(fields,iNum);
    }
    return nullptr;
  }

  const Type* TupleTypeStruct::GetFieldDetails(TypeFieldName iFieldName)const
  {
    
    {
      FieldVector const& fields = m_Data;
      return TupleTypeAdapter::S_GetFieldDetails(fields,iFieldName);
    }
    return nullptr;
  }

  void* TupleTypeStruct::GetField (void* iObj,unsigned int iField,Type const*& oType)const
  {
    
    {
      FieldVector const& fields = m_Data;
      return TupleTypeAdapter::S_GetField(fields,iObj,iField,oType);
    }
    return nullptr;
  }

  void* TupleTypeStruct::GetField (void* iObj,TypeFieldName iName,Type const*& oType)const
  {
    if(iObj != nullptr)
    {
      FieldVector const& fields = m_Data;
      return TupleTypeAdapter::S_GetField(fields,iObj,iName,oType);
    }
    return nullptr;
  }
  
  void const* TupleTypeStruct::GetField (void const* iObj,unsigned int iField,Type const*& oType)const
  {
    
    {
      FieldVector const& fields = m_Data;
      return TupleTypeAdapter::S_GetField(fields,iObj,iField,oType);
    }
    return nullptr;
  }
  
  void const* TupleTypeStruct::GetField (void const* iObj,TypeFieldName iName,Type const*& oType)const
  {
    if(iObj != nullptr)
    {
      FieldVector const& fields = m_Data;
      return TupleTypeAdapter::S_GetField(fields,iObj,iName,oType);
    }
    return nullptr;
  }
  
  void* TupleTypeStruct::Construct(void* iObj)const
  {
    if(iObj!=nullptr)
    {
      FieldVector const& fields = m_Data;
      FieldVector::const_iterator iter = fields.begin();
      FieldVector::const_iterator iterEnd = fields.end();
      for(;iter!=iterEnd;iter++)
      {
        const Type* type = iter->GetType();
        void* fieldPtr = (unsigned char*)iObj + iter->GetOffset();
        type->Construct(fieldPtr);
      }
      return iObj;
    }
    return nullptr;
  }
  
  void TupleTypeStruct::Destruct(void* iObj)const
  {
    if(iObj!=nullptr)
    {
      FieldVector const& fields = m_Data;
      FieldVector::const_iterator iter = fields.begin();
      FieldVector::const_iterator iterEnd = fields.end();
      for(;iter!=iterEnd;iter++)
      {
        const Type* type = iter->GetType();
        void* fieldPtr = (unsigned char*)iObj + iter->GetOffset();
        type->Destruct(fieldPtr);
      }
    }
  }

  Err TupleTypeStruct::Copy_Uninit(void const* iData, void* oData) const
  {
    
    {
      if(IsPOD())
      {
        memcpy(oData,iData,GetSize());
      }
      else
      {
        FieldVector const& fields = m_Data;
        FieldVector::const_iterator iter = fields.begin();
        FieldVector::const_iterator iterEnd = fields.end();
        for(;iter!=iterEnd;iter++)
        {
          const Type* type = iter->GetType();
          void* iFieldPtr = (unsigned char*)iData + iter->GetOffset();
          void* oFieldPtr = (unsigned char*)oData + iter->GetOffset();
          Err res = type->Copy_Uninit(iFieldPtr,oFieldPtr);
          if(!res)
            return res;
        }
      }
      RETURN_SUCCESS;
    }
    RETURN_FAILURE;
  }
#ifdef EXL_LUA
  luabind::object TupleTypeStruct::ConvertToLua(void const* iObj,lua_State* iState) const
  {
    if(iObj!=nullptr)
    {
      FieldVector const& fields = m_Data;
      luabind::object ret = luabind::newtable(iState);
      for(unsigned int i=0;i<fields.size();i++)
      {
        const unsigned int fieldIdx = i;
        Type const* type;
        void const* field = GetField(iObj,fieldIdx,type);
        eXl_ASSERT_MSG(field!=nullptr,"Get field failure");
        ret[fields[fieldIdx].GetName().c_str()]=type->ConvertToLua(field,iState);
      }
      return ret;
    }
    return luabind::object();
  }

  Err TupleTypeStruct::ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const
  {
    if(!lua_istable(iState,ioIndex))
    {
      LOG_WARNING<<"Need a table"<<"\n";
      RETURN_FAILURE;
    }

    if(oObj != nullptr)
    {

      unsigned int numFieldsSet=0;
      //Parse table, from lua.org
      lua_pushnil(iState);  // first key, idx 5 //
      while (lua_next(iState, ioIndex) != 0)
      {
        // uses 'key') (at index -2) and 'value' (at index -1) 
        //printf("%s - %s\n",
        //  lua_typename(L, lua_type(L, -2)),
        //  lua_typename(L, lua_type(L, -1)));
      
        eXl_ASSERT_MSG(lua_isstring(iState,-2),"Must index table with strings");
        //value at index ioIndex+2
        TypeFieldName key(lua_tostring(iState,-2));
        unsigned int idx=0;
        const Type* fieldType;
        void* field = GetField(oObj,key,fieldType);
        if(field!=nullptr)
        {
          unsigned int ioIdx2=ioIndex+2;
          Err err = fieldType->ConvertFromLua_Uninit(iState,ioIdx2,field);
          eXl_ASSERT_MSG(err == Err::Success,"Conversion failed");
          numFieldsSet++;
        }
        // removes 'value'); keeps 'key' for next iteration
        lua_pop(iState, 1);
      }
      if(numFieldsSet<GetNumField())
      {
        LOG_WARNING<<"Not enough fields in lua table"<<"\n";
        //if(alloced)eXl_DELETE oObj;
          //return nullptr;
      }
      ioIndex++;
      RETURN_SUCCESS;
    }
    RETURN_FAILURE;
  }

  Err TupleTypeStruct::ConvertFromLuaRaw_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const
  {
    if( oObj!=nullptr || iState != nullptr)
    {
      for(unsigned int i=0;i<GetNumField();i++)
      {
        const unsigned int fieldIdx = i;
        Type const* fieldType;
        void* field = GetField(oObj,fieldIdx,fieldType);
        eXl_ASSERT_MSG(field!=nullptr,"Get field failure");
        fieldType->ConvertFromLua_Uninit(iState,ioIndex,field);
      }

      RETURN_SUCCESS;
    }
    RETURN_FAILURE;
  }

  

  void TupleTypeStruct::RegisterLua(lua_State* iState) const
  {
    luabind::detail::class_base newClass(GetName().c_str());
    newClass.init(this, luabind::detail::allocate_class_id(this), nullptr, luabind::detail::allocate_class_id(nullptr));
    newClass.add_member(new type_constructor_registration(this));
    newClass.add_default_member(new type_constructor_registration(this));

    for (uint32_t i = 0; i<m_Data.size(); ++i)
    {
      newClass.add_member(new type_field_registration(this, i));
    }

    luabind::module(iState, "eXl")
      [
        newClass
      ];
  }

#endif
  Err TupleTypeStruct::ResolveFieldPath(AString const& iPath, unsigned int& oOffset, Type const*& oType)const
  {
    
    {
      FieldVector const& fields = m_Data;
      return TupleTypeAdapter::S_ResolveFieldPath(fields,iPath,oOffset,oType);
    }
    RETURN_FAILURE;
  }
}
