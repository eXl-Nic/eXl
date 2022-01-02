/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>
#include <core/type/typedefs.hpp>
#include <core/corelibexp.hpp>

#define DEFINE_TYPE_EX(type, friendlyname, DLL) \
  DLL Type const* Get_##friendlyname##_NativeType(); \
                                           \
  template <>                              \
  struct IsNativeType<type>                \
  {                                        \
    static constexpr bool s_Value = true;  \
  };                                       \
                                           \
  template <>                              \
  struct GetNativeTypeHandler<type>        \
  {                                        \
    static Type const* Get()               \
    {                                      \
      return Get_##friendlyname##_NativeType(); \
    }                                      \
  };                                       \
                                           \
  template <>                              \
  inline TypeName GetTypeName<type>()      \
  {                                        \
    return TypeName(#type);                \
  }

#define REDIRECT_TYPE_EX(type, baseType) \
  template <>                              \
  struct IsNativeType<type>                \
  {                                        \
    static constexpr bool s_Value = true;  \
  };                                       \
                                           \
  template <>                              \
  struct GetNativeTypeHandler<type>        \
  {                                        \
    static Type const* Get()               \
    {                                      \
      return GetNativeTypeHandler<baseType>::Get(); \
    }                                      \
  };                                       \
                                           \
  template <>                              \
  inline TypeName GetTypeName<type>()      \
  {                                        \
    return TypeName(#type);                \
  }

#define DEFINE_CORE_TYPE_EX(type, friendlyname) DEFINE_TYPE_EX(type, friendlyname, EXL_CORE_API)
#define DEFINE_CORE_TYPE(type) DEFINE_CORE_TYPE_EX(type, type)

#define IMPLEMENT_TYPE_EX(type, friendlyname) \
  Type const* Get_##friendlyname##_NativeType() \
  {                                             \
     static T_CoreType<type> s_Type;            \
     return &s_Type;                            \
  }

#define IMPLEMENT_TAG_TYPE_EX(type, friendlyname) \
  Type const* Get_##friendlyname##_NativeType() \
  {                                             \
     static T_TagType<type> s_Type;            \
     return &s_Type;                            \
  }

#define DEFINE_ENUM_TYPE(Enum, friendlyName, DLL)      \
template <>                              \
struct IsEnumType<Enum>                  \
{                                        \
  static constexpr bool s_Value = true;  \
};                                       \
DLL Type const* Get_##friendlyName##_EnumType();       \
template <>                                            \
struct GetEnumTypeHandler<Enum>                        \
{                                                      \
  static Type const* Get()                             \
  {                                                    \
    return Get_##friendlyName##_EnumType();            \
  }                                                    \
};                                                      \
template <>                              \
inline TypeName GetTypeName<Enum>()      \
{                                        \
return TypeName(#Enum);                \
}

#define IMPLEMENT_TYPE(type) IMPLEMENT_TYPE_EX(type, type)
#define IMPLEMENT_TAG_TYPE(type) IMPLEMENT_TAG_TYPE_EX(type, type)

#define EXL_REFLECTION_MARKER eXl_NeedReflection
#define EXL_REFLECTION_MARKER_STR eXl_TO_STR(EXL_REFLECTION_MARKER)

#ifdef EXL_REFLANG_COMPILER
  #define EXL_REFLECT           \
    public: \
    static void EXL_REFLECTION_MARKER()
  #define EXL_REFLECT_ENUM(Enum, FriendlyName, DLL) \
    void DeclareEnumReflection(Enum iArg) {}
#else
  #define EXL_REFLECT           \
  public:                       \
    static constexpr bool eXl_Reflected = true; \
    static Type const* GetType(); \
    Err Stream(Streamer& iStreamer) const; \
    Err Unstream(Unstreamer& iStreamer);
  #define EXL_REFLECT_ENUM(Enum, FriendlyName, DLL) DEFINE_ENUM_TYPE(Enum, FriendlyName, DLL)
#endif

#include <core/name.hpp>

extern "C" struct lua_State;
namespace eXl
{
  template <typename T>
  struct IsNativeType
  {
    static constexpr bool s_Value = false;
  };

  template <typename T>
  struct IsVectorType
  {
    static constexpr bool s_Value = false;
  };

  template <typename T>
  struct IsVectorType<Vector<T>>
  {
    static constexpr bool s_Value = true;
  };

  template <typename T, uint32_t Size>
  struct IsVectorType<SmallVector<T, Size>>
  {
    static constexpr bool s_Value = true;
  };

  template <typename T>
  struct IsEnumType 
  {
    static constexpr bool s_Value = false;
  };

  template <typename T>
  struct IsNameType
  {
    static constexpr bool s_Value = false;
  };

  template <typename T>
  struct GetNativeTypeHandler;

  template <typename T>
  struct GetEnumTypeHandler;

  class Type;
  class CoreType;
  class EnumType;
  class DynObject;
  class ConstDynObject;
  class Streamer;
  class Unstreamer;
  class Rtti;

  namespace ResourceManager
  {
    EXL_CORE_API Type const* GetHandleType(Rtti const& iRtti);
  }

  template <typename T>
  class ResourceHandle;

  template <typename T>
  struct IsNativeType<ResourceHandle<T>>
  {
    static constexpr bool s_Value = true;
  };

  template <typename T>
  struct GetNativeTypeHandler<ResourceHandle<T>>
  {
    static Type const* Get()
    {
      return ResourceManager::GetHandleType(T::StaticRtti());
    }
  };

  template <class T,bool b>
  struct CompareStruct;

  template<class T>
  TypeName GetTypeName();

  namespace TypeTraits
  {

    template <class T>
    inline void* Alloc(){return eXl_ALLOC(sizeof(T));}

    template <class T>
    inline void  Free(void* iPtr){eXl_FREE(iPtr);}

    template <class T>
    inline void  DTor(void* iPtr){
      ((T*)iPtr)->~T();
    }
    template <class T>
    inline void* DefaultCTor(void* iPtr){return new(iPtr) T;}

    //If Copy do not build the object in place, it must dealloc the old buffer
    template <class T>
    inline void* Copy(void* iPtr,const void* iVal){return new(iPtr) T(*((T*)iVal));}

    template <class T>
    inline size_t GetSize(){return sizeof(T);}

    template <class T>
    struct IsOrdered{static constexpr bool s_Value = false;};

    template <class T>
    struct IsComparable{static constexpr bool s_Value = true;};

    template <class T>
    inline CompRes Compare(void const* iVal1,void const* iVal2){return CompareStruct<T,IsOrdered<T>::s_Value>::Compare(iVal1,iVal2);}

    template <class T>
    inline Err Stream(T const* iObj, Streamer& iStreamer);

    template <class T>
    inline Err Unstream(T* oObj, Unstreamer& iUnstreamer);

  };

  template <class T>
  struct CompareStruct<T,true>
  {
    inline static CompRes Compare(void const* iVal1,void const* iVal2)
    {
      T const& iObj1 = *(T*)iVal1;
      T const& iObj2 = *(T*)iVal2;

      if(iObj1<iObj2)
        return CompLesser;
      else if(iObj1>iObj2)
        return CompGreater;
      else
        return CompEqual;
    }
  };

  template <class T>
  struct CompareStruct<T,false>
  {
    inline static CompRes Compare(void const* iVal1,void const* iVal2)
    {
      T const& iObj1 = *(T*)iVal1;
      T const& iObj2 = *(T*)iVal2;

      if(iObj1==iObj2)
        return CompEqual;
      else
        return CompDifferent;
    }
  };

  namespace TypeTraits
  {
    template <>
    struct IsOrdered<unsigned int> { static constexpr bool s_Value = true; };

    template <>
    struct IsOrdered<int> { static constexpr bool s_Value = true; };

    template <>
    struct IsOrdered<float> { static constexpr bool s_Value = true; };

    template <>
    struct IsOrdered<unsigned char> { static constexpr bool s_Value = true; };
  }

  DEFINE_CORE_TYPE(uint32_t)
  DEFINE_CORE_TYPE(uint64_t)
  DEFINE_CORE_TYPE(int32_t)
  DEFINE_CORE_TYPE(float)
  DEFINE_CORE_TYPE(bool)
  DEFINE_CORE_TYPE(uint8_t)
  DEFINE_CORE_TYPE(uint16_t)
#ifndef __ANDROID__
  //DEFINE_CORE_TYPE(WString)
#endif
  DEFINE_CORE_TYPE(AString)
  DEFINE_CORE_TYPE(KString)
  DEFINE_CORE_TYPE_EX(Name, NameBase_T)

  DEFINE_CORE_TYPE(ConstDynObject)
  DEFINE_CORE_TYPE(DynObject)
  DEFINE_CORE_TYPE(Rtti)
  DEFINE_CORE_TYPE(Err)
}

#include <core/stream/streamer.hpp>
#include <core/stream/unstreamer.hpp>

namespace eXl
{
  namespace TypeTraits
  {
    template <class T>
    inline Err Stream(T const* iObj, Streamer& iStreamer)
    {
      return iObj->Stream(iStreamer);
    }

    template <class T>
    inline Err Unstream(T* oObj, Unstreamer& iUnstreamer)
    {
      return oObj->Unstream(iUnstreamer);
    }
  }
}

