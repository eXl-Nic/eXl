/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#ifdef EXL_LUA

#include <core/coredef.hpp>
#include <luabind/object.hpp>
#include <boost/optional.hpp>
#include <core/rtti.hpp>
#include <core/type/typemanager_get.hpp>
#include <core/type/dynobject.hpp>

extern "C" struct lua_State;

#define LUA_REG_FUN(FunctionName) int FunctionName(lua_State* iState)

namespace eXl
{
  class Type;
  class LuaImplState;
  class LuaScript;

  template<typename... Args>
  struct PushArgsImpl;

  class EXL_CORE_API LuaStateHandle
  {
  public:
    LuaStateHandle(LuaImplState* iImpl);

    ~LuaStateHandle();

    LuaStateHandle(LuaStateHandle const&);
    LuaStateHandle& operator=(LuaStateHandle const&);
    LuaStateHandle(LuaStateHandle&&);
    LuaStateHandle& operator=(LuaStateHandle&&);

    inline lua_State* GetState() { return m_State; }

    inline LuaImplState* GetImpl() { return m_Impl; }

    RttiObject* GetUserPtr();

    // Ensure stack is cleaned up after.
    class EXL_CORE_API CallCtx
    {
    public:
      ~CallCtx();

      CallCtx(CallCtx const&) = delete;
      CallCtx& operator=(CallCtx const&) = delete;
      CallCtx(CallCtx&&) = default;
      CallCtx& operator=(CallCtx&&) = default;

      void Push(luabind::object const& iObj)
      {
        iObj.push(m_State);
        m_NumArgs++;
      }

      template <typename... Args>
      void PushArgs(Args&&... iArgs)
      {
        m_NumArgs += PushArgsImpl<Args...>::Push(m_State, std::forward<Args>(iArgs)...);
      }

      void ArgPushed()
      {
        ++m_NumArgs;
      }

      boost::optional<uint32_t> Call(uint32_t iExpectedRes);

    protected:
      friend LuaStateHandle;
      CallCtx(lua_State* iState, luabind::object const& iFunObj);

      lua_State* m_State;
      uint32_t m_StackTop;
      uint32_t m_NumArgs = 0;
    };

    CallCtx PrepareCall(luabind::object const& iFunObj);

  protected:
    LuaImplState* m_Impl;
    lua_State* m_State;
  private:

    void* operator new(size_t);
    void* operator new(size_t, void*);
    void operator delete(void*);
    void operator delete(void*, void*);
    void* operator new[](size_t size);
    void* operator new[](size_t size, void*);
    void operator delete[](void*);
    void operator delete[](void*, void*);
  };

  class EXL_CORE_API LuaWorld
  {
  public:
    struct Impl;

    LuaWorld(std::unique_ptr<LuaImplState> iState);
    LuaWorld(LuaWorld const&) = delete;
    LuaWorld& operator=(LuaWorld const&) = delete;
    LuaWorld(LuaWorld&&);
    LuaWorld& operator=(LuaWorld&&);

    LuaStateHandle GetState();

    ~LuaWorld();

    Err DoString(const AString& iCode, luabind::object& oRes);

    Err DoString(const AString& iCode, AString& oStr, luabind::object& oRes);

    //Err LoadScript(LuaScript const&);

    Impl& GetImpl();

  protected:
    std::unique_ptr<Impl> m_Impl;
  };

  using LuaRegFun = int(*) (lua_State *L);

  namespace LuaManager
  {
    typedef void(*ConvertTypeFunction)(lua_State*, void*);
    typedef void(*CoopyTypeFunction)(lua_State*, void const*);

    EXL_CORE_API void Reset();

    EXL_CORE_API LuaWorld CreateWorld(RttiObject* iUserPtr);

    EXL_CORE_API void PushRefToLua(lua_State*, Type const* iType, void* iObject, bool iIsConst);
    EXL_CORE_API void PushRefToLua(lua_State*, Type const* iType, void const* iObject);

    EXL_CORE_API void PushCopyToLua(lua_State*, Type const* iType, void const* iObject);

    EXL_CORE_API LuaStateHandle GetHandle(lua_State* iState);

    EXL_CORE_API LuaStateHandle GetCurrentState();

    //Object must be pushed on the stack
    EXL_CORE_API Type const* GetObjectType(LuaStateHandle iHandle);

    EXL_CORE_API void AddRegFun(LuaRegFun iFun);

    EXL_CORE_API Err DoString(const AString& iCode, LuaStateHandle& iState, luabind::object& oRet);

    EXL_CORE_API Err DoString(const AString& iCode, AString& oStr, LuaStateHandle& iState, luabind::object& oRet);

    EXL_CORE_API void GetFunction(lua_State* iState, AString const& iName, unsigned iDepth = 0);

    EXL_CORE_API AString StackDump(lua_State* iState);

    EXL_CORE_API DynObject GetObjectRef(luabind::object const& iObject, Type const* iType);
    EXL_CORE_API DynObject GetPushedObjectRef(lua_State* iState, Type const* iType);

    EXL_CORE_API luabind::object GetLuaRef(lua_State* iState, DynObject const& iObject);
    EXL_CORE_API luabind::object GetLuaRef(lua_State* iState, ConstDynObject const& iObject);

    namespace detail
    {
      void CallFunction(lua_State* iState,unsigned int iArgs,const AString& iFun);
    }
    
  };

  template<typename Arg1, typename... Args>
  struct PushArgsImpl<Arg1, Args...>
  {
    static uint32_t Push(lua_State* iState, Arg1&& iArg1, Args&&... iArgs)
    {
      luabind::object arg(iState, std::forward<Arg1>(iArg1));
      arg.push(iState);
      return PushArgsImpl<Args...>::Push(iState, std::forward<Args>(iArgs)...) + 1;
    }
  };

  template<typename Arg>
  struct PushArgsImpl<Arg>
  {
    static uint32_t Push(lua_State* iState, Arg&& iArg)
    {
      luabind::object arg(iState, std::forward<Arg>(iArg));
      arg.push(iState);

      return 1;
    }
  };

  template<>
  struct PushArgsImpl<void>
  {
    static uint32_t Push(lua_State* iState)
    {
      return 0;
    }
  };

}

#endif