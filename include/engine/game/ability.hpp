/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include <engine/enginelib.hpp>
#include <engine/gfx/tileset.hpp>
#include <engine/common/world.hpp>

namespace eXl
{
  class World;
  class Tileset;
  class GfxSpriteComponent;

  MAKE_NAME(AbilityName);
  MAKE_NAME(EffectName);
  MAKE_NAME(GameTagName);
  MAKE_NAME(GameEventName);
  MAKE_NAME(GameCueName);

  class AbilityDesc;

  struct EffectState
  {
    ObjectHandle m_Target;
  };

  enum class AbilityUseState
  {
    None,
    CannotUse,
    Using,
    OnCooldown,
  };

  struct AbilityState
  {
    ObjectHandle m_User;
    ObjectHandle m_Target;
    AbilityUseState m_UseState = AbilityUseState::None;
  };

  template <typename T>
  struct ChangeT
  {
    enum Change
    {
      Added,
      Removed,
      Fired,
    };

    ChangeT() = default;
    ChangeT(T iObject, Change iChange)
      : m_Object(iObject)
      , m_Change(iChange)
    {}

    bool operator == (ChangeT const& iOther) const
    {
      return m_Object == iOther.m_Object && m_Change == iOther.m_Change;
    }

    T m_Object;
    Change m_Change;
  };

  template <typename T>
  inline size_t hash_value(ChangeT<T> const& iVal)
  {
    size_t seed = iVal.m_Change;
    boost::hash_combine(seed, hash_value(iVal.m_Object));
    return seed;
  }

  template <typename T>
  inline size_t hash_value(ChangeT<T*> const& iVal)
  {
    size_t seed = iVal.m_Change;
    boost::hash_combine(seed, (ptrdiff_t)iVal.m_Object);
    return seed;
  }

  using TagChange = ChangeT<GameTagName>;
  using AbilityUseChange = ChangeT<AbilityDesc*>;
  using GameCueChange = ChangeT<GameCueName>;

  using EffectTable = ObjectTable<EffectState>;
  using EffectHandle = EffectTable::Handle;

  using AbilityStateTable = ObjectTable<AbilityState>;
  using AbilityStateHandle = AbilityStateTable::Handle;

  class AbilitySystem;

  class EXL_ENGINE_API EffectDesc : public RttiObject
  {
    DECLARE_RTTI(EffectDesc, RttiObject);
  public:

    EffectName GetName() const { return m_Name; }

    void Register(AbilitySystem& iSys)
    {
      m_System = &iSys;
    }

    virtual void Apply(EffectHandle)
    {}
    virtual void Tick(float iDelta)
    {}
    virtual void Remove(EffectHandle)
    {}
    virtual void EraseEffect(EffectHandle) = 0;

  protected:
    friend AbilitySystem;

    void ApplyTags(EffectState& iState);
    void RemoveTags(EffectState& iState);

    EffectDesc(EffectName iName)
      : m_Name(iName)
    {}
    EffectName m_Name;
    AbilitySystem* m_System;
    UnorderedSet<GameTagName> m_AppliedTags;
  };

  template <typename StateType>
  class EffectDescT : public EffectDesc
  {
  public:

    void Apply(EffectHandle iId) override;
    void Remove(EffectHandle iToRemove) override;

    bool IsValid(EffectHandle iId)
    {
      return m_States.IsValid(reinterpret_cast<typename ObjectTable<StateType>::Handle&>(iId));
    }

    StateType& Get(EffectHandle iId)
    {
      return m_States.Get(reinterpret_cast<typename ObjectTable<StateType>::Handle&>(iId));
    }

    EffectHandle Create(ObjectHandle iTarget)
    {
      auto handle = m_States.Alloc();
      auto& state = m_States.Get(handle);
      state.m_Target = iTarget;
      return reinterpret_cast<EffectHandle&>(handle);
    }

    void EraseEffect(EffectHandle iId) override
    {
      m_States.Release(reinterpret_cast<typename ObjectTable<StateType>::Handle&>(iId));
    }

  protected:  

    EffectDescT(EffectName iName)
      : EffectDesc(iName)
    {}
    
    ObjectTable<StateType> m_States;
  };

  class EXL_ENGINE_API AbilityDesc : public RttiObject
  {
    DECLARE_RTTI(AbilityDesc, RttiObject);
  public:

    AbilityName GetName() const { return m_Name; }

    void Register(AbilitySystem& iSys)
    {
      m_System = &iSys;
    }

    virtual bool CanUse(AbilitySystem& iSystem, ObjectHandle iUser, ObjectHandle& ioTarget)
    {
      return IsUserValid(iUser) && (!m_RequireTarget || IsTargetValid(ioTarget));
    };

