#pragma once

#include <engine/game/ability.hpp>

namespace eXl
{
  class ThrowAbility;
  struct ThrowAbilityState : public AbilityState
  {
    friend ThrowAbility;
  private:
    ObjectHandle m_ThrownObject;
    ObjectHandle m_ProjectileProxy;
    TimerHandle m_ThrowTimer;
  };

  class EXL_ENGINE_API ThrowAbility : public AbilityDescT<ThrowAbilityState>
  {
    DECLARE_RTTI(ThrowAbility, AbilityDesc);
  public:

    static GameCueName ThrowingCue();
    static AbilityName Name();

    ThrowAbility();

    AbilityUseState GetUseState(AbilityStateHandle iId) override;
    AbilityUseState Use(AbilityStateHandle iId, ObjectHandle iTarget) override;
    AbilityUseState StopUsing(AbilityStateHandle iId) override;

  protected:
  };

}