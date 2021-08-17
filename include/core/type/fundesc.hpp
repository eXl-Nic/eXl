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