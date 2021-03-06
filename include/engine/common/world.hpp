/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <core/rtti.hpp>
#include <engine/common/object.hpp>
#include <engine/enginelib.hpp>
#include <core/type/typetraits.hpp>
#include <core/stream/serializer.hpp>
#include <functional>

namespace eXl
{
  struct ObjectInfo
  {
    String m_DisplayName;
    uint64_t m_PersistentId;
    uint32_t m_Components = 0;
    bool m_PendingDeletion = false;
  };

  struct ObjectCreationInfo
  {
    // Object won't be addressable
    static constexpr uint64_t s_AnonymousFlag = 1llu << 63;
    // A persistent Id will be created.
    static constexpr uint64_t s_AutoNamedFlag  = 1llu << 62;

    String m_DisplayName;
    uint64_t m_PersistentId = s_AnonymousFlag;

    static ObjectCreationInfo Anonymous(String const& iDebugName = String())
    {
      return ObjectCreationInfo{ iDebugName, s_AnonymousFlag };
    }

    static ObjectCreationInfo AutoNamed(String const& iDebugName = String())
    {
      return ObjectCreationInfo{iDebugName, s_AutoNamedFlag};
    }

    static ObjectCreationInfo Named(uint64_t iId, String const& iDebugName = String())
    {
      return ObjectCreationInfo{ iDebugName, iId };
    }
  };

  typedef ObjectTable<ObjectInfo> WorldObjects;
  typedef WorldObjects::Handle ObjectHandle;

  struct ObjectReference
  {
  public:

    void Set(uint64_t iId) 
    {
      m_Object = ObjectHandle();
      m_PersistentId = iId;
    }
    ObjectHandle Get() const { return m_Object; }
    ObjectHandle Resolve(World& iWorld) const;

    uint64_t GetPersistentId() const
    {
      return m_PersistentId;
    }

    bool operator ==(ObjectReference const& iOther) const
    {
      return m_PersistentId == iOther.m_PersistentId;
    }

    SERIALIZE_METHODS;

  private:
    uint64_t m_PersistentId = 0;
    mutable ObjectHandle m_Object;
  };


  DEFINE_TYPE_EX(ObjectHandle, ObjectHandle, EXL_ENGINE_API);
  DEFINE_TYPE_EX(ObjectReference, ObjectReference, EXL_ENGINE_API);

  template <>
  struct StreamerTemplateHandler<ObjectHandle>
  {
    static Err Do(Streamer& iStreamer, ObjectHandle const* iObj)
    {
      static const int i = 0;
      return iStreamer.WriteInt(&i);
      //return Err::Unexpected;
    }
  };

  template <>
  struct UnstreamerTemplateHandler<ObjectHandle>
  {
    static Err Do(Unstreamer& iStreamer, ObjectHandle* oObj)
    {
      *oObj = ObjectHandle();
      return Err::Success;
      //return Err::Unexpected;
    }
  };

  class World;

  struct ProfilingState
  {
    float m_LastFrameTime;
    float m_FrameStartTime;
    float m_NeighETime;
    float m_PrePhysicsTime;
    float m_PhysicTime;
    float m_PostPhysicsTime;
    float m_AbilitiesTime;
    float m_PostAbilitiesTime;
    float m_RendererTime;
    float m_TransformsTickTime;
    float m_CurFrameTime;
  };

  using TickDelegate = std::function<void(World&, float)>;

  struct TimerDesc
  {
    std::function<void(World&)> timerDelegate;
    float time;
    bool loop;
    bool gameTimer;
  };

  using TimerTable = ObjectTable<TimerDesc>;
  using TimerHandle = TimerTable::Handle;

  class EXL_ENGINE_API WorldSystem : public RttiObject
  {
    DECLARE_RTTI(WorldSystem, RttiObject);
  protected:
    friend class World;
    World& GetWorld() const
    {
      return *m_World;
    }
    virtual void Register(World& iWorld)
    {
      m_World = &iWorld;
    }
    World* m_World = nullptr;
  };

  class EXL_ENGINE_API ComponentManager : public WorldSystem
  {
    DECLARE_RTTI(ComponentManager, WorldSystem);

  protected:
    friend class World;    
    void CreateComponent(ObjectHandle);
    virtual void DeleteComponent(ObjectHandle);
    uint32_t m_SysIdx;
  };

  class PhysicsSystem;
  class AbilitySystem;
  class Transforms;
  class GameDatabase;
  class EventSystem;

  MAKE_NAME(PropertySheetName);
  MAKE_NAME(ComponentName);

  using ComponentFactory = std::function<void(World&, ObjectHandle)>;

  class EXL_ENGINE_API ComponentManifest : public RttiObject
  {
    DECLARE_RTTI(ComponentManifest, RttiObject);
  public:

