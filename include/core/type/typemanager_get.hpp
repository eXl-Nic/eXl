/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>
#include <core/type/typetraits.hpp>
#include <core/type/fielddesc.hpp>

namespace luabind
{
  struct null_type;
}

namespace eXl
{
  class Type;
  class ClassType;
  class ObjectPtrType;
  class ResourceHandleType;
  class CoreType;
  class ArrayType;
  class EnumType;
  class RttiObject;
  class TupleType;
  namespace TypeManager
  {

    EXL_CORE_API ArrayType const* GetArrayType(Type const* iType);
    EXL_CORE_API void RegisterArrayType(ArrayType const* iType);
    EXL_CORE_API ArrayType const* GetSmallArrayType(Type const* iType, uint32_t iBufferSize);
    EXL_CORE_API void RegisterSmallArrayType(ArrayType const* iType, uint32_t iBufferSize);

    template <typename T>
    ArrayType const* GetArrayType();

    template <typename T, uint32_t S>
    ArrayType const* GetSmallArrayType();

    template<class T, typename std::enable_if<std::is_same<T, void>::value, bool>::type = true>
    inline Type const* GetTypeDispatched()
    {
      return nullptr;
    }

    template<class T, typename std::enable_if<std::is_same<T, luabind::null_type>::value, bool>::type = true>
    inline Type const* GetTypeDispatched()
    {
      return nullptr;
    }

    template<class T, typename std::enable_if<IsNativeType<T>::s_Value, bool>::type = true>
    inline Type const* GetTypeDispatched()
    {
      return GetNativeTypeHandler<T>::Get();
    }

    template<typename T, typename std::enable_if<IsEnumType<T>::s_Value, bool>::type = true>
    inline Type const* GetTypeDispatched()
    {
      return GetEnumTypeHandler<T>::Get();
    }

    template<class T, typename std::enable_if<IsVectorType<T>::s_Value, bool>::type = true>
    inline Type const* GetTypeDispatched()
    {
      return GetArrayType<typename T::value_type>();
    }

    template<typename T>
    struct SmallVectorStaticSize;

    template<typename T, uint32_t S>
    struct SmallVectorStaticSize<SmallVector<T, S>>
    {
      static constexpr uint32_t value = S;
    };

    template<class T, typename std::enable_if<IsSmallVectorType<T>::s_Value, bool>::type = true>
    inline Type const* GetTypeDispatched()
    {
      return GetSmallArrayType<typename T::value_type, SmallVectorStaticSize<T>::value> ();
    }

    template<class T, decltype(T::eXl_Reflected) R = true>
    inline Type const* GetTypeDispatched()
    {
      return T::GetType();
    }

    template<class T, typename std::enable_if<std::is_same_v<typename T::TheRttiClass, T>, bool>::type = true>
    inline Type const* GetTypeDispatched()
    {
      return T::GetType();
    }

    template <typename T>
    struct GetStructTypeHandler_Dispatcher
    {
      static inline Type const* GetType()
      {
        return GetTypeDispatched<T>();
      }
    };

    template <typename T>
    struct GetStructTypeHandler_Dispatcher<T&>
    {
      static inline Type const* GetType()
      {
        return GetStructTypeHandler_Dispatcher<T>::GetType();
      }
    };

    template <typename T>
    struct GetStructTypeHandler_Dispatcher<T const>
    {
      static inline Type const* GetType()
      {
        return GetStructTypeHandler_Dispatcher<T>::GetType();
      }
    };

    template <typename T>
    struct GetStructTypeHandler_Dispatcher<T volatile>
    {
      static inline Type const* GetType()
      {
        return GetStructTypeHandler_Dispatcher<T>::GetType();
      }
    };

    template <typename T>
    struct GetStructTypeHandler_Dispatcher<T*>
    {
      static inline Type const* GetType()
      {
        return GetStructTypeHandler_Dispatcher<T>::GetType();
      }
    };

    template<class T>
    inline Type const* GetType()
    {
      return GetStructTypeHandler_Dispatcher<T>::GetType();
    }
  }
}