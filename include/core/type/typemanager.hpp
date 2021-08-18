/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/type/typemanager_get.hpp>
#include <core/type/arraytype.hpp>
#include <core/type/tupletypestruct.hpp>

namespace luabind
{
  struct null_type;
}

namespace eXl
{
  namespace TypeManager
  {
    /**

    **********************************************************************/
    EXL_CORE_API const Type* RegisterType(Type const* iType);

    template <typename T>
    const Type* RegisterCoreType()
    {
      Type const* coreType = GetType<T>();
      RegisterType(coreType);
      GetArrayType<T>();

      return coreType;
    }

    EXL_CORE_API const Type* GetCoreTypeFromName(TypeName);

    EXL_CORE_API Vector<Type const*> GetCoreTypes();

    /**
       Registraction class used to create user types which may not have C++ counterparts.
    **********************************************************************/
    class EXL_CORE_API UsrTypeReg
    {
      friend EXL_CORE_API UsrTypeReg BeginTypeRegistration(TypeName iName);
    public:
      
      UsrTypeReg& AddField(TypeFieldName iName, Type const* iType);

      UsrTypeReg& AddFieldsFrom(const TupleType* iType);

      const TupleType* EndRegistration();
    private:
      UsrTypeReg(TypeName iName);
      TypeName m_Name;
      List<FieldDesc> m_Fields;
      size_t m_Offset;
    };

    /**
       Return a registration class to create dynamic objects.
    **********************************************************************/
    EXL_CORE_API UsrTypeReg BeginTypeRegistration(TypeName iName);

    class EXL_CORE_API SignTypeReg
    {
      friend EXL_CORE_API SignTypeReg BeginSignatureRegistration();
    public:

      SignTypeReg& AddParam(const Type* iType);

      const TupleType* EndRegistration();

    private:
      SignTypeReg();
      List<FieldDesc> m_Fields;
      size_t m_Offset;
    };

    /**
       Return a registration class to create signatures.
    **********************************************************************/
    EXL_CORE_API SignTypeReg BeginSignatureRegistration();

    class EnumTypeReg;

    class EXL_CORE_API EnumTypeReg
    {
      friend EXL_CORE_API EnumTypeReg BeginEnumTypeRegistration(TypeName iName);
    public:
      
      EnumTypeReg& AddValue(TypeEnumName iName);

      Type const* EndRegistration();
    private:
      EnumTypeReg(TypeName iName);
      TypeName m_Name;
      Vector<TypeEnumName> m_Enums;
    };

    EXL_CORE_API EnumTypeReg BeginEnumTypeRegistration(TypeName iName);
    
    template <class T>
    class NativeTypeReg;
    
    /**
       Returns a NativeTypeReg to register C++ types in the reflexion system.
    **********************************************************************/
    template <class T>
    NativeTypeReg<T> BeginNativeTypeRegistration(TypeName iName);

    /**
       This version of the call assumes that an TypeID has been declared for T, so it can be used in Signatures.
    **********************************************************************/
    template <class T>
    NativeTypeReg<T> BeginNativeTypeRegistrationWithSign(TypeName iName);
    
    /**
       Registraction class used to map C++ structs to the reflexion system.
    **********************************************************************/
    template <class T>
    class NativeTypeReg
    {
      friend NativeTypeReg<T> BeginNativeTypeRegistration<T>(TypeName iName);
      friend NativeTypeReg<T> BeginNativeTypeRegistrationWithSign<T>(TypeName iName);
    public:
    
      template <class U>
      NativeTypeReg& AddField(const String& iName,U T::* iOffset);

      template <class U>
      NativeTypeReg& AddField(const String& iName, Vector<U> T::* iOffset);

      template <class U>
      NativeTypeReg& AddCustomField(const String& iName, U T::* iOffset, Type const* iFieldType);
    
      //inline NativeTypeReg& SetCtors(const CtorsDesc& iCtors){
      //  if(iCtors.IsOk())
      //    m_Ctors = iCtors;
      //  return *this;
      //}
      
      const TupleType* EndRegistration(/*ResourceContainer* iCont*/);
    private:
      NativeTypeReg(TypeName iName);
      String m_Name;
      List<FieldDesc> m_Fields;
    };
    
    /**
       Registraction class used to map C++ types to the reflexion system.
       This is not a Type, as these objects will be references through RttiOPtr
    **********************************************************************/
    class ClassTypeRegImpl;

    template <class T>
    class ClassTypeReg;

    /**
       Register a class type, to be able to create it, manipulate its properties.
       If the object inherits RttiObjectRefC, you'll also be able to retrieve an ObjectPtr to reference it.
       NB : Object must have static methods CreateInstance and CopyInstance
       NB : Direct parent class must be RttiObjectRefC, or be registered
    **********************************************************************/
    template <class T>
    ClassTypeReg<T> BeginClassRegistration();

    /**
       Same as previous method, but htis time you can specify the ctor yourself.
    **********************************************************************/
    template <class T>
    ClassTypeReg<T> BeginClassRegistrationWithCtor(RttiObject* (*iObjectFactory)());

