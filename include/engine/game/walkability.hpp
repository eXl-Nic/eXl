#pragma once

#include <engine/game/ability.hpp>

namespace eXl
{
  class WalkAbility;

  struct WalkAbilityState : public AbilityState
  {
    EXL_REFLECT;

    friend WalkAbility;

  public:
    Vector2f m_WalkDirection;
    float m_WalkSpeed = 10.0;
  private:

    Vector2f GetActualWalkDir();

    bool m_Walking = false;
    bool m_XLocked = false;
    bool m_YLocked = false;
    bool m_AnimLocked = false;
  };

  class EXL_ENGINE_API WalkAbility : public AbilityDescT<WalkAbilityState>
  {
    DECLARE_RTTI(WalkAbility, AbilityDesc);
  public:

    static GameTagName XLocked();
    static GameTagName YLocked();
    static AbilityName Name();

    WalkAbility();

    static void SetWalkDirection(AbilitySystem* iSys, ObjectHandle iObj, Vector2f const& iDir);

    AbilityUseState GetUseState(AbilityStateHandle iId) override;
    AbilityUseState Use(AbilityStateHandle iId, ObjectHandle) override;
    AbilityUseState StopUsing(AbilityStateHandle iId) override;
    void OnTagChanged(AbilityStateHandle iId, TagChange const& iChange) override;

  protected:

    void UpdateController(WalkAbilityState& iState);
  };
}