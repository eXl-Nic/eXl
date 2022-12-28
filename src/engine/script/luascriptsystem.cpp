/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/script/luascriptsystem.hpp>
#include <engine/script/luaeventhandler.hpp>
#include <engine/script/luafunctionlibrary.hpp>
#include <engine/script/luacoroutine.hpp>

#include <core/resource/resourceloader.hpp>
#include <core/stream/serializer.hpp>
#include <boost/optional.hpp>

namespace eXl
{
#ifdef EXL_LUA

  namespace
  {
    enum ScriptType
    {
      FunctionLib,
      EventHandler,
      Coroutine
    };

    struct ScriptEntry
    {
      //ResourceHandle<LuaEventHandler> m_ScriptHandle;
      luabind::object m_ScriptObject;
      luabind::object m_InitFunction;
      UnorderedMap<Name, luabind::object> m_ScriptFunctions;

      ScriptType m_Type;
    };
  }

  struct LuaCoroutineHandler
  {
    ScriptEntry const* m_Script = nullptr;
    LuaWorld* m_LuaWorld = nullptr;
    luabind::object m_ScriptData;

    LuaCoroutineHandler() = default;

    LuaCoroutineHandler(ScriptEntry const& iScript, LuaWorld& iLuaWorld)
      : m_Script(&iScript)
      , m_LuaWorld(&iLuaWorld)
    {}