    template <class T>
    class ClassTypeReg
    {
      friend ClassTypeReg<T> BeginClassRegistration<T>();
      friend ClassTypeReg<T> BeginClassRegistrationWithCtor<T>(RttiObject* (*iObjectFactory)());
    public:
      inline ClassTypeReg(ClassTypeReg const&);
      inline ~ClassTypeReg();

      //template <class U, U const& (T::*GetterMeth)()const, void (T::*SetterMeth)(U const& iValue) >
      //inline ClassTypeReg& AddProperty(const String& iName);
      //
      //template <class U, U (T::*GetterMeth)()const, void (T::*SetterMeth)(U iValue) >
      //inline ClassTypeReg& AddPropertyVal(const String& iName);
      //
      //template <class U, U const& (T::*GetterMeth)()const>
      //inline ClassTypeReg& AddReadOnlyProperty(const String& iName);
      //
      //template <class U, U (T::*GetterMeth)()const>
      //inline ClassTypeReg& AddReadOnlyPropertyVal(const String& iName);
      //
      //inline ClassTypeReg& AddProperty(String const& iName, Type const* iType
      //  , void(*GetterFun)(Type const*, void const* , void* )
      //  , void(*SetterFun)(Type const*, void* , void const* ));
      
      const ClassType* EndRegistration();
    private:
      //Provide an Id to make the glue between an introspected struct and signatures.
      inline ClassTypeReg(RttiObject* (*iObjectFactory)());
      mutable ClassTypeRegImpl* m_Impl;
    };

    /**
       Register a class for which you want to retrieve an ObjectPtr or a ResourceHandle, but with no additionnal info.
       You won't be able to create it through a class type.
       Eg : Have a pointer to an abstract class to be able to specify derived classes.
       NB : Direct parent class must be registered
    **********************************************************************/
    EXL_CORE_API Err RegisterTransientClassForRtti(Rtti const& iRtti);

    template <class T>
    inline Err RegisterTransientClassFor(){return RegisterTransientClassForRtti(T::StaticRtti());}

    /**
       Register a custom class type.
    **********************************************************************/
    EXL_CORE_API Err RegisterClassForRtti(Rtti const& iRtti, ClassType const* iType);

    template <class T>
    inline Err RegisterClassFor(ClassType const* iClass){return RegisterClassForRtti(T::StaticRtti(),iClass);}

    /**
       
    **********************************************************************/
    EXL_CORE_API ClassType const* GetClassForRtti(Rtti const& iRtti);

    /**
       
    **********************************************************************/
    EXL_CORE_API ClassType const* GetClassForName(TypeName iName);

    template <class T>
    inline ClassType const* GetClassFor(){return GetClassForRtti(T::StaticRtti());}

    /**
       
    **********************************************************************/
    EXL_CORE_API ObjectPtrType const* GetObjectPtrForRtti(Rtti const& iRtti);

    template <class T>
    inline ObjectPtrType const* GetObjectPtrFor(){return GetObjectPtrForRtti(T::StaticRtti());}

    /**
       List rtti derived from a specified rtti, and registered in the manager.
    **********************************************************************/
    EXL_CORE_API Err ListDerivedClassesForRtti(Rtti const& iRtti, List<Rtti const*>& oList);

    /**
       
    **********************************************************************/
    EXL_CORE_API ResourceHandleType const* GetResourceHandleForRtti(Rtti const& iRtti); 

    template <class T>
    inline ResourceHandleType const* GetResourceHandleFor(){return GetResourceHandleForRtti(T::StaticRtti());}

    /**
       (internal use)
    **********************************************************************/
    EXL_CORE_API const TupleType* MakeTuple(TypeName iName,const List<FieldDesc>& iList,size_t* iId);

    namespace detail
    {
      void _TMClear();

      EnumType const* _MakeEnumType(TypeName iName, Vector<TypeEnumName>& iVal);

      template <class T>
      const TupleType* RegisterSignature(size_t iId,const List<FieldDesc>& iFields);

      EXL_CORE_API ClassTypeRegImpl* BeginClassRegistration(Rtti const& iRtti
        , RttiObject* (*iObjectFactory)());

      EXL_CORE_API ClassType const* EndClassReg(ClassTypeRegImpl* iRegImpl);
    }
  }
}

#include <core/plugin.hpp>
//#include <core/type/coretype.hpp>
//#include <core/type/arraytype.hpp>

namespace eXl
{
  #include "typemanager.inl"

  template <class Storage,class T,unsigned int I>
  FieldDesc FieldDesc::MakeField()
  {
    return FieldDesc(AString("Field")+StringUtil::FromInt(I), Storage::template GetOffset<I>(), TypeManager::GetType<T>());
  }

  template <class T,class U>
  FieldDesc FieldDesc::MakeField(TypeFieldName iName,U T::* iOffset)
  {
    union {U T::* clOffset;unsigned int uiOffset;} horribleCast;
    horribleCast.clOffset = iOffset;
    return FieldDesc(iName,horribleCast.uiOffset, TypeManager::GetType<U>());
  }
}

