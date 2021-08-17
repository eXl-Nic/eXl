  
namespace eXl
{
  template <class StateCommand>
  class StateCommandHandler
  {
  public:

    inline StateCommand const& GetCurrentCommand() const{return m_CurrentCmd;}
    inline StateCommand const& GetDefaultCommand() const{return m_DefaultCmd;}

    inline void SetCurrentCommand(StateCommand const& iCmd){m_CurrentCmd = iCmd;}
    inline void SetDefaultCommand_Init(StateCommand const& iCmd){m_DefaultCmd = iCmd;}

  protected:
    StateCommand m_DefaultCmd;
    StateCommand m_CurrentCmd;
  };

  template <typename... Commands>
  class StateCommandUnwrapper;

  template <typename Command, typename... Commands>
  class StateCommandUnwrapper<Command, Commands...> : public StateCommandHandler<Command>, public StateCommandUnwrapper<Commands...>
  {
  };
  
  template <typename Command>
  class StateCommandUnwrapper<Command> : public StateCommandHandler<Command>
  {
  };

  template <typename... Commands>
  class OGLStateCollection : public StateCommandUnwrapper<Commands...>
  {
  public:

    template <class StateCommand>
    inline void SetDefaultCommand(StateCommand const& iCmd);
    
    template <class StateCommand>
    inline void SetCommand(StateCommand const& iCmd);

    inline void InitForPush();

    inline void InitForRender();

    inline void ApplyCommand(unsigned char iCmdId);

    inline uint16_t GetStateId();

    inline uint16_t GetCurState() const {return m_CurrentState;}

  protected:

    template <class StateCommand>
    inline StateCommand const& GetDefaultCommand();
    
    template <class StateCommand>
    inline StateCommand const& GetCommand();

    template <class StateCommand>
    struct InitHandler
    {
      typedef OGLStateCollection* value_type1;
      typedef void*& value_type2;
      inline static void Do(OGLStateCollection* iCol, void*& oCmd);
    };

    template <class StateCommand>
    struct MakeCommand
    {
      typedef OGLStateCollection* value_type1;
      typedef unsigned int value_type2;
      typedef void*& value_type3;
      inline static void Do(OGLStateCollection* iCol, unsigned int iSetCmd, void*& oCmd);
    };

    unsigned int m_CurrentSetCommands;
    uint16_t m_CurrentState;
    bool m_CurStateUpToDate;

    std::vector<unsigned char> m_StateAlloc;
    
    //Hashtable ?
    std::vector<void*> m_StateGroups;
  };
}