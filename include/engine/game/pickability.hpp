/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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