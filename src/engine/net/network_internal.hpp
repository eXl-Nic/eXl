/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/net/network.hpp>
#include <core/utils/mphf.hpp>

namespace eXl
{
  namespace Network
  {
    struct CommandDictionary
    {
      void Build(NetDriver const&);
      void Build_Client(NetDriver const&);
      void ReceiveCommand(uint64_t iClient, uint32_t iId, ConstDynObject const& iArgs, DynObject& oRes) const;
      StringMPH m_CommandsHash;
      UnorderedMap<void*, uint32_t> m_CommandsTable;
      UnorderedMap<uint32_t, CommandDesc const*> m_Commands;
    };

    struct OutgoingCommand
    {
      uint32_t m_CommandId;
      DynObject m_Args;
      bool m_Reliable;
      std::function<void(ConstDynObject const&)> m_CompletionCallback;
    };

    struct SentCommand
    {
      uint32_t m_CommandId;
      std::function<void(ConstDynObject const&)> m_CompletionCallback;
    };

    using CommandQueue = Vector<OutgoingCommand>;
    using SentCommandsMap = UnorderedMap<uint64_t, SentCommand>;

    struct CommandHandler
    {
      CommandHandler(CommandDictionary const& iDictionary)
        : m_Dictionary(iDictionary)
      {

      }
      CommandDictionary const& m_Dictionary;
      CommandQueue m_Queue;
      SentCommandsMap m_CompletionMap;
      uint64_t m_CmdAlloc = 0;

      Err Enqueue(CommandCallData&& iCall);
      
      void ReceiveResponse(uint64_t iId, ConstDynObject const& iResponse);
      void Clear();

      template <typename Fn>
      void ProcessQueue(Fn const& iFn)
      {
        for (auto& item : m_Queue)
        {
          uint64_t queryId = 0;
          if (item.m_CompletionCallback)
          {
            queryId = ++m_CmdAlloc;
            SentCommand sentCmd = { item.m_CommandId, std::move(item.m_CompletionCallback) };
            m_CompletionMap.insert(std::make_pair(queryId, std::move(sentCmd)));
          }
          iFn(item.m_CommandId, std::move(item.m_Args), queryId, item.m_Reliable);
        }
        m_Queue.clear();
      }
    };
  }
}