    struct ComponentEntry
    {
      UnorderedSet<PropertySheetName> requiredData;
      ComponentFactory factory;
    };

    UnorderedSet<PropertySheetName> const* GetRequiredDataForComponent(ComponentName iName) const;
    ComponentFactory const* GetComponentFactory(ComponentName iName) const;
    Vector<ComponentName> GetComponents() const;
    void RegisterComponent(ComponentName iName, ComponentFactory iFactory, std::initializer_list<PropertySheetName> iReqData);

  protected:
    UnorderedMap<ComponentName, ComponentEntry> m_Components;
  };

  class EXL_ENGINE_API World
  {
  public:

    World(ComponentManifest const& iManifest);
    World(World const&) = delete;
    ~World();
    World& operator=(World const&) = delete;

    ComponentManifest const& GetComponents() const { return m_Components; }

    ObjectHandle CreateObject();

    ObjectHandle CreateObject(ObjectCreationInfo iInfo);

    ObjectHandle GetObjectFromPersistentId(uint64_t iId);

    void DeleteObject(ObjectHandle);

    ObjectInfo& GetObjectInfo(ObjectHandle iHandle);

    ObjectInfo* TryGetObjectInfo(ObjectHandle iHandle);

    bool IsObjectValid(ObjectHandle iHandle) const;

    bool IsObjectBeingDestroyed(ObjectHandle iHandle) const;

    template <typename T>
    T* AddSystem(std::unique_ptr<T>&& iSystem)
    {
      std::unique_ptr<WorldSystem> baseType(iSystem.release());
      return reinterpret_cast<T*>(AddSystem(std::move(baseType)));
    }
    WorldSystem* AddSystem(std::unique_ptr<WorldSystem>&&);

    template <typename T>
    T* GetSystem()
    {
      return static_cast<T*>(GetSystem(T::StaticRtti()));
    }

    WorldSystem* GetSystem(Rtti const& iRtti);

    enum Stage
    {
      FrameStart,
      //Timers
      //Ai
      PrePhysics,
      //Physics
      PostPhysics,
      //Abilities
      PostAbilites,

      NumStages
    };

    void AddTick(Stage iStage, TickDelegate&& iDelegate);

    void Tick(ProfilingState& ioProfiling);

    TimerHandle AddTimer(float iTimeInSec, bool iLoop, std::function<void(World&)>&& iDelegate);
    TimerHandle AddGameTimer(float iTimeInSec, bool iLoop, std::function<void(World&)>&& iDelegate);
    void RemoveTimer(TimerHandle);

    double GetRealTimeInSec();
    double GetGameTimeInSec() { return m_ElapsedGameTime; }

    float GetGameTimeScaling() const { return m_GameTimeScaling; }
    void SetGameTimeScaling(float iScaling) { m_GameTimeScaling = iScaling; }

    uint64_t GetCurrentTime() const { return m_CurrentTimestamp; }
    uint64_t GetElapsedTime() const { return m_ElapsedTimestamp; }

  protected:
    void FlushObjectsToDelete();

    void ProcessTimers();

    struct SysReg
    {
      std::unique_ptr<WorldSystem> m_System;
      uint32_t m_ComponentIdx;
    };

    WorldObjects m_Objects;
    UnorderedMap<uint64_t, ObjectHandle> m_PersistentIdToObjects;

    Vector<ObjectHandle> m_ObjectsToDelete;
    UnorderedMap<Rtti const*, SysReg> m_Systems;
    Vector<ComponentManager*> m_CompManagers;
    Vector<Vector<TickDelegate>> m_Tick;

    TimerTable m_Timers;
    struct TimerSchedule
    {
      uint64_t nextTick;
      TimerHandle timer;

      bool operator <(TimerSchedule const& iOther) const
      {
        return nextTick > iOther.nextTick;
      }
    };

    struct GameTimerSchedule
    {
      double nextTick;
      TimerHandle timer;

      bool operator <(GameTimerSchedule const& iOther) const
      {
        return nextTick > iOther.nextTick;
      }
    };
    Vector<TimerSchedule> m_TimerSchedule;
    Vector<GameTimerSchedule> m_GameTimerSchedule;

    ComponentManifest const& m_Components;

    GameDatabase* m_Database = nullptr;
    EventSystem* m_Events = nullptr;
    PhysicsSystem* m_PhSystem = nullptr;
    AbilitySystem* m_AbilitySystem = nullptr;
    Transforms* m_Transforms = nullptr;

    uint64_t m_StartTimestamp = 0;
    uint64_t m_CurrentTimestamp = 0;
    uint64_t m_ElapsedTimestamp = 0;

    double m_ElapsedGameTime = 0;
    float m_GameTimeScaling = 1.0;
  };
}