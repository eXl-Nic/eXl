/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/ability.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(AbilitySystem);
  IMPLEMENT_RTTI(EffectDesc);
  IMPLEMENT_RTTI(AbilityDesc);

  void EffectDesc::ApplyTags(EffectState& iState)
  {
    ObjectHandle target = iState.m_Target;
    for (auto tag : m_AppliedTags)
    {
      m_System->AddTag(target, tag);
    }
  }

  void EffectDesc::RemoveTags(EffectState& iState)
  {
    ObjectHandle target = iState.m_Target;
    for (auto tag : m_AppliedTags)
    {
      m_System->RemoveTag(target, tag);
    }
  }

  bool AbilityDesc::IsUserValid(ObjectHandle iUser) const
  {
    if (m_System->GetWorld().IsObjectValid(iUser))
    {
      for (auto tag : m_BlockedByTags)
      {
        if (m_System->HasTag(iUser, tag))
        {
          return false;
        }
      }
      for (auto tag : m_RequireUserTags)
      {
        if (!m_System->HasTag(iUser, tag))
        {
          return false;
        }
      }
      return true;
    }
    return false;
  }

  bool AbilityDesc::IsTargetValid(ObjectHandle iTarget) const
  {
    if (m_System->GetWorld().IsObjectValid(iTarget))
    {
      for (auto tag : m_BlockedByTargetTags)
      {
        if (m_System->HasTag(iTarget, tag))
        {
          return false;
        }
      }
      for (auto tag : m_RequireTargetTags)
      {
        if (!m_System->HasTag(iTarget, tag))
        {
          return false;
        }
      }
      return true;
    }
    return false;
  }

  void AbilityDesc::Using_Begin(AbilityState& iState, ObjectHandle iTarget) const
  {
    for (auto tag : m_ApplyUserTags)
    {
      m_System->AddTag(iState.m_User, tag);
    }
    if (m_RequireTarget)
    {
      iState.m_Target = iTarget;
      for (auto tag : m_ApplyTargetTags)
      {
        m_System->AddTag(iState.m_Target, tag);
      }
    }
  }

  void AbilityDesc::StopUsing_End(AbilityState& iState) const
  {
    for (auto tag : m_ApplyUserTags)
    {
      m_System->RemoveTag(iState.m_User, tag);
    }
    if (m_RequireTarget)
    {
      for (auto tag : m_ApplyTargetTags)
      {
        m_System->RemoveTag(iState.m_Target, tag);
      }
      iState.m_Target = ObjectHandle();
    }
  }

  AbilitySystem::AbilitySystem()
  {
    m_Entries.reserve(4096);
    m_ChangeFrames.resize(16);
  }

  void AbilitySystem::CreateComponent(ObjectHandle iObject)
  {
    if (!GetWorld().IsObjectValid(iObject))
    {
      return;
    }

    ComponentManager::CreateComponent(iObject);
    while (m_Entries.size() <= iObject.GetId())
    {
      m_Entries.emplace_back(Entry());
    }

    Entry& newEntry = m_Entries[iObject.GetId()];
    newEntry.m_Abilities.clear();
    newEntry.m_AppliedEffects.clear();
    newEntry.m_Tags.clear();
  }

  void AbilitySystem::DeleteComponent(ObjectHandle iObject)
  {
    if (m_Entries.size() <= iObject.GetId())
    {
      return;
    }

    Entry& toClear = m_Entries[iObject.GetId()];

    for (auto const& effectCat : toClear.m_AppliedEffects)
    {
      for (auto const& effect : effectCat.second)
      {
        RemoveEffect(iObject, effectCat.first, effect);
      }
    }

    ComponentManager::DeleteComponent(iObject);
  }

  void AbilitySystem::RegisterAbility(AbilityDesc* iAbility)
  {
    if (m_Abilities.find(iAbility->GetName()) == m_Abilities.end())
    {
      iAbility->Register(*this);
      m_Abilities.insert(std::make_pair(iAbility->GetName(), iAbility));
    }
  }

  void AbilitySystem::RegisterEffect(EffectDesc* iEffect)
  {
    if (m_Effects.find(iEffect->GetName()) == m_Effects.end())
    {
      iEffect->Register(*this);
      m_Effects.insert(std::make_pair(iEffect->GetName(), iEffect));
    }
  }

  void AbilitySystem::NewFrame()
  {
  }

  void AbilitySystem::Tick(float iDelta)
  {
    for (auto const& abilityEntry : m_Abilities)
    {
      abilityEntry.second->Tick(iDelta);
    }

    for (auto const& effectEntry : m_Effects)
    {
      ChangeFrameGuard guard(this);
      effectEntry.second->Tick(iDelta);
    }
  }

  void AbilitySystem::AddCueChangeCallback(std::function<void(ObjectHandle, GameCueChange const&)>&& iCallback)
  {
    m_CueChangeCallbacks.emplace_back(std::move(iCallback));
  }

  void AbilitySystem::Entry::AddWatchedTags(AbilityDesc* iDesc)
  {
    for (auto tag : iDesc->m_WatchedTags)
    {
      auto iterTags = m_WatchedTags.insert(std::make_pair(tag, UnorderedSet<AbilityDesc*>())).first;
      iterTags->second.insert(iDesc);
    }
  }

  void AbilitySystem::Entry::RemoveWatchedTags(AbilityDesc* iDesc)
  {
    for (auto tag : iDesc->m_WatchedTags)
    {
      auto iterTags = m_WatchedTags.find(tag);
      if (iterTags != m_WatchedTags.end())
      {
        iterTags->second.erase(iDesc);
        if (iterTags->second.empty())
        {
          m_WatchedTags.erase(iterTags);
        }
      }
    }
  }

  AbilityUseState AbilitySystem::UseAbility(ObjectHandle iObject, AbilityName iName, ObjectHandle iTarget)
  {
    if (!GetWorld().IsObjectValid(iObject) || m_Entries.size() <= iObject.GetId())
    {
      return AbilityUseState::CannotUse;
    }

    Entry& entry = m_Entries[iObject.GetId()];
    auto iterAbilityState = entry.m_Abilities.find(iName);
    if (iterAbilityState != entry.m_Abilities.end())
    {
      auto iterAbility = m_Abilities.find(iName);
      if (iterAbility != m_Abilities.end())
      {
        AbilityDesc* desc = iterAbility->second;
        AbilityStateHandle stateHandle = iterAbilityState->second;
        AbilityUseState state = desc->GetUseState(stateHandle);
        if (state == AbilityUseState::None && desc->CanUse(*this, iObject, iTarget))
        {
          {
            ChangeFrameGuard guard(this);
            state = desc->Use(iterAbilityState->second, iTarget);
          }

          if (m_CurChangeFrame == 0)
          {
            entry.AddWatchedTags(desc);
          }
          else
          {
            ChangeFrame& curFrame = m_ChangeFrames[m_CurChangeFrame - 1];
            curFrame.m_AbilityWatchChanges.insert(std::make_pair(&entry, AbilityUseChange(desc, AbilityUseChange::Added)));
          }
        }
        return state;
      }
    }

    return AbilityUseState::CannotUse;
  }

  AbilityUseState AbilitySystem::StopUsingAbility(ObjectHandle iObject, AbilityName iName)
  {
    if (!GetWorld().IsObjectValid(iObject) || m_Entries.size() <= iObject.GetId())
    {
      return AbilityUseState::CannotUse;
    }

    Entry& entry = m_Entries[iObject.GetId()];
    auto iterAbilityState = entry.m_Abilities.find(iName);
    if (iterAbilityState != entry.m_Abilities.end())
    {
      auto iterAbility = m_Abilities.find(iName);
      if (iterAbility != m_Abilities.end())
      {
        AbilityDesc* desc = iterAbility->second;
        AbilityUseState state = desc->GetUseState(iterAbilityState->second);
        if (state == AbilityUseState::Using)
        {
          {
            ChangeFrameGuard guard(this);
            state = desc->StopUsing(iterAbilityState->second);
          }

          if (m_CurChangeFrame == 0)
          {
            entry.RemoveWatchedTags(desc);
          }
          else
          {
            ChangeFrame& curFrame = m_ChangeFrames[m_CurChangeFrame - 1];
            curFrame.m_AbilityWatchChanges.insert(std::make_pair(&entry, AbilityUseChange(desc, AbilityUseChange::Removed)));
          }
        }
        return state;
      }
    }

    return AbilityUseState::CannotUse;
  }

  void AbilitySystem::AddAbility(ObjectHandle iObject, AbilityName iName)
  {
    if (!GetWorld().IsObjectValid(iObject) || m_Entries.size() <= iObject.GetId())
    {
      return;
    }
    Entry& entry = m_Entries[iObject.GetId()];
    if (entry.m_Abilities.find(iName) == entry.m_Abilities.end())
    {
      auto iterAbility = m_Abilities.find(iName);
      if (iterAbility != m_Abilities.end())
      {
        entry.m_Abilities.insert(std::make_pair(iName, iterAbility->second->AddTo(iObject)));
      }
    }
  }

  void AbilitySystem::RemoveAbility(ObjectHandle iObject, AbilityName iName)
  {
    if (!GetWorld().IsObjectValid(iObject) || m_Entries.size() <= iObject.GetId())
    {
      return;
    }
    Entry& entry = m_Entries[iObject.GetId()];
    auto iterAbilityState = entry.m_Abilities.find(iName);
    if (iterAbilityState != entry.m_Abilities.end())
    {
      auto iterAbility = m_Abilities.find(iName);
      if (iterAbility != m_Abilities.end())
      {
        iterAbility->second->Remove(iterAbilityState->second);
      }
      entry.m_Abilities.erase(iterAbilityState);
    }
  }

  bool AbilitySystem::HasAbility(ObjectHandle iObject, AbilityName iName)
  {
    if (!GetWorld().IsObjectValid(iObject) || m_Entries.size() <= iObject.GetId())
    {
      return false;
    }
    Entry& entry = m_Entries[iObject.GetId()];
    auto iterAbilityState = entry.m_Abilities.find(iName);
    if (iterAbilityState != entry.m_Abilities.end())
    {
      return true;
    }
    return false;
  }

  AbilityStateHandle AbilitySystem::GetAbilityState(ObjectHandle iObject, AbilityName iName)
  {
    if (!GetWorld().IsObjectValid(iObject) || m_Entries.size() <= iObject.GetId())
    {
      return AbilityStateHandle();
    }
    Entry& entry = m_Entries[iObject.GetId()];
    auto iterAbilityState = entry.m_Abilities.find(iName);
    if (iterAbilityState != entry.m_Abilities.end())
    {
      return iterAbilityState->second;
    }
    return AbilityStateHandle();
  }

  AbilityUseState AbilitySystem::GetAbilityUseState(ObjectHandle iObject, AbilityName iName)
  {
    if (!HasAbility(iObject, iName))
    {
      return AbilityUseState::CannotUse;
    }
    AbilityStateHandle handle = GetAbilityState(iObject, iName);
    if (handle.IsAssigned())
    {
      return m_Abilities[iName]->GetUseState(handle);
    }
    return AbilityUseState::None;
  }
  
  AbilitySystem::ChangeFrameGuard::ChangeFrameGuard(AbilitySystem* iSys)
  {
    m_Sys = iSys;
    m_Sys->PushChangeFrame();
  }
  void AbilitySystem::PushChangeFrame()
  {
    ++m_CurChangeFrame;
    eXl_ASSERT(m_ChangeFrames.size() > m_CurChangeFrame);
    //{
    //  m_ChangeFrames.push_back(ChangeFrame());
    //}
  }

  AbilitySystem::ChangeFrameGuard::~ChangeFrameGuard()
  {
    m_Sys->PopChangeFrame();
  }

  void AbilitySystem::Entry::DispatchTagChange(AbilitySystem* iSystem, TagChange const& iChange)
  {
    ChangeFrameGuard guard(iSystem);
    auto abilityEntry = m_WatchedTags.find(iChange.m_Object);
    if (abilityEntry != m_WatchedTags.end())
    {
      for (AbilityDesc* desc : abilityEntry->second)
      {
        desc->OnTagChanged(m_Abilities[desc->GetName()], iChange);
      }
    }
  }

  void AbilitySystem::PopChangeFrame()
  {
    eXl_ASSERT(m_CurChangeFrame > 0);
    ChangeFrame& curFrame = m_ChangeFrames[m_CurChangeFrame - 1];

    for (auto const& removal : curFrame.m_EffectRemoval)
    {
      RemoveEffectInternal(removal.object, removal.effect, removal.effectId);
    }
    curFrame.m_EffectRemoval.clear();

    for (auto const& tagChange : curFrame.m_TagChanges)
    {
      Entry* entry = tagChange.first;
      entry->DispatchTagChange(this, tagChange.second);
    }
    curFrame.m_TagChanges.clear();

    for (auto const& abilityUseChange : curFrame.m_AbilityWatchChanges)
    {
      Entry* entry = abilityUseChange.first;
      if (abilityUseChange.second.m_Change == AbilityUseChange::Added)
      {
        entry->AddWatchedTags(abilityUseChange.second.m_Object);
      }
      else
      {
        entry->RemoveWatchedTags(abilityUseChange.second.m_Object);
      }
    }
    curFrame.m_AbilityWatchChanges.clear();

    for (auto const& gameCueChange : curFrame.m_Cues)
    {
      for (auto const& callback : m_CueChangeCallbacks)
      {
        callback(gameCueChange.first, gameCueChange.second);
      }
    }
    curFrame.m_Cues.clear();

    --m_CurChangeFrame;
  }

  void AbilitySystem::StartCue(ObjectHandle iObj, GameCueName iName)
  {
    if (!GetWorld().IsObjectValid(iObj) || m_Entries.size() <= iObj.GetId())
    {
      return ;
    }
    Entry& entry = m_Entries[iObj.GetId()];
    eXl_ASSERT_REPAIR_RET(entry.m_ActiveCues.count(iName) == 0, );
    entry.m_ActiveCues.insert(iName);

    GameCueChange change(iName, GameCueChange::Added);
    ProcessCue(iObj, change);
  }

  void AbilitySystem::EndCue(ObjectHandle iObj, GameCueName iName)
  {
    if (!GetWorld().IsObjectValid(iObj) || m_Entries.size() <= iObj.GetId())
    {
      return;
    }
    Entry& entry = m_Entries[iObj.GetId()];
    auto iterCue = entry.m_ActiveCues.find(iName);
    eXl_ASSERT_REPAIR_RET(iterCue != entry.m_ActiveCues.end(), );
    entry.m_ActiveCues.erase(iterCue);

    GameCueChange change(iName, GameCueChange::Removed);
    ProcessCue(iObj, change);
  }

  void AbilitySystem::FireCue(ObjectHandle iObj, GameCueName iName)
  {
    if (!GetWorld().IsObjectValid(iObj) || m_Entries.size() <= iObj.GetId())
    {
      return;
    }
    GameCueChange change(iName, GameCueChange::Fired);
    ProcessCue(iObj, change);
  }

  void AbilitySystem::ProcessCue(ObjectHandle iObj, GameCueChange const& iChange)
  {
    if (m_CurChangeFrame == 0)
    {
      for (auto const& callback : m_CueChangeCallbacks)
      {
        callback(iObj, iChange);
      }
    }
    else
    {
      ChangeFrame& curFrame = m_ChangeFrames[m_CurChangeFrame - 1];
      curFrame.m_Cues.push_back(std::make_pair(iObj, iChange));
    }
  }

  bool AbilitySystem::HasTag(ObjectHandle iObject, GameTagName iName)
  {
    if (!GetWorld().IsObjectValid(iObject) || m_Entries.size() <= iObject.GetId())
    {
      return false;
    }
    return m_Entries[iObject.GetId()].m_Tags.count(iName) > 0;
  }

  void AbilitySystem::AddTag(ObjectHandle iObject, GameTagName iTag)
  {
    if (!GetWorld().IsObjectValid(iObject) || m_Entries.size() <= iObject.GetId())
    {
      return;
    }
    Entry& entry = m_Entries[iObject.GetId()];
    auto tagEntry = entry.m_Tags.insert(std::make_pair(iTag, 0)).first;
    if (tagEntry->second == 0)
    {
      TagChange change(iTag, TagChange::Added);
      if (m_CurChangeFrame > 0)
      {
        ChangeFrame& curFrame = m_ChangeFrames[m_CurChangeFrame - 1];
        auto iter = curFrame.m_TagChanges.find(std::make_pair(&entry, TagChange(iTag, TagChange::Removed)));
        if (iter != curFrame.m_TagChanges.end())
        {
          curFrame.m_TagChanges.erase(iter);
        }
        else
        {
          curFrame.m_TagChanges.insert(std::make_pair(&entry, change));
        }
      }
      else
      {
        entry.DispatchTagChange(this, change);
      }
    }
    ++tagEntry->second;
  }

  void AbilitySystem::RemoveTag(ObjectHandle iObject, GameTagName iTag)
  {
    if (!GetWorld().IsObjectValid(iObject) || m_Entries.size() <= iObject.GetId())
    {
      return;
    }
    Entry& entry = m_Entries[iObject.GetId()];
    auto tagEntry = entry.m_Tags.find(iTag);
    if (tagEntry != entry.m_Tags.end())
    {
      if (--tagEntry->second == 0)
      {
        entry.m_Tags.erase(tagEntry);

        TagChange change(iTag, TagChange::Removed);
        if (m_CurChangeFrame > 0)
        {
          ChangeFrame& curFrame = m_ChangeFrames[m_CurChangeFrame - 1];
          auto iter = curFrame.m_TagChanges.find(std::make_pair(&entry, TagChange(iTag, TagChange::Added)));
          if (iter != curFrame.m_TagChanges.end())
          {
            curFrame.m_TagChanges.erase(iter);
          }
          else
          {
            curFrame.m_TagChanges.insert(std::make_pair(&entry, change));
          }
        }
        else
        {
          entry.DispatchTagChange(this, change);
        }
      }
    }
  }

  void AbilitySystem::AddCreatedEffect(ObjectHandle iObject, EffectDesc* iEffect, EffectHandle iNewEffect)
  {
    AddEffectInternal(iObject, iEffect->GetName(), iNewEffect);
    {
      ChangeFrameGuard guard(this);
      iEffect->Apply(iNewEffect);
    }
  }

  void AbilitySystem::RemoveEffect(ObjectHandle iObject, EffectName iName, EffectHandle iId)
  {
    if (!GetWorld().IsObjectValid(iObject) || m_Entries.size() <= iObject.GetId())
    {
      return;
    }

    auto iterEffect = m_Effects.find(iName);
    if (iterEffect != m_Effects.end())
    {
      {
        ChangeFrameGuard guard(this);
        iterEffect->second->Remove(iId);
      }
      
      if (m_CurChangeFrame > 0)
      {
        DelayedRemoval removal;
        removal.object = iObject;
        removal.effect = iterEffect->second;
        removal.effectId = iId;

        m_ChangeFrames[m_CurChangeFrame - 1].m_EffectRemoval.push_back(removal);
      }
      else
      {
        RemoveEffectInternal(iObject, iterEffect->second, iId);
      }
    }
  }

  void AbilitySystem::AddEffectInternal(ObjectHandle iObject, EffectName iName, EffectHandle iId)
  {
    Entry& entry = m_Entries[iObject.GetId()];
    auto effectEntry = entry.m_AppliedEffects.insert(std::make_pair(iName, UnorderedSet<EffectHandle>())).first;
    effectEntry->second.insert(iId);
  }

  void AbilitySystem::RemoveEffectInternal(ObjectHandle iObject, EffectDesc* iEffect, EffectHandle iId)
  {
    Entry& entry = m_Entries[iObject.GetId()];
    auto effectEntry = entry.m_AppliedEffects.find(iEffect->GetName());
    if (effectEntry != entry.m_AppliedEffects.end())
    {
      effectEntry->second.erase(iId);
      if (effectEntry->second.empty())
      {
        entry.m_AppliedEffects.erase(effectEntry);
      }
      iEffect->EraseEffect(iId);
    }
  }
}