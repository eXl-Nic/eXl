/*
Copyright 2009-2019 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef EXL_LUA

#include <core/lua/luamanager.hpp>
#include <core/lua/luascript.hpp>

#ifdef EXL_THREADAWARE
#include <boost/thread.hpp>
#include <boost/thread/tss.hpp>
#endif

#include <core/heapobject.hpp>

#include <core/type/typemanager.hpp>
#include <core/type/type.hpp>

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
}

#include <luabind/luabind.hpp>
#include <luabind/class_info.hpp>
#include <core/lua/luabind_eXl.hpp>

#include <sstream>
#include <boost/optional.hpp>

namespace eXl
{
  void *LuaBinderImplGetTypeID(lua_State*);

  static void stackDump (lua_State *L, AString& oStr) 
  {
    oStr.clear();
    int top = lua_gettop(L);
    for (int i = 1; i <= top; i++) 
    {  /* repeat for each level */
      int t = lua_type(L, i);
      switch (t) 
      {
      case LUA_TSTRING:  /* strings */
        oStr.append(lua_tostring(L,i));
        break;
    
      case LUA_TBOOLEAN:  /* booleans */
        if (lua_toboolean(L, i))
        {
          oStr.append("true");
        }
        else
        {
          oStr.append("false");
        }
        break;
    
      case LUA_TNUMBER:  /* numbers */
        {
          char number[80];
          number[0]='\0';
          sprintf(number,"%g", lua_tonumber(L, i));
          oStr.append(number);
        }
        break;
    
      default:  /* other values */
        luabind::detail::object_rep* obj = luabind::detail::get_instance(L, i);
        luabind::detail::class_rep* cls = obj != nullptr ? obj->crep() : nullptr;
        Type const* type = cls != nullptr ? cls->type().get_id() : nullptr;
        if (type != nullptr)
        {
          oStr.append(type->GetName());
        }
        else
        {
          oStr.append(lua_typename(L, t));
        }
        break;
      }
      if (i <= top - 1)
      {
        oStr.append(";");  /* put a separator */
      }
    }
  }

  AString LuaManager::StackDump(lua_State* iState)
  {
    AString str;
    stackDump(iState, str);

    return str;
  }

  class LuaImplState : public HeapObject
  {
  public:
    typedef UnorderedMap<void*, Type const*> TypeMap;
    //typedef UnorderedMap<Type const*, LuaManager::PushFuncs> ConversionMap;

    LuaImplState(RttiObject* iUserPtr)
      : m_State(nullptr)
      , m_InLua(0)
      , m_UserPtr(iUserPtr)
    {}

    ~LuaImplState()
    {
      if (m_State != nullptr)
      {
        lua_close(m_State);
      }
    }

    void CloseState()
    {
      if (m_State != nullptr)
      {
        lua_close(m_State);
        m_State = nullptr;
      }
    }

    void BeginUseState();
    void EndUseState();
    lua_State* GetState();
    bool IsInLua() { return m_InLua > 0; }
    TypeMap& GetTypeMap() { return m_TypeMap; }
    //ConversionMap& GetConversionMap() { return m_ConvertMap; }

    RttiObject* GetUserPtr() const { return m_UserPtr; }

    AString m_PrintOutput;
    AString m_ErrOutput;

    bool m_HasPendingLoads = false;

  private:

    friend LuaWorld LuaManager::CreateWorld(RttiObject*);

    unsigned int m_InLua;
    lua_State* m_State;

    RttiObject* m_UserPtr;
    
    TypeMap m_TypeMap;
    //ConversionMap m_ConvertMap;
  };

  struct LuaWorld::Impl
  {
    std::unique_ptr<LuaImplState> m_State;
    UnorderedSet<ResourceHandle<LuaScript>> m_LoadedScripts;
    UnorderedSet<ResourceHandle<LuaScript>/*, std::function<void(luabind::object)>*/> m_PendingLoads;

    Err LoadScript(LuaScript const& iScript/*, std::function<void(luabind::object)> iOnLoadCompleted*/);
    void FlushPendingLoads();
  };

  namespace LuaManager
  {
    namespace
    {

#ifdef EXL_THREADAWARE
      boost::thread_specific_ptr<LuaImplState> currentState;
#else
      std::unique_ptr<LuaImplState> currentState;
#endif

      struct Impl
      {
        List<LuaRegFun> m_RegList;
        uint32_t m_Revision = 0;
        UnorderedMap<lua_State*, LuaWorld::Impl*> m_StateToWorld;
      };

      boost::optional<Impl> s_Impl;

      void *l_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
      {
        (void)ud;
        if (nsize == 0)
        {
          eXl_FREE(ptr);
          return nullptr;
        }
        else
        {
          if (osize > 0)
          {
            if (osize >= nsize)
            {
              return ptr;
            }
            else
            {
              void* newPtr = eXl_ALLOC(nsize);
              if (ptr)
              {
                memcpy(newPtr, ptr, osize);
              }
              eXl_FREE(ptr);
              return newPtr;
            }
          }
          else
          {
            return eXl_ALLOC(nsize);
          }
        }
      }

      int onError(lua_State* L)
      {
        lua_Debug d;
        lua_getstack(L, 1, &d);
        lua_getinfo(L, "Sln", &d);
        std::string err = lua_tostring(L, -1);
        lua_pop(L, 1);

        std::stringstream msg;
        msg << d.short_src << ":" << d.currentline;

        if (d.name != 0)
        {
          msg << "(" << d.namewhat << " " << d.name << ")";
        }
        msg << " " << err.c_str();

        //lua_pushliteral(L, "_TRACEBACK"); 
        //luaL_loadstring(L,"debug.traceback()");
        //lua_getfield(L, LUA_GLOBALSINDEX, "debug");
        lua_getglobal(L, "debug");
        lua_getfield(L, -1, "traceback");
        lua_call(L, 0, 1);
        err = lua_tostring(L, -1);
        lua_pop(L, 1);

        msg << "\n Traceback : " << err;
        //lua_pushstring(L, msg.str().c_str());
        //eXl_ASSERT_MSG(false,msg.str().c_str());

        if (currentState.get() != nullptr)
        {
          currentState->m_ErrOutput = msg.str().c_str();
        }

        return 0;
      }

      void pushErrorHandler(lua_State* L)
      {
        lua_pushcfunction(L, &onError);
      }

      struct LuaSyncState
      {
        LuaSyncState()
        {
          inLua = 0;
          dirty = false;
        }
        void SetDirty()
        {
          dirty = true;
        }

        void AddRegFun(LuaRegFun iFun)
        {
          s_Impl->m_RegList.push_back(iFun);
          dirty = true;
        }

        void IncreaseInLua()
        {
          ++inLua;
        }

        void DecreaseInLua()
        {
          --inLua;
        }
        int inLua;
        bool dirty;
      };

      LuaSyncState s_SyncState;

      int l_my_print(lua_State* L)
      {
        LuaImplState* myState = currentState.get();
        if (myState && myState->IsInLua())
        {
          int nargs = lua_gettop(L);
          for (int i = 1; i <= nargs; ++i)
          {
            if (const char* str = lua_tostring(L, i))
            {
              myState->m_PrintOutput.append(str);
            }
            else
            {
              return luaL_error(L, "Could not print argument %d", i - 1);
            }
          }
          myState->m_PrintOutput.append("\n");
        }

        return 0;
      }

      const struct luaL_Reg printlib[] =
      {
        {"print", l_my_print},
        {nullptr, nullptr} /* end of array */
      };


      void BuildState(LuaStateHandle iStateHandle)
      {
        lua_State* iState = iStateHandle.GetState();
        luaL_openlibs(iState);
        luabind::open(iState);
        luabind::bind_class_info(iState);
        luabind::set_pcall_callback(&pushErrorHandler);

        lua_getglobal(iState, "_G");
        luaL_setfuncs(iState, printlib, 0);
        lua_pop(iState, 1);

        lua_pushcfunction(iState, &onError);
        unsigned int stackTop = lua_gettop(iState);

        List<LuaRegFun>::iterator iter = s_Impl->m_RegList.begin();
        List<LuaRegFun>::iterator iterEnd = s_Impl->m_RegList.end();
        for (; iter != iterEnd; iter++)
        {
          try
          {
            lua_pushcfunction(iState, *iter);
            lua_pcall(iState, 0, LUA_MULTRET, -2);
          }
          catch (std::exception& e)
          {
            std::string errMsg("Err during LuaReg : ");
            errMsg = errMsg + e.what();
            eXl_ASSERT_MSG(false, errMsg.c_str());
          }
        }
        lua_remove(iState, stackTop);
      }

      bool IsInLua()
      {
        LuaImplState* myState = currentState.get();
        if (myState == nullptr)
        {
          return false;
        }
        return myState->IsInLua();
      }

      lua_State* GetLocalState_Impl()
      {
        LuaImplState* myState = currentState.get();
        if (myState == nullptr)
        {
          return nullptr;
        }
        return myState->GetState();
      }

      void EndLocalState()
      {
        LuaImplState* myState = currentState.get();
        myState->EndUseState();
      }
    }

    void Init()
    {
      s_Impl.emplace();
    }

    void Destroy()
    {
      s_Impl.reset();
    }
  }

  void LuaImplState::BeginUseState()
  {
    if(m_InLua == 0)
    {
      eXl_ASSERT(LuaManager::currentState.get() == nullptr);
      LuaManager::currentState.reset(this);
      //LuaManager::s_SyncState.IncreaseInLua();
    }
    ++m_InLua;
  }

  void LuaImplState::EndUseState()
  {
    m_InLua--;
    if(m_InLua == 0)
    {
      //LuaManager::s_SyncState.DecreaseInLua();
      if(!m_PrintOutput.empty())
      {
        Log_Manager::Log(eXl::LUA_OUT_STREAM) << m_PrintOutput << "\n";
        m_PrintOutput.clear();
      }
      if(!m_ErrOutput.empty())
      {
        eXl::Log_Manager::Log(eXl::LUA_ERR_STREAM) << m_ErrOutput << "\n";
        m_ErrOutput.clear();
      }
      eXl_ASSERT(LuaManager::currentState.get() == this);
      LuaManager::currentState.release();

      if (m_HasPendingLoads)
      {
        m_HasPendingLoads = false;
        auto worldIter = LuaManager::s_Impl->m_StateToWorld.find(m_State);
        if (worldIter != LuaManager::s_Impl->m_StateToWorld.end())
        {
          worldIter->second->FlushPendingLoads();
        }
      }
    }
  }

  lua_State* LuaImplState::GetState()
  {
    return m_State;
  }

  LuaStateHandle::LuaStateHandle(LuaImplState* iImpl)
    : m_Impl(iImpl)
    , m_State(iImpl ? iImpl->GetState() : nullptr)
  {
    if (m_Impl)
    {
      m_Impl->BeginUseState();
    }
  }

  LuaStateHandle::~LuaStateHandle()
  {
    if (m_Impl)
    {
      m_Impl->EndUseState();
    }
  }

  LuaStateHandle::LuaStateHandle(LuaStateHandle const& iOther)
    : m_Impl(iOther.m_Impl)
    , m_State(iOther.m_State)
  {
    if (m_Impl)
    {
      m_Impl->BeginUseState();
    }
  }

  LuaStateHandle& LuaStateHandle::operator=(LuaStateHandle const& iOther)
  {
    if (&iOther == this)
    {
      return *this;
    }

    iOther.m_Impl->BeginUseState();
    if(m_Impl != nullptr)
    {
      m_Impl->EndUseState();
    }
    m_Impl = iOther.m_Impl;
    m_State = iOther.m_State;

    return *this;
  }

  LuaStateHandle::LuaStateHandle(LuaStateHandle&& iOther)
    : m_Impl(iOther.m_Impl)
    , m_State(iOther.m_State)
  {
    iOther.m_Impl = nullptr;
    iOther.m_State = nullptr;
  }

  LuaStateHandle& LuaStateHandle::operator=(LuaStateHandle&& iOther)
  {
    if (&iOther == this)
    {
      return *this;
    }

    m_Impl = iOther.m_Impl;
    m_State = iOther.m_State;
    iOther.m_Impl = nullptr;
    iOther.m_State = nullptr;

    return *this;
  }

  RttiObject* LuaStateHandle::GetUserPtr()
  {
    if (m_Impl == nullptr)
    {
      return nullptr;
    }
    return m_Impl->GetUserPtr();
  }

  namespace LuaManager
  {
    //void RegisterType(LuaStateHandle iHandle, void* iBinderId, Type const* iType, PushFuncs iConvertFunction)
    //{
    //  iHandle.GetImpl()->GetTypeMap().insert(std::make_pair(iBinderId, iType));
    //  iHandle.GetImpl()->GetConversionMap().insert(std::make_pair(iType, iConvertFunction));
    //}

    //void PushRefToLua(LuaStateHandle iHandle, void* iObject, Type const* iType)
    //{
    //  if (iHandle.GetImpl())
    //  {
    //    auto const& conversionMap = iHandle.GetImpl()->GetConversionMap();
    //    auto iter = conversionMap.find(iType);
    //    if (iter != conversionMap.end())
    //    {
    //      iter->second.pushRefFunc(iHandle.GetState(), iObject);
    //    }
    //  }
    //}
    //
    //void PushCopyToLua(LuaStateHandle iHandle, void const* iObject, Type const* iType)
    //{
    //  if (iHandle.GetImpl())
    //  {
    //    auto const& conversionMap = iHandle.GetImpl()->GetConversionMap();
    //    auto iter = conversionMap.find(iType);
    //    if (iter != conversionMap.end())
    //    {
    //      iter->second.pushCopyFunc(iHandle.GetState(), iObject);
    //    }
    //  }
    //}

    LuaStateHandle GetHandle(lua_State* iState)
    {
      auto iter = s_Impl->m_StateToWorld.find(iState);
      if(iter != s_Impl->m_StateToWorld.end())
      {
        return LuaStateHandle(iter->second->m_State.get());
      }
      return LuaStateHandle(nullptr);
    }

    //Object must be pushed on the stack
    Type const* GetObjectType(LuaStateHandle iHandle)
    {
      lua_State* iState = iHandle.GetState();
      
      if(lua_isnil(iState, -1))
      {
        return nullptr;
      }
      else if(lua_isboolean(iState, -1))
      {
        return TypeManager::GetType<bool>();
      }
      else if(lua_isnumber(iState, -1))
      {
        return TypeManager::GetType<float>();
      }
      else if(lua_isstring(iState, -1))
      {
        return TypeManager::GetType<AString>();
      }
      else if(lua_istable(iState, -1))
      {
        eXl_ASSERT_MSG("False","Cannot operate on tables !!!!");
        return nullptr;
      }
      else if(lua_isuserdata(iState, -1))
      {
        void* id = LuaBinderImplGetTypeID(iState);
        LuaImplState* impl = iHandle.GetImpl();
        auto iter = impl->GetTypeMap().find(id);
        if (iter == impl->GetTypeMap().end())
        {
          return nullptr;
        }
        return iter->second;
      }

      return nullptr;
    }

    LuaStateHandle GetLocalState()
    {
      LuaImplState* localState = currentState.get();
      return LuaStateHandle(localState);
    }

    void AddRegFun(LuaRegFun iFun)
    {
      s_SyncState.AddRegFun(iFun);
    }

    void Lua_Error(lua_State* iState, int res, AString const& info)
    {
      if(res!=0)
      {
        luabind::object error_msg(luabind::from_stack(iState, -1));
        std::stringstream ss;
        ss<<error_msg;

        currentState->m_ErrOutput.append(AString(AString("Err while ") + info + " : " + ss.str().c_str()).c_str());

        lua_pop(iState, 1);
      }
      else
      {
        AString stack;
        stackDump(iState, stack);
        currentState->m_ErrOutput.append(stack);
        
        int top = lua_gettop(iState);
        lua_pop(iState,top);
      }

    }

    namespace detail
    {

      //void DoFile(LuaFile* iFile,lua_State* iState)
      //{
      //  lua_State* myState = nullptr;
      //  if(iState==nullptr)
      //    myState = GetLocalState_Impl();
      //  else
      //    myState = iState;
      //  {
      //    std::vector<DataVault*>::iterator iter = sources.begin();
      //    std::vector<DataVault*>::iterator iterEnd = sources.end();
      //  
      //    bool found = false;
      //    for(;iter!=iterEnd;iter++)
      //    {
      //      StorageInfo info;
      //      found = (*iter)->GetStorage(iFile->GetPath(),info);
      //      if(found)break;
      //    }
      //    InputStream* stream = (*iter)->OpenStorage(iFile->GetPath());
      //    size_t size = stream->GetSize();
      //    char* source = (char*)eXl_ALLOC(size+1);
      //    stream->Read(0,size,source);
      //    source[size]='\0';
      //    //int res = luaL_dofile(iState,iFile.c_str());
      //    //int res = luaL_dostring(myState,source);
      //    lua_pushcfunction(myState,&onError);
      //    int res = luaL_loadstring(myState,source);
      //    if(res == 0)
      //      res = lua_pcall(myState,0,LUA_MULTRET,-2);
      //    else
      //      Lua_Error(myState,res,std::string("executing ")+iFile->GetName());
      //    lua_pop(myState,1);
      //    eXl_FREE(source);
      //    eXl_DELETE stream;
      //  }
      //  if(iState == nullptr)
      //  //  EndLocalState();
      //}
    }

    Err DoString(AString const& iCode, AString& oStr,LuaStateHandle& iStat, luabind::object& oRet)
    {
      oStr.clear();
      Err res = DoString(iCode, iStat, oRet);
      oStr = iStat.GetImpl()->m_PrintOutput;
      oStr.append(iStat.GetImpl()->m_ErrOutput);

      return res;
    }

    Err DoString(AString const& iCode, LuaStateHandle& iStat, luabind::object& oRet)
    {
      Err opRes = Err::Success;

      lua_State* myState = iStat.GetState();
      lua_pushcfunction(myState,&onError);
      unsigned int stackTop = lua_gettop(myState);
      int res = luaL_loadstring(myState,iCode.c_str());
      if(res == 0)
      {
        res = lua_pcall(myState, 0, 1, -2);
        if (res != 0)
        {
          opRes = Err::Error;
        }
        //Log_Manager::Log(LUA_OUT_STREAM) << StackDump(myState);
        uint32_t curTop = lua_gettop(myState);
        if (curTop > stackTop)
        {
          
          oRet = luabind::object(luabind::from_stack(myState, curTop));
          lua_remove(myState, curTop);
        }
      }
      else
      {
        Lua_Error(myState,res, AString("executing ") + iCode);
        opRes = Err::Error;
      }
      lua_remove(myState,stackTop);
      return opRes;
    }

    void GetFunction(lua_State* iState, AString const& iName, unsigned iDepth)
    {
      if(iDepth == 0)
      {
        size_t pos = iName.find(".");
        if(pos == std::string::npos)
        {
          lua_getglobal(iState, iName.c_str());
          eXl_ASSERT_MSG(!lua_isnil(iState,-1),"Could not get table");
        }
        else
        {
          AString globName = iName.substr(0,pos);
          lua_getglobal(iState,globName.c_str());
          eXl_ASSERT_MSG(!lua_isnil(iState,-1),"Could not get table");
          globName = iName.substr(pos+1);
          GetFunction(iState,globName,iDepth + 1);
        }
      }
      else
      {
        size_t pos = iName.find(".");
        if(pos == std::string::npos)
        {
          lua_getfield(iState, -1, iName.c_str());
          eXl_ASSERT_MSG(!lua_isnil(iState,-1),"Could not get table");
          lua_replace(iState,-2);        
        }
        else
        {
          AString globName = iName.substr(0,pos);
          lua_getfield(iState,-1,globName.c_str());
          eXl_ASSERT_MSG(!lua_isnil(iState,-1),"Could not get table");
          lua_replace(iState,-2);
        
          globName = iName.substr(pos+1);
          GetFunction(iState,globName,iDepth + 1);
        }
      }
    }

    namespace detail
    {
      void CallFunction(lua_State* iState, unsigned int iArgs, AString const& iFun)
      {
        {
          lua_pushcfunction(iState,&onError);
          unsigned int stackTop = lua_gettop(iState);
          lua_insert(iState,(stackTop - iArgs - 1));
          
          int res = lua_pcall(iState,iArgs,0,-(2 + (int)iArgs));
          Lua_Error(iState, res, AString("calling ") + iFun);
          lua_remove(iState,stackTop - iArgs - 1);
        }
      }
    }
  }

  LuaStateHandle GetCurrentState();

  LuaWorld::LuaWorld(std::unique_ptr<LuaImplState> iState)
  {
    m_Impl = std::make_unique<Impl>();
    m_Impl->m_State = std::move(iState);
    LuaManager::s_Impl->m_StateToWorld.insert(std::make_pair(m_Impl->m_State->GetState(), m_Impl.get()));
  }

  LuaWorld::~LuaWorld()
  {
    if (m_Impl)
    {
      lua_State* state = m_Impl->m_State->GetState();
      LuaManager::s_Impl->m_StateToWorld.erase(state);
    }
  }

  LuaWorld::LuaWorld(LuaWorld&&) = default;
  LuaWorld& LuaWorld::operator=(LuaWorld&&) = default;

  LuaStateHandle LuaWorld::GetState()
  {
    return LuaStateHandle(m_Impl->m_State.get());
  }

  Err LuaWorld::DoString(const AString& iCode, luabind::object& oRes)
  {
    LuaStateHandle stateHandle = GetState();
    return LuaManager::DoString(iCode, stateHandle, oRes);
  }

  Err LuaWorld::DoString(const AString& iCode, AString& oStr, luabind::object& oRes)
  {
    LuaStateHandle stateHandle = GetState();
    return LuaManager::DoString(iCode, oStr, stateHandle, oRes);
  }

  LuaWorld::Impl& LuaWorld::GetImpl()
  {
    return *m_Impl;
  }


  LuaWorld LuaManager::CreateWorld(RttiObject* iUserPtr)
  {
    auto newState = std::make_unique<LuaImplState>(iUserPtr);

    newState->m_State = lua_newstate(&LuaManager::l_alloc, nullptr);
    
    LuaWorld newWorld(std::move(newState));

    BuildState(LuaStateHandle(newWorld.GetState()));

    return newWorld;
  }

  LuaStateHandle LuaManager::GetCurrentState()
  {
    LuaStateHandle handle(currentState.get());
    return handle;
  }

  //Err LuaWorld::LoadScript(LuaScript const& iScript)
  //{
  //  return m_Impl->LoadScript(iScript);
  //}

  Err LuaWorld::Impl::LoadScript(LuaScript const& iScript)
  {
    ResourceHandle<LuaScript> handle;
    handle.Set(&iScript);

    if (m_LoadedScripts.count(handle) > 0)
    {
      return Err::Failure;
    }

    //if (m_State->IsInLua())
    //{
    //  m_PendingLoads.insert(handle);
    //  m_State->m_HasPendingLoads = true;
    //}

    luabind::object dummy;
    LuaStateHandle stateHandle(m_State.get());
    Err res = LuaManager::DoString(iScript.m_Script, stateHandle, dummy);
    if (!res)
    {
      return res;
    }

    m_LoadedScripts.insert(handle);

    return Err::Success;
  }

  void LuaWorld::Impl::FlushPendingLoads()
  {
    UnorderedSet<ResourceHandle<LuaScript>> toLoad;
    std::swap(toLoad, m_PendingLoads);

    for (auto& handle : toLoad)
    {
      if (LuaScript const* script = handle.Get())
      {
        LoadScript(*script);
      }
    }
  }

  LuaStateHandle::CallCtx::~CallCtx()
  {
    int32_t curTop = lua_gettop(m_State);
    int32_t toRemove = curTop - m_StackTop;
    if (toRemove > 0)
    {
      lua_pop(m_State, toRemove);
    }
  }

  boost::optional<uint32_t> LuaStateHandle::CallCtx::Call(uint32_t iExpectedRes)
  {
    int res = lua_pcall(m_State, m_NumArgs, iExpectedRes, -(2 + (int)m_NumArgs));

    m_NumArgs = 0;
    if (res == 0)
    {
      int32_t functionPos = m_StackTop + 1;
      int32_t ret = lua_gettop(m_State) - functionPos;
      eXl_ASSERT(ret >= 0);
      return ret;
    }
    else
    {
      return {};
    }
  }

  LuaStateHandle::CallCtx::CallCtx(lua_State* iState, luabind::object const& iFunObj)
    : m_State(iState)
    , m_StackTop(lua_gettop(iState))
  {
    lua_pushcfunction(m_State, &LuaManager::onError);
    iFunObj.push(iState);
  }
      
  LuaStateHandle::CallCtx LuaStateHandle::PrepareCall(luabind::object const& iFunObj)
  {
    return CallCtx(GetState(), iFunObj);
  }

  DynObject LuaManager::GetPushedObjectRef(lua_State* iState, Type const* iType)
  {
    DynObject objRef;
    
    luabind::detail::object_rep* obj = luabind::detail::get_instance(iState, -1);
    if (!obj)
    {
      return objRef;
    }

    auto res = obj->get_instance(luabind::detail::allocate_class_id(iType));

    if (res.first == nullptr)
    {
      return objRef;
    }

    objRef.SetType(iType, res.first);

    return objRef;
  }

  DynObject LuaManager::GetObjectRef(luabind::object const& iObject, Type const* iType)
  {
    DynObject objRef;

    lua_State* state = iObject.interpreter();
    iObject.push(state);
    luabind::detail::stack_pop stackPop(state, 1);
    return GetPushedObjectRef(state, iType);
  }

  luabind::detail::class_rep* GetClassRepFromType(lua_State* iState, Type const* iType)
  {
    lua_pushliteral(iState, "__luabind_class_id_map");
    lua_rawget(iState, LUA_REGISTRYINDEX);
    luabind::detail::class_id_map& class_ids = *static_cast<luabind::detail::class_id_map*>(lua_touserdata(iState, -1));
    lua_pop(iState, 1);

    lua_pushliteral(iState, "__luabind_class_map");
    lua_rawget(iState, LUA_REGISTRYINDEX);
    luabind::detail::class_map const& classes = *static_cast<luabind::detail::class_map*>(lua_touserdata(iState, -1));
    lua_pop(iState, 1);

    luabind::detail::class_id clsId = class_ids.get_local(iType);
    luabind::detail::class_rep* cls = classes.get(clsId);

    return cls;
  }

  luabind::object GetLuaRef(lua_State* iState, Type const* iType, void* iMem, bool iIsConst)
  {
    LuaManager::PushRefToLua(iState, iType, iMem, iIsConst);
    luabind::object retObj(luabind::from_stack(iState, -1));
    lua_pop(iState, 1);

    return retObj;
  }

  void LuaManager::PushRefToLua(lua_State* iState, Type const* iType, void* iObject, bool iIsConst)
  {
    DynObject objRef;
    objRef.SetType(iType, iObject);

    luabind::detail::class_rep* cls = GetClassRepFromType(iState, iType);

    if (!cls)
    {
      Log_Manager::Log(LUA_ERR_STREAM) << "Trying to use unregistered class: " << iType->GetName();
      return ;
    }

    luabind::detail::object_rep* instance = luabind::detail::push_new_instance(iState, cls);

    void* storage = instance->allocate(sizeof(dynobject_holder));
    dynobject_holder* holder = new (storage) dynobject_holder(std::move(objRef), iIsConst);

    instance->set_instance(holder);
  }

  void LuaManager::PushRefToLua(lua_State* iState, Type const* iType, void const* iObject)
  {
    PushRefToLua(iState, iType, (void*)iObject, true);
  }

  void LuaManager::PushCopyToLua(lua_State* iState, Type const* iType, void const* iObject)
  {
    ConstDynObject objRef(iType, iObject);

    luabind::detail::class_rep* cls = GetClassRepFromType(iState, iType);

    if (!cls)
    {
      Log_Manager::Log(LUA_ERR_STREAM) << "Trying to use unregistered class: " << iType->GetName();
      return ;
    }

    luabind::detail::object_rep* instance = luabind::detail::push_new_instance(iState, cls);

    void* storage = instance->allocate(sizeof(dynobject_holder));
    dynobject_holder* holder = new (storage) dynobject_holder(DynObject(&objRef), false);

    instance->set_instance(holder);
  }

  luabind::object LuaManager::GetLuaRef(lua_State* iState, DynObject const& iObject)
  {
    if (iObject.IsValid())
    {
      return GetLuaRef(iState, iObject.GetType(), (void*)iObject.GetBuffer(), false);
    }
    return luabind::object();
  }

  luabind::object LuaManager::GetLuaRef(lua_State* iState, ConstDynObject const& iObject)
  {
    if (iObject.IsValid())
    {
      return GetLuaRef(iState, iObject.GetType(), (void*)iObject.GetBuffer(), true);
    }
    return luabind::object();
  }

  void LuaManager::Reset()
  {
    eXl_ASSERT(s_Impl->m_StateToWorld.empty());
    luabind::detail::reset_class_id();
  }
}

#endif
