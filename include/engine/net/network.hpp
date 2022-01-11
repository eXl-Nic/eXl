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
      inline bool operator == (ClientId const& iOther) const
      {
        return id == iOther.id;
      }
      inline bool operator != (ClientId const& iOther) const
      {
        return !(*this == iOther);
      }
    };

    struct ObjectId
    {
      uint64_t id;
      inline bool operator == (ObjectId const& iOther) const
      {
        return id == iOther.id;
      }
      inline bool operator != (ObjectId const& iOther) const
      {
        return !(*this == iOther);
      }
    };

    inline size_t hash_value(ClientId const& iId)
    {
      return static_cast<size_t>(iId.id);
    }

    inline size_t hash_value(ObjectId const& iId)
    {
      return static_cast<size_t>(iId.id);
    }

    struct ClientInputData
    {
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

    struct CommandDesc
    {
      FunDesc m_Desc;
      NetRole m_Executor;
      bool m_Reliable;
    };

    template<typename RetType, typename... Args>
    struct CallRetTypeDispatcher
    {
      template <typename Function>
      static void Execute(Function const& iFun, ConstDynObject& iArgsBuffer, DynObject& oOutput)
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
      static void Execute(Function const& iFun, ConstDynObject& iArgsBuffer, DynObject& oOutput)
      {
        Invoker<Args...>::template Call<void>(iFun, iArgsBuffer);
      }
    };

    class NetDriver
    {
    public:

    protected:

      template<typename T, typename RetType, typename... Args>
      void DeclareCommand(NetRole iExecutor, CommandName iName, RetType(T::* iMemFun)(Args...), bool iReliable)
      {
        CommandDesc desc;
        desc.m_Desc = FunDesc::Create<RetType(Args...)>();
        desc.m_Executor = iExecutor;
        desc.m_Reliable = iReliable;
        auto callback = [this, iMemFun](ConstDynObject& iArgsBuffer, DynObject& oOutput)
        {
          CallRetTypeDispatcher<RetType, Args...>::Execute([this, iMemFun](Args... iArgs)
            {
              return (static_cast<T*>(this)->*iMemFun)(std::forward<Args>(iArgs)...);
            }, iArgsBuffer, oOutput);
        };
        DeclareCommand(iName, callback, desc);
      }

      NetDriver(NetCtx& iCtx);
      using CommandCallback = std::function<void(ConstDynObject& iArgs, DynObject& oOutput) >;
      void DeclareCommand(CommandName iName, CommandCallback iCallback, CommandDesc iSettings );

      NetCtx& m_Ctx;
    };

    struct ClientEvents
    {
      virtual void OnNewObject(uint32_t, ObjectId, ClientData const&) = 0;
      virtual void OnObjectDeleted(uint32_t, ObjectId) = 0;
      virtual void OnObjectUpdated(uint32_t, ObjectId, ClientData const&) = 0;

      virtual void OnAssignPlayer(uint32_t, ObjectId) = 0;

      //virtual void OnServerCommand() = 0;
    };

    struct ServerEvents
    {
      virtual void OnClientConnected(ClientId) = 0;
      virtual void OnClientDisconnected(ClientId) = 0;

      virtual void OnClientCommand(ClientId, ClientInputData const&) = 0;
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
      ClientEvents* m_ClientEvents = nullptr;
      ServerEvents* m_ServerEvents = nullptr;

      std::unique_ptr<Server> m_Server;
      Vector<std::unique_ptr<Client>> m_Clients;
    };

    class EXL_ENGINE_API Client : public HeapObject
    {
    public:

      static boost::optional<uint32_t> Connect(NetCtx& iCtx, String const& iURL);
      static boost::optional<uint32_t> ConnectLoopback(NetCtx& iCtx);

      ClientState GetState();

      ~Client();
      void Tick();
      void Flush();

      void SetClientInput(ClientInputData const& iInput);

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

      void AssignPlayer(ClientId, ObjectId);

      Server_Impl& GetImpl() { return *m_Impl; }
      ServerDispatcher& GetDispatcher() { return m_Dispatcher; }

    protected:
      friend Server_Impl;
      Server(NetCtx&, std::unique_ptr<Server_Impl>);
      
      NetCtx& m_Ctx;
      std::unique_ptr<Server_Impl> m_Impl;
      ServerDispatcher m_Dispatcher;
    };
  }
}