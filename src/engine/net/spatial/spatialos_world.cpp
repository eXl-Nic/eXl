#include "spatialos_world.hpp"

#ifdef ENABLE_SPATIALOS_DRIVER

#include <improbable/c_schema.h>

namespace eXl
{

  ComponentOp::ComponentOp() = default;
  ComponentOp::ComponentOp(Worker_ComponentData const& iData)
    : m_Op(Op::Add)
    , m_Comp(iData.component_id)
  {
    m_Data = Schema_CopyComponentData(iData.schema_type);
  }

  ComponentOp::ComponentOp(Worker_ComponentUpdate const& iUpdate)
    : m_Op(Op::Update)
    , m_Comp(iUpdate.component_id)
  {
    m_Data = Schema_CreateComponentData();
    Schema_ApplyComponentUpdateToData(iUpdate.schema_type, m_Data);
  }

  ComponentOp::ComponentOp(ComponentOp&& iOther)
  {
    m_Comp = iOther.m_Comp;
    m_Op = iOther.m_Op;
    if (iOther.m_Op == Op::Add
      || iOther.m_Op == Op::Update)
    {
      m_Data = iOther.m_Data;
    }
    iOther.m_Op = Op::None;
  }

  ComponentOp& ComponentOp::operator=(ComponentOp&& iOther)
  {
    this->~ComponentOp();
    new(this) ComponentOp(std::move(iOther));

    return *this;

  }

  ComponentOp::~ComponentOp()
  {
    Clear();
  }

  void ComponentOp::Clear()
  {
    if (m_Op == Op::Add
      || m_Op == Op::Update)
    {
      Schema_DestroyComponentData(m_Data);
    }
    m_Op = Op::None;
  }

  void SpatialWorld::OnAddEntity(Worker_AddEntityOp const& iOp)
  {
    auto iterPending = m_PendingUpdates.find(iOp.entity_id);
    if (iterPending != m_PendingUpdates.end())
    {
      if (iterPending->second.m_Removed)
      {
        // So, add happens after removal. Removal might contain a list of deleted components to keep deleted.
        iterPending->second.m_Removed = false;
      }
    }

    if (m_World.count(iOp.entity_id) == 0)
    {
      if (iterPending == m_PendingUpdates.end())
      {
        iterPending = m_PendingUpdates.insert(std::make_pair(iOp.entity_id, EntityUpdate(iOp.entity_id))).first;
      }
      
      iterPending->second.m_Added = true;
    }
  }

  void SpatialWorld::OnRemoveEntity(Worker_RemoveEntityOp const& iOp)
  {
    auto iterPending = m_PendingUpdates.find(iOp.entity_id);
    if (iterPending != m_PendingUpdates.end())
    {
      if (iterPending->second.m_Added)
      {
        // Pending add entity remove, just forget about it.
        m_PendingUpdates.erase(iOp.entity_id);
      }
      else
      {
        iterPending->second.m_Removed = true;
      }
      return;
    }

    auto iterWorld = m_World.find(iOp.entity_id);
    if (iterWorld != m_World.end())
    {
      // Should have pending components remove otherwise ??
      eXl_ASSERT(iterWorld->second.m_Components.empty());

      EntityUpdate newUpdate(iOp.entity_id);
      newUpdate.m_Removed = true;
      m_PendingUpdates.emplace(std::make_pair(iOp.entity_id, std::move(newUpdate)));
    }
  }

  void SpatialWorld::OnAddComponent(Worker_AddComponentOp const& iOp)
  {
    auto iterPending = m_PendingUpdates.find(iOp.entity_id);
    if (iterPending != m_PendingUpdates.end())
    {
      EntityUpdate& update = iterPending->second;

      auto iterChange = update.m_CompChange.find(iOp.data.component_id);
      if (iterChange != update.m_CompChange.end())
      {
        if (iterChange->second.m_Op == ComponentOp::Op::Add
          || iterChange->second.m_Op == ComponentOp::Op::Update)
        {
          Schema_DestroyComponentData(iterChange->second.m_Data);
          iterChange->second.m_Data = Schema_CopyComponentData(iOp.data.schema_type);

          return;
        }
        else if (iterChange->second.m_Op == ComponentOp::Op::Remove)
        {
          update.m_CompChange.erase(iterChange);
        }
      }
    }

    SpatialEntity* currentEntity = nullptr;

    if (iterPending == m_PendingUpdates.end())
    {
      auto iterWorld = m_World.find(iOp.entity_id);
      eXl_ASSERT(iterWorld != m_World.end());
      currentEntity = &iterWorld->second;

      iterPending = m_PendingUpdates.insert(std::make_pair(iOp.entity_id, EntityUpdate(iOp.entity_id))).first;
    }

    EntityUpdate& update = iterPending->second;
    if (currentEntity)
    {
      auto iterComp = currentEntity->m_Components.find(iOp.data.component_id);

      if (iterComp != currentEntity->m_Components.end() && !iterComp->second.m_IsNotAdded)
      {
        ComponentOp updateOp;
        updateOp.m_Comp = iOp.data.component_id;
        updateOp.m_Op = ComponentOp::Op::Update;
        updateOp.m_Data = Schema_CopyComponentData(iOp.data.schema_type);
        update.m_CompChange.insert(std::make_pair(iOp.data.component_id, std::move(updateOp)));
      }

      return;
    }
    
    update.m_CompChange.insert(std::make_pair(iOp.data.component_id, ComponentOp(iOp.data)));
    
  }

  void SpatialWorld::OnRemoveComponent(Worker_RemoveComponentOp const& iOp)
  {
    auto iterPending = m_PendingUpdates.find(iOp.entity_id);
    if (iterPending != m_PendingUpdates.end())
    {
      EntityUpdate& update = iterPending->second;

      auto iterChange = update.m_CompChange.find(iOp.component_id);
      if (iterChange != update.m_CompChange.end())
      {
        if (iterChange->second.m_Op == ComponentOp::Op::Add
          || iterChange->second.m_Op == ComponentOp::Op::Update)
        {
          update.m_CompChange.erase(iterChange);
        }
      }
    }

    auto iterWorld = m_World.find(iOp.entity_id);
    if(iterWorld != m_World.end())
    {  
      SpatialEntity& entity = iterWorld->second;
      auto iterComp = entity.m_Components.find(iOp.component_id);
      if (iterComp == entity.m_Components.end() || iterComp->second.m_IsNotAdded)
      {
        if (iterPending != m_PendingUpdates.end())
        {
          iterPending = m_PendingUpdates.insert(std::make_pair(iOp.entity_id, EntityUpdate(iOp.entity_id))).first;
        }

        ComponentOp removeOp;
        removeOp.m_Comp = iOp.component_id;
        removeOp.m_Op = ComponentOp::Op::Remove;

        iterPending->second.m_CompChange.insert(std::make_pair(iOp.component_id, std::move(removeOp)));
      }
    }
  }

  void SpatialWorld::OnComponentUpdate(Worker_ComponentUpdateOp const& iOp)
  {
    auto iterPending = m_PendingUpdates.find(iOp.entity_id);
    if (iterPending != m_PendingUpdates.end())
    {
      EntityUpdate& update = iterPending->second;

      auto iterChange = update.m_CompChange.find(iOp.update.component_id);
      
      if (iterChange != update.m_CompChange.end())
      {
        eXl_ASSERT(iterChange->second.m_Op != ComponentOp::Op::Remove);

        Schema_ComponentData* oldData = iterChange->second.m_Data;
        Schema_ComponentData* newData = Schema_CopyComponentData(oldData);
        Schema_ApplyComponentUpdateToData(iOp.update.schema_type, newData);
        iterChange->second.m_Data = newData;
        Schema_DestroyComponentData(oldData);

        return;
      }
    }

    auto iterWorld = m_World.find(iOp.entity_id);
    if (iterWorld != m_World.end())
    {
      SpatialEntity& entity = iterWorld->second;
      auto iterComp = entity.m_Components.find(iOp.update.component_id);
      eXl_ASSERT( iterComp != entity.m_Components.end() && !iterComp->second.m_IsNotAdded);

      if (iterPending == m_PendingUpdates.end())
      {
        iterPending = m_PendingUpdates.emplace(std::make_pair(iOp.entity_id, EntityUpdate(iOp.entity_id))).first;
      }

      iterPending->second.m_CompChange.insert(std::make_pair(iOp.update.component_id, ComponentOp(iOp.update)));
      
    }
  }

  void SpatialWorld::OnAuthorityChange(Worker_AuthorityChangeOp const& iOp)
  {
    if (iOp.authority == WORKER_AUTHORITY_AUTHORITY_LOSS_IMMINENT)
      return;

    ComponentOp authOp;
    authOp.m_Comp = iOp.component_id;
    authOp.m_Op = iOp.authority == WORKER_AUTHORITY_AUTHORITATIVE ? ComponentOp::Op::AuthGained : ComponentOp::Op::AuthLost;

    auto iterPending = m_PendingUpdates.find(iOp.entity_id);
    if (iterPending != m_PendingUpdates.end())
    {
      EntityUpdate& update = iterPending->second;
      
      auto iterAuth = update.m_CompAuthChange.insert(std::make_pair(iOp.component_id, ComponentOp())).first;
      iterAuth->second = std::move(authOp);

      return;
    }

    auto iterWorld = m_World.find(iOp.entity_id);
    if (iterWorld != m_World.end())
    {
      EntityUpdate newUpdate(iOp.entity_id);
      newUpdate.m_CompAuthChange.insert(std::make_pair(iOp.component_id, std::move(authOp)));
      m_PendingUpdates.emplace(std::make_pair(iOp.entity_id, std::move(newUpdate)));
    }
  }

  Err SpatialWorld::UpdateWorld(Worker_Connection* iConnection, Vector<EntityUpdate>& oUpdates)
  {
    Err res = Err::Success;
    oUpdates.clear();

    Worker_OpList* op_list = Worker_Connection_GetOpList(iConnection, 0);
    for (size_t i = 0; i < op_list->op_count; ++i)
    {
      Worker_Op* op = &op_list->ops[i];
      switch (op->op_type)
      {
      case WORKER_OP_TYPE_DISCONNECT:
      case WORKER_OP_TYPE_FLAG_UPDATE:
      case WORKER_OP_TYPE_LOG_MESSAGE:
      case WORKER_OP_TYPE_METRICS:
      case WORKER_OP_TYPE_RESERVE_ENTITY_IDS_RESPONSE:
      case WORKER_OP_TYPE_CREATE_ENTITY_RESPONSE:
      case WORKER_OP_TYPE_DELETE_ENTITY_RESPONSE:
      case WORKER_OP_TYPE_ENTITY_QUERY_RESPONSE:
      case WORKER_OP_TYPE_COMMAND_REQUEST:
      case WORKER_OP_TYPE_COMMAND_RESPONSE:
        if (m_ExternalOpHandler)
        {
          // Mayyyybe I should respect CS.
          res = m_ExternalOpHandler(*op);
          if (!res)
          {
            i = op_list->op_count;
          }
        }
        break;
      case WORKER_OP_TYPE_CRITICAL_SECTION:
      {
        Worker_CriticalSectionOp& csOp = op->op.critical_section;
        eXl_ASSERT(m_InCS == !csOp.in_critical_section);
        m_InCS = csOp.in_critical_section != 0;
      }
        break;
      case WORKER_OP_TYPE_ADD_ENTITY:
      {
        Worker_AddEntityOp& addEntityOp = op->op.add_entity;
        OnAddEntity(addEntityOp);
      }
        break;
      case WORKER_OP_TYPE_REMOVE_ENTITY:
      {
        Worker_RemoveEntityOp& removeEntityOp = op->op.remove_entity;
        OnRemoveEntity(removeEntityOp);
      }
        break;
      case WORKER_OP_TYPE_ADD_COMPONENT:
      {
        Worker_AddComponentOp& addComponentOp = op->op.add_component;
        OnAddComponent(addComponentOp);
      }
        break;
      case WORKER_OP_TYPE_REMOVE_COMPONENT:
      {
        Worker_RemoveComponentOp& removeComponentOp = op->op.remove_component;
        OnRemoveComponent(removeComponentOp);
      }
        break;
      case WORKER_OP_TYPE_AUTHORITY_CHANGE:
      {
        Worker_AuthorityChangeOp& authChangeOp = op->op.authority_change;
        OnAuthorityChange(authChangeOp);
      }
        break;
      case WORKER_OP_TYPE_COMPONENT_UPDATE:
      {
        Worker_ComponentUpdateOp& updateComponentOp = op->op.component_update;
        OnComponentUpdate(updateComponentOp);
      }
        break;
     
      default:
        break;
      }
    }
    Worker_OpList_Destroy(op_list);

    if (!res)
    {
      oUpdates.clear();
      return res;
    }

    if (!m_InCS)
    {
      // Synchronize local world.
      for (auto& entry : m_PendingUpdates)
      {
        EntityUpdate& update = entry.second;
        auto iterWorld = m_World.find(update.m_EntityId);

        eXl_ASSERT(!(update.m_Added && update.m_Removed ));

        if (update.m_Added)
        {
          eXl_ASSERT(iterWorld == m_World.end());
          iterWorld = m_World.insert(std::make_pair(update.m_EntityId, SpatialEntity())).first;
        }

        if (update.m_Removed)
        {
          eXl_ASSERT(iterWorld != m_World.end());
          m_World.erase(iterWorld);
          iterWorld = m_World.end();
        }
        if (iterWorld != m_World.end())
        {
          SpatialEntity& entity = iterWorld->second;

          for (auto& compChange : update.m_CompChange)
          {
            ComponentOp& curOp = compChange.second;

            auto iterComp = entity.m_Components.find(curOp.m_Comp);

            if (curOp.m_Op == ComponentOp::Op::Add)
            {
              eXl_ASSERT(iterComp == entity.m_Components.end() || iterComp->second.m_IsNotAdded);
              if (iterComp == entity.m_Components.end())
              {
                iterComp = entity.m_Components.insert(std::make_pair(curOp.m_Comp, SpatialComponentInfo())).first;
              }
              iterComp->second.m_IsNotAdded = false;
            }
            if (curOp.m_Op == ComponentOp::Op::Update)
            {
              eXl_ASSERT(iterComp != entity.m_Components.end() && !iterComp->second.m_IsNotAdded);
            }
            if (curOp.m_Op == ComponentOp::Op::Remove)
            {
              eXl_ASSERT(iterComp != entity.m_Components.end() && !iterComp->second.m_IsNotAdded);
              entity.m_Components.erase(iterComp);
            }
          }

          for (auto& compAuthChange : update.m_CompAuthChange)
          {
            ComponentOp& curOp = compAuthChange.second;

            auto iterComp = entity.m_Components.find(curOp.m_Comp);
            if (iterComp == entity.m_Components.end())
            {
              iterComp = entity.m_Components.insert(std::make_pair(curOp.m_Comp, SpatialComponentInfo())).first;
              iterComp->second.m_IsNotAdded = true;
            }

            iterComp->second.m_HasAuth = curOp.m_Op == ComponentOp::Op::AuthGained;
          }
        }

        oUpdates.emplace_back(std::move(update));
      }

      m_PendingUpdates.clear();
    }
    return Err::Success;
  }

  bool SpatialWorld::HasAuthority(Worker_EntityId iId, Worker_ComponentId iCompId)
  {
    auto iterWorld = m_World.find(iId);
    if (iterWorld != m_World.end())
    {
      SpatialEntity& entity = iterWorld->second;
      auto iterComp = entity.m_Components.find(iCompId);
      if (iterComp != entity.m_Components.end())
      {
        return iterComp->second.m_HasAuth;
      }
    }

    return false;
  }
}

#endif