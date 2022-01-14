/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

template <typename T>
ArrayType const* TypeManager::GetArrayType()
{
  ArrayType const* registeredType = GetArrayType(GetType<T>());
  if (registeredType == nullptr)
  {
    registeredType = new CoreArrayType<T>();
    RegisterArrayType(registeredType);
  }

  return registeredType;
}

template <typename T, uint32_t S>
ArrayType const* TypeManager::GetSmallArrayType()
{
  ArrayType const* registeredType = GetSmallArrayType(GetType<T>(), S);
  if (registeredType == nullptr)
  {
    registeredType = new CoreSmallArrayType<T, S>();
    RegisterSmallArrayType(registeredType, S);
  }

  return registeredType;
}

template <>
inline ArrayType const* TypeManager::GetArrayType<bool>()
{
  return nullptr;
}

template <class T>
const TupleType* TypeManager::detail::RegisterSignature(size_t iId,const List<FieldDesc>& iFields)
{
  //const Type* res = TypeManager::MakeTuple(String("Signature")+StringUtil::FromSizeT(iId),iFields,&iId, nullptr);
  TupleType* newType = TupleTypeStruct::MakeTuple(TypeName(String("Signature")+StringUtil::FromSizeT(iId)),iFields,iId);
  Type const* regType = RegisterType(newType);
  if(regType)
  {
    return regType->IsTuple();
  }
  eXl_ASSERT_MSG(regType!=nullptr,"Wrong behaviour for RegisterSignature");
  return nullptr;
}

template <class T>
TypeManager::NativeTypeReg<T> TypeManager::BeginNativeTypeRegistration(TypeName iName){
  return NativeTypeReg<T>(iName);
}

//template <class T>
//TypeManager::NativeTypeReg<T> TypeManager::BeginNativeTypeRegistrationWithSign(const String& iName)
//{
//  return NativeTypeReg<T>(iName,GetTypeID<T>());
//}
  
template <class T>
TypeManager::NativeTypeReg<T>::NativeTypeReg(TypeName iName):m_Name(iName)
{
    
}

template <class T>
template <class U>
TypeManager::NativeTypeReg<T>& TypeManager::NativeTypeReg<T>::AddField(String const& iName,U T::* iOffset)
{
  BOOST_STATIC_ASSERT(sizeof(U T::*) <= sizeof(size_t));
  size_t offsetBuff = 0;
  memcpy(&offsetBuff, &iOffset, sizeof(U T::*));
  eXl_ASSERT(offsetBuff < sizeof(T));
  m_Fields.push_back(FieldDesc::MakeField(iName,iOffset));
  return *this;
}

template <class T>
template <class U>
TypeManager::NativeTypeReg<T>& TypeManager::NativeTypeReg<T>::AddField(const String& iName, Vector<U> T::* iOffset)
{
  BOOST_STATIC_ASSERT(sizeof(Vector<U> T::*) <= sizeof(size_t));
  size_t offsetBuff = 0;
  memcpy(&offsetBuff, &iOffset, sizeof(Vector<U> T::*));
  eXl_ASSERT(offsetBuff < sizeof(T));
  m_Fields.push_back(FieldDesc(TypeFieldName(iName), offsetBuff, GetArrayType<U>()));
  return *this;
}

template <class T>
template <class U>
TypeManager::NativeTypeReg<T>& TypeManager::NativeTypeReg<T>::AddCustomField(const String& iName, U T::* iOffset, Type const* iFieldType)
{
  BOOST_STATIC_ASSERT(sizeof(U T::*) <= sizeof(size_t));
  size_t offsetBuff = 0;
  memcpy(&offsetBuff, &iOffset, sizeof(U T::*));
  eXl_ASSERT(offsetBuff < sizeof(T));
  m_Fields.push_back(FieldDesc(TypeFieldName(iName), offsetBuff, iFieldType));
  return *this;
}
  
template <class T>
const TupleType* TypeManager::NativeTypeReg<T>::EndRegistration()
{
  TupleType* temp = CoreTupleType<T>::MakeTuple(m_Name, m_Fields, 0);
  const Type* res = RegisterType(temp);
  ArrayType const* registeredType = GetArrayType(res);
  if (registeredType == nullptr)
  {
    registeredType = new CoreArrayType<T>(res);
    RegisterArrayType(registeredType);
  }
  return res->IsTuple(); 
}
