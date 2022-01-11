/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/type/typemanager.hpp>
#include <core/type/tupletype.hpp>

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

  class EXL_CORE_API ArgsBuffer : public TupleType
  {
  public:
    ArgsBuffer(Vector<Type const*> const& iArgs);

    size_t GetNumField()const override;
    const Type* GetFieldDetails(unsigned int iNum)const override;
    const Type* GetFieldDetails(TypeFieldName iFieldName)const override;
    const Type* GetFieldDetails(unsigned int iNum, TypeFieldName& oFieldName)const override;
    const Type* GetFieldDetails(TypeFieldName iFieldName, unsigned int& oNumField)const override;
    Err ResolveFieldPath(AString const& iPath, unsigned int& oOffset, Type const*& oType)const override;
    void* GetField(void* iObj, unsigned int iIdx, Type const*& oType)const override;
    void* GetField(void* iObj, TypeFieldName iName, Type const*& oType)const override;
    void const* GetField(void const* iObj, unsigned int, Type const*& oType)const override;
    void const* GetField(void const* iObj, TypeFieldName iName, Type const*& oType)const override;
#ifdef EXL_LUA
    Err ConvertFromLuaRaw_Uninit(lua_State* iState, unsigned int& ioIndex, void* oObj)const override;
    luabind::object ConvertToLua(void const* iObj, lua_State* iState) const override;
    Err ConvertFromLua_Uninit(lua_State* iState, unsigned int& ioIndex, void* oObj) const override;
    void RegisterLua(lua_State* iState) const override;
#endif
  protected:
    Vector<Type const*> const& m_Args;
    Vector<size_t> m_Offsets;
  };

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

  template <typename Type, uint32_t Position >
  struct PositionalArg
  {
    using ArgType = typename std::remove_reference<typename std::remove_const<Type>::type>::type;
    static constexpr uint32_t ArgPos = Position;
  };

  template <uint32_t Step>
  struct ParsingSeparator {};

  template <typename... Args>
  struct PositionalList{};

  template <typename Dummy, uint32_t Step, uint32_t Max, typename... Args >
  struct MakePositionalList_Impl;

  template <typename Dummy, uint32_t Max, typename... Args >
  struct MakePositionalList_Impl<Dummy, Max, Max, Args...>
  {
    using type = PositionalList<Args...>;
  };

  template <uint32_t Step, uint32_t Max, typename NextArg, typename... Args >
  struct MakePositionalList_Impl <typename std::enable_if<Step != Max, bool>::type, Step, Max, NextArg, Args...>
     : MakePositionalList_Impl<bool, Step + 1, Max, Args..., PositionalArg<NextArg, Step>>
  {
    //using type = typename MakePositionalList_Impl<bool, Step + 1, Max, Args..., PositionalArg<NextArg, Step>>::type;
  };

  template <typename... Args>
  struct MakePositionalList : MakePositionalList_Impl<bool, 0, sizeof...(Args), Args...>
  {
    //using type = typename MakePositionalList_Impl<bool, 0, sizeof...(Args), Args...>::type;
  };

  template <typename ArgsList>
  struct PositionalInvoker;

  template <typename... Args>
  struct PositionalInvoker<PositionalList<Args...>>
  {
    template <typename RetType, typename Function>
    static RetType Call(Function const& iFun, ConstDynObject const& iBuffer)
    {
      return iFun(*iBuffer.GetField<typename Args::ArgType>(uint32_t(Args::ArgPos))...);
    }
    //template <typename Positional>
    //typename Positional::ArgType const* Apply()
    //{}
  };

  template <typename... Args>
  struct Invoker
  {
    template <typename RetType, typename Function>
    static RetType Call(Function const& iFun, ConstDynObject const& iBuffer)
    {
      return PositionalInvoker<typename MakePositionalList<Args...>::type>::template Call<RetType>(iFun, iBuffer);
    }
  };
}