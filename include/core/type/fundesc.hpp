/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/type/typemanager.hpp>

namespace eXl
{
  template<typename... Args>
  struct GetArgList;

  template<typename Arg1, typename... Args>
  struct GetArgList<Arg1, Args...>
  {
    static void GetArgs(Vector<Type const*>& oArgs)
    {
      Type const* typePtr = TypeManager::GetType<Arg1>();
      if (typePtr)
      {
        oArgs.push_back(typePtr);
      }
      GetArgList<Args...>::GetArgs(oArgs);
    }
  };

  template<typename Arg>
  struct GetArgList<Arg>
  {
    static void GetArgs(Vector<Type const*>& oArgs)
    {
      Type const* typePtr = TypeManager::GetType<Arg>();
      if (typePtr)
      {
        oArgs.push_back(typePtr);
      }
    }
  };

  template<>
  struct GetArgList<void>
  {
    static void GetArgs(Vector<Type const*>& oArgs)
    {}
  };

  template<typename... Args>
  struct ValidateArgList;

  template<typename Arg1, typename... Args>
  struct ValidateArgList<Arg1, Args...>
  {
    static bool Validate(uint32_t iIdx, Vector<Type const*> const& iArgs)
    {
      if (iIdx >= iArgs.size())
      {
        return false;
      }

      using BaseType = typename std::remove_cv<typename std::remove_reference<Arg1>::type>::type;
      Type const* typePtr = TypeManager::GetType<BaseType>();
      if (typePtr != iArgs[iIdx])
      {
        return false;
      }
      return ValidateArgList<Args...>::Validate(iIdx + 1, iArgs);
    }
  };

  template<typename Arg>
  struct ValidateArgList<Arg>
  {
    static bool Validate(uint32_t iIdx, Vector<Type const*> const& iArgs)
    {
      if (iIdx >= iArgs.size())
      {
        return false;
      }

      using BaseType = typename std::remove_cv<typename std::remove_reference<Arg>::type>::type;
      Type const* typePtr = TypeManager::GetType<BaseType>();
      if (typePtr != iArgs[iIdx])
      {
        return false;
      }

      return iIdx == iArgs.size() - 1;
    }
  };

  template<>
  struct ValidateArgList<void>
  {
    static bool Validate(uint32_t iIdx, Vector<Type const*> const& iArgs)
    {
      return iIdx == 0 && iArgs.empty();
    }
  };

  template<typename FunType>
  struct GetFunDesc_Impl;

  struct FunDesc
  {
    Type const* returnType;
    Vector<Type const*> arguments;

    template <typename RetType, typename... Args>
    bool ValidateSignature() const
    {
      return TypeManager::GetType<RetType>() == returnType
        && ValidateArgList<Args...>::Validate(0, arguments);
    }
#ifdef EXL_LUA
    void PushLuaArgs(LuaStateHandle)
    {

    }
#endif

    template <class FunType>
    static FunDesc Create()
    {
      return GetFunDesc_Impl<FunType>::Create();
    }
  };

  template<typename RetType, typename... Args>
  struct GetFunDesc_Impl<RetType(Args...)>
  {
    static FunDesc Create()
    {
      FunDesc newDesc;
      newDesc.returnType = TypeManager::GetType<RetType>();
      GetArgList<Args...>::GetArgs(newDesc.arguments);

      return newDesc;
    }
  };
}