    void Start(World& iWorld, ObjectHandle iObj)
    {
      static const Name s_Name("Coroutine::Start");

      auto iter = m_Script->m_ScriptFunctions.find(s_Name);
      eXl_ASSERT_REPAIR_RET(iter != m_Script->m_ScriptFunctions.end(), void());

      LuaStateHandle stateHandle = m_LuaWorld->GetState();
      {
        lua_State* state = stateHandle.GetState();
        auto call = stateHandle.PrepareCall(iter->second);
        call.PushArgs(iObj);
        if (auto res = call.Call(1))
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
            m_ScriptData = scriptObj;
          }
        }
      }
    }

    void Step(CoroutineAPI& iApi, World& iWorld, ObjectHandle iObj, float iTime)
    {
      static const Name s_Name("Coroutine::Step");

      auto iter = m_Script->m_ScriptFunctions.find(s_Name);
      eXl_ASSERT_REPAIR_RET(iter != m_Script->m_ScriptFunctions.end(), void());

      LuaStateHandle stateHandle = m_LuaWorld->GetState();
      {
        auto call = stateHandle.PrepareCall(iter->second);
        call.Push(m_ScriptData);
        call.PushArgs(&iApi, iObj, iTime);
        call.Call(0);
      }
    }
    void Terminate(World& iWorld, ObjectHandle iObj)
    {
      static const Name s_Name("Coroutine::Terminate");

      auto iter = m_Script->m_ScriptFunctions.find(s_Name);
      eXl_ASSERT_REPAIR_RET(iter != m_Script->m_ScriptFunctions.end(), void());

      LuaStateHandle stateHandle = m_LuaWorld->GetState();
      {
        auto call = stateHandle.PrepareCall(iter->second);
        call.Push(m_ScriptData);
        call.PushArgs(iObj);
        call.Call(0);
      }
    }
    void Paused(World& iWorld, ObjectHandle iObj)
    {
      static const Name s_Name("Coroutine::Paused");

      auto iter = m_Script->m_ScriptFunctions.find(s_Name);
      eXl_ASSERT_REPAIR_RET(iter != m_Script->m_ScriptFunctions.end(), void());

      LuaStateHandle stateHandle = m_LuaWorld->GetState();
      {
        auto call = stateHandle.PrepareCall(iter->second);
        call.Push(m_ScriptData);
        call.PushArgs(iObj);
        call.Call(0);
      }
    }
    void Resume(World& iWorld, ObjectHandle iObj)
    {
      static const Name s_Name("Coroutine::Resume");

      auto iter = m_Script->m_ScriptFunctions.find(s_Name);
      eXl_ASSERT_REPAIR_RET(iter != m_Script->m_ScriptFunctions.end(), void());

      LuaStateHandle stateHandle = m_LuaWorld->GetState();
      {
        auto call = stateHandle.PrepareCall(iter->second);
        call.Push(m_ScriptData);
        call.PushArgs(iObj);
        call.Call(0);
      }
    }
  };

  struct LuaScriptSystem::Impl
  {
    Impl(LuaScriptSystem& iSys, World& iWorld)
      : m_Sys(iSys)
      , m_World(iWorld)
      , m_LuaWorld(LuaManager::CreateWorld(&iSys, nullptr))
      , m_ObjectsScripts(iWorld)
    {

    }

    ~Impl()
    {
      m_ObjectsScripts.Clear();
      m_LoadedScripts.clear();
      m_Scripts.Reset();
    }

    void LoadScript(const LuaScript& iScript);

    Err AddHandler(ObjectHandle, const LuaEventHandler& iHandler);
    Err AddCoroutine(ObjectHandle, const LuaCoroutine& iHandler);

    void PauseCoroutine(ObjectHandle);
    void ResumeCoroutine(ObjectHandle);

    Err DeleteComponent(ObjectHandle);

    void Reload();

    void Tick();

    ObjectTable<ScriptEntry> m_Scripts;
    using ScriptHandle = ObjectTableHandle<ScriptEntry>;
    UnorderedMap<Resource::UUID, ScriptHandle> m_LoadedScripts;

    struct ObjectScript
    {
      ScriptHandle m_LoadedScript;
      luabind::object m_Self;
    };

    void CallInitData(ObjectHandle iObject, ObjectScript& oObj, ScriptHandle iScript);

    DenseGameDataStorage<UnorderedMap<Name, ObjectScript>> m_ObjectsScripts;

    struct DependenciesStack
    {
      struct LoadingLib
      {
        LoadingLib(DependenciesStack&, bool iOk);
        ~LoadingLib();
        explicit operator bool() const;
        DependenciesStack& m_Stack;
        bool m_Ok;
      };

      LoadingLib StartLoading(LuaFunctionLibrary const& iLib);

      Vector<Resource::UUID> m_Ids;
    };

    ScriptHandle LoadScript_Handler(const LuaEventHandler& iHandler);
    ScriptHandle LoadScript_Coroutine(const LuaCoroutine& iCoroutine);
    ScriptHandle LoadScript_FunctionLib(DependenciesStack& Deps, const LuaFunctionLibrary& iLib);
    Err LoadDependencies(DependenciesStack& Deps, Resource const& iScript, Vector<ResourceHandle<LuaFunctionLibrary>> const& iDeps);
    Err LoadInterface(lua_State* state, ScriptEntry& iEntry, String const& iItfName, EventsManifest::FunctionsMap const& iFunctions);

    static void CallbackDispatcher(World& iWorld, ObjectHandle iObject, Name iFunction, uint8_t const* const*, DynObject& oOutput, void* iPayload);
    void DispatchCallback(ObjectHandle iObject, Name iFunction, uint8_t const* const*, DynObject& oOutput);

    LuaScriptSystem& m_Sys;
    World& m_World;
    LuaWorld m_LuaWorld;
    lua_State* m_ExtState = nullptr;

    T_CoroutineManager<LuaCoroutineHandler> m_Coroutines;
  };

  IMPLEMENT_RTTI(LuaScriptSystem);

  LuaScriptSystem::LuaScriptSystem()
  {}

  LuaScriptSystem::~LuaScriptSystem()
  {
    m_Impl.reset();
  }

  void LuaScriptSystem::SetExternalState(lua_State* iExternal)
  {
    m_Impl->m_ExtState = iExternal;
    Reload();
  }

  void LuaScriptSystem::Register(World& iWorld)
  {
    ComponentManager::Register(iWorld);
    m_Impl = std::make_unique<Impl>(*this, iWorld);
  }

  void LuaScriptSystem::Reload()
  {
    m_Impl->Reload();
  }

  void LuaScriptSystem::Impl::Reload()
  {
    UnorderedMap<Resource::UUID, ScriptHandle> temp;
    m_LoadedScripts.swap(temp);

    m_ObjectsScripts.Iterate([](ObjectHandle, UnorderedMap<Name, ObjectScript>& iMap)
      {
        for (auto& entry : iMap)
        {
          entry.second.m_Self = luabind::object();
        }
      });
    m_Scripts.Reset();

    m_LuaWorld.~LuaWorld();
    new(&m_LuaWorld) LuaWorld(LuaManager::CreateWorld(&m_Sys, m_ExtState));

    
    for (auto const& loadedScript : temp)
    {
      Resource const* rsc = ResourceManager::Load(loadedScript.first, nullptr);
      if (LuaScript const* script = LuaScript::DynamicCast(rsc))
      {
        LoadScript(*script);
      }
    }
    
    m_ObjectsScripts.Iterate([this](ObjectHandle iObj, UnorderedMap<Name, ObjectScript>& iMap)
      {
        UnorderedSet<KString> itfNames;
        for (auto& entry : iMap)
        {
          size_t separator = entry.first.get().find("::");
          KString itfName = entry.first.get().substr(0, separator);
          if (itfNames.insert(itfName).second)
          {
            CallInitData(iObj, entry.second, entry.second.m_LoadedScript);
          }
        }
      });
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

  LuaWorld& LuaScriptSystem::GetLuaWorld()
  {
    return m_Impl->m_LuaWorld;
  }

  void LuaScriptSystem::LoadScript(const LuaScript& iScript)
  {
    m_Impl->LoadScript(iScript);
  }

  void LuaScriptSystem::Impl::LoadScript(const LuaScript& iScript)
  {
    auto iter = m_LoadedScripts.find(iScript.GetHeader().m_ResourceId);
    if (iter != m_LoadedScripts.end())
    {
      return;
    }
    if (auto handler = LuaEventHandler::DynamicCast(&iScript))
    {
      LoadScript_Handler(*handler);
      return;
    }
    else if (auto coroutine = LuaCoroutine::DynamicCast(&iScript))
    {
      LoadScript_Coroutine(*coroutine);
      return;
    }
    else if (auto library = LuaFunctionLibrary::DynamicCast(&iScript))
    {
      DependenciesStack deps;
      LoadScript_FunctionLib(deps, *library);
      return;
    }
    eXl_FAIL_MSG(eXl_FORMAT("Script %s is not of a supported type", iScript.GetName()));
  }

  static luabind::object LoadScriptAsTable(LuaWorld& iWorld, const LuaScript& iScript)
  {
    luabind::object scriptObject;
    String executionRes;
    if (!iWorld.DoString(iScript.m_Script, executionRes, scriptObject))
    {
      LOG_ERROR << "Script loading failed with error " << executionRes << "\n";
      return luabind::object();
    }

    if (!scriptObject.is_valid())
    {
      LOG_ERROR << "Script did not return a valid script object " << "\n";
      return luabind::object();
    }

    LuaStateHandle stateHandle = iWorld.GetState();
    lua_State* state = stateHandle.GetState();
    scriptObject.push(state);

    bool const isTable = lua_istable(state, -1);
    lua_pop(state, 1);
    if (!isTable)
    {
      LOG_ERROR << "Script did not return a valid script object " << "\n";
      return luabind::object();
    }

    return scriptObject;
  }

  LuaScriptSystem::Impl::DependenciesStack::LoadingLib::LoadingLib(DependenciesStack& iStack, bool iOk)
    : m_Stack(iStack)
    , m_Ok(iOk)
  {

  }
  LuaScriptSystem::Impl::DependenciesStack::LoadingLib::~LoadingLib()
  {
    if (m_Ok)
    {
      m_Stack.m_Ids.pop_back();
    }
  }

  LuaScriptSystem::Impl::DependenciesStack::LoadingLib::operator bool() const
  {
    return m_Ok;
  }

  LuaScriptSystem::Impl::DependenciesStack::LoadingLib LuaScriptSystem::Impl::DependenciesStack::StartLoading(LuaFunctionLibrary const& iLib)
  {
    auto iter = std::find(m_Ids.begin(), m_Ids.end(), iLib.GetHeader().m_ResourceId);
    bool isOk = iter == m_Ids.end();
    if (isOk)
    {
      m_Ids.push_back(iLib.GetHeader().m_ResourceId);
    }
    return LoadingLib(*this, isOk);
  }

  Err LuaScriptSystem::Impl::LoadDependencies(DependenciesStack& Deps, Resource const& iScript, Vector<ResourceHandle<LuaFunctionLibrary>> const& iDeps)
  {
    for (auto depHandle : iDeps)
    {
      LuaFunctionLibrary const* dep = depHandle.GetOrLoad();
      if (dep == nullptr)
      {
        LOG_ERROR << "Script " << iScript.GetName() << " has an unresolved resource handle as dependency : " << depHandle.GetUUID().ToString() << "\n";
        return Err::Failure;
      }
      auto handle = LoadScript_FunctionLib(Deps, *dep);
      if (!handle.IsAssigned())
      {
        LOG_ERROR << "Script " << iScript.GetName() << " could not load dependency : " << dep->GetName() << "\n";
        return Err::Failure;
      }
    }

    return Err::Success;
  }

  LuaScriptSystem::Impl::ScriptHandle LuaScriptSystem::Impl::LoadScript_FunctionLib(DependenciesStack& Deps, const LuaFunctionLibrary& iLibrary)
  {
    Resource::UUID const& rscId = iLibrary.GetHeader().m_ResourceId;
    auto loadedScipt = m_LoadedScripts.find(rscId);
    if (loadedScipt != m_LoadedScripts.end())
    {
      return loadedScipt->second;
    }

    auto loadingLib = Deps.StartLoading(iLibrary);
    if (!loadingLib)
    {
      LOG_ERROR << "Library " << iLibrary.GetName() << " has a cyclic dependency" << "\n";
      return ScriptHandle();
    }

    if (!LoadDependencies(Deps, iLibrary, iLibrary.m_Dependencies))
    {
      return ScriptHandle();
    }

    luabind::object scriptObject = LoadScriptAsTable(m_LuaWorld, iLibrary);
    if (!scriptObject)
    {
      return ScriptHandle();
    }

    luabind::object libNamespaceRef = scriptObject["namespace"];
    luabind::object functionsTable = scriptObject["functions"];
    
    if (!libNamespaceRef.is_valid())
    {
      LOG_ERROR << "Library " << iLibrary.GetName() << " missing a namespace in the returned table" << "\n";
      return ScriptHandle();
    }

    if (!functionsTable.is_valid())
    {
      LOG_ERROR << "Library " << iLibrary.GetName() << " missing a function table" << "\n";
      return ScriptHandle();
    }

    String libNamespace = luabind::to_string(libNamespaceRef).c_str();
    
    luabind::object libScope = luabind::globals(scriptObject.interpreter());
    while (!libNamespace.empty())
    {
      String remainder;
      auto dotPos = libNamespace.find(".");
      if (dotPos != String::npos)
      {
        remainder = libNamespace.substr(dotPos + 1);
        libNamespace = libNamespace.substr(0, dotPos);
      }
      luabind::object nextScope = libScope[libNamespace.c_str()];
      if (!nextScope)
      {
        nextScope = luabind::newtable(libScope.interpreter());
        libScope[libNamespace.c_str()] = nextScope;
      }
      libScope = nextScope;
      libNamespace = std::move(remainder);
    }

    for (auto iter = luabind::iterator(functionsTable); iter != luabind::iterator(); ++iter)
    {
      libScope[iter.key()] = iter.operator*();
    }

    ScriptEntry newEntry;
    newEntry.m_ScriptObject = scriptObject;
    newEntry.m_Type = FunctionLib;

    ScriptHandle entryHandle = m_Scripts.Alloc();
    m_Scripts.Get(entryHandle) = std::move(newEntry);
    m_LoadedScripts.insert(std::make_pair(rscId, entryHandle));

    return entryHandle;
  }

  Err LuaScriptSystem::Impl::LoadInterface(lua_State* state, ScriptEntry& iEntry, String const& iItfName, EventsManifest::FunctionsMap const& iFunctions)
  {
    for (auto const& functionEntry : iFunctions)
    {
      luabind::object function = iEntry.m_ScriptObject[functionEntry.first.c_str()];
      if (function.is_valid())
      {
        luabind::detail::stack_pop(state, 1);
        function.push(state);
        if (!lua_isfunction(state, -1))
        {
          LOG_ERROR << "Script missing function " << functionEntry.first << "\n";
          return Err::Failure;
        }

        iEntry.m_ScriptFunctions.insert(std::make_pair(Name(iItfName + "::" + functionEntry.first), function));
      }
      else
      {
        LOG_ERROR << "Script missing function " << functionEntry.first << "\n";
        return Err::Failure;
      }
    }

    return Err::Success;
  }

  LuaScriptSystem::Impl::ScriptHandle LuaScriptSystem::Impl::LoadScript_Coroutine(const LuaCoroutine& iCoroutine)
  {
    Resource::UUID const& rscId = iCoroutine.GetHeader().m_ResourceId;
    auto loadedScipt = m_LoadedScripts.find(rscId);
    if (loadedScipt != m_LoadedScripts.end())
    {
      return loadedScipt->second;
    }

    DependenciesStack deps;
    if (!LoadDependencies(deps, iCoroutine, iCoroutine.m_Dependencies))
    {
      return ScriptHandle();
    }

    luabind::object scriptObject = LoadScriptAsTable(m_LuaWorld, iCoroutine);
    if (!scriptObject)
    {
      return ScriptHandle();
    }

    LuaStateHandle stateHandle = m_LuaWorld.GetState();
    lua_State* state = stateHandle.GetState();

    ScriptEntry newEntry;
    newEntry.m_ScriptObject = scriptObject;
    newEntry.m_Type = Coroutine;

    static const EventsManifest::FunctionsMap s_CoroutineDesc = []
    {
      EventsManifest::FunctionsMap functions
        =
      {
        {"Start", FunDesc::Create<void(ObjectHandle)>()},
        {"Terminate", FunDesc::Create<void(ObjectHandle)>()},
        {"Step", FunDesc::Create<void(CoroutineAPI, ObjectHandle, float)>()},
        {"Paused", FunDesc::Create<void(ObjectHandle)>()},
        {"Resume", FunDesc::Create<void(ObjectHandle)>()},
      };

      return functions;
    }();

    if (!LoadInterface(state, newEntry, "Coroutine", s_CoroutineDesc))
    {
      return ScriptHandle();
    }

    //newEntry.m_ScriptHandle.Set(&iCoroutine);

    ScriptHandle entryHandle = m_Scripts.Alloc();
    m_Scripts.Get(entryHandle) = std::move(newEntry);
    m_LoadedScripts.insert(std::make_pair(rscId, entryHandle));

    return entryHandle;
  }

  LuaScriptSystem::Impl::ScriptHandle LuaScriptSystem::Impl::LoadScript_Handler(const LuaEventHandler& iHandler)
  {
    Resource::UUID const& rscId = iHandler.GetHeader().m_ResourceId;
    auto loadedScipt = m_LoadedScripts.find(rscId);
    if (loadedScipt != m_LoadedScripts.end())
    {
      return loadedScipt->second;
    }

    DependenciesStack deps;
    if (!LoadDependencies(deps, iHandler, iHandler.m_Dependencies))
    {
      return ScriptHandle();
    }

    EventSystem& events = *m_World.GetSystem<EventSystem>();

    auto itfIter = events.GetManifest().m_Interfaces.find(iHandler.m_InterfaceName);
    if (itfIter == events.GetManifest().m_Interfaces.end())
    {
      LOG_ERROR << "Behaviour " << iHandler.m_InterfaceName << " missing" << "\n";
      return ScriptHandle();
    }

    luabind::object scriptObject = LoadScriptAsTable(m_LuaWorld, iHandler);
    if (!scriptObject)
    {
      return ScriptHandle();
    }

    LuaStateHandle stateHandle = m_LuaWorld.GetState();
    lua_State* state = stateHandle.GetState();

    ScriptEntry newEntry;
    newEntry.m_ScriptObject = scriptObject;
    newEntry.m_Type = EventHandler;

    newEntry.m_InitFunction = scriptObject["Init"];

    if (!LoadInterface(state, newEntry, itfIter->first, itfIter->second))
    {
      return ScriptHandle();
    }

    //newEntry.m_ScriptHandle.Set(&iHandler);
    
    ScriptHandle entryHandle = m_Scripts.Alloc();
    m_Scripts.Get(entryHandle) = std::move(newEntry);
    m_LoadedScripts.insert(std::make_pair(rscId, entryHandle));

    return entryHandle;
  }

  void LuaScriptSystem::Impl::CallbackDispatcher(World& iWorld, ObjectHandle iObject, Name iFunction, uint8_t const* const* iArgs, DynObject& oOutput, void* iPayload)
  {
    iWorld.GetSystem<LuaScriptSystem>()->m_Impl->DispatchCallback(iObject, iFunction, iArgs, oOutput);
  }

  void LuaScriptSystem::Impl::DispatchCallback(ObjectHandle iObject, Name iFunction, uint8_t const* const* iArgs, DynObject& oOutput)
  {
    EventSystem& events = *m_World.GetSystem<EventSystem>();

    FunDesc const* desc = events.GetFunDesc(iFunction);
    eXl_ASSERT_REPAIR_RET(desc != nullptr, void());

    ObjectScript* objScript = nullptr;
    if (auto scriptMap = m_ObjectsScripts.Get(iObject))
    {
      auto iter = scriptMap->find(iFunction);
      if (iter != scriptMap->end())
      {
        objScript = &iter->second;
      }
    }

    eXl_ASSERT_REPAIR_RET(m_Scripts.IsValid(objScript->m_LoadedScript), void());

    ScriptEntry const& script = m_Scripts.Get(objScript->m_LoadedScript);
    auto funIter = script.m_ScriptFunctions.find(iFunction);
    eXl_ASSERT_REPAIR_RET(funIter != script.m_ScriptFunctions.end(), void());

    luabind::object function = funIter->second;

    LuaStateHandle stateHandle = m_LuaWorld.GetState();
    lua_State* state = stateHandle.GetState();
    int32_t curTop = lua_gettop(state);

    if (!function.is_valid())
    {
      return;
    }

    uint32_t numRet = desc->GetRetType() == nullptr ? 0 : 1;
    {
      auto call = stateHandle.PrepareCall(function);
      call.Push(objScript->m_Self);
      call.PushArgs(iObject);
      
      for (uint32_t i = 0; i < desc->GetArgs().size(); ++i)
      {
        Type const* fieldType = desc->GetArgs()[i];
        LuaManager::PushArgToLua(state, fieldType, iArgs[i]);
        call.ArgPushed();
      }
      
      auto res = call.Call(numRet);

      if (!res || *res != numRet)
      {
        return;
      }

      if (numRet == 1)
      {
        if(!oOutput.IsValid())
        {
          oOutput.SetType(desc->GetRetType(), desc->GetRetType()->Alloc(), true);
        }
        uint32_t index = lua_gettop(state);
        //Err conversion = desc->GetRetType()->ConvertFromLua_Uninit(state, index, oOutput.GetBuffer());
        uint8_t const* arg;
        Err conversion = LuaManager::ArgFromLua(state, index, desc->GetRetType(), oOutput, arg);
        eXl_ASSERT_REPAIR_RET(conversion, void());
      }
    }

  };

  void LuaScriptSystem::AddHandler(ObjectHandle iObject, const LuaEventHandler& iHandler)
  {
    if (m_Impl->AddHandler(iObject, iHandler))
    {
      ComponentManager::CreateComponent(iObject);
    }
  }

  void LuaScriptSystem::Impl::CallInitData(ObjectHandle iObject, ObjectScript& oObj, ScriptHandle iScript)
  {
    ScriptEntry const& scriptDesc = m_Scripts.Get(iScript);

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
    oObj.m_LoadedScript = iScript;
    oObj.m_Self = scriptData;
  }

  Err LuaScriptSystem::Impl::AddHandler(ObjectHandle iObject, const LuaEventHandler& iHandler)
  {
    EventSystem& events = *m_World.GetSystem<EventSystem>();

    auto itfIter = events.GetManifest().m_Interfaces.find(iHandler.m_InterfaceName);
    if (itfIter == events.GetManifest().m_Interfaces.end())
    {
      return Err::Failure;
    }

    ScriptHandle loadedScript = LoadScript_Handler(iHandler);
    if (!loadedScript.IsAssigned())
    {
      return Err::Failure;
    }

    ObjectScript objScr;
    CallInitData(iObject, objScr, loadedScript);

    ScriptEntry const& scriptDesc = m_Scripts.Get(loadedScript);
    UnorderedMap<Name, ObjectScript>& funMap = m_ObjectsScripts.GetOrCreate(iObject);

    for (auto const& fun : scriptDesc.m_ScriptFunctions)
    {
      funMap.insert(std::make_pair(fun.first, objScr));
      events.AddEventHandlerInternal(iObject, fun.first, &LuaScriptSystem::Impl::CallbackDispatcher, nullptr);
    }
    return Err::Success;
  }

  void LuaScriptSystem::DeleteComponent(ObjectHandle iHandle)
  {
    if (m_Impl->DeleteComponent(iHandle))
    {
      ComponentManager::DeleteComponent(iHandle);
    }
  }

  Err LuaScriptSystem::Impl::DeleteComponent(ObjectHandle iHandle)
  {
    Err res = Err::Failure;
    if (m_ObjectsScripts.Get(iHandle))
    {
      m_ObjectsScripts.Erase(iHandle);
      res = Err::Success;
    }

    if (m_Coroutines.RemoveCoroutine(m_World, iHandle))
    {
      res = Err::Success;
    }

    return res;
  }

  void LuaScriptSystem::AddCoroutine(ObjectHandle iHandle, const LuaCoroutine& iCoroutine)
  {
    if (m_Impl->AddCoroutine(iHandle, iCoroutine))
    {
      ComponentManager::CreateComponent(iHandle);
    }
  }

  Err LuaScriptSystem::Impl::AddCoroutine(ObjectHandle iHandle, const LuaCoroutine& iCoroutine)
  {
    ScriptHandle script = LoadScript_Coroutine(iCoroutine);

    if (!script.IsAssigned())
    {
      return Err::Failure;
    }

    ScriptEntry const& entry = m_Scripts.Get(script);

    return m_Coroutines.AddCoroutine(m_World, iHandle, LuaCoroutineHandler(entry, m_LuaWorld), iCoroutine.m_DefaultTickRate, iCoroutine.m_DefaultStartPaused);
  }

  void LuaScriptSystem::Impl::PauseCoroutine(ObjectHandle iHandle)
  {
    m_Coroutines.Pause(m_World, iHandle);
  }

  void LuaScriptSystem::Impl::ResumeCoroutine(ObjectHandle iHandle)
  {
    m_Coroutines.Resume(m_World, iHandle);
  }

  void LuaScriptSystem::PauseCoroutine(ObjectHandle iHandle)
  {
    m_Impl->PauseCoroutine(iHandle);
  }

  void LuaScriptSystem::ResumeCoroutine(ObjectHandle iHandle)
  {
    m_Impl->ResumeCoroutine(iHandle);
  }

  void LuaScriptSystem::Impl::Tick()
  {
    m_Coroutines.Tick(m_World);
  }

  void LuaScriptSystem::Tick()
  {
    m_Impl->Tick();
  }
#endif
}
