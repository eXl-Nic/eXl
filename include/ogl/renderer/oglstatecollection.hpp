/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

namespace eXl
{
  template <typename... Commands>
  struct GetListSize;

  template <typename Command, typename... Commands>
  struct GetListSize<Command, Commands...>
  {
    static const uint32_t result = GetListSize<Commands...>::result + 1;
  };

  template <typename Command>
  struct GetListSize<Command>
  {
    static const uint32_t result = 1;
  };

  template <class StateCommand>
  class StateCommandHandler
  {
  public:
    uint16_t GetCommandId(StateCommand const& iCmd);
    StateCommand const& GetCommand(uint16_t iId) { return m_Commands[iId]; }
  protected:
    UnorderedMap<StateCommand, uint16_t> m_CommandsId;
    Vector<StateCommand> m_Commands;
  };

  template <template <typename> class Handler, typename... List>
  class ListWrapper;

  template <template <typename> class Handler, typename Item, typename... List>
  class ListWrapper<Handler, Item, List...> : public Handler<Item>, public ListWrapper<Handler, List...>
  {
  };
  
  template <template <typename> class Handler, typename Item>
  class ListWrapper<Handler, Item> : public Handler<Item>
  {
  };

  template <typename Command>
  struct CommandStorage
  {
    Command m_Command;
  };

  template <typename... Commands>
  class OGLStateCollection : public ListWrapper<StateCommandHandler, Commands...>
  {
  public:

    template <class StateCommand>
    inline void SetDefaultCommand(StateCommand const& iCmd);
    
    template <class StateCommand>
    inline void SetCommand(StateCommand const& iCmd);

    inline void InitForPush();

    inline void InitForRender();

    inline void ApplyCommand(uint16_t iCmdId);

    inline uint16_t GetStateId();

    inline uint16_t GetCurState() const {return m_CurrentState;}

    template <class StateCommand>
    inline StateCommand const& GetCurrentSetCommand();

  protected:

    template <class StateCommand>
    struct InitHandler
    {
      inline static void Do(OGLStateCollection* iCol, void*& oCmd);
    };

    template <class StateCommand>
    struct MakeCommand
    {
      inline static void Do(OGLStateCollection* iCol, uint32_t iSetCmd, void*& oCmd);
    };

    struct StateStorage : ListWrapper<CommandStorage, Commands...>
    {};

    uint16_t m_CurrentState;
    bool m_CurStateUpToDate = false;

    struct StateIds
    {
      bool operator ==(StateIds const& iOther) const
      {
        return memcmp(m_Commands, iOther.m_Commands, sizeof(m_Commands)) == 0;
      }

      uint16_t m_Commands[GetListSize<Commands...>::result];
    };

    StateIds m_NextCommand;

    struct StateIdHasher
    {
      std::size_t operator()(StateIds const& val) const
      {
        return boost::hash<decltype(val.m_Commands)>()(val.m_Commands);
      }
    };

    UnorderedMap<StateIds, uint16_t, StateIdHasher, std::equal_to<StateIds>> m_StateAssoc;
    Vector<StateStorage> m_States;
    Vector<StateIds> m_StateIds;
  };
}