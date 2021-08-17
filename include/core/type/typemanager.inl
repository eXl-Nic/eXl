/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

template <typename T>
ArrayType const* TypeManager::GetArrayType()
{
  if (GetType<T>() == GetType<bool>())
  {
    return nullptr;
  }
  ArrayType const* registeredType = GetArrayType(GetType<T>());
  if (registeredType == nullptr)
  {
    registeredType = new CoreArrayType<T>();
    RegisterArrayType(registeredType);
  }

  return registeredType;
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

template <class T, class U, U const& (T::*GetterMeth)()const>
void GetterTemplate(Type const*, void const* iObj, void* oProp)
{
  *reinterpret_cast<U*>(oProp) = (reinterpret_cast<T const*>(iObj)->*GetterMeth)();
}

template <class T, class U, U (T::*GetterMeth)()const>
void GetterValTemplate(Type const*, void const* iObj, void* oProp)
{
  *reinterpret_cast<U*>(oProp) = (reinterpret_cast<T const*>(iObj)->*GetterMeth)();
}

template <class T, class U, void(T::*SetterMeth)(U const& )>
void SetterTemplate(Type const*, void* iObj, void const* iProp)
{
  (reinterpret_cast<T*>(iObj)->*SetterMeth)(*reinterpret_cast<U const*>(iProp));
}

template <class T, class U, void(T::*SetterMeth)(U)>
void SetterValTemplate(Type const*, void* iObj, void const* iProp)
{
  (reinterpret_cast<T*>(iObj)->*SetterMeth)(*reinterpret_cast<U const*>(iProp));
}

//template <class T>
//template <class U, U const& (T::*GetterMeth)()const, void (T::*SetterMeth)(U const& iValue) >
//inline TypeManager::ClassTypeReg<T>& TypeManager::ClassTypeReg<T>::AddProperty(const String& iName)
//{
//  TypeManager::detail::AddProperty(m_Impl, TypeManager::GetCoreType<U>(), iName,
//    &GetterTemplate<T,U,GetterMeth>,&SetterTemplate<T,U,SetterMeth>);
//  return *this;
//}
//
//template <class T>
//template <class U, U (T::*GetterMeth)()const, void (T::*SetterMeth)(U iValue) >
//inline TypeManager::ClassTypeReg<T>& TypeManager::ClassTypeReg<T>::AddPropertyVal(const String& iName)
//{
//  TypeManager::detail::AddProperty(m_Impl, TypeManager::GetCoreType<U>(), iName,
//    &GetterValTemplate<T,U,GetterMeth>,&SetterValTemplate<T,U,SetterMeth>);
//  return *this;
//}
//
//template <class T>
//template <class U, U const& (T::*GetterMeth)()const>
//inline TypeManager::ClassTypeReg<T>& TypeManager::ClassTypeReg<T>::AddReadOnlyProperty(const String& iName)
//{
//  TypeManager::detail::AddProperty(m_Impl, TypeManager::GetCoreType<U>(), iName,
//    &GetterTemplate<T,U,GetterMeth>,nullptr);
//  return *this;
//}
//
//template <class T>
//template <class U, U (T::*GetterMeth)()const>
//inline TypeManager::ClassTypeReg<T>& TypeManager::ClassTypeReg<T>::AddReadOnlyPropertyVal(const String& iName)
//{
//  TypeManager::detail::AddProperty(m_Impl, TypeManager::GetCoreType<U>(), iName,
//    &GetterValTemplate<T,U,GetterMeth>,nullptr);
//  return *this;
//}
//
//template <class T>
//inline TypeManager::ClassTypeReg<T>& TypeManager::ClassTypeReg<T>::AddProperty(String const& iName, Type const* iType
//        , void(*GetterFun)(Type const*, void const* , void* )
//        , void(*SetterFun)(Type const*, void* , void const* ))
//{
//  TypeManager::detail::AddProperty(m_Impl, iType, iName,GetterFun,SetterFun);
//  return *this;
//}
      
template <class T>
inline const ClassType* TypeManager::ClassTypeReg<T>::EndRegistration()
{
  ClassType const* ret = TypeManager::detail::EndClassReg(m_Impl);
  m_Impl = nullptr;
  return ret;
}

template <class T>
inline TypeManager::ClassTypeReg<T>::ClassTypeReg(RttiObject* (*iObjectFactory)())
{
  m_Impl = TypeManager::detail::BeginClassRegistration(T::StaticRtti(),iObjectFactory);
}

template <class T>
inline TypeManager::ClassTypeReg<T>::ClassTypeReg(ClassTypeReg<T> const& iOther)
{
  m_Impl = iOther.m_Impl;
  iOther.m_Impl = nullptr;
}

template <class T>
inline TypeManager::ClassTypeReg<T>::~ClassTypeReg()
{
  if(m_Impl != nullptr)
    eXl_DELETE m_Impl;
  m_Impl = nullptr;
}

template <class T>
inline TypeManager::ClassTypeReg<T> TypeManager::BeginClassRegistration()
{
  return ClassTypeReg<T>(&T::CreateInstance);
}

template <class T>
inline TypeManager::ClassTypeReg<T> TypeManager::BeginClassRegistrationWithCtor(RttiObject* (*iObjectFactory)())
{
  return ClassTypeReg<T>(iObjectFactory);
}