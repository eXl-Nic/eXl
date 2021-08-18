/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/common/world.hpp>
#include <core/type/coretype.hpp>
#include <core/type/tagtype.hpp>

#include <engine/physics/physicsys.hpp>
#include <engine/game/ability.hpp>
#include <engine/common/transforms.hpp>

#include <core/clock.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(WorldSystem);
  IMPLEMENT_RTTI(ComponentManager);
  IMPLEMENT_RTTI(ComponentManifest);

  IMPLEMENT_TYPE(ObjectHandle)
  IMPLEMENT_TYPE(ObjectReference)
  IMPLEMENT_TAG_TYPE(World)

  IMPLEMENT_SERIALIZE_METHODS(ObjectReference)

  Err ObjectReference::Serialize(Serializer iSerializer)
  {
    iSerializer.BeginStruct();
    iSerializer.PushKey("PersistentId");
    iSerializer &= m_PersistentId;
    iSerializer.PopKey();
    iSerializer.EndStruct();

    return Err::Success;
  }

  ObjectHandle ObjectReference::Resolve(World& iWorld) const
  {
    m_Object = iWorld.GetObjectFromPersistentId(m_PersistentId);
    return m_Object;
  }

  void ComponentManager::CreateComponent(ObjectHandle iObject)
  {
    ObjectInfo* info = m_World->TryGetObjectInfo(iObject);
    eXl_ASSERT(info != nullptr);

    info->m_Components |= 1 << m_SysIdx;
  }

  void ComponentManager::DeleteComponent(ObjectHandle iObject)
  {
    ObjectInfo* info = m_World->TryGetObjectInfo(iObject);
    if (info != nullptr)
    {
      info->m_Components &= ~(1 << m_SysIdx);
    }
  }

  void ComponentManifest::RegisterComponent(ComponentName iName, Type const* iType, ComponentFactory iFactory)
  {
    eXl_ASSERT(iType != nullptr);
    eXl_ASSERT(m_Components.count(iName) == 0);

    ComponentEntry newEntry;
    newEntry.type = iType;
    newEntry.factory = iFactory;
    m_Components.insert(std::make_pair(iName, newEntry));
  }

  Type const* ComponentManifest::GetComponentTypeFromName(ComponentName iName) const
  {
    auto iter = m_Components.find(iName);
    if (iter == m_Components.end())
    {
      return nullptr;
    }

    return iter->second.type;
  }

  ComponentFactory const* ComponentManifest::GetComponentFactory(ComponentName iName) const
  {
    auto iter = m_Components.find(iName);
    if (iter == m_Components.end())
    {
      return nullptr;
    }

    return &iter->second.factory;
  }

  Vector<ComponentName> ComponentManifest::GetComponents() const
  {
    Vector<ComponentName> names;
    for (auto const& entry : m_Components)
    {
      names.push_back(entry.first);
    }

    return names;
  }

  World::World(ComponentManifest const& iComponents)
    : m_Components(iComponents)
  {
    m_Tick.resize(NumStages);
  }

  ObjectHandle World::CreateObject()
  {
    ObjectHandle handle = m_Objects.Alloc();
    ObjectInfo& info = m_Objects.Get(handle);
    info.m_PersistentId = s_AnonymousFlag | handle.GetGeneration() | handle.GetId();

    return handle;
  }

  ObjectHandle World::CreateObject(ObjectCreationInfo iInfo)
  {
    ObjectHandle handle = m_Objects.Alloc();
    ObjectInfo& info = m_Objects.Get(handle);
    info.m_DisplayName = std::move(iInfo.m_DisplayName);
    info.m_PersistentId = iInfo.m_PersistentId;
    if (info.m_PersistentId & s_AnonymousFlag)
    {
      info.m_PersistentId = s_AnonymousFlag | handle.GetGeneration() | handle.GetId();
    }
    else
    {
      m_PersistentIdToObjects.insert(std::make_pair(info.m_PersistentId, handle));
    }
    
    return handle;
  }

  ObjectHandle World::GetObjectFromPersistentId(uint64_t iId)
  {
    if (iId & s_AnonymousFlag)
    {
      ObjectHandle dynHandle;
      *reinterpret_cast<uint32_t*>(&dynHandle) = static_cast<uint32_t>(iId);
      if (IsObjectValid(dynHandle))
      {
        return dynHandle;
      }
    }
    else
    {
      auto iter = m_PersistentIdToObjects.find(iId);
      if (iter != m_PersistentIdToObjects.end())
      {
        return iter->second;
      }
    }

    return ObjectHandle();
  }

  ObjectInfo& World::GetObjectInfo(ObjectHandle iHandle)
  {
    ObjectInfo* info = TryGetObjectInfo(iHandle);
    eXl_ASSERT(info);
    return *info;
  }

  ObjectInfo* World::TryGetObjectInfo(ObjectHandle iHandle)
  {
    ObjectInfo* info = m_Objects.TryGet(iHandle);
    return info ? (info->m_PendingDeletion ? nullptr : info) : nullptr;
  }

  bool World::IsObjectValid(ObjectHandle iHandle)
  {
    return TryGetObjectInfo(iHandle) != nullptr;
  }

  void World::DeleteObject(ObjectHandle iObject)
  {
    if (ObjectInfo* info = TryGetObjectInfo(iObject))
    {
      info->m_PendingDeletion = true;
      m_ObjectsToDelete.push_back(iObject);

      FlushObjectsToDelete();
    }
  }

  void World::FlushObjectsToDelete()
  {
    for (ObjectHandle toDelete : m_ObjectsToDelete)
    {
      ObjectInfo* info = m_Objects.TryGet(toDelete);
      uint32_t compBits = info->m_Components;
      uint32_t compIdx = 0;
      while (compBits != 0)
      {
        if (compBits & 1)
        {
          m_CompManagers[compIdx]->DeleteComponent(toDelete);
        }
        compBits >>= 1;
        compIdx++;
      }
      if ((info->m_PersistentId & s_AnonymousFlag) == 0)
      {
        m_PersistentIdToObjects.erase(info->m_PersistentId);
      }
      m_Objects.Release(toDelete);
    }
    m_ObjectsToDelete.clear();
  }

  WorldSystem* World::AddSystem(std::unique_ptr<WorldSystem>&& iSystem)
  {
    if (!iSystem)
    {
      return nullptr;
    }

    for (auto const& sys : m_Systems)
    {
      eXl_ASSERT(!sys.second.m_System->GetRtti().IsKindOf(iSystem->GetRtti())
              && !iSystem->GetRtti().IsKindOf(sys.second.m_System->GetRtti()));
    }

    SysReg reg;
    if (auto* compMgr = ComponentManager::DynamicCast(iSystem.get()))
    {
      reg.m_ComponentIdx = m_CompManagers.size();
      m_CompManagers.push_back(compMgr);
      compMgr->m_SysIdx = reg.m_ComponentIdx;
    }
    reg.m_System = std::move(iSystem);
    
    auto newEntry = m_Systems.insert(std::make_pair(&reg.m_System->GetRtti(), std::move(reg))).first;
    WorldSystem* system = newEntry->second.m_System.get();
    system->Register(*this);
    
    if (PhysicsSystem* phSys = PhysicsSystem::DynamicCast(system))
    {
      m_PhSystem = phSys;
    }
    if (AbilitySystem* abilitySys = AbilitySystem::DynamicCast(system))
    {
      m_AbilitySystem = abilitySys;
    }
    if (Transforms* trans = Transforms::DynamicCast(system))
    {
      m_Transforms = trans;
    }

    return system;
  }

  WorldSystem* World::GetSystem(Rtti const& iRtti)
  {
    auto iter = m_Systems.find(&iRtti);
    if (iter != m_Systems.end())
    {
      return iter->second.m_System.get();
    }
    for (auto const& sys : m_Systems)
    {
      if (sys.first->IsKindOf(iRtti))
      {
        return sys.second.m_System.get();
      }
    }

    return nullptr;
  }

  void World::Tick(float iDelta, ProfilingState& ioProfiling)
  {
    ioProfiling.m_LastFrameTime = ioProfiling.m_CurFrameTime;
    Clock profiler;

    FlushObjectsToDelete();
    
    if (m_AbilitySystem)
    {
      m_AbilitySystem->NewFrame();
    }

    if (m_PhSystem)
    {
      m_PhSystem->SyncTriggersTransforms();
    }

    if (m_Transforms)
    {
      m_Transforms->NextFrame();
    }

    for (auto const& tickFn : m_Tick[FrameStart])
    {
      tickFn(*this, iDelta);
    }

    ProcessTimers();

    ioProfiling.m_FrameStartTime = profiler.GetTime();
    
    if (m_PhSystem)
    {
      m_PhSystem->GetNeighborhoodExtraction().Run(Vector3f(0.0, 0.0, 0.0), 10.0);
    }

    ioProfiling.m_NeighETime = profiler.GetTime();

    for (auto const& tickFn : m_Tick[PrePhysics])
    {
      tickFn(*this, iDelta);
    }

    if (m_PhSystem)
    {
      m_PhSystem->Step(iDelta);
    }

    ioProfiling.m_PhysicTime = profiler.GetTime() * 1000.0;

    for (auto const& tickFn : m_Tick[PostPhysics])
    {
      tickFn(*this, iDelta);
    }

    ioProfiling.m_PostPhysicsTime = profiler.GetTime() * 1000.0;

    if (m_AbilitySystem)
    {
      m_AbilitySystem->Tick(iDelta);
    }

    ioProfiling.m_AbilitiesTime = profiler.GetTime() * 1000.0;

    for (auto const& tickFn : m_Tick[PostAbilites])
    {
      tickFn(*this, iDelta);
    }

    ioProfiling.m_PostAbilitiesTime = profiler.GetTime() * 1000.0;
  }

  void World::AddTick(Stage iStage, TickDelegate&& iDelegate)
  {
    m_Tick[iStage].emplace_back(std::move(iDelegate));
  }

  TimerHandle World::AddTimer(float iTimeInSec, bool iLoop, std::function<void(World&)>&& iDelegate)
  {
    TimerSchedule schedule;
    uint64_t curTime = Clock::GetTimestamp();
    schedule.nextTick = curTime + uint64_t(Clock::GetTicksPerSecond() * iTimeInSec);

    TimerHandle handle = m_Timers.Alloc();
    TimerDesc& desc = m_Timers.Get(handle);
    desc.loop = iLoop;
    desc.time = iTimeInSec;
    desc.timerDelegate = std::move(iDelegate);

    schedule.timer = handle;

    m_TimerSchedule.push_back(schedule);
    std::push_heap(m_TimerSchedule.begin(), m_TimerSchedule.end());

    return handle;
  }

  void World::RemoveTimer(TimerHandle iHandle)
  {
    m_Timers.Release(iHandle);
  }

  void World::ProcessTimers()
  {
    uint64_t currentTime = Clock::GetTimestamp();
    while (!m_TimerSchedule.empty() && m_TimerSchedule.front().nextTick <= currentTime)
    {
      TimerHandle curTimer = m_TimerSchedule.front().timer;
      TimerDesc const& desc = m_Timers.Get(curTimer);
      desc.timerDelegate(*this);
      std::pop_heap(m_TimerSchedule.begin(), m_TimerSchedule.end());
      if (desc.loop)
      {
        m_TimerSchedule.back().nextTick = Clock::GetTimestamp() + uint64_t(Clock::GetTicksPerSecond() * desc.time);
      }

      if (m_TimerSchedule.back().nextTick <= currentTime)
      {
        m_TimerSchedule.pop_back();
        m_Timers.Release(curTimer);
      }
      else
      {
        std::push_heap(m_TimerSchedule.begin(), m_TimerSchedule.end());
      }
    }
  }
}