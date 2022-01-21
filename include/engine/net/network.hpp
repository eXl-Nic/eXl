/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>
#include <core/type/fundesc.hpp>
#include <engine/enginelib.hpp>

#include <math/vector3.hpp>
#include <engine/common/world.hpp>

#define CUR_CLASS std::remove_pointer<decltype(this)>::type
#define DECLARE_CLIENT_RELIABLE_COMMAND(Function) DeclareClientCommand(#Function, Function, true)
#define DECLARE_CLIENT_UNRELIABLE_COMMAND(Function) DeclareClientCommand(#Function, Function, false)
#define DECLARE_SERVER_RELIABLE_COMMAND(Function) DeclareServerCommand(#Function, Function, true)
#define DECLARE_SERVER_UNRELIABLE_COMMAND(Function) DeclareServerCommand(#Function, Function, false)

namespace eXl
{
  namespace Network
  {
    enum class NetRole
    {
      None,
      Server,
      Client
    };

    struct ClientId
    {
      uint64_t id;
      bool operator == (ClientId const& iOther) const;
      bool operator != (ClientId const& iOther) const;
      Err Stream(Streamer& iStreamer) const;
      Err Unstream(Unstreamer& iStreamer);
    };

    struct ObjectId
    {
      uint64_t id;
      bool operator == (ObjectId const& iOther) const;
      bool operator != (ObjectId const& iOther) const;
      Err Stream(Streamer& iStreamer) const;
      Err Unstream(Unstreamer& iStreamer);
    };

    size_t hash_value(ClientId const& iId);
    size_t hash_value(ObjectId const& iId);

    struct EXL_ENGINE_API ClientInputData
    {
      EXL_REFLECT

      bool m_Moving = false;
      Vector3f m_Dir;
    };

    struct ClientData
    {
      bool m_Moving = false;
      Vector3f m_Dir;
      Vector3f m_Pos;
    };

    struct MovedObject
    {
      ObjectId object;
      ClientData data;
    };

    class Client_Impl;
    class Server_Impl;

    enum class ClientState
    {
      Connecting,
      Connected,
      Disconnected,
      Unknown
    };

    struct NetCtx;

    MAKE_NAME_DECL(CommandName);

    using CommandCallback = std::function<void(uint64_t, ConstDynObject const&, DynObject&) >;
    struct CommandDesc
    {
      CommandDesc() = default;
      CommandDesc(CommandDesc const&) = delete;
      CommandDesc(CommandDesc&&);
      CommandName m_Name;
      FunDesc m_FunDesc;
      NetRole m_Executor;
      bool m_Reliable;
      Optional<ArgsBuffer> m_Args;
      CommandCallback m_Callback;
    };

    template<typename RetType, typename... Args>
    struct CallRetTypeDispatcher;

    struct CommandCallData
    {
      NetRole m_Executor;
      void* m_CommandPtr;
      DynObject m_Args;
      std::function<void(ConstDynObject const&)> m_CompletionCallback;
    };

    struct CommandDictionary;

    class EXL_ENGINE_API NetDriver
    {
      friend CommandDictionary;
    public:

      template<typename RetType, typename... Args>
      struct CommandCaller
      {
      public:

        using CallbackArgsType = typename std::conditional<
          !std::is_same<RetType, void>::value, 
          typename std::add_lvalue_reference<typename std::add_const<RetType>::type>::type, 
          void>::type;

        [[nodiscard]] CommandCaller& WithCompletionCallback(std::function<void(CallbackArgsType)> iCompletionCallback);
        [[nodiscard]] CommandCaller& WithArgs(Args&&... iArgs);
        Err Send();
      protected:
        friend NetDriver;
        CommandCaller(NetDriver& iDriver, NetRole iExecutor, void* iCommandPtr, uint64_t iClientId);
        NetDriver& m_Driver;
        CommandDesc const* m_Command = nullptr;
        uint64_t m_ClientId;
        CommandCallData m_Data;
      };

      template<typename RetType, typename... Args>
      [[nodiscard]] CommandCaller<RetType, Args...> CallClientCommand(ClientId iClient, std::function<RetType(uint32_t, Args...)>& iFun)
      {
        return CommandCaller<RetType, Args...>(*this, NetRole::Client, &iFun, iClient.id);
      }

      template<typename RetType, typename... Args>
      [[nodiscard]] CommandCaller<RetType, Args...> CallServerCommand(uint32_t iLocalClient, std::function<RetType(ClientId, Args...)>& iFun)
      {
        return CommandCaller<RetType, Args...>(*this, NetRole::Server, &iFun, iLocalClient);
      }

