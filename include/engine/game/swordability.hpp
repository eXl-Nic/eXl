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