#pragma once

#include <engine/game/ability.hpp>

namespace eXl
{
  class EXL_ENGINE_API GrabbedEffect : public EffectDescT<EffectState>
  {
    DECLARE_RTTI(GrabbedEffect, EffectDesc)
  public:

    static GameTagName GrabbedTagName();
    static EffectName Name();

    GrabbedEffect()
      : EffectDescT(Name())
    {
      m_AppliedTags.insert(GrabbedTagName());
    }
  };
  class GrabAbility;

  struct GrabAbilityState : public AbilityState
  {
    friend GrabAbility;
  private:
    Vector2f m_GrabDirection;
    ObjectHandle m_GrabbedObject;
    EffectHandle m_GrabbedEffect;
    bool m_XLocked = false;
  };

  class EXL_ENGINE_API GrabAbility : public AbilityDescT<GrabAbilityState>
  {
    DECLARE_RTTI(GrabAbility, AbilityDesc);
  public:

    static GameCueName GrabCue();
    static GameTagName GrabbableTagName();
    static AbilityName Name();

    GrabAbility();

    static void SetGrabDirection(AbilitySystem* iSys, ObjectHandle iObj, Vector2f const& iDir);
    static Vector2f const& GetGrabDirection(AbilitySystem* iSys, ObjectHandle iObj);
    static ObjectHandle GetGrabbedObject(AbilitySystem* iSys, ObjectHandle iObj);

    bool CanUse(AbilitySystem& iSys, ObjectHandle iUser, ObjectHandle& ioTarget) override;
    AbilityUseState GetUseState(AbilityStateHandle iId) override;
    AbilityUseState Use(AbilityStateHandle iId, ObjectHandle iTarget) override;
    AbilityUseState StopUsing(AbilityStateHandle iId) override;
  };
}