    protected:

      template<typename RetType, typename... Args>
      void DeclareClientCommand(CommandName iName, std::function<RetType(uint32_t, Args...)>& iFun, bool iReliable);

      template<typename RetType, typename... Args>
      void DeclareServerCommand(CommandName iName, std::function<RetType(ClientId, Args...)>& iFun, bool iReliable);

      NetDriver(NetCtx& iCtx);
      NetDriver(NetDriver const&) = delete;

    private:
      void DeclareCommand(NetRole iExecutor, CommandName iName, CommandCallback iCb, FunDesc iArgs, void* iCommandPtr, bool iReliable);
      Vector<CommandDesc> m_Commands;
      UnorderedMap<CommandName, uint32_t> m_CommandsByName;
      UnorderedMap<void*, uint32_t> m_CommandsByPtr;
      NetCtx& m_Ctx;
    };

    struct ClientEvents
    {
      std::function<void(uint32_t, ObjectId, ClientData const&)> OnNewObject;
      std::function<void(uint32_t, ObjectId)> OnObjectDeleted;
      std::function<void(uint32_t, ObjectId, ClientData const&)> OnObjectUpdated;
    };

    struct ServerEvents
    {
      std::function<void(ClientId)> OnClientConnected;
      std::function<void(ClientId)> OnClientDisconnected;
    };

    class Server;
    class Client;

    struct NetCtx
    {
      NetCtx(uint16_t iServerPort)
        : m_ServerPort(iServerPort)
      {}

      uint16_t const m_ServerPort;

      NetDriver* m_NetDriver = nullptr;
      ClientEvents m_ClientEvents;
      ServerEvents m_ServerEvents;

      std::unique_ptr<Server> m_Server;
      Vector<std::unique_ptr<Client>> m_Clients;
    };

    class EXL_ENGINE_API Client : public HeapObject
    {
    public:

      static Optional<uint32_t> Connect(NetCtx& iCtx, String const& iURL);
      static Optional<uint32_t> ConnectLoopback(NetCtx& iCtx);

      ClientState GetState();

      ~Client();
      void Tick();
      void Flush();

      Err SendServerCommand(CommandCallData&& iCall);

      uint32_t GetLocalIndex() const;
      Client_Impl& GetImpl() { return *m_Impl; }

    protected:
      friend Client_Impl;
      Client(NetCtx&, std::unique_ptr<Client_Impl>);

      NetCtx& m_Ctx;
      std::unique_ptr<Client_Impl> m_Impl;
    };

    using ClientObjectMap = UnorderedMap<Network::ClientId, Network::ObjectId>;

    class EXL_ENGINE_API ServerDispatcher
    {
    public:

      void CreateObject(ObjectId, ClientData const& iData);
      void UpdateObject(ObjectId, ClientData const& iData);
      void DeleteObject(ObjectId);

      void AddClient(ClientId, ObjectId);

      void Flush(Server& iServer);

      UnorderedMap<ObjectId, ClientData> const& GetObjects() { return m_Objects; }
      ClientObjectMap const& GetClients() { return m_ConnectedClients; }

    protected:
      UnorderedMap<ObjectId, ClientData> m_PendingUpdates;
      UnorderedMap<ObjectId, ClientData> m_Objects;
      UnorderedSet<ObjectId> m_DeletedObjects;
      UnorderedMap<ClientId, ObjectId> m_ConnectedClients;
      UnorderedMap<ClientId, ObjectId> m_NewClients;
    };

    class EXL_ENGINE_API Server : public HeapObject
    {
    public:
      static Server* Start(NetCtx& iCtx, String const& iURL);

      ~Server();
      void Tick();
      void Flush();

      void CreateObject(ClientId, ObjectId, ClientData const& iData);
      void UpdateObject(ClientId, ObjectId, ClientData const& iData);

      Err SendClientCommand(CommandCallData&& iCall, ClientId iId);

      Server_Impl& GetImpl() { return *m_Impl; }
      ServerDispatcher& GetDispatcher() { return m_Dispatcher; }

    protected:
      friend Server_Impl;
      Server(NetCtx&, std::unique_ptr<Server_Impl>);

      NetCtx& m_Ctx;
      std::unique_ptr<Server_Impl> m_Impl;
      ServerDispatcher m_Dispatcher;
    };


#include "network.inl"
  }
  DEFINE_ENGINE_TYPE_EX(Network::ClientId, NetClientId);
  DEFINE_ENGINE_TYPE_EX(Network::ObjectId, NetObjectId);
}