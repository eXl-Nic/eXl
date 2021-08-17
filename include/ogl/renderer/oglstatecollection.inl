namespace eXl
{
  
  template <typename Command, typename... Commands>
  struct GetListSize
  {
    static const unsigned int result = GetListSize<Commands...>::result + 1;
  };

  template <typename Command>
  struct GetListSize<Command>
  {
    static const unsigned int result = 1;
  };

  template <typename CommandToLookFor, typename Command, typename... Commands>
  struct GetRank
  {
    static const unsigned int result = GetRank<CommandToLookFor, Commands...>::result + 1;
  };

  template <typename CommandToLookFor, typename... Commands>
  struct GetRank<CommandToLookFor, CommandToLookFor, Commands...>
  {
    static const unsigned int result = 0;
  };

  template<template<class> class Handler, typename Current, typename... List>
  struct ForEach1 
  {
    typedef typename Handler<int>::value_type1 value_type1;

    static void Do(value_type1 const& val1) 
    {
      Handler<Current>::Do(val1);
      ForEach1<Handler, List...>::Do(val1);
    }
  };

  template<template<class> class Handler, typename Current>
  struct ForEach1<Handler, Current>
  {
    typedef typename Handler<int>::value_type1 value_type1;

    static void Do(value_type1 const& val1) 
    {
      Handler<Current>::Do(val1);
    }
  };

  template<template<class> class Handler, typename Current, typename... List>
  struct ForEach2 
  {
    typedef typename Handler<int>::value_type1 value_type1;
    typedef typename Handler<int>::value_type2 value_type2;

    static void Do(value_type1 val1, value_type2 val2) 
    {
      Handler<Current>::Do(val1, val2);
      ForEach2<Handler, List...>::Do(val1, val2);
    }
  };

  template<template<class> class Handler, typename Current>
  struct ForEach2<Handler, Current>
  {
    typedef typename Handler<int>::value_type1 value_type1;
    typedef typename Handler<int>::value_type2 value_type2;

    static void Do(value_type1 val1, value_type2 val2) 
    {
      Handler<Current>::Do(val1, val2);
    }
  };

  template<template<class> class Handler, typename Current, typename... List>
  struct ForEach3 
  {
    typedef typename Handler<int>::value_type1 value_type1;
    typedef typename Handler<int>::value_type2 value_type2;
    typedef typename Handler<int>::value_type3 value_type3;

    static void Do(value_type1 val1, value_type2 val2, value_type3 val3) 
    {
      Handler<Current>::Do(val1, val2, val3);
      ForEach3<Handler, List...>::Do(val1, val2, val3);
    }
  };

  template<template<class> class Handler, typename Current>
  struct ForEach3<Handler, Current>
  {
    typedef typename Handler<int>::value_type1 value_type1;
    typedef typename Handler<int>::value_type2 value_type2;
    typedef typename Handler<int>::value_type3 value_type3;

    static void Do(value_type1 val1, value_type2 val2, value_type3 val3) 
    {
      Handler<Current>::Do(val1, val2, val3);
    }
  };

  template <typename... Commands>
  struct GetStateColSize;

  template <typename Command>
  struct GetStateColSize<Command>{static size_t const size = sizeof(Command);};

  template <typename Command, typename... Commands>
  struct GetStateColSize<Command, Commands...>{static size_t const size = sizeof(Command) + GetStateColSize<Commands...>::size;};

  template <class StateCommand>
  struct ComputeStateColSize
  {
    typedef unsigned int& value_type1;
    typedef unsigned int value_type2;
    typedef size_t& value_type3;
    inline static void Do(unsigned int& iCounter, unsigned int iFlags, size_t& oSize)
    {
      if(1<<iCounter & iFlags)
        oSize += sizeof(StateCommand);

      ++iCounter;
    }
  };

  template <typename... Commands>
  template <class StateCommand>
  inline void OGLStateCollection<Commands...>::InitHandler<StateCommand>::Do(OGLStateCollection* iCol, void*& oCmd)
  {
    iCol->StateCommandHandler<StateCommand>::SetCurrentCommand(iCol->GetDefaultCommand<StateCommand>());
    *((StateCommand*)oCmd) = iCol->GetDefaultCommand<StateCommand>();
    oCmd = ((unsigned char*)oCmd) + sizeof(StateCommand);
  }

  template <typename... Commands>
  template <class StateCommand>
  inline void OGLStateCollection<Commands...>::MakeCommand<StateCommand>::Do(OGLStateCollection* iCol, unsigned int iSetCmd, void*& oCmd)
  {
    if(iSetCmd & 1<<GetRank<StateCommand, Commands...>::result)
    {
      *((StateCommand*)oCmd) = iCol->GetCommand<StateCommand>();
      oCmd = ((unsigned char*)oCmd) + sizeof(StateCommand);
    }
  }
  
  template <typename... Commands>
  template <class StateCommand>
  inline StateCommand const& OGLStateCollection<Commands...>::GetDefaultCommand()
  {
    return StateCommandHandler<StateCommand>::GetDefaultCommand();
  }
  
  template <typename... Commands>
  template <class StateCommand>
  inline StateCommand const& OGLStateCollection<Commands...>::GetCommand()
  {
    return StateCommandHandler<StateCommand>::GetCurrentCommand();
  }


  template <typename... Commands>
  template <class StateCommand>
  inline void OGLStateCollection<Commands...>::SetDefaultCommand(StateCommand const& iCmd)
  {
    StateCommandHandler<StateCommand>::SetDefaultCommand_Init(iCmd);
  }

  template <typename... Commands>
  template <class StateCommand>
  inline void OGLStateCollection<Commands...>::SetCommand(StateCommand const& iCmd)
  {
    if(GetCommand<StateCommand>() != iCmd)
    {
      StateCommandHandler<StateCommand>::SetCurrentCommand(iCmd);
      if(GetDefaultCommand<StateCommand>() != iCmd)
        m_CurrentSetCommands |= 1 << GetRank<StateCommand, Commands...>::result;
      else
        m_CurrentSetCommands &= ~(1 << GetRank<StateCommand, Commands...>::result);

      m_CurStateUpToDate = false;
    }
  }

  template <typename... Commands>
  inline void OGLStateCollection<Commands...>::InitForPush()
  {
    for(unsigned int i = 0; i<m_StateGroups.size(); ++i)
    {
      //Faire alloc dummy par la suite.
      free(m_StateGroups[i]);
    }
    m_StateGroups.clear();
    void* defaultCommand = malloc(sizeof(unsigned int) + GetStateColSize<Commands...>::size);
    *(unsigned int*)defaultCommand = 0;
    void* commandIter = (unsigned char*)defaultCommand + sizeof(unsigned int);

    ForEach2<InitHandler, Commands...>::Do(this,commandIter);

    m_StateGroups.push_back(defaultCommand);
    m_CurrentSetCommands = 0;
    m_CurrentState = 0;
    m_CurStateUpToDate = true;
  }

  template <typename... Commands>
  inline void OGLStateCollection<Commands...>::InitForRender()
  {
    m_CurrentSetCommands = (1<<(GetListSize<Commands...>::result + 1)) - 1;
    m_CurrentState = 255;
    ApplyCommand(0);
  }

  struct ApplyCommandCtx
  {
    int curIdx = 0;
    void const*  defaultCommand;
    void const*  currentCommand;
    unsigned int prevCmdStates;
    unsigned int curCmdStates;
  };

  template <class StateCommand>
  struct ApplyCommandHandler
  {
    typedef ApplyCommandCtx& value_type1;
    inline static void Do(ApplyCommandCtx& iCtx)
    {
      if(iCtx.curCmdStates & 1<<iCtx.curIdx)
      {
        //Besoin de garder la commande courante si je veux pouvoir comparer.
        ((StateCommand*)iCtx.currentCommand)->Apply();
        iCtx.currentCommand = (StateCommand*)iCtx.currentCommand+1;
      }
      else
      {
        if(iCtx.prevCmdStates & 1<<iCtx.curIdx)
        {
          ((StateCommand*)iCtx.defaultCommand)->Apply();
        }
      }
      iCtx.defaultCommand = (StateCommand*)iCtx.defaultCommand+1;
      ++iCtx.curIdx;
    }
  };

  template <typename... Commands>
  inline void OGLStateCollection<Commands...>::ApplyCommand(unsigned char iCmdId)
  {
    if(iCmdId != m_CurrentState)
    {
      ApplyCommandCtx ctx;
      ctx.currentCommand = (unsigned int*)m_StateGroups[iCmdId] + 1;
      ctx.defaultCommand = (unsigned int*)m_StateGroups[0] + 1;
      ctx.prevCmdStates = m_CurrentSetCommands;
      ctx.curCmdStates = *(unsigned int*)m_StateGroups[iCmdId];

      ForEach1<ApplyCommandHandler, Commands...>::Do(ctx);

      m_CurrentState = iCmdId;
      m_CurrentSetCommands = ctx.curCmdStates;
    }
  }

  template <class StateCommand>
  struct CompareCommands
  {
    typedef void const*& value_type1;
    typedef void const*& value_type2;
    typedef bool& value_type3;
    inline static void Do(void const*& iCmd1, void const*& iCmd2, bool& oRes)
    {
      if(!oRes)
        return;

      if(*(StateCommand*)iCmd1 != *(StateCommand*)iCmd2)
        oRes = false;
      else
      {
        iCmd1 = (StateCommand*)iCmd1 + 1;
        iCmd2 = (StateCommand*)iCmd2 + 1;
      }
    }
  };

  template <typename... Commands>
  inline uint16_t OGLStateCollection<Commands...>::GetStateId()
  {
    if(m_CurStateUpToDate)
      return m_CurrentState;
    else
    {
      if(m_CurrentSetCommands == 0)
      {
        m_CurrentState = 0;
        m_CurStateUpToDate = true;
        return 0;
      }
      else
      {
        unsigned char tempCommand[sizeof(unsigned int) + GetStateColSize<Commands...>::size];
        size_t commandSize = 0;
        unsigned int counter = 0;
        ForEach3<ComputeStateColSize, Commands...>::Do(counter, m_CurrentSetCommands,commandSize);
        //void* newCommand = malloc(sizeof(unsigned int) + commandSize);


        *(unsigned int*)tempCommand = m_CurrentSetCommands;
        void* commandIter = tempCommand + sizeof(unsigned int);
        ForEach3<MakeCommand, Commands...>::Do(this,m_CurrentSetCommands,commandIter);

        for(unsigned int i = 0; i<m_StateGroups.size(); ++i)
        {
          if(*(unsigned int*)m_StateGroups[i] == m_CurrentSetCommands)
          {
            bool res = true;
            void const* iter1 = (unsigned int*)m_StateGroups[i] + 1;
            void const* iter2 = tempCommand + sizeof(unsigned int) + 1;
            ForEach3<CompareCommands, Commands...>::Do(iter1, iter2, res);
            if(res)
            {
              m_CurrentState = i;
              m_CurStateUpToDate = true;
              return i;
            }
          }
        }

        void* newCommand = eXl_ALLOC(sizeof(unsigned int) + commandSize);
        memcpy(newCommand,tempCommand,sizeof(unsigned int) + commandSize);
        m_StateGroups.push_back(newCommand);
        return m_StateGroups.size() - 1;
      }
    }
  }
}