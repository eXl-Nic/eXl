
inline bool ClientId::operator == (ClientId const& iOther) const
{
  return id == iOther.id;
}
inline bool ClientId::operator != (ClientId const& iOther) const
{
  return !(*this == iOther);
}

inline bool ObjectId::operator == (ObjectId const& iOther) const
{
  return id == iOther.id;
}
inline bool ObjectId::operator != (ObjectId const& iOther) const
{
  return !(*this == iOther);
}

inline size_t hash_value(ClientId const& iId)
{
  return static_cast<size_t>(iId.id);
}

inline size_t hash_value(ObjectId const& iId)
{
  return static_cast<size_t>(iId.id);
}

inline Err ClientId::Stream(Streamer& iStreamer) const
{
  return iStreamer.WriteUInt64(&id);
}
inline Err ClientId::Unstream(Unstreamer& iStreamer)
{
  return iStreamer.ReadUInt64(&id);
}
inline Err ObjectId::Stream(Streamer& iStreamer) const
{
  return iStreamer.WriteUInt64(&id);
}
inline Err ObjectId::Unstream(Unstreamer& iStreamer)
{
  return iStreamer.ReadUInt64(&id);
}


inline CommandDesc::CommandDesc(CommandDesc&& iDesc)
{
  m_Name = iDesc.m_Name;
  m_Callback = std::move(iDesc.m_Callback);
  m_Executor = iDesc.m_Executor;
  iDesc.m_Args.reset();
  m_FunDesc = std::move(iDesc.m_FunDesc);
  m_Args.emplace(m_FunDesc.arguments);
  m_Reliable = iDesc.m_Reliable;
}

template<typename RetType, typename... Args>
struct CallRetTypeDispatcher
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
struct CallRetTypeDispatcher<void, Args...>
{
  template <typename Function>
  static void Execute(Function const& iFun, ConstDynObject const& iArgsBuffer, DynObject& oOutput)
  {
    Invoker<Args...>::template Call<void>(iFun, iArgsBuffer);
  }
};

template<typename RetType, typename... Args>
void NetDriver::DeclareClientCommand(CommandName iName, std::function<RetType(uint32_t, Args...)>& iFun, bool iReliable)
{
  auto cb = [this, &iFun](uint64_t iClient, ConstDynObject const& iArgsBuffer, DynObject& oOutput)
  {
    CallRetTypeDispatcher<RetType, Args...>::Execute([this, &iFun, iClient](Args... iArgs)
      {
        eXl_ASSERT_REPAIR_RET(!(!iFun), RetType());
        return iFun(iClient, std::forward<Args>(iArgs)...);
      }, iArgsBuffer, oOutput);
  };
  DeclareCommand(NetRole::Client, iName, std::move(cb), FunDesc::Create<RetType(Args...)>(), &iFun, iReliable);
}

template<typename RetType, typename... Args>
void NetDriver::DeclareServerCommand(CommandName iName, std::function<RetType(ClientId, Args...)>& iFun, bool iReliable)
{
  auto cb = [this, &iFun](uint64_t iClient, ConstDynObject const& iArgsBuffer, DynObject& oOutput)
  {
    CallRetTypeDispatcher<RetType, Args...>::Execute([this, &iFun, iClient](Args... iArgs)
      {
        eXl_ASSERT_REPAIR_RET(!(!iFun), RetType());
        ClientId id{ iClient };
        return iFun(id, std::forward<Args>(iArgs)...);
      }, iArgsBuffer, oOutput);
  };
  DeclareCommand(NetRole::Server, iName, std::move(cb), FunDesc::Create<RetType(Args...)>(), &iFun, iReliable);
}

template<typename RetType, typename... Args>
NetDriver::CommandCaller<RetType, Args...>::CommandCaller(NetDriver& iDriver, NetRole iExecutor, void* iCommandPtr, uint64_t iClientId)
  : m_Driver(iDriver)
  , m_ClientId(iClientId)
{
  m_Data.m_Executor = iExecutor;
  m_Data.m_CommandPtr = iCommandPtr;

  auto iter = m_Driver.m_CommandsByPtr.find(iCommandPtr);
  if (iter == m_Driver.m_CommandsByPtr.end())
  {
    return;
  }
  CommandDesc const& desc = m_Driver.m_Commands[iter->second];
  if (desc.m_Executor != iExecutor)
  {
    return;
  }
  m_Command = &desc;
}

template<typename RetType, typename... Args>
template <typename T, typename std::enable_if<std::is_convertible<RetType, T>::value, bool >::type>
NetDriver::CommandCaller<RetType, Args...>& NetDriver::CommandCaller<RetType, Args...>::WithCompletionCallback(std::function<void(T)> iCompletionCallback)
{
  if (m_Command != nullptr
    && iCompletionCallback)
  {
    // To not have to bother with timeout, double send, etc...
    eXl_ASSERT_MSG_RET(m_Command->m_Reliable, "Only reliable commands can have completion callbacks", *this);
    m_Data.m_CompletionCallback = [userCallback = std::move(iCompletionCallback)](ConstDynObject const& iResBuffer)
    {
      userCallback(iResBuffer.CastBuffer<RetType>());
    };
  }
  return *this;
}

template<typename RetType, typename... Args>
NetDriver::CommandCaller<RetType, Args...>& NetDriver::CommandCaller<RetType, Args...>::WithCompletionCallback(std::function<void()> iCompletionCallback)
{
  if (m_Command != nullptr
    && iCompletionCallback)
  {
    // To not have to bother with timeout, double send, etc...
    eXl_ASSERT_MSG_RET(m_Command->m_Reliable, "Only reliable commands can have completion callbacks", *this);
    m_Data.m_CompletionCallback = [userCallback = std::move(iCompletionCallback)](ConstDynObject const& iResBuffer)
    {
      userCallback();
    };
  }
  return *this;
}

template<typename RetType, typename... Args>
NetDriver::CommandCaller<RetType, Args...>& NetDriver::CommandCaller<RetType, Args...>::WithArgs(Args... iArgs)
{
  if (m_Command != nullptr)
  {
    Err res = m_Command->m_FunDesc.PopulateArgBuffer(*m_Command->m_Args, m_Data.m_Args, std::forward<Args>(iArgs)...);
    eXl_ASSERT(res);
  }
  return *this;
}

template<typename RetType, typename... Args>
Err NetDriver::CommandCaller<RetType, Args...>::Send()
{
  if (m_Command != nullptr)
  {
    if (m_Data.m_Executor == NetRole::Server)
    {
      eXl_ASSERT_REPAIR_RET(m_ClientId < m_Driver.m_Ctx.m_Clients.size(), Err::Failure);
      return m_Driver.m_Ctx.m_Clients[m_ClientId]->SendServerCommand(std::move(m_Data));
    }
    else
    {
      eXl_ASSERT_REPAIR_RET(m_Driver.m_Ctx.m_Server != nullptr, Err::Failure);
      ClientId clientId{ m_ClientId };
      return m_Driver.m_Ctx.m_Server->SendClientCommand(std::move(m_Data), clientId);
    }
  }
  return Err::Failure;
}