    virtual void OnTagChanged(AbilityStateHandle iId, TagChange const& iChange)
    {
    }

    virtual AbilityUseState GetUseState(AbilityStateHandle iId)
    {
      return AbilityUseState::None;
    }
    virtual AbilityStateHandle AddTo(ObjectHandle iObject)
    {
      return AbilityStateHandle();
    }
    virtual AbilityUseState Use(AbilityStateHandle iId, ObjectHandle iTarget)
    {
      return AbilityUseState::Using;
    }
    virtual AbilityUseState StopUsing(AbilityStateHandle iId)
    {
      return AbilityUseState::None;
    }
    virtual void Tick(float iDelta)
    {}
    virtual void Remove(AbilityStateHandle iId)
    {}

    bool IsUserValid(ObjectHandle iUser) const;
    bool IsTargetValid(ObjectHandle iTarget) const;

    void Using_Begin(AbilityState& iState, ObjectHandle iTarget) const;
    void StopUsing_End(AbilityState& iState) const;

  protected:
    friend AbilitySystem;

    AbilityDesc(AbilityName iName)
      : m_Name(iName)
    {}

    UnorderedSet<GameTagName> m_WatchedTags;

    UnorderedSet<GameTagName> m_ApplyUserTags;
    UnorderedSet<GameTagName> m_RequireUserTags;
    UnorderedSet<GameTagName> m_BlockedByTags;
    UnorderedSet<GameTagName> m_ApplyTargetTags;
    UnorderedSet<GameTagName> m_RequireTargetTags;
    UnorderedSet<GameTagName> m_BlockedByTargetTags;
    bool m_RequireTarget = false;

    AbilityName m_Name;
    AbilitySystem* m_System;
  };

  template <typename StateType>
  class AbilityDescT : public AbilityDesc
  {
  public:

    bool IsValid(AbilityStateHandle iId)
    {
      return m_States.IsValid(reinterpret_cast<typename ObjectTable<StateType>::Handle&>(iId));
    }

    StateType& Get(AbilityStateHandle iId)
    {
      return m_States.Get(reinterpret_cast<typename ObjectTable<StateType>::Handle&>(iId));
    }

    StateType const& Get(AbilityStateHandle iId) const
    {
      return const_cast<AbilityDescT<StateType>*>(this)->Get(iId);
    }

    AbilityStateHandle AddTo(ObjectHandle iUser)
    {
      auto handle = m_States.Alloc();
      auto& state = m_States.Get(handle);
      state.m_User = iUser;
      return reinterpret_cast<AbilityStateHandle&>(handle);
    }

    void Remove(AbilityStateHandle iToRemove) override
    {
      EraseState(reinterpret_cast<typename ObjectTable<StateType>::Handle const&>(iToRemove));
    }

    AbilityUseState Use(AbilityStateHandle iId, ObjectHandle iTarget) override;
    AbilityUseState StopUsing(AbilityStateHandle iId) override;

  protected:

    void EraseState(typename ObjectTable<StateType>::Handle iId)
    {
      m_States.Release(iId);
    }

    AbilityDescT(AbilityName iName)
      : AbilityDesc(iName)
    {}

    ObjectTable<StateType> m_States;
  };

  class EXL_ENGINE_API AbilitySystem : public ComponentManager
  {
    DECLARE_RTTI(AbilitySystem, ComponentManager);
  public:
    AbilitySystem();

    using ComponentManager::GetWorld;

    void CreateComponent(ObjectHandle);
    void DeleteComponent(ObjectHandle) override;

    void RegisterAbility(AbilityDesc* iAbility);
    void RegisterEffect(EffectDesc* iEffect);

    template <typename T>
    T* GetAbilityDesc(AbilityName iName)
    {
      auto iterAbility = m_Abilities.find(iName);
      if (iterAbility != m_Abilities.end())
      {
        return T::DynamicCast(iterAbility->second);
      }
      return nullptr;
    }

    void NewFrame();
    void Tick(float iDelta);
    void AddCueChangeCallback(std::function<void(ObjectHandle, GameCueChange const&)>&& iCallback);

    // External API
    template <typename Effect, typename Functor>
    EffectHandle CreateEffect(ObjectHandle iObject, EffectName iName, Functor&& iFunctor)
    {
      if (!GetWorld().IsObjectValid(iObject) || m_Entries.size() <= iObject.GetId())
      {
        return EffectHandle();
      }

      auto iterEffect = m_Effects.find(iName);
      if (iterEffect != m_Effects.end())
      {
        Effect* handler = Effect::DynamicCast(iterEffect->second);
        eXl_ASSERT(handler != nullptr);
        EffectHandle newEffect = iFunctor(iObject, *handler);
        if (newEffect.IsAssigned())
        {
          AddCreatedEffect(iObject, handler, newEffect);
        }
        return newEffect;
      }

      return EffectHandle();
    }

    void RemoveEffect(ObjectHandle iObject, EffectName iName, EffectHandle iId);

    bool HasTag(ObjectHandle iObject, GameTagName iName);

    AbilityUseState UseAbility(ObjectHandle iObject, AbilityName iName, ObjectHandle iTarget = ObjectHandle());
    AbilityUseState StopUsingAbility(ObjectHandle iObject, AbilityName iName);

    void AddAbility(ObjectHandle iObject, AbilityName iName);
    void RemoveAbility(ObjectHandle iObject, AbilityName iName);
    bool HasAbility(ObjectHandle iObject, AbilityName iName);
    AbilityStateHandle GetAbilityState(ObjectHandle iObject, AbilityName iName);
    AbilityUseState GetAbilityUseState(ObjectHandle iObject, AbilityName iName);

    void StartCue(ObjectHandle iObj, GameCueName);
    void EndCue(ObjectHandle iObj, GameCueName);
    void FireCue(ObjectHandle iObj, GameCueName);

    // Internal API
    void AddTag(ObjectHandle iObject, GameTagName iTag);
    void RemoveTag(ObjectHandle iObject, GameTagName iTag);

    void AddCreatedEffect(ObjectHandle iObject, EffectDesc* iEffect, EffectHandle iNewEffect);

  protected:

    void ProcessCue(ObjectHandle iObj, GameCueChange const& iChange);

    struct ChangeFrameGuard
    {
    public:
      ChangeFrameGuard(AbilitySystem* iSys);
      ~ChangeFrameGuard();
    protected:
      AbilitySystem* m_Sys;
    };

    struct DelayedRemoval
    {
      ObjectHandle object;
      EffectDesc* effect;
      EffectHandle effectId;
    };

    struct Entry
    {
      UnorderedMap<AbilityName, AbilityStateHandle> m_Abilities;

      UnorderedMap<GameTagName, uint32_t> m_Tags;
      UnorderedMap<EffectName, UnorderedSet<EffectHandle>> m_AppliedEffects;
      UnorderedMap<GameTagName, UnorderedSet<AbilityDesc*>> m_WatchedTags;
      UnorderedSet<GameCueName> m_ActiveCues;

      void DispatchTagChange(AbilitySystem* iSystem, TagChange const& iChange);
      void AddWatchedTags(AbilityDesc* iDesc);
      void RemoveWatchedTags(AbilityDesc* iDesc);
    };

    struct ChangeFrame
    {
      UnorderedSet<std::pair<Entry*, TagChange>> m_TagChanges;
      UnorderedSet<std::pair<Entry*, AbilityUseChange>> m_AbilityWatchChanges;
      Vector<std::pair<ObjectHandle, GameCueChange>> m_Cues;
      Vector<DelayedRemoval> m_EffectRemoval;
    };

    void PushChangeFrame();
    void PopChangeFrame();

    Vector<ChangeFrame> m_ChangeFrames;
    uint32_t m_CurChangeFrame = 0;

    void AddEffectInternal(ObjectHandle iObject, EffectName iName, EffectHandle iId);
    void RemoveEffectInternal(ObjectHandle iObject, EffectDesc* iEffect, EffectHandle iId);

    UnorderedMap<EffectName, EffectDesc*> m_Effects;
    UnorderedMap<AbilityName, AbilityDesc*> m_Abilities;

    Vector<Entry> m_Entries;

    // Might need a filter per cue name eventually.
    Vector<std::function<void(ObjectHandle, GameCueChange const&)>> m_CueChangeCallbacks;
  };

  template <typename T>
  void EffectDescT<T>::Apply(EffectHandle iId)
  {
    ApplyTags(Get(iId));
  }

  template <typename T>
  void EffectDescT<T>::Remove(EffectHandle iId)
  {
    RemoveTags(Get(iId));
  }

  template <typename T>
  AbilityUseState AbilityDescT<T>::Use(AbilityStateHandle iId, ObjectHandle iTarget)
  {
    AbilityState& state = Get(iId);
    Using_Begin(state, iTarget);
    return AbilityUseState::Using;
  }

  template <typename T>
  AbilityUseState AbilityDescT<T>::StopUsing(AbilityStateHandle iId)
  {
    AbilityState& state = Get(iId);
    StopUsing_End(state);
    return AbilityUseState::None;
  }

}