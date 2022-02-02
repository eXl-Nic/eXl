/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

template <class T>
TupleType* CoreTupleType<T>::MakeTuple(TypeName iName,const List<FieldDesc>& iList,size_t iId)
{
  size_t totSize = 0;
  List<FieldDesc>::const_iterator iter = iList.begin();
  List<FieldDesc>::const_iterator iterEnd = iList.end();
  for(;iter!=iterEnd;iter++)
  {
    //eXl_ASSERT_MSG(iter->GetOffset()>=totSize,"Err, wrong offset");
    size_t candidate = iter->GetOffset() + iter->GetType()->GetSize();
    totSize = totSize > candidate ? totSize : candidate;
  }
  TupleType* newType = eXl_NEW CoreTupleType<T>(iName,iId,iList);
  
  return newType;
}

template <class T>
CoreTupleType<T>::CoreTupleType(TypeName iName,size_t iId,const List<FieldDesc>& iList)
  : TupleTypeAdapter(iName,iId,sizeof(T),Type_Is_CoreType)
{
  bool isPod = true;
  List<FieldDesc>::const_iterator iter = iList.begin();
  List<FieldDesc>::const_iterator iterEnd = iList.end();
  for(;iter!=iterEnd;iter++)
  {
    m_Fields.push_back(*iter);
    if(!(iter->GetType()->IsPOD()))
      isPod = false;
  }
  if(isPod)
  {
    m_Flags |= Type_Is_POD;
  }
}

template <class T>
size_t CoreTupleType<T>::GetNumField()const
{
  return m_Fields.size();
}

template <class T>
void* CoreTupleType<T>::Alloc()const
{
  return TypeTraits::Alloc<T>();
}

template <class T>
void CoreTupleType<T>::Free(void* iObj)const
{
  TypeTraits::Free<T>(iObj);
}
    
template <class T>
void* CoreTupleType<T>::Construct(void* iObj)const
{
  return TypeTraits::DefaultCTor<T>(iObj);
}

template <class T>
void CoreTupleType<T>::Destruct(void* iObj)const
{
  TypeTraits::DTor<T>(iObj);
}

template <class T>
Err CoreTupleType<T>::Copy_Uninit(void const* iData, void* oData) const
{
  if(iData != nullptr && oData != nullptr)
  {
    void* res = TypeTraits::Copy<T>(oData,iData);
    eXl_ASSERT_MSG(res !=nullptr , (String(GetName() + " cannot be built in place")).c_str());
  }
  RETURN_SUCCESS;
}
#ifdef EXL_LUA
template <class T>
luabind::object CoreTupleType<T>::ConvertToLua(void const* iObj,lua_State* iState)const
{
   if(iState==nullptr)return luabind::object();
  return eXl::LuaConverter<T>::ConvertToLua(iObj,this,iState);
}

template <class T>
Err CoreTupleType<T>::ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const
{
  if(iState==nullptr || oObj == nullptr)
    RETURN_FAILURE;
  
  return eXl::LuaConverter<T>::ConvertFromLua(this,oObj,iState,ioIndex);
}
#endif
template <class T>
void* CoreTupleType<T>::GetField (void* iObj, unsigned int iNum,Type const*& oType)const
{
  return TupleTypeAdapter::S_GetField(m_Fields,iObj,iNum,oType);
}

template <class T>
void* CoreTupleType<T>::GetField (void* iObj, TypeFieldName iName,Type const*& oType)const
{
  return TupleTypeAdapter::S_GetField(m_Fields,iObj,iName,oType);
}
    
template <class T>
void const* CoreTupleType<T>::GetField (void const* iObj, unsigned int iNum, Type const*& oType)const
{
  return TupleTypeAdapter::S_GetField(m_Fields,iObj,iNum,oType);
}

template <class T>
void const* CoreTupleType<T>::GetField (void const* iObj, TypeFieldName iName,Type const*& oType)const
{
  return TupleTypeAdapter::S_GetField(m_Fields,iObj,iName,oType);
}

template <class T>
const Type* CoreTupleType<T>::GetFieldDetails(unsigned int iNum, TypeFieldName& oFieldName)const
{
  return TupleTypeAdapter::S_GetFieldDetails(m_Fields,iNum,oFieldName);
}

template <class T>
const Type* CoreTupleType<T>::GetFieldDetails(TypeFieldName iFieldName,unsigned int& oNumField)const
{
  return TupleTypeAdapter::S_GetFieldDetails(m_Fields,iFieldName,oNumField);
}

template <class T>
const Type* CoreTupleType<T>::GetFieldDetails(unsigned int iNum)const
{
  return TupleTypeAdapter::S_GetFieldDetails(m_Fields,iNum);
}

template <class T>
const Type* CoreTupleType<T>::GetFieldDetails(TypeFieldName iFieldName)const
{
  return TupleTypeAdapter::S_GetFieldDetails(m_Fields,iFieldName);
}
#ifdef EXL_LUA
template <class T>
Err CoreTupleType<T>::ConvertFromLuaRaw_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const
{
  return Err::Undefined;
}

template <class T>
void CoreTupleType<T>::RegisterLua(lua_State* iState) const
{
  luabind::class_<T> newClass(GetName().c_str());
  newClass.def(luabind::constructor<>());

  for (uint32_t i = 0; i < m_Fields.size(); ++i)
  {
    newClass.add_member(new type_field_registration(this, i));
  }

  luabind::module(iState, "eXl")
    [
      newClass
    ];
}
#endif