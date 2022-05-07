/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/common/world.hpp>
#include <core/type/coretype.hpp>
#include <core/type/tagtype.hpp>
#include <core/random.hpp>

#include <engine/common/transforms.hpp>
#include <engine/common/gamedatabase.hpp>
#include <engine/physics/physicsys.hpp>
#include <engine/game/ability.hpp>

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

  void ComponentManifest::RegisterComponent(ComponentName iName, ComponentFactory iFactory, std::initializer_list<PropertySheetName> iReqData)
  {
    eXl_ASSERT(m_Components.count(iName) == 0);

    ComponentEntry newEntry;
    newEntry.requiredData = std::move(iReqData);
    newEntry.factory = iFactory;
    m_Components.insert(std::make_pair(iName, newEntry));
  }

  UnorderedSet<PropertySheetName> const* ComponentManifest::GetRequiredDataForComponent(ComponentName iName) const
  {
    auto iter = m_Components.find(iName);
    if (iter == m_Components.end())
    {
      return nullptr;
    }

    return &iter->second.requiredData;
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

  World::~World()
  {
    m_Objects.Reset();
  }

  ObjectHandle World::CreateObject()
  {
    ObjectHandle handle = m_Objects.Alloc();
    ObjectInfo& info = m_Objects.Get(handle);
    info.m_PersistentId = ObjectCreationInfo::s_AnonymousFlag | handle.GetGeneration() | handle.GetId();

    return handle;
  }

  ObjectHandle World::CreateObject(ObjectCreationInfo iInfo)
  {
    ObjectHandle handle = m_Objects.Alloc();
    ObjectInfo& info = m_Objects.Get(handle);
    info.m_DisplayName = std::move(iInfo.m_DisplayName);
    info.m_PersistentId = iInfo.m_PersistentId;

    if (info.m_PersistentId & ObjectCreationInfo::s_AutoNamedFlag)
    {
      do
      {
        info.m_PersistentId = Random::AllocateUUID();
        info.m_PersistentId &= ~(ObjectCreationInfo::s_AnonymousFlag | ObjectCreationInfo::s_AutoNamedFlag);
      } while (m_PersistentIdToObjects.count(info.m_PersistentId) != 0);
    }

    if (info.m_PersistentId & ObjectCreationInfo::s_AnonymousFlag)
    {
      info.m_PersistentId = ObjectCreationInfo::s_AnonymousFlag | handle.GetGeneration() | handle.GetId();
    }
    else
    {
      m_PersistentIdToObjects.insert(std::make_pair(info.m_PersistentId, handle));
    }
    
    return handle;
  }

  ObjectHandle World::GetObjectFromPersistentId(uint64_t iId)
  {
    if (iId & ObjectCreationInfo::s_AnonymousFlag)
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

  bool World::IsObjectValid(ObjectHandle iHandle) const
  {
    return m_Objects.TryGet(iHandle) != nullptr;
  }

  bool World::IsObjectBeingDestroyed(ObjectHandle iHandle) const
  {
    ObjectInfo* info = m_Objects.TryGet(iHandle);
    return info ? info->m_PendingDeletion : false;
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
      if ((info->m_PersistentId & ObjectCreationInfo::s_AnonymousFlag) == 0)
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
    if (GameDatabase* database = GameDatabase::DynamicCast(system))
    {
      m_Database = database;
    }
    if (EventSystem* events = EventSystem::DynamicCast(system))
    {
      m_Events = events;
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

  void World::Tick(ProfilingState& ioProfiling)
  {
    float iDelta = 0;
    if (m_CurrentTimestamp != 0)
    {
      uint64_t prevStamp = m_CurrentTimestamp;
      m_CurrentTimestamp = Clock::GetTimestamp();
      m_ElapsedTimestamp = m_CurrentTimestamp - m_StartTimestamp;
      double gameTimeDelta = (double(m_CurrentTimestamp - prevStamp) * m_GameTimeScaling) / Clock::GetTicksPerSecond();
      m_ElapsedGameTime += gameTimeDelta;
      iDelta = gameTimeDelta;
    }
    else
    {
      m_StartTimestamp = Clock::GetTimestamp();
      m_CurrentTimestamp = m_StartTimestamp;
    }

    ioProfiling.m_LastFrameTime = ioProfiling.m_CurFrameTime;
    Clock profiler;

    FlushObjectsToDelete();
    if (m_Database)
    {
      m_Database->GarbageCollect();
    }

    if (m_Events)
    {
      m_Events->GarbageCollect();
    }
    
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
      m_PhSystem->GetNeighborhoodExtraction().Run(Vec3(0.0, 0.0, 0.0), 10.0);
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

    ioProfiling.m_CurFrameTime = 1000 * double(Clock::GetTimestamp() - m_CurrentTimestamp) / Clock::GetTicksPerSecond();
  }

  double World::GetRealTimeInSec()
  {
    return double(m_ElapsedGameTime) / Clock::GetTicksPerSecond();
  }

  void World::AddTick(Stage iStage, TickDelegate&& iDelegate)
  {
    m_Tick[iStage].emplace_back(std::move(iDelegate));
  }

  TimerHandle World::AddTimer(float iTimeInSec, bool iLoop, std::function<void(World&)>&& iDelegate)
  {
    TimerSchedule schedule;
    schedule.nextTick = m_CurrentTimestamp + uint64_t(Clock::GetTicksPerSecond() * iTimeInSec);

    TimerHandle handle = m_Timers.Alloc();
    TimerDesc& desc = m_Timers.Get(handle);
    desc.loop = iLoop;
    desc.time = iTimeInSec;
    desc.timerDelegate = std::move(iDelegate);
    desc.gameTimer = false;

    schedule.timer = handle;

    m_TimerSchedule.push_back(schedule);
    std::push_heap(m_TimerSchedule.begin(), m_TimerSchedule.end());

    return handle;
  }

  TimerHandle World::AddGameTimer(float iTimeInSec, bool iLoop, std::function<void(World&)>&& iDelegate)
  {
    GameTimerSchedule schedule;
    schedule.nextTick = m_ElapsedGameTime + iTimeInSec;

    TimerHandle handle = m_Timers.Alloc();
    TimerDesc& desc = m_Timers.Get(handle);
    desc.loop = iLoop;
    desc.time = iTimeInSec;
    desc.timerDelegate = std::move(iDelegate);
    desc.gameTimer = true;

    schedule.timer = handle;

    m_GameTimerSchedule.push_back(schedule);
    std::push_heap(m_GameTimerSchedule.begin(), m_GameTimerSchedule.end());

    return handle;
  }

  void World::RemoveTimer(TimerHandle iHandle)
  {
    m_Timers.Release(iHandle);
  }

  void World::ProcessTimers()
  {
    while (!m_TimerSchedule.empty() && m_TimerSchedule.front().nextTick <= m_CurrentTimestamp)
    {
      TimerHandle curTimer = m_TimerSchedule.front().timer;
      TimerDesc const& desc = m_Timers.Get(curTimer);
      std::pop_heap(m_TimerSchedule.begin(), m_TimerSchedule.end());
      m_TimerSchedule.pop_back();
      desc.timerDelegate(*this);
      // Account for in-callback deletion
      if (!m_Timers.IsValid(curTimer))
      {
        continue;
      }

      if (desc.loop)
      {
        m_TimerSchedule.push_back(TimerSchedule({ m_CurrentTimestamp + uint64_t(Clock::GetTicksPerSecond() * desc.time), curTimer}));
        std::push_heap(m_TimerSchedule.begin(), m_TimerSchedule.end());
      }
      else
      {
        m_Timers.Release(curTimer);
      }
    }

    while (!m_GameTimerSchedule.empty() && m_GameTimerSchedule.front().nextTick <= m_ElapsedGameTime)
    {
      TimerHandle curTimer = m_GameTimerSchedule.front().timer;
      TimerDesc const& desc = m_Timers.Get(curTimer);
      std::pop_heap(m_GameTimerSchedule.begin(), m_GameTimerSchedule.end());
      m_GameTimerSchedule.pop_back();
      desc.timerDelegate(*this);
      // Account for in-callback deletion
      if (!m_Timers.IsValid(curTimer))
      {
        continue;
      }

      if (desc.loop)
      {
        m_GameTimerSchedule.push_back({ m_ElapsedGameTime + desc.time, curTimer });
        std::push_heap(m_GameTimerSchedule.begin(), m_GameTimerSchedule.end());
      }
      else
      {
        m_Timers.Release(curTimer);
      }
    }
  }
}