/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#ifdef EXL_LUA

#include <core/lua/luamanager.hpp>
#include <core/lua/luascript.hpp>
#include <core/type/fundesc.hpp>
#include <engine/common/world.hpp>
#include <boost/optional.hpp>
#include <core/path.hpp>

namespace eXl
{
  class LuaScriptBehaviour;

  struct BehaviourDesc
  {
    Name behaviourName;
    UnorderedMap<Name, FunDesc> functions;
  };

  class EXL_ENGINE_API LuaScriptSystem : public ComponentManager
  {
    DECLARE_RTTI(LuaScriptSystem, ComponentManager);
  public:

    LuaScriptSystem();
    ~LuaScriptSystem();

    static void AddBehaviourDesc(BehaviourDesc iDesc);
    static BehaviourDesc const* GetBehaviourDesc(Name iName);

    static Vector<Name> GetBehaviourNames();

    void LoadScript(const LuaScriptBehaviour& iBehaviour);

    void AddBehaviour(ObjectHandle, const LuaScriptBehaviour& iBehaviour);
    void DeleteComponent(ObjectHandle) override;

    static World* GetWorld();

    template <typename Ret, typename... Args>
    static bool ValidateCall(Name iBehaviour, Name iFunction)
    {
      BehaviourDesc const* desc = GetBehaviourDesc(iBehaviour);
      if (desc == nullptr)
      {
        return false;
      }
      auto iter = desc->functions.find(iFunction);
      if (iter == desc->functions.end())
      {
        return false;
      }
      if (!iter->second.ValidateSignature<Ret, Args...>())
      {
        return false;
      }

      return true;
    }

    template <typename Ret, typename... Args>
    struct Caller
    {
      static boost::optional<Ret> Call(LuaScriptSystem& iSys, ObjectHandle iHandle, Name iBehaviour, Name iFunction, Args&&... iArgs)
      {
        if (!iSys.ValidateCall<Ret, Args...>(iBehaviour, iFunction))
        {
          return {};
        }

        LuaStateHandle stateHandle = iSys.m_LuaWorld.GetState();
        lua_State* state = stateHandle.GetState();
        int32_t curTop = lua_gettop(state);


        luabind::object scriptObject;
        luabind::object function = iSys.FindFunction(iHandle, iBehaviour, iFunction, scriptObject);
        if (!function.is_valid())
        {
          return {};
        }

        boost::optional<Ret> res;
        {
          auto call = stateHandle.PrepareCall(function);
          call.Push(scriptObject);
          call.PushArgs(std::forward<Args>(iArgs)...);
          auto res = call.Call(1);

          if (!res || *res == 0)
          {
            return {};
          }

          luabind::object retObj(luabind::from_stack(state, -1));
          luabind::default_converter<Ret> converter;
          if (converter.match(state, luabind::decorate_type_t<Ret>(), -1) < 0)
          {
            luabind::detail::cast_error<Ret>(state);
            return {};
          }
          //return converter.to_cpp(state, luabind::decorate_type_t<Ret>(), -1);
          res = converter.to_cpp(state, luabind::decorate_type_t<Ret>(), -1);
        }
        eXl_ASSERT(curTop == lua_gettop(state));
        return res;
      }
    };

    template <typename... Args>
    struct Caller<void, Args...>
    {
      static Err Call(LuaScriptSystem& iSys, ObjectHandle iHandle, Name iBehaviour, Name iFunction, Args&&... iArgs)
      {
        if (!iSys.ValidateCall<void, Args...>(iBehaviour, iFunction))
        {
          return Err::Failure;
        }

        LuaStateHandle stateHandle = iSys.m_LuaWorld.GetState();
        lua_State* state = stateHandle.GetState();
        int32_t curTop = lua_gettop(state);

        luabind::object scriptObject;
        luabind::object function = iSys.FindFunction(iHandle, iBehaviour, iFunction, scriptObject);
        if (!function.is_valid())
        {
          return Err::Failure;
        }
        {
          auto call = stateHandle.PrepareCall(function);
          call.Push(scriptObject);
          call.PushArgs(std::forward<Args>(iArgs)...);
          auto res = call.Call(0);

          if (!res)
          {
            return Err::Failure;
          }
        }
        eXl_ASSERT(curTop == lua_gettop(state));

        return Err::Success;
      }
    };

    template <typename Ret, typename... Args, typename std::enable_if<!std::is_same<Ret, void>::value, bool>::type = true>
    boost::optional<Ret> CallBehaviour(ObjectHandle iHandle, Name iBehaviour, Name iFunction, Args&&... iArgs)
    {
      return Caller<Ret, Args...>::Call(*this, iHandle, iBehaviour, iFunction, std::forward<Args>(iArgs)...);
    }

    template <typename Ret, typename... Args, typename std::enable_if<std::is_same<Ret, void>::value, bool>::type = true>
    Err CallBehaviour(ObjectHandle iHandle, Name iBehaviour, Name iFunction, Args&&... iArgs)
    {
      return Caller<void, Args...>::Call(*this, iHandle, iBehaviour, iFunction, std::forward<Args>(iArgs)...);
    }

  protected:

    luabind::object FindFunction(ObjectHandle iHandle, Name iBehaviour, Name iFunction, luabind::object& oScriptObj);
    
    struct ScriptEntry
    {
      ResourceHandle<LuaScriptBehaviour> m_ScriptHandle;
      luabind::object m_ScriptObject;
      luabind::object m_InitFunction;
      UnorderedMap<Name, luabind::object> m_ScriptFunctions;
    };

    ObjectTable<ScriptEntry> m_Scripts;
    using ScriptHandle = ObjectTableHandle<ScriptEntry>;
    UnorderedMap<Resource::UUID, ScriptHandle> m_LoadedScripts;

    ScriptHandle LoadScript_Internal(const LuaScriptBehaviour& iBehaviour);

    struct ObjectEntry
    {
      ScriptHandle m_Script;
      luabind::object m_ScriptData;
    };

    struct BehaviourReg
    {
      UnorderedMap<ObjectHandle, ObjectEntry> m_RegisteredObjects;
    };

    UnorderedMap<Name, BehaviourReg> m_ObjectToBehaviour;

    LuaWorld m_LuaWorld;
  };
}
#endif