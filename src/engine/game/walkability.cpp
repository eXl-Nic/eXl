/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/walkability.hpp>
#include <engine/game/commondef.hpp>
#include <engine/game/character.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(WalkAbility);

  WalkAbility::WalkAbility()
    : AbilityDescT<WalkAbilityState>(Name())
  {
    m_RequireTarget = false;
    m_ApplyUserTags.insert(EngineCommon::WalkingTag());
    m_BlockedByTags.insert(EngineCommon::AnimLocked());
    m_WatchedTags.insert(EngineCommon::AnimLocked());
    m_WatchedTags.insert(XLocked());
    m_WatchedTags.insert(YLocked());
  }

  Vec2 WalkAbilityState::GetActualWalkDir()
  {
    if (m_AnimLocked)
    {
      return Zero<Vec2>();
    }

    Vec2 walkDir = m_WalkDirection;
    if (m_XLocked)
    {
      walkDir.y = 0;
    }
    if (m_YLocked)
    {
      walkDir.x = 0;
    }

    walkDir = normalize(walkDir);

    return walkDir;
  }

  void WalkAbility::SetWalkDirection(AbilitySystem* iSys, ObjectHandle iObj, Vec2 const& iDir)
  {
    WalkAbility* self = iSys->GetAbilityDesc<WalkAbility>(Name());
    AbilityStateHandle stateHandle = iSys->GetAbilityState(iObj, Name());
    if (stateHandle.IsAssigned())
    {
      WalkAbilityState& state = self->Get(stateHandle);
      state.m_WalkDirection = iDir;
      if (state.m_Walking)
      {
        self->UpdateController(state);
      }
    }
  }
  
  AbilityUseState WalkAbility::GetUseState(AbilityStateHandle iId)
  {
    if(!IsValid(iId))
    { 
      return AbilityUseState::CannotUse;
    }
    return Get(iId).m_Walking ? AbilityUseState::Using : AbilityUseState::None;
  }

  AbilityUseState WalkAbility::Use(AbilityStateHandle iId, ObjectHandle iTarget)
  {
    WalkAbilityState& state = Get(iId);
    Using_Begin(state, iTarget);

    if (m_System->HasTag(state.m_User, XLocked()))
    {
      state.m_XLocked = true;
    }
    else
    {
      state.m_XLocked = false;
    }

    if (m_System->HasTag(state.m_User, YLocked()))
    {
      state.m_YLocked = true;
    }
    else
    {
      state.m_YLocked = false;
    }

    if (m_System->HasTag(state.m_User, EngineCommon::AnimLocked()))
    {
      state.m_AnimLocked = true;
    }
    else
    {
      state.m_AnimLocked = false;
    }

    UpdateController(state);
    state.m_Walking = true;

    return AbilityUseState::Using;
  }

  void WalkAbility::UpdateController(WalkAbilityState& iState)
  {
    World& world = m_System->GetWorld();
    CharacterSystem& controller = *world.GetSystem<CharacterSystem>();
    Vec2 actualDir = iState.GetActualWalkDir();

    if (actualDir != Zero<Vec2>())
    {
      controller.SetCurDir(iState.m_User, Vec3(actualDir.x, actualDir.y, 0.0));
      controller.SetSpeed(iState.m_User, iState.m_WalkSpeed);
    }
    else
    {
      controller.SetSpeed(iState.m_User, 0.0);
    }
  }

  AbilityUseState WalkAbility::StopUsing(AbilityStateHandle iId)
  {
    WalkAbilityState& state = Get(iId);
    World& world = m_System->GetWorld();

    CharacterSystem& controller = *world.GetSystem<CharacterSystem>();
    controller.SetSpeed(state.m_User, 0.0);
    state.m_Walking = false;

    StopUsing_End(state);
    return AbilityUseState::None;
  }

  void WalkAbility::OnTagChanged(AbilityStateHandle iId, TagChange const& iChange)
  {
    auto& state = Get(iId);
    if (iChange.m_Change == TagChange::Added)
    {
      if (iChange.m_Object == EngineCommon::AnimLocked())
      {
        state.m_AnimLocked = true;
        UpdateController(state);
        return;
      }
      if (iChange.m_Object == XLocked())
      {
        state.m_XLocked = true;
        UpdateController(state);
        return;
      }
      if (iChange.m_Object == YLocked())
      {
        state.m_YLocked = true;
        UpdateController(state);
        return;
      }
    }
    if (iChange.m_Change == TagChange::Removed)
    {
      if (iChange.m_Object == EngineCommon::AnimLocked())
      {
        state.m_AnimLocked = false;
        UpdateController(state);
        return;
      }
      if (iChange.m_Object == XLocked())
      {
        state.m_XLocked = false;
        UpdateController(state);
        return;
      }
      if (iChange.m_Object == YLocked())
      {
        state.m_YLocked = false;
        UpdateController(state);
        return;
      }
    }
  }
}