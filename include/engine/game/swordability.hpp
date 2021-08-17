#pragma once

#include <engine/game/ability.hpp>

namespace eXl
{
  class SwordAbility;

  struct SwordAbilityState : public AbilityState
  {
    EXL_REFLECT;

    friend SwordAbility;

  public:
    Vector2f m_SwingDirection;
    
  private:
    ObjectHandle m_SwordActor;
    bool m_Swinging = false;
  };

  class EXL_ENGINE_API SwordAbility : public AbilityDescT<SwordAbilityState>
  {
    DECLARE_RTTI(SwordAbility, AbilityDesc);
  public:

    static AbilityName Name();
    static GameCueName UseSwordCue();

    SwordAbility();

    static void SetSwingDirection(AbilitySystem* iSys, ObjectHandle iObj, Vector2f const& iDir);

    AbilityUseState GetUseState(AbilityStateHandle iId) override;
    AbilityUseState Use(AbilityStateHandle iId, ObjectHandle) override;
    AbilityUseState StopUsing(AbilityStateHandle iId) override;

    AbilityStateHandle AddTo(ObjectHandle iUser) override;
    void Remove(AbilityStateHandle iToRemove) override;
    //void OnTagChanged(AbilityStateHandle iId, TagChange const& iChange) override;

  protected:

    void UpdateController(SwordAbilityState& iState);
  };
}