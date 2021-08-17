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
    m_ApplyUserTags.insert(DunAtk::WalkingTag());
    m_BlockedByTags.insert(DunAtk::AnimLocked());
    m_WatchedTags.insert(DunAtk::AnimLocked());
    m_WatchedTags.insert(XLocked());
    m_WatchedTags.insert(YLocked());
  }

  Vector2f WalkAbilityState::GetActualWalkDir()
  {
    if (m_AnimLocked)
    {
      return Vector2f::ZERO;
    }

    Vector2f walkDir = m_WalkDirection;
    if (m_XLocked)
    {
      walkDir.Y() = 0;
    }
    if (m_YLocked)
    {
      walkDir.X() = 0;
    }

    walkDir.Normalize();

    return walkDir;
  }

  void WalkAbility::SetWalkDirection(AbilitySystem* iSys, ObjectHandle iObj, Vector2f const& iDir)
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

    if (m_System->HasTag(state.m_User, DunAtk::AnimLocked()))
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
    Vector2f actualDir = iState.GetActualWalkDir();

    if (actualDir != Vector2f::ZERO)
    {
      controller.SetCurDir(iState.m_User, Vector3f(actualDir.X(), actualDir.Y(), 0.0));
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
      if (iChange.m_Object == DunAtk::AnimLocked())
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
      if (iChange.m_Object == DunAtk::AnimLocked())
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