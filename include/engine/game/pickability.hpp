#pragma once

#include <engine/game/ability.hpp>
#include <engine/common/transformanim.hpp>

namespace eXl
{
  class PickAbility;
  struct PickAbilityState : public AbilityState
  {
    friend PickAbility;
  private:
    ObjectHandle m_PickedObject;
    EffectHandle m_PickedEffect;
    TimerHandle m_LiftingTimer;
    TransformAnimManager::TimelineHandle m_AnimHandle;
  };

  class EXL_ENGINE_API PickAbility : public AbilityDescT<PickAbilityState>
  {
    DECLARE_RTTI(PickAbility, AbilityDesc);
  public:

    static GameTagName PickableTagName();
    static GameTagName CarryingTagName();
    static GameCueName LiftingCue();
    static GameCueName CarryingCue();
    static AbilityName Name();

    PickAbility();

    static ObjectHandle GetCarriedObject(AbilitySystem* iSys, ObjectHandle iObj);

    AbilityUseState GetUseState(AbilityStateHandle iId) override;
    AbilityUseState Use(AbilityStateHandle iId, ObjectHandle iTarget) override;
    AbilityUseState StopUsing(AbilityStateHandle iId) override;

  protected:
    void LiftingDone(AbilityStateHandle iId);
    void LiftingCleanup(PickAbilityState& iState);
  };

}