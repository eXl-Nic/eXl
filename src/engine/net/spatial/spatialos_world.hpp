/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/coredef.hpp>

#ifndef __ANDROID__
//#define ENABLE_SPATIALOS_DRIVER
#endif

#ifdef ENABLE_SPATIALOS_DRIVER

#include <improbable/c_worker.h>

namespace eXl
{
  struct SpatialComponentInfo
  {
    Worker_ComponentId m_ComponentId;
    bool m_IsNotAdded;
    bool m_HasAuth;
  };

  struct SpatialEntity
  {
    Worker_EntityId m_EntityId;
    UnorderedMap<Worker_ComponentId, SpatialComponentInfo> m_Components;
  };

  using EntityComponent = std::pair<Worker_EntityId, Worker_ComponentId>;

  struct ComponentOp
  {
    enum class Op
    {
      None,
      Add,
      Remove,
      Update,
      AuthGained,
      AuthLost
    };

    Op m_Op = Op::None;

    ComponentOp();
    ComponentOp(Worker_ComponentData const&);
    ComponentOp(Worker_ComponentUpdate const&);
    ComponentOp(ComponentOp&&);
    ComponentOp& operator =(ComponentOp&&);
    ComponentOp(ComponentOp const&) = delete;
    ComponentOp& operator =(ComponentOp const&) = delete;
    ~ComponentOp();

    void Clear();

    Worker_ComponentId m_Comp;
    
    Schema_ComponentData* m_Data = nullptr;
  };

  struct EntityUpdate
  {
    EntityUpdate(Worker_EntityId iId) : m_EntityId(iId)
    {}

    Worker_EntityId m_EntityId;
    bool m_Added = false;
    bool m_Removed = false;

    UnorderedMap<Worker_ComponentId, ComponentOp> m_CompChange;
    UnorderedMap<Worker_ComponentId, ComponentOp> m_CompAuthChange;
  };

  class SpatialWorld
  {
  public:

    std::function<Err(Worker_Op const&)> m_ExternalOpHandler;

    Err UpdateWorld(Worker_Connection* iConnection, Vector<EntityUpdate>& oUpdates);

    bool HasAuthority(Worker_EntityId iId, Worker_ComponentId iCompId);

  protected:

    void OnAddEntity(Worker_AddEntityOp const&);
    void OnRemoveEntity(Worker_RemoveEntityOp const&);
    void OnAddComponent(Worker_AddComponentOp const&);
    void OnRemoveComponent(Worker_RemoveComponentOp const&);
    void OnAuthorityChange(Worker_AuthorityChangeOp const&);
    void OnComponentUpdate(Worker_ComponentUpdateOp const&);

    bool m_InCS = false;
    UnorderedMap<Worker_EntityId, EntityUpdate> m_PendingUpdates;

    UnorderedMap<Worker_EntityId, SpatialEntity> m_World;
  };
}

#endif