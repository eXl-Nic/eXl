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
  IMPLEMENT_RTTI(LuaScriptSystem);

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
    iStreamer.PushKey("Behaviour");
    iStreamer &= m_BehaviourName;
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

  namespace
  {
    struct BehaviourRegistry
    {
      UnorderedMap<Name, BehaviourDesc> m_Behaviours;
    };
  }

  boost::optional<BehaviourRegistry> s_Registry;

  void Script_StaticInit()
  {
    s_Registry.emplace();
  }

  void Script_StaticDestroy()
  {
    s_Registry.reset();
  }

  LuaScriptSystem::LuaScriptSystem()
    : m_LuaWorld(LuaManager::CreateWorld(this))
  {}

  World* LuaScriptSystem::GetWorld()
  {
    LuaStateHandle curState = LuaManager::GetCurrentState();
    LuaScriptSystem* self = LuaScriptSystem::DynamicCast(curState.GetUserPtr());
    if (self)
    {
      return self->m_World;
    }

    return nullptr;
  }

  void LuaScriptSystem::AddBehaviourDesc(BehaviourDesc iDesc)
  {
    s_Registry->m_Behaviours.emplace(std::make_pair(iDesc.behaviourName, std::move(iDesc)));
  }

  BehaviourDesc const* LuaScriptSystem::GetBehaviourDesc(Name iName)
  {
    auto iter = s_Registry->m_Behaviours.find(iName);
    if (iter != s_Registry->m_Behaviours.end())
    {
      return &iter->second;
    }
    return nullptr;
  }

  Vector<Name> LuaScriptSystem::GetBehaviourNames()
  {
    Vector<Name> names;
    for (auto const& entry : s_Registry->m_Behaviours)
    {
      names.push_back(entry.first);
    }

    return names;
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

    BehaviourDesc const* desc = GetBehaviourDesc(iBehaviour.m_BehaviourName);
    if (desc == nullptr)
    {
      LOG_ERROR << "Behaviour " << iBehaviour.m_BehaviourName << " missing" << "\n";
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

    for (auto const& functionEntry : desc->functions)
    {
      luabind::object function = scriptObject[functionEntry.first.get().c_str()];
      if (function.is_valid())
      {
        function.push(state);
        if (!lua_isfunction(state, -1))
        {
          LOG_ERROR << "Script missing function " << functionEntry.first << "\n";
          return ScriptHandle();
        }
        newEntry.m_ScriptFunctions.insert(std::make_pair(functionEntry.first, function));
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

  void LuaScriptSystem::AddBehaviour(ObjectHandle iHandle, const LuaScriptBehaviour& iBehaviour)
  {
    BehaviourDesc const* desc = GetBehaviourDesc(iBehaviour.m_BehaviourName);
    if (desc == nullptr)
    {
      return;
    }
    ScriptHandle loadedScript = LoadScript_Internal(iBehaviour);
    if (!loadedScript.IsAssigned())
    {
      return;
    }
    BehaviourReg& behaviourReg = m_ObjectToBehaviour.insert(std::make_pair(desc->behaviourName, BehaviourReg())).first->second;
     
    ObjectEntry newEntry;
    newEntry.m_Script = loadedScript;

    ScriptEntry const& scriptDesc = m_Scripts.Get(loadedScript);

    LuaStateHandle luaHandle = m_LuaWorld.GetState();
    {
      lua_State* state = luaHandle.GetState();
      scriptDesc.m_InitFunction.push(state);
      if (lua_isfunction(state, -1))
      {
        lua_pop(state, 1);
        auto callCtx = luaHandle.PrepareCall(scriptDesc.m_InitFunction);
        callCtx.PushArgs(iHandle);
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
            newEntry.m_ScriptData = scriptObj;
          }
        }
      }
    }
    behaviourReg.m_RegisteredObjects.insert(std::make_pair(iHandle, std::move(newEntry)));
  }

  void LuaScriptSystem::DeleteComponent(ObjectHandle iHandle)
  {
    for (auto& behaviourEntry : m_ObjectToBehaviour)
    {
      behaviourEntry.second.m_RegisteredObjects.erase(iHandle);
    }
  }

  luabind::object LuaScriptSystem::FindFunction(ObjectHandle iHandle, Name iBehaviour, Name iFunction, luabind::object& oScriptObj)
  {
    auto iter = m_ObjectToBehaviour.find(iBehaviour);

    if (iter == m_ObjectToBehaviour.end())
    {
      return luabind::object();
    }

    BehaviourReg const& behaviourReg = iter->second;

    auto iterScript = behaviourReg.m_RegisteredObjects.find(iHandle);
    if (iterScript == behaviourReg.m_RegisteredObjects.end())
    {
      return luabind::object();
    }

    ScriptHandle handle = iterScript->second.m_Script;
    ScriptEntry& script = m_Scripts.Get(handle);
    auto iterFunction = script.m_ScriptFunctions.find(iFunction);
    if (iterFunction == script.m_ScriptFunctions.end())
    {
      return luabind::object();
    }
    oScriptObj = iterScript->second.m_ScriptData;
    return iterFunction->second;
  }
}
