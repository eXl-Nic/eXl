#include "network_internal.hpp"
#include <core/log.hpp>
#include <core/clock.hpp>
#include <core/type/coretype.hpp>

namespace eXl
{
  IMPLEMENT_TYPE_EX(Network::ClientId, NetClientId);
  IMPLEMENT_TYPE_EX(Network::ObjectId, NetObjectId);
  namespace Network
  {

    Type const* ClientInputData::GetType()
    {
      static Type const* s_Type = []
      {
        return TypeManager::BeginNativeTypeRegistration<ClientInputData>("ClientInputData")
          .AddField("Moving", &ClientInputData::m_Moving)
          .AddField("Dir", &ClientInputData::m_Dir)
          .EndRegistration();
      }();
      return s_Type;
    }

    Err ClientInputData::Stream(Streamer& iStreamer) const
    {
      return GetType()->Stream(this, &iStreamer);
    }

    Err ClientInputData::Unstream(Unstreamer& iStreamer)
    {
      void* This = this;
      return GetType()->Unstream(This, &iStreamer);
    }

    NetDriver::NetDriver(NetCtx& iCtx)
      : m_Ctx(iCtx)
    {
      eXl_ASSERT(m_Ctx.m_NetDriver == nullptr);
      m_Ctx.m_NetDriver = this;
    }

    void NetDriver::DeclareCommand(NetRole iExecutor, CommandName iName, CommandCallback iCb, FunDesc iArgs, void* iCommandPtr, bool iReliable)
    {
      CommandDesc desc;
      desc.m_FunDesc = std::move(iArgs);
      desc.m_Args.emplace(desc.m_FunDesc.arguments);
      desc.m_Executor = iExecutor;
      desc.m_Reliable = iReliable;
      desc.m_Callback = std::move(iCb);

      uint32_t commandNum = m_Commands.size();
      m_Commands.push_back(std::move(desc));
      m_CommandsByName.insert(std::make_pair(iName, commandNum));
      m_CommandsByPtr.insert(std::make_pair(iCommandPtr, commandNum));
    }

    namespace
    {
      void GatherFieldNames(Type const* iType, UnorderedSet<String>& oMap)
      {
        if (TupleType const* tuple = iType->IsTuple())
        {
          for (uint32_t i = 0; i < tuple->GetNumField(); ++i)
          {
            TypeFieldName fieldName;
            Type const* fieldType = tuple->GetFieldDetails(i, fieldName);
            oMap.insert(fieldName);
            GatherFieldNames(fieldType, oMap);
          }
        }
        else if (ArrayType const* arrayType = ArrayType::DynamicCast(iType))
        {
          GatherFieldNames(arrayType->GetElementType(), oMap);
        }
      }
    }

    void CommandDictionary::Build(NetDriver const& iDriver)
    {
      UnorderedSet<String> names;
      for (auto const& cmdName : iDriver.m_CommandsByName)
      {
        names.insert(cmdName.first.c_str());
      }
      for (auto const& cmd : iDriver.m_Commands)
      {
        GatherFieldNames(&(*cmd.m_Args), names);
      }

      m_CommandsHash.Build(names.begin(), names.end());

      Vector<void*> functions;
      functions.resize(iDriver.m_Commands.size());
      for (auto const& cmdPtr : iDriver.m_CommandsByPtr)
      {
        functions[cmdPtr.second] = cmdPtr.first;
      }

      for (auto const& cmdName : iDriver.m_CommandsByName)
      {
        uint32_t targetId = m_CommandsHash.Compute(cmdName.first.get());
        m_Commands.insert(std::make_pair(targetId, &iDriver.m_Commands[cmdName.second]));
        m_CommandsTable.insert(std::make_pair(functions[cmdName.second], targetId));
      }
    }

    void CommandDictionary::Build_Client(NetDriver const& iDriver)
    {
      Vector<void*> functions;
      functions.resize(iDriver.m_Commands.size());
      for (auto const& cmdPtr : iDriver.m_CommandsByPtr)
      {
        functions[cmdPtr.second] = cmdPtr.first;
      }

      for (auto const& cmdName : iDriver.m_CommandsByName)
      {
        uint32_t targetId = m_CommandsHash.Compute(cmdName.first.get());
        m_Commands.insert(std::make_pair(targetId, &iDriver.m_Commands[cmdName.second]));
        m_CommandsTable.insert(std::make_pair(functions[cmdName.second], targetId));
      }
    }

    void CommandDictionary::ReceiveCommand(uint64_t iClient, uint32_t iId, ConstDynObject const& iArgs, DynObject& oRes) const
    {
      auto iter = m_Commands.find(iId);
      eXl_ASSERT_REPAIR_RET(iter != m_Commands.end(), void());
      iter->second->m_Callback(iClient, iArgs, oRes);
    }

    Err CommandHandler::Enqueue(CommandCallData&& iCall)
    {
      OutgoingCommand newCommand;
      newCommand.m_Args = std::move(iCall.m_Args);
      auto iter = m_Dictionary.m_CommandsTable.find(iCall.m_CommandPtr);
      eXl_ASSERT_REPAIR_RET(iter != m_Dictionary.m_CommandsTable.end(), Err::Failure);
      CommandDesc const* cmd = m_Dictionary.m_Commands.find(iter->second)->second;
      newCommand.m_CommandId = iter->second;
      newCommand.m_Reliable = cmd->m_Reliable;
      newCommand.m_CompletionCallback = std::move(iCall.m_CompletionCallback);
      m_Queue.push_back(std::move(newCommand));

      return Err::Success;
    }

    void CommandHandler::ReceiveResponse(uint64_t iId, ConstDynObject const& iResponse)
    {
      auto iter = m_CompletionMap.find(iId);
      eXl_ASSERT_REPAIR_RET(iter != m_CompletionMap.end(), void());
      iter->second.m_CompletionCallback(iResponse);

      m_CompletionMap.erase(iter);
    }

    void CommandHandler::Clear()
    {
      m_CmdAlloc = 0;
      m_Queue.clear();
      m_CompletionMap.clear();
    }
  }
}