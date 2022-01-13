#include <engine/net/network.hpp>
#include <core/log.hpp>
#include <core/clock.hpp>

namespace eXl
{
  namespace Network
  {
    class TestNetDriver : public NetDriver
    {
    public:

      static void TestCommand(NetDriver* Self, uint32_t iParam1, float iParam2, String const& iParam3)
      {

      }

      void TestCommand2()
      {}

      TestNetDriver(NetCtx& iCtx)
        : NetDriver(iCtx)
      {
        DECLARE_CLIENT_RELIABLE_COMMAND(TestCommand);

        CallClientCommand(ClientId(), &TestCommand).WithArgs(9, 0.4, "Blah").Send();
      }
    };

    void NetDriver::DeclareCommand(CommandName iName, void* iCommandPtr, CommandCallback iCallback, CommandDesc iDesc)
    {
      CommandEntry entry{ iDesc, std::move(iCallback) };
      uint32_t commandNum = m_Commands.size();
      m_Commands.push_back(std::move(entry));
      m_CommandsByName.insert(std::make_pair(iName, commandNum));
      m_CommandsByPtr.insert(std::make_pair(iCommandPtr, commandNum));
    }
  }
}