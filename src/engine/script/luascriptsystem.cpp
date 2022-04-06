/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/script/luascriptsystem.hpp>

#include <engine/script/luascriptbehaviour.hpp>

#include <core/resource/resourceloader.hpp>
#include <core/stream/serializer.hpp>
#include <boost/optional.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(LuaScriptBehaviour);

  using LuaScriptBehaviourLoader = LuaScriptLoader_T<LuaScriptBehaviour>;

  void LuaScriptBehaviour::Init()
  {
    ResourceManager::AddLoader(&TResourceLoader<LuaScriptBehaviour, LuaScriptLoader>::Get(), LuaScriptBehaviour::StaticRtti());
  }

  ResourceLoaderName LuaScriptBehaviour::StaticLoaderName()
  {
    return ResourceLoaderName("LuaScriptBehaviour");
  }

#ifdef EXL_RSC_HAS_FILESYSTEM
  LuaScriptBehaviour* LuaScriptBehaviour::Create(Path const& iPath, String const& iName)
  {
    return LuaScriptBehaviourLoader::Get().Create(iPath, iName);
  }
#endif

  Err LuaScriptBehaviour::Serialize(Serializer iStreamer)
  {
    iStreamer.BeginStruct();
    iStreamer.PushKey("Interface");
    iStreamer &= m_InterfaceName;
    iStreamer.PopKey();
    iStreamer.EndStruct();

    return Err::Success;
  }

  Err LuaScriptBehaviour::Stream_Data(Streamer& iStreamer) const
  {
    return const_cast<LuaScriptBehaviour*>(this)->Serialize(iStreamer);
  }

  Err LuaScriptBehaviour::Unstream_Data(Unstreamer& iStreamer)
  {
    return Serialize(iStreamer);
  }

  LuaScriptBehaviour::LuaScriptBehaviour(ResourceMetaData& iMeta)
    : LuaScript(iMeta)
  {

  }
#ifdef EXL_LUA

  IMPLEMENT_RTTI(LuaScriptSystem);

  LuaScriptSystem::LuaScriptSystem()
    : m_LuaWorld(LuaManager::CreateWorld(this))
  {}

  LuaScriptSystem::~LuaScriptSystem()
  {
    m_LoadedScripts.clear();
    m_Scripts.Reset();
  }

  World* LuaScriptSystem::GetWorld_Static()
  {
    LuaStateHandle curState = LuaManager::GetCurrentState();
    LuaScriptSystem* self = LuaScriptSystem::DynamicCast(curState.GetUserPtr());
    if (self)
    {
      return self->m_World;
    }

    return nullptr;
  }

  void LuaScriptSystem::LoadScript(const LuaScriptBehaviour& iBehaviour)
  {
    LoadScript_Internal(iBehaviour);
  }

  LuaScriptSystem::ScriptHandle LuaScriptSystem::LoadScript_Internal(const LuaScriptBehaviour& iBehaviour)
  {
    Resource::UUID const& rscId = iBehaviour.GetHeader().m_ResourceId;
    auto loadedScipt = m_LoadedScripts.find(rscId);
    if (loadedScipt != m_LoadedScripts.end())
    {
      return loadedScipt->second;
    }
    EventSystem& events = *m_World->GetSystem<EventSystem>();

    auto itfIter = events.GetManifest().m_Interfaces.find(iBehaviour.m_InterfaceName);
    if (itfIter == events.GetManifest().m_Interfaces.end())
    {
      LOG_ERROR << "Behaviour " << iBehaviour.m_InterfaceName << " missing" << "\n";
      return ScriptHandle();
    }

    luabind::object scriptObject;
    String executionRes;
    if (!m_LuaWorld.DoString(iBehaviour.m_Script, executionRes, scriptObject))
    {
      LOG_ERROR << "Script loading failed with error " << executionRes << "\n";
      return ScriptHandle();
    }

    if (!scriptObject.is_valid())
    {
      LOG_ERROR << "Script did not return a valid script object " << "\n";
      return ScriptHandle();
    }
    LuaStateHandle stateHandle = m_LuaWorld.GetState();
    lua_State* state = stateHandle.GetState();
    scriptObject.push(state);

    bool const isTable = lua_istable(state, -1);
    lua_pop(state, 1);
    if (!isTable)
    {
      LOG_ERROR << "Script did not return a valid script object " << "\n";
      return ScriptHandle();
    }

    ScriptEntry newEntry;
    newEntry.m_ScriptObject = scriptObject;

    newEntry.m_InitFunction = scriptObject["Init"];

    for (auto const& functionEntry : itfIter->second)
    {
      luabind::object function = scriptObject[functionEntry.first.c_str()];
      if (function.is_valid())
      {
        function.push(state);
        if (!lua_isfunction(state, -1))
        {
          LOG_ERROR << "Script missing function " << functionEntry.first << "\n";
          return ScriptHandle();
        }

        newEntry.m_ScriptFunctions.insert(std::make_pair(Name(itfIter->first + "::" + functionEntry.first), function));
      }
      else
      {
        LOG_ERROR << "Script missing function " << functionEntry.first << "\n";
        return ScriptHandle();
      }
    }
    newEntry.m_ScriptHandle.Set(&iBehaviour);
    
    ScriptHandle entryHandle = m_Scripts.Alloc();
    m_Scripts.Get(entryHandle) = std::move(newEntry);
    m_LoadedScripts.insert(std::make_pair(rscId, entryHandle));

    return entryHandle;
  }

  void LuaScriptSystem::AddBehaviour(ObjectHandle iObject, const LuaScriptBehaviour& iBehaviour)
  {
    EventSystem& events = *m_World->GetSystem<EventSystem>();

    auto itfIter = events.GetManifest().m_Interfaces.find(iBehaviour.m_InterfaceName);
    if (itfIter == events.GetManifest().m_Interfaces.end())
    {
      return;
    }

    ScriptHandle loadedScript = LoadScript_Internal(iBehaviour);
    if (!loadedScript.IsAssigned())
    {
      return;
    }

    ScriptEntry const& scriptDesc = m_Scripts.Get(loadedScript);
    luabind::object scriptData;
    {
      LuaStateHandle luaHandle = m_LuaWorld.GetState();
      lua_State* state = luaHandle.GetState();
      scriptDesc.m_InitFunction.push(state);
      if (lua_isfunction(state, -1))
      {
        lua_pop(state, 1);
        auto callCtx = luaHandle.PrepareCall(scriptDesc.m_InitFunction);
        callCtx.PushArgs(iObject);
        if (auto res = callCtx.Call(1))
        {
          if (*res == 1)
          {
            luabind::object scriptObj(luabind::from_stack(state, -1));
            scriptObj.push(state);
            if (!lua_isnil(state, -1))
            {
              if (!lua_istable(state, -1))
              {
                LOG_ERROR << "Not a table" << "\n";
              }
            }
            scriptData = scriptObj;
          }
        }
      }
    }

    EventSystem::GenericHandler callback = [this, loadedScript, scriptData](ObjectHandle iObject, Name iFunction, ConstDynObject const& iArgsBuffer, DynObject& oOutput)
    {
      EventSystem& events = *m_World->GetSystem<EventSystem>();
      FunDesc const* desc = events.GetFunDesc(iFunction);
      eXl_ASSERT_REPAIR_RET(desc != nullptr, void());

      ScriptEntry const& script = m_Scripts.Get(loadedScript);
      auto funIter = script.m_ScriptFunctions.find(iFunction);
      eXl_ASSERT_REPAIR_RET(funIter != script.m_ScriptFunctions.end(), void());

      luabind::object function = funIter->second;

      LuaStateHandle stateHandle = m_LuaWorld.GetState();
      lua_State* state = stateHandle.GetState();
      int32_t curTop = lua_gettop(state);

      if (!function.is_valid())
      {
        return ;
      }

      uint32_t numRet = desc->returnType == nullptr ? 0 : 1;
      {
        auto call = stateHandle.PrepareCall(function);
        call.Push(scriptData);
        TupleType const* args = TupleType::DynamicCast(iArgsBuffer.GetType());
        for (uint32_t i = 0; i < args->GetNumField(); ++i)
        {
          Type const* fieldType;
          void const* fieldPtr = args->GetField(iArgsBuffer.GetBuffer(), i, fieldType);
          LuaManager::PushRefToLua(state, fieldType, fieldPtr);
          call.ArgPushed();
        }
        auto res = call.Call(numRet);

        if (!res || *res != numRet)
        {
          return;
        }

        if (numRet == 1)
        {
          oOutput.SetType(desc->returnType, desc->returnType->Alloc(), true);
          uint32_t index = lua_gettop(state);
          Err conversion = desc->returnType->ConvertFromLua_Uninit(state, index, oOutput.GetBuffer());
          eXl_ASSERT_REPAIR_RET(conversion, void());
        }
      }

    };

    for (auto const& fun : scriptDesc.m_ScriptFunctions)
    {
      events.AddEventHandlerInternal(iObject, fun.first, callback);
    }

  }

  void LuaScriptSystem::DeleteComponent(ObjectHandle iHandle)
  {
   
  }
#endif
}