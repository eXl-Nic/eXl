/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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