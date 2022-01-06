namespace eXl
{
  template <typename CommandToLookFor, typename Command, typename... Commands>
  struct GetRank
  {
    static const uint32_t result = GetRank<CommandToLookFor, Commands...>::result + 1;
  };

  template <typename CommandToLookFor, typename... Commands>
  struct GetRank<CommandToLookFor, CommandToLookFor, Commands...>
  {
    static const uint32_t result = 0;
  };

  template<template<class> class Handler, typename Current, typename... List>
  struct ForEachItem
  {
    template <typename... Args>
    static void Do(Args&&... iArgs)
    {
      Handler<Current>::Do(std::forward<Args>(iArgs)...);
      ForEachItem<Handler, List...>::Do(std::forward<Args>(iArgs)...);
    }
  };

  template<template<class> class Handler, typename Current>
  struct ForEachItem<Handler, Current>
  {
    template <typename... Args>
    static void Do(Args&&... iArgs)
    {
      Handler<Current>::Do(std::forward<Args>(iArgs)...);
    }
  };

  template<template<class> class Handler, typename... List>
  struct ForEachUnwrapper;

  template<template<class> class Handler, typename Current, typename... List>
  struct ForEachUnwrapper<Handler, Current, List...>
  {
    template <template <typename> class Wrapper, typename... Args>
    static void Do(ListWrapper<Wrapper, Current, List...>& iWrapped, Args&&... iArgs)
    {
      Handler<Current>::Do(static_cast<Wrapper<Current>&>(iWrapped), std::forward<Args>(iArgs)...);
      ForEachItem<Handler, List...>::Do(static_cast<ListWrapper<Wrapper, List...>&>(iWrapped), std::forward<Args>(iArgs)...);
    }
  };
  
  template<template<class> class Handler, typename Current>
  struct ForEachUnwrapper<Handler, Current>
  {
    template <template <typename> class Wrapper, typename... Args>
    static void Do(ListWrapper<Wrapper, Current>& iWrapped, Args&&... iArgs)
    {
      Handler<Current>::Do(static_cast<Wrapper<Current>&>(iWrapped), std::forward<Args>(iArgs)...);
    }
  };

  template <class StateCommand>
  uint16_t StateCommandHandler<StateCommand>::GetCommandId(StateCommand const& iCmd)
  {
    auto iter = m_CommandsId.find(iCmd);
    if (iter != m_CommandsId.end())
    {
      return iter->second;
    }

    uint16_t newId = m_Commands.size();

    m_CommandsId.insert(std::make_pair(iCmd, newId));
    m_Commands.push_back(iCmd);

    return newId;
  }

  //template <typename... Commands>
  //template <class StateCommand>
  //inline StateCommand const& OGLStateCollection<Commands...>::GetDefaultCommand()
  //{
  //  return StateCommandHandler<StateCommand>::GetDefaultCommand();
  //}
  
  template <typename... Commands>
  template <class StateCommand>
  inline StateCommand const& OGLStateCollection<Commands...>::GetCurrentSetCommand()
  {
    uint16_t cmdRank = GetRank<StateCommand, Commands...>::result;
    uint16_t curCmdId = m_NextCommand.m_Commands[cmdRank];
    return StateCommandHandler<StateCommand>::m_Commands[curCmdId];
  }

  template <typename... Commands>
  template <class StateCommand>
  inline void OGLStateCollection<Commands...>::SetDefaultCommand(StateCommand const& iCmd)
  {
    uint16_t cmdRank = GetRank<StateCommand, Commands...>::result;
    uint16_t nextCmdId = StateCommandHandler<StateCommand>::GetCommandId(iCmd);
    m_NextCommand.m_Commands[cmdRank] = nextCmdId;
    m_CurStateUpToDate = false;
  }

  template <typename... Commands>
  template <class StateCommand>
  inline void OGLStateCollection<Commands...>::SetCommand(StateCommand const& iCmd)
  {
    if(GetCurrentSetCommand<StateCommand>() != iCmd)
    {
      uint16_t cmdRank = GetRank<StateCommand, Commands...>::result;
      uint16_t nextCmdId = StateCommandHandler<StateCommand>::GetCommandId(iCmd);
      m_NextCommand.m_Commands[cmdRank] = nextCmdId;
      m_CurStateUpToDate = false;
    }
  }

  template <typename... Commands>
  inline void OGLStateCollection<Commands...>::InitForPush()
  {
    
  }

  struct ApplyCommandCtx
  {
    int curIdx = 0;
    uint16_t const* prevState;
    uint16_t const* nextState;
  };

  template <class StateCommand>
  struct ApplyCommandHandler
  {
    typedef ApplyCommandCtx& value_type1;
    inline static void Do(CommandStorage<StateCommand>& iCmd, ApplyCommandCtx& iCtx)
    {
      if (*iCtx.prevState != *iCtx.nextState)
      {
        iCmd.m_Command.Apply();
      }

      ++iCtx.curIdx;
      ++iCtx.prevState;
      ++iCtx.nextState;
    }
  };

  template <typename... Commands>
  inline void OGLStateCollection<Commands...>::InitForRender()
  {
    if (!m_StateIds.empty())
    {
      StateIds dummy;
      std::fill(dummy.m_Commands, dummy.m_Commands + GetListSize<Commands...>::result, 0xFFFF);

      ApplyCommandCtx ctx;
      ctx.prevState = dummy.m_Commands;
      ctx.nextState = m_StateIds[0].m_Commands;
      ctx.curIdx = 0;

      ForEachUnwrapper<ApplyCommandHandler, Commands...>::Do(m_States[0], ctx);
      m_CurrentState = 0;
    }
  }

  template <typename... Commands>
  inline void OGLStateCollection<Commands...>::ApplyCommand(uint16_t iCmdId)
  {
    if(iCmdId != m_CurrentState)
    {
      eXl_ASSERT_REPAIR_RET(iCmdId < m_States.size(), );

      ApplyCommandCtx ctx;
      ctx.prevState = m_StateIds[m_CurrentState].m_Commands;
      ctx.nextState = m_StateIds[iCmdId].m_Commands;
      ctx.curIdx = 0;
      
      ForEachUnwrapper<ApplyCommandHandler, Commands...>::Do(m_States[iCmdId], ctx);

      m_CurrentState = iCmdId;
    }
  }

  template <typename... Commands>
  struct GatherCommandsCtx
  {
    GatherCommandsCtx(OGLStateCollection<Commands...>& iCollection)
      : collection(iCollection)
    {}

    OGLStateCollection<Commands...>& collection;
  };

  template <class StateCommand>
  struct GatherCommandsHandler
  {
    template <typename... Commands>
    inline static void Do(CommandStorage<StateCommand>& iCmd, GatherCommandsCtx<Commands...>& iCtx)
    {
      iCmd.m_Command = iCtx.collection.template GetCurrentSetCommand<StateCommand>();
    }
  };

  template <typename... Commands>
  inline uint16_t OGLStateCollection<Commands...>::GetStateId()
  {
    if (m_CurStateUpToDate)
    {
      return m_CurrentState;
    }
    else
    {
      auto iter = m_StateAssoc.find(m_NextCommand);
      if (iter != m_StateAssoc.end())
      {
        m_CurStateUpToDate = true;
        return iter->second;
      }

      uint16_t newId = m_StateIds.size();

      m_StateIds.push_back(StateIds());
      m_States.push_back(StateStorage());

      StateIds& nextIds = m_StateIds.back();
      StateStorage& nextStates = m_States.back();
      GatherCommandsCtx ctx(*this);
      nextIds = m_NextCommand;
      ForEachUnwrapper<GatherCommandsHandler, Commands...>::Do(nextStates, ctx);

      m_StateAssoc.insert(std::make_pair(m_NextCommand, newId));
      m_CurrentState = newId;
      m_CurStateUpToDate = true;

      return newId;
    }
  }
}