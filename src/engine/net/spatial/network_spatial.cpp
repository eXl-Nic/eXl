#include <engine/net/network.hpp>
#include <core/log.hpp>
#include <core/idgenerator.hpp>

#include <engine/common/transforms.hpp>

#include "spatial/spatialos_world.hpp"

#ifdef ENABLE_SPATIALOS_DRIVER

#include <improbable/c_worker.h>
#include "spatial/schema_utils.hpp"


#endif 

#include <boost\uuid\random_generator.hpp>

namespace eXl
{
  namespace Network
  {
    namespace
    {
      constexpr uint32_t s_LoginComponentId = 1000;
      constexpr uint32_t s_ClientDataComponentId = 1001;
      constexpr uint32_t s_ClientInputDataComponentId = 1002;
      constexpr uint32_t s_ClientPresenceComponentId = 1003;

#ifdef ENABLE_SPATIALOS_DRIVER

      struct Impl
      {
        Impl()
          : m_IdGen(UINT32_MAX)
        {
          memset(&m_DefaultVTable, 0, sizeof(m_DefaultVTable));

          boost::uuids::uuid workerId = boost::uuids::random_generator()();
          m_UniqueId.append(StringUtil::FromInt(((uint32_t*)workerId.data)[0]));
          m_UniqueId.append(StringUtil::FromInt(((uint32_t*)workerId.data)[1]));

          m_SpatialWorld.m_ExternalOpHandler = [this](Worker_Op const& iOp) {return ProcessNonWorldOps(iOp); };
        }

        Worker_ComponentVtable m_DefaultVTable;
        String m_UniqueId;
        String m_WorkerId;

        Worker_Connection* m_Connection = nullptr;
        Network::NetRole m_Role;

        static Impl& Get()
        {
          static Impl s_Impl;
          return s_Impl;
        }

        struct ClientInfo
        {
          ObjectHandle m_Character;
          Worker_RequestId m_CreationRequest;
          Worker_RequestId m_ReadyRequest;
          bool m_Connected = false;
          bool m_Ready = false;
          bool m_Spawned = false;
        };

        void CreateClientInterestEntity(String const& iClient);
        void CreateServerInterestEntity();

        void OnCreateEntityResponse(const Worker_CreateEntityResponseOp& op);
        void OnCommandRequest(const Worker_CommandRequestOp& op);
        void OnCommandResponse(const Worker_CommandResponseOp& op);

        void SpawnClient(String const& iClient, ClientInfo& ioInfo);

        Err Connect(NetRole iRole, String const& iURL, uint16_t iPort);

        void Tick(float iDelta);

        void OnLogMessage(Worker_LogMessageOp const& iLog);

        Err ProcessNonWorldOps(Worker_Op const& iOp);

        IdGenerator m_IdGen;

        UnorderedMap<Worker_EntityId, ObjectHandle> m_EntityToObjectMap;
        UnorderedMap<ObjectHandle, Worker_EntityId> m_ObjectToEntityMap;

        SpatialWorld m_SpatialWorld;
        Vector<Network::MovedObject> m_ObjUpdates;

        Worker_EntityId m_PlayerEntity = 0;

        UnorderedMap<String, ClientInfo> m_ClientInfo;
      };

#endif
    }
  }

  std::function<ObjectHandle(Network::ClientData const&)> Network::OnNewObjectReceived;
  std::function<ObjectHandle(void)> Network::OnNewPlayer;
  std::function<void(ObjectHandle)> Network::OnPlayerAssigned;
  std::function<void(ObjectHandle, Network::ClientInputData const&)> Network::OnClientCommand;

#ifdef ENABLE_SPATIALOS_DRIVER

  void Network::Impl::OnLogMessage(Worker_LogMessageOp const& iLog)
  {
    switch (iLog.level)
    {
    case WORKER_LOG_LEVEL_WARN:
      LOG_WARNING << "Network : " << iLog.message;
      break;
    case WORKER_LOG_LEVEL_ERROR:
      LOG_ERROR << "Network : " << iLog.message;
      break;
    default:
      LOG_INFO << "Network : " << iLog.message;
      break;
    }
  }

  void ReadClientData(Network::ClientData& oData, Schema_ComponentData* iSchema)
  {
    Schema_Object* object = Schema_GetComponentDataFields(iSchema);
    oData.m_Moving = Schema_GetBool(object, 1);
    oData.m_Dir = SpatialGDK_C::GetVectorFromSchema(object, 2);
    oData.m_Pos = SpatialGDK_C::GetVectorFromSchema(object, 3);
  }

  void WriteClientData(Network::ClientData const& iData, Schema_Object* object)
  {
    Schema_AddBool(object, 1, iData.m_Moving);
    SpatialGDK_C::AddVectorToSchema(object, 2, iData.m_Dir);
    SpatialGDK_C::AddVectorToSchema(object, 3, iData.m_Pos);
  }

  void ReadClientInputData(Network::ClientInputData& oData, Schema_ComponentData* iSchema)
  {
    Schema_Object* object = Schema_GetComponentDataFields(iSchema);
    oData.m_Moving = Schema_GetBool(object, 1);
    oData.m_Dir = SpatialGDK_C::GetVectorFromSchema(object, 2);
  }

  void WriteClientInputData(Network::ClientInputData const& iData, Schema_Object* object)
  {
    Schema_AddBool(object, 1, iData.m_Moving);
    SpatialGDK_C::AddVectorToSchema(object, 2, iData.m_Dir);
  }

  void Network::Impl::SpawnClient(String const& iClient, ClientInfo& ioInfo)
  {
    String workerIdReq("workerId:");
    workerIdReq.append(m_WorkerId);

    String clientIdReq("workerId:");
    clientIdReq.append(iClient);

    SpatialGDK_C::EntityACL ACLComponent;

    SpatialGDK_C::WorkerRequirementSet authServer({ {{workerIdReq}} });
    SpatialGDK_C::WorkerRequirementSet authClient({ {{clientIdReq}} });

    ACLComponent.m_ReadACL.push_back({ "server" });
    ACLComponent.m_ReadACL.push_back({ "client" });
    ACLComponent.m_WriteACL.insert(std::make_pair(SpatialGDK_C::s_PositionComponentId, authServer));
    ACLComponent.m_WriteACL.insert(std::make_pair(SpatialGDK_C::s_EntityACLComponentId, authServer));
    ACLComponent.m_WriteACL.insert(std::make_pair(SpatialGDK_C::s_InterestComponentId, authServer));
    ACLComponent.m_WriteACL.insert(std::make_pair(s_ClientDataComponentId, authServer));
    ACLComponent.m_WriteACL.insert(std::make_pair(s_ClientInputDataComponentId, authClient));

    SpatialGDK_C::Interest InterestComponent;

    Vector<SpatialGDK_C::QueryConstraint> charConstraints;
    charConstraints.push_back(SpatialGDK_C::QueryConstraint::CreateComponent(s_ClientDataComponentId));
    charConstraints.push_back(SpatialGDK_C::QueryConstraint::CreateRelativeCylinder(100.0f));

    SpatialGDK_C::Query otherCharQuery;
    otherCharQuery.m_FullSnapshotResult = false;
    otherCharQuery.m_ResultComponentId = {s_ClientDataComponentId};
    otherCharQuery.Constraint = SpatialGDK_C::QueryConstraint::CreateAnd(std::move(charConstraints));

    InterestComponent.ComponentInterestMap.insert(
      std::make_pair(s_ClientInputDataComponentId, SpatialGDK_C::ComponentInterest({ Vector<SpatialGDK_C::Query>({ otherCharQuery }) })));

    Vector<Worker_ComponentData> Components;
    {
      Worker_ComponentData entityACLData = {};
      entityACLData.component_id = SpatialGDK_C::s_EntityACLComponentId;
      entityACLData.schema_type = Schema_CreateComponentData();
      ACLComponent.WriteData(*Schema_GetComponentDataFields(entityACLData.schema_type));
      Components.push_back(entityACLData);
    }

    Components.push_back(InterestComponent.CreateInterestData());

    {
      Worker_ComponentData positionData = {};
      positionData.component_id = SpatialGDK_C::s_PositionComponentId;
      positionData.schema_type = Schema_CreateComponentData();
      SpatialGDK_C::AddVectorToSchema(Schema_GetComponentDataFields(positionData.schema_type), 1, Vector3f::ZERO);
      Components.push_back(positionData);
    }

    {
      Worker_ComponentData metaData = {};
      metaData.component_id = SpatialGDK_C::s_MetaDataComponentId;
      metaData.schema_type = Schema_CreateComponentData();
      SpatialGDK_C::AddStringToSchema(Schema_GetComponentDataFields(metaData.schema_type), 1, "Character");
      Components.push_back(metaData);
    }
    {
      Worker_ComponentData clientData = {};
      clientData.component_id = s_ClientDataComponentId;
      clientData.schema_type = Schema_CreateComponentData();
      WriteClientData(ClientData(), Schema_GetComponentDataFields(clientData.schema_type));

      Components.push_back(clientData);
    }
    {
      Worker_ComponentData clientInputData = {};
      clientInputData.component_id = s_ClientInputDataComponentId;
      clientInputData.schema_type = Schema_CreateComponentData();
      WriteClientInputData(ClientInputData(), Schema_GetComponentDataFields(clientInputData.schema_type));

      Components.push_back(clientInputData);
    }
    
    ioInfo.m_CreationRequest = Worker_Connection_SendCreateEntityRequest(m_Connection, Components.size(), Components.data(), nullptr, nullptr);
  }

  void Network::Impl::OnCreateEntityResponse(const Worker_CreateEntityResponseOp& op)
  {
    if (op.status_code == WORKER_STATUS_CODE_SUCCESS)
    {
      for (auto& clientInfoEntry : m_ClientInfo)
      {
        ClientInfo& clientInfo = clientInfoEntry.second;
        if (clientInfo.m_CreationRequest == op.request_id)
        {
          clientInfo.m_Character = OnNewPlayer();
          m_EntityToObjectMap.insert(std::make_pair(op.entity_id, clientInfo.m_Character));
          m_ObjectToEntityMap.insert(std::make_pair(clientInfo.m_Character, op.entity_id));

          Worker_CommandResponse response;
          response.command_index = 1;
          response.component_id = s_LoginComponentId;
          response.schema_type = Schema_CreateCommandResponse();
          Schema_AddEntityId(Schema_GetCommandResponseObject(response.schema_type), 1, op.entity_id);

          Worker_Connection_SendCommandResponse(m_Connection, clientInfo.m_ReadyRequest, &response);

          clientInfo.m_ReadyRequest = 0;
          clientInfo.m_CreationRequest = 0;
          break;
        }
      }
    }
  }

  void Network::Impl::OnCommandRequest(const Worker_CommandRequestOp& op)
  {
    if(m_Role == NetRole::Server)
    {
      if (op.request.component_id == s_LoginComponentId)
      {
        if (op.request.command_index == 1)
        {
          String worker = op.caller_worker_id;
          auto clientIter = m_ClientInfo.insert(std::make_pair(worker, ClientInfo())).first;
          if (clientIter->second.m_Spawned)
          {
            return;
          }
          clientIter->second.m_Ready = true;
          if (clientIter->second.m_Ready && clientIter->second.m_Connected)
          {
            clientIter->second.m_ReadyRequest = op.request_id;
            SpawnClient(worker, clientIter->second);
          }
        }
      }
    }
  }

  void Network::Impl::OnCommandResponse(const Worker_CommandResponseOp& op)
  {
    if (m_Role == NetRole::Client)
    {
      if (op.status_code == WORKER_STATUS_CODE_SUCCESS)
      {
        Schema_Object* object = Schema_GetCommandResponseObject(op.response.schema_type);
        m_PlayerEntity = Schema_GetEntityId(object, 1);
        auto iter = m_EntityToObjectMap.find(m_PlayerEntity);
        if (iter != m_EntityToObjectMap.end())
        {
          OnPlayerAssigned(iter->second);
        }
      }
    }
  }

  Err Network::Impl::ProcessNonWorldOps(Worker_Op const& iOp)
  {
    switch (iOp.op_type)
    {
    case WORKER_OP_TYPE_DISCONNECT:
      Worker_Connection_Destroy(m_Connection);
      m_Connection = nullptr;
      return Err::Failure;
      break;
    case WORKER_OP_TYPE_FLAG_UPDATE:
      break;
    case WORKER_OP_TYPE_LOG_MESSAGE:
      OnLogMessage(iOp.op.log_message);
      break;
    case WORKER_OP_TYPE_METRICS:
      break;
    case WORKER_OP_TYPE_RESERVE_ENTITY_IDS_RESPONSE:
      break;
    case WORKER_OP_TYPE_CREATE_ENTITY_RESPONSE:
      OnCreateEntityResponse(iOp.op.create_entity_response);
      break;
    case WORKER_OP_TYPE_DELETE_ENTITY_RESPONSE:
      break;
    case WORKER_OP_TYPE_ENTITY_QUERY_RESPONSE:
      break;
    case WORKER_OP_TYPE_COMMAND_REQUEST:
      OnCommandRequest(iOp.op.command_request);
      break;
    case WORKER_OP_TYPE_COMMAND_RESPONSE:
      OnCommandResponse(iOp.op.command_response);
      break;
    }
    return Err::Success;
  }

  void Network::Impl::CreateClientInterestEntity(String const& iClient)
  {
    String workerIdReq("workerId:");
    workerIdReq.append(m_WorkerId);

    String clientIdReq("workerId:");
    clientIdReq.append(iClient);

    SpatialGDK_C::EntityACL ACLComponent;

    SpatialGDK_C::WorkerRequirementSet authServer({ {{workerIdReq}} });
    SpatialGDK_C::WorkerRequirementSet authClient({ {{clientIdReq}} });

    ACLComponent.m_ReadACL.push_back({ "server" });
    ACLComponent.m_ReadACL.push_back({ "client" });
    ACLComponent.m_WriteACL.insert(std::make_pair(SpatialGDK_C::s_PositionComponentId, authServer));
    ACLComponent.m_WriteACL.insert(std::make_pair(SpatialGDK_C::s_EntityACLComponentId, authServer));
    ACLComponent.m_WriteACL.insert(std::make_pair(SpatialGDK_C::s_InterestComponentId, authServer));
    ACLComponent.m_WriteACL.insert(std::make_pair(s_ClientPresenceComponentId, authClient));

    SpatialGDK_C::Interest InterestComponent;

    SpatialGDK_C::Query loginQuery;
    loginQuery.m_FullSnapshotResult = true;
    loginQuery.Constraint = SpatialGDK_C::QueryConstraint::CreateComponent(s_LoginComponentId);

    SpatialGDK_C::Query selfQuery;
    selfQuery.m_FullSnapshotResult = true;
    selfQuery.Constraint = SpatialGDK_C::QueryConstraint::CreateComponent(s_ClientPresenceComponentId);

    InterestComponent.ComponentInterestMap.insert(
      std::make_pair(s_ClientPresenceComponentId, SpatialGDK_C::ComponentInterest({ Vector<SpatialGDK_C::Query>({ loginQuery }) })));

    Vector<Worker_ComponentData> Components;
    {
      Worker_ComponentData entityACLData = {};
      entityACLData.component_id = SpatialGDK_C::s_EntityACLComponentId;
      entityACLData.schema_type = Schema_CreateComponentData();
      ACLComponent.WriteData(*Schema_GetComponentDataFields(entityACLData.schema_type));
      Components.push_back(entityACLData);
    }

    Components.push_back(InterestComponent.CreateInterestData());

    {
      Worker_ComponentData positionData = {};
      positionData.component_id = SpatialGDK_C::s_PositionComponentId;
      positionData.schema_type = Schema_CreateComponentData();
      SpatialGDK_C::AddVectorToSchema(Schema_GetComponentDataFields(positionData.schema_type), 1, Vector3f::ZERO);
      Components.push_back(positionData);
    }

    {
      Worker_ComponentData metaData = {};
      metaData.component_id = SpatialGDK_C::s_MetaDataComponentId;
      metaData.schema_type = Schema_CreateComponentData();
      SpatialGDK_C::AddStringToSchema(Schema_GetComponentDataFields(metaData.schema_type), 1, m_WorkerId + "_ClientPresence");
      Components.push_back(metaData);
    }
    {
      Worker_ComponentData clientPresenceData = {};
      clientPresenceData.component_id = s_ClientPresenceComponentId;
      clientPresenceData.schema_type = Schema_CreateComponentData();
      Components.push_back(clientPresenceData);
    }

    Worker_RequestId requestId = Worker_Connection_SendCreateEntityRequest(m_Connection, Components.size(), Components.data(), nullptr, nullptr);
  }

  void Network::Impl::CreateServerInterestEntity()
  {
    String workerIdReq("workerId:");
    workerIdReq.append(m_WorkerId);

    SpatialGDK_C::EntityACL ACLComponent;

    SpatialGDK_C::WorkerRequirementSet authServer({ {{workerIdReq}} });

    ACLComponent.m_ReadACL.push_back({ "server" });
    ACLComponent.m_ReadACL.push_back({ "client" });
    ACLComponent.m_WriteACL.insert(std::make_pair(SpatialGDK_C::s_PositionComponentId, authServer));
    ACLComponent.m_WriteACL.insert(std::make_pair(SpatialGDK_C::s_EntityACLComponentId, authServer));
    ACLComponent.m_WriteACL.insert(std::make_pair(SpatialGDK_C::s_InterestComponentId, authServer));
    ACLComponent.m_WriteACL.insert(std::make_pair(s_LoginComponentId, authServer));

    SpatialGDK_C::Interest InterestComponent;
    SpatialGDK_C::ComponentInterest temp;

    SpatialGDK_C::Query workersQuery;
    workersQuery.m_FullSnapshotResult = true;
    workersQuery.Constraint = SpatialGDK_C::QueryConstraint::CreateComponent(SpatialGDK_C::s_WorkerComponentId);

    temp.Queries.push_back(workersQuery);

    SpatialGDK_C::Query clientInputQuery;
    workersQuery.m_FullSnapshotResult = false;
    workersQuery.m_ResultComponentId = { s_ClientInputDataComponentId };
    workersQuery.Constraint = SpatialGDK_C::QueryConstraint::CreateComponent(s_ClientInputDataComponentId);

    temp.Queries.push_back(workersQuery);

    InterestComponent.ComponentInterestMap.insert(
      std::make_pair(SpatialGDK_C::s_PositionComponentId, std::move(temp)));

    Vector<Worker_ComponentData> Components;
    {
      Worker_ComponentData entityACLData = {};
      entityACLData.component_id = SpatialGDK_C::s_EntityACLComponentId;
      entityACLData.schema_type = Schema_CreateComponentData();
      ACLComponent.WriteData(*Schema_GetComponentDataFields(entityACLData.schema_type));
      Components.push_back(entityACLData);
    }
    
    Components.push_back(InterestComponent.CreateInterestData());

    {
      Worker_ComponentData positionData = {};
      positionData.component_id = SpatialGDK_C::s_PositionComponentId;
      positionData.schema_type = Schema_CreateComponentData();
      SpatialGDK_C::AddVectorToSchema(Schema_GetComponentDataFields(positionData.schema_type), 1, Vector3f::ZERO);
      Components.push_back(positionData);
    }

    {
      Worker_ComponentData metaData = {};
      metaData.component_id = SpatialGDK_C::s_MetaDataComponentId;
      metaData.schema_type = Schema_CreateComponentData();
      SpatialGDK_C::AddStringToSchema(Schema_GetComponentDataFields(metaData.schema_type), 1, m_WorkerId + "_ServerInterest");
      Components.push_back(metaData);
    }

    {
      Worker_ComponentData loginData = {};
      loginData.component_id = s_LoginComponentId;
      loginData.schema_type = Schema_CreateComponentData();
      Components.push_back(loginData);
    }

    Worker_RequestId requestId = Worker_Connection_SendCreateEntityRequest(m_Connection, Components.size(), Components.data(), nullptr, nullptr);
  }

  void Network::Impl::Tick(float iDelta)
  {
    m_ObjUpdates.clear();
    if (m_Connection)
    {
      Vector<EntityUpdate> worldUpdates;
      if (m_SpatialWorld.UpdateWorld(m_Connection, worldUpdates))
      {
        for (auto& update : worldUpdates)
        {
          for (auto& compChange : update.m_CompChange)
          {
            ComponentOp& op = compChange.second;

            if (op.m_Comp == s_LoginComponentId)
            {
              if (m_Role == NetRole::Client)
              {
                Worker_CommandRequest request = {};
                request.component_id = op.m_Comp;
                request.command_index = 1;
                request.schema_type = Schema_CreateCommandRequest();
                Worker_Connection_SendCommandRequest(m_Connection, update.m_EntityId, &request, nullptr, nullptr);
              }
            }
            if (op.m_Comp == SpatialGDK_C::s_WorkerComponentId)
            {
              if (op.m_Op == ComponentOp::Op::Add)
              {
                String workerType = SpatialGDK_C::GetStringFromSchema(Schema_GetComponentDataFields(op.m_Data), 2);
                if (workerType == "client")
                {
                  String workerId = SpatialGDK_C::GetStringFromSchema(Schema_GetComponentDataFields(op.m_Data), 1);

                  auto iterClient = m_ClientInfo.insert(std::make_pair(workerId, ClientInfo())).first;
                  if (!iterClient->second.m_Connected)
                  {
                    iterClient->second.m_Connected = true;
                    CreateClientInterestEntity(workerId);
                  }
                }
              }
            }
            if (op.m_Comp == s_ClientDataComponentId)
            {
              if (m_Role == NetRole::Client)
              {
                if (op.m_Op == ComponentOp::Op::Add)
                {
                  ClientData data;
                  ReadClientData(data, op.m_Data);
                  ObjectHandle newObject = OnNewObjectReceived(data);

                  m_EntityToObjectMap.insert(std::make_pair(update.m_EntityId, newObject));
                  m_ObjectToEntityMap.insert(std::make_pair(newObject, update.m_EntityId));

                  if (m_PlayerEntity == update.m_EntityId)
                  {
                    OnPlayerAssigned(newObject);
                  }
                }
                else if (op.m_Op == ComponentOp::Op::Update)
                {
                  auto iter = m_EntityToObjectMap.find(update.m_EntityId);
                  if (iter != m_EntityToObjectMap.end())
                  {
                    MovedObject update;
                    update.object = iter->second;
                    ReadClientData(update.data, op.m_Data);
                    m_ObjUpdates.push_back(update);
                  }
                }
              }
            }
            if (op.m_Comp == s_ClientInputDataComponentId)
            {
              if (m_Role == NetRole::Server)
              {
                auto iter = m_EntityToObjectMap.find(update.m_EntityId);
                if (iter != m_EntityToObjectMap.end())
                {
                  ClientInputData inputData;
                  ReadClientInputData(inputData, op.m_Data);

                  OnClientCommand(iter->second, inputData);
                }
              }
            }
          }
        }
      }
    }
  }

  Err Network::Impl::Connect(NetRole iRole, String const& iURL, uint16_t iPort)
  {
    Worker_ConnectionParameters parameters = Worker_DefaultConnectionParameters();
    parameters.worker_type = iRole == NetRole::Server ? "server" : "client";
    parameters.network.connection_type = Worker_NetworkConnectionType::WORKER_NETWORK_CONNECTION_TYPE_MODULAR_KCP;
    parameters.network.use_external_ip = 2;
    parameters.network.modular_kcp.security_type = WORKER_NETWORK_SECURITY_TYPE_INSECURE;
    parameters.default_component_vtable = &m_DefaultVTable;
    parameters.enable_dynamic_components = true;

#if _DEBUG
    Worker_HeartbeatParameters HeartbeatParams{ WORKER_DEFAULTS_HEARTBEAT_INTERVAL_MILLIS, INT64_MAX };
    parameters.network.modular_kcp.upstream_heartbeat = &HeartbeatParams;
    parameters.network.modular_kcp.downstream_heartbeat = &HeartbeatParams;
#endif

    m_WorkerId = (iRole == NetRole::Server ? String("Worker") : String("Client")) + m_UniqueId;

    Worker_ConnectionFuture* connectionFuture = Worker_ConnectAsync(iURL.c_str(), iPort, m_WorkerId.c_str(), &parameters);

    uint32_t timeOut = 4000;
    Impl::Get().m_Connection = Worker_ConnectionFuture_Get(connectionFuture, &timeOut);

    Worker_ConnectionFuture_Destroy(connectionFuture);

    if (m_Connection == nullptr)
    {
      LOG_ERROR << "Failed to connect to the receptionist." << "\n";
      //LOG_ERROR << "Reason: " << connection-> << std::endl;
      return Err::Failure;
    }
    LOG_INFO << "Successfully connected using the Receptionist" << "\n";

    m_Role = iRole;

    if (m_Role == NetRole::Server)
    {
      Worker_LogMessage message = { WORKER_LOG_LEVEL_WARN, "Server", "Connected successfully", NULL };
      Worker_Connection_SendLogMessage(m_Connection, &message);
      CreateServerInterestEntity();
    }
    if (m_Role == NetRole::Client)
    {
      Worker_LogMessage message = { WORKER_LOG_LEVEL_WARN, "Client", "Connected successfully", NULL };
      Worker_Connection_SendLogMessage(m_Connection, &message);
    }

    return Err::Success;
  }
#endif

  Err Network::Connect(NetRole iRole, String const& iURL, uint16_t iPort)
  {
#ifdef ENABLE_SPATIALOS_DRIVER
    return Impl::Get().Connect(iRole, iURL, iPort);
#else
    return Err::Error;
#endif
  }

  void Network::Tick(float iDelta)
  {
#ifdef ENABLE_SPATIALOS_DRIVER
    Impl::Get().Tick(iDelta);
#endif
  }

  Vector<Network::MovedObject> const& Network::GetMovedObjects()
  {
#ifdef ENABLE_SPATIALOS_DRIVER
    return Impl::Get().m_ObjUpdates;
#else
    static Vector<Network::MovedObject> dummy;
    return dummy;
#endif
  }

  void Network::UpdateObjects(Vector<MovedObject> const& iObjects)
  {
#ifdef ENABLE_SPATIALOS_DRIVER
    Impl& impl = Impl::Get();
    if (impl.m_Connection && impl.m_Role == NetRole::Server)
    {
      for (auto const& update : iObjects)
      {
        auto iter = impl.m_ObjectToEntityMap.find(update.object);
        if (iter != impl.m_ObjectToEntityMap.end())
        {
          Worker_ComponentUpdate compUpdate = {};
          compUpdate.component_id = s_ClientDataComponentId;
          compUpdate.schema_type = Schema_CreateComponentUpdate();
          WriteClientData(update.data, Schema_GetComponentUpdateFields(compUpdate.schema_type));

          Worker_Connection_SendComponentUpdate(impl.m_Connection, iter->second, &compUpdate, nullptr);
        }
      }
    }
#endif
  }

  void Network::SetClientInput(ObjectHandle iChar, ClientInputData const& iInput)
  {
#ifdef ENABLE_SPATIALOS_DRIVER
    Impl& impl = Impl::Get();
    if (impl.m_Connection && impl.m_Role == NetRole::Client)
    {
      auto iter = impl.m_ObjectToEntityMap.find(iChar);
      if (iter != impl.m_ObjectToEntityMap.end())
      {
        if (impl.m_PlayerEntity != 0 && impl.m_PlayerEntity == iter->second)
        {
          Worker_ComponentUpdate compUpdate = {};
          compUpdate.component_id = s_ClientInputDataComponentId;
          compUpdate.schema_type = Schema_CreateComponentUpdate();
          WriteClientInputData(iInput, Schema_GetComponentUpdateFields(compUpdate.schema_type));

          Worker_Connection_SendComponentUpdate(impl.m_Connection, iter->second, &compUpdate, nullptr);
        }
      }
    }
#endif
  }

}