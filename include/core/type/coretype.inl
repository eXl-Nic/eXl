/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

template <class T>
T_CoreType<T>::T_CoreType():CoreType(GetTypeName<T>(),
                             /*TypeTraits::GetID<T>()*/ 0,
                             TypeTraits::GetSize<T>(),
                             Type_Is_CoreType | (::eXl::TypeTraits::IsOrdered<T>::s_Value ? Type_Is_Ordered : 0) /*| (boost::is_pod<T>::value ? Type_Is_POD : 0)*/ )
{}
template <class T>
void* T_CoreType<T>::Alloc()const
{
  return TypeTraits::Alloc<T>();
}

template <class T>
void T_CoreType<T>::Free(void* iObj)const
{
  TypeTraits::Free<T>(iObj);
}

template <class T>
void* T_CoreType<T>::Construct(void* iObj)const
{
  memset(iObj, 0, GetSize());
  return TypeTraits::DefaultCTor<T>(iObj);
}

template <class T>
void T_CoreType<T>::Destruct(void* iObj)const
{
  TypeTraits::DTor<T>(iObj);
}

template <class T>
Err T_CoreType<T>::Unstream_Uninit(void* oData, Unstreamer* iUnstreamer)const
{
  return iUnstreamer->Read(reinterpret_cast<T*>(oData));
}

template <class T>
Err T_CoreType<T>::Stream(void const* iData, Streamer* iStreamer)const
{
  if(iData == nullptr || iStreamer == nullptr)
    return Err::Error;

  return iStreamer->Write(reinterpret_cast<T const*>(iData));
}


template <class T>
Err T_CoreType<T>::Copy_Uninit(void const* iData, void* oData) const{
  eXl_ASSERT_MSG(oData != nullptr && iData != nullptr,"Wrong parameters");
  void* res = TypeTraits::Copy<T>(oData,iData);
  if(res != nullptr)
    RETURN_SUCCESS;
  LOG_ERROR<<GetName()<<" cannot be built in place"<<"\n";
  return Err::Error;
}

#ifdef EXL_LUA

template <class T>
luabind::object T_CoreType<T>::ConvertToLua(void const* iObj,lua_State* iState)const{
  if(iState==nullptr)
    return luabind::object();
  return eXl::LuaConverter<T>::ConvertToLua(iObj,this,iState);
}

template <class T>
Err T_CoreType<T>::ConvertFromLua_Uninit(lua_State* iState,unsigned int& ioIndex,void* oObj)const{
  if(iState==nullptr)
    RETURN_FAILURE;
  if(oObj==nullptr)
    RETURN_FAILURE;

  eXl::LuaConverter<T>::ConvertFromLua(this,oObj,iState,ioIndex);
  
  RETURN_SUCCESS;
}

template <class T>
void T_CoreType<T>::RegisterLua(lua_State* iState) const
{

}

template <class T>
luabind::object T_CoreType<T>::MakePropertyAccessor(lua_State* iState, Type const* iHolder, uint32_t iField) const
{
  using get_result_type = typename luabind::detail::reference_result<T>::type;
  using get_signature = luabind::meta::type_list<get_result_type, luabind::argument const&>;
  using injected_list = typename luabind::detail::inject_dependency_policy< T, luabind::no_policies >::type;

  luabind::object get_function = luabind::make_function(iState, access_field<T, get_result_type>(iHolder, iField), get_signature(), injected_list());

  using argument_type = typename luabind::detail::reference_argument<T>::type;
  using signature_type = luabind::meta::type_list<void, luabind::argument const&, argument_type>;
  
  luabind::object set_function = luabind::make_function(iState, access_field<T>(iHolder, iField), signature_type(), luabind::no_policies());

  return luabind::property(get_function, set_function);
}

template <class T>
luabind::object T_CoreType<T>::MakeElementAccessor(lua_State* iState, ArrayType const* iHolder) const
{
  using get_result_type = typename luabind::detail::reference_result<T>::type;
  using get_signature = luabind::meta::type_list<get_result_type, luabind::argument const&, int>;
  using injected_list = typename luabind::detail::inject_dependency_policy< luabind::object, luabind::no_policies >::type;

  luabind::object get_function = luabind::make_function(iState, access_element<T, get_result_type>(iHolder), get_signature(), injected_list());

  using argument_type = typename luabind::detail::reference_argument<T>::type;
  using signature_type = luabind::meta::type_list<void, luabind::argument const&, int, argument_type>;

  luabind::object set_function = luabind::make_function(iState, access_element<T>(iHolder), signature_type(), luabind::no_policies());

  return luabind::array_access(get_function, set_function);
}

#endif

template <class T,bool b>
struct DoCompare;

template <class T>
struct DoCompare<T,false>
{
  inline static Err Compare(void const* iVal1,void const* iVal2,CompRes& oRes)
  {
    return Err::Undefined;
  }
};

template <class T>
struct DoCompare<T,true>
{
  inline static Err Compare(void const* iVal1,void const* iVal2,CompRes& oRes)
  {
    oRes = TypeTraits::Compare<T>(iVal1,iVal2);

    RETURN_SUCCESS;
  }
};

template <class T>
Err T_CoreType<T>::Compare(void const* iVal1, void const* iVal2, CompRes& oRes)const
{
  if(iVal1 == nullptr || iVal2 == nullptr)
    RETURN_FAILURE;

  return DoCompare<T,::eXl::TypeTraits::IsComparable<T>::s_Value>::Compare(iVal1,iVal2,oRes);
}
