/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/type/typemanager.hpp>
#include <core/type/tupletype.hpp>
#include <core/type/dynobject.hpp>
#include <core/utils/positional.hpp>

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

  template<>
  struct GetArgList<>
  {
    static void GetArgs(Vector<Type const*>& oArgs)
    {}
  };

  template<typename... Args>
  struct ComputeArgListStorage;

  template<typename Arg1, typename... Args>
  struct ComputeArgListStorage<Arg1, Args...>
  {
    static constexpr size_t size = sizeof(Arg1) + ComputeArgListStorage<Args...>::size;
  };

  template<>
  struct ComputeArgListStorage<>
  {
    static constexpr size_t size = 0;
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
    ArgsBuffer(Vector<Type const*>&& iArgs);

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
    size_t GetOffset(uint32_t iField) const { return m_Offsets[iField]; }

    Vector<Type const*> const m_Args;
  protected:
    Vector<size_t> const m_Offsets;
  };

  template<uint32_t Step, typename... Args>
  struct BufferPopulator;

  struct FunDesc
  {
    FunDesc(Type const* iRetType, Vector<Type const*> iArguments)
      : m_RetType(iRetType)
      , m_Type(std::move(iArguments))
    {}

    template <typename RetType, typename... Args>
    bool ValidateSignature() const
    {
      return TypeManager::GetType<RetType>() == GetRetType()
        && ValidateArgList<Args...>::Validate(0, m_Type.m_Args);
    }

    template <typename... Args>
    Err PopulateArgBuffer(DynObject& oBuffer, Args&&... iArgs) const
    {
      if (!ValidateArgList<Args...>::Validate(0, m_Type.m_Args))
      {
        return Err::Failure;
      }

      ArgsBuffer const& type = GetType();

      oBuffer.SetType(&type, type.Alloc(), true);
      BufferPopulator<0, Args...>::Populate(iType, oBuffer, std::forward<Args>(iArgs)...);
      return Err::Success;
    }

    template <class FunType>
    static FunDesc Create()
    {
      return GetFunDesc_Impl<FunType>::Create();
    }

    Type const* GetRetType() const { return m_RetType; }
    Vector<Type const*> const& GetArgs() const { return m_Type.m_Args; }

    ArgsBuffer const& GetType() const
    {
      return m_Type;
    }

  private:
    Type const* m_RetType;
    ArgsBuffer m_Type;
  };

  template<uint32_t Step, typename Arg, typename... Args>
  struct BufferPopulator<Step, Arg, Args...>
  {
    static void Populate(ArgsBuffer const& iType, DynObject& oData, Arg&& iArg, Args&&... iArgs)
    {
      using ConcreteType = typename std::remove_const<typename std::remove_reference<Arg>::type>::type;
      new(((uint8_t*)oData.GetBuffer()) + iType.GetOffset(Step)) ConcreteType(std::forward<Arg>(iArg));
      BufferPopulator<Step + 1, Args...>::Populate(iType, oData, std::forward<Args>(iArgs)...);
    }
  };

  template<uint32_t Step>
  struct BufferPopulator<Step>
  {
    static void Populate(ArgsBuffer const& iType, DynObject& oData)
    {
    }
  };

  template<typename RetType, typename... Args>
  struct GetFunDesc_Impl<RetType(Args...)>
  {
    static FunDesc Create()
    {
      Type const* returnType = TypeManager::GetType<RetType>();
      Vector<Type const*> arguments;
      GetArgList<Args...>::GetArgs(arguments);

      return FunDesc(returnType, std::move(arguments));
    }
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

  template<typename RetType, typename... Args>
  struct Invoker_RetWrapper
  {
    template <typename Function>
    static void Execute(Function const& iFun, ConstDynObject const& iArgsBuffer, DynObject& oOutput)
    {
      Type const* retType = TypeManager::GetType<RetType>();
      oOutput.SetType(retType, retType->Build(), true);
      *oOutput.CastBuffer<RetType>() = Invoker<Args...>:: template Call<RetType>(iFun, iArgsBuffer);
    }
  };

  template<typename... Args>
  struct Invoker_RetWrapper<void, Args...>
  {
    template <typename Function>
    static void Execute(Function const& iFun, ConstDynObject const& iArgsBuffer, DynObject& oOutput)
    {
      Invoker<Args...>::template Call<void>(iFun, iArgsBuffer);
    }
  };
}