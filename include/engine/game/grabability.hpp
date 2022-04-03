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
    Vec2 m_GrabDirection;
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

    static void SetGrabDirection(AbilitySystem* iSys, ObjectHandle iObj, Vec2 const& iDir);
    static Vec2 const& GetGrabDirection(AbilitySystem* iSys, ObjectHandle iObj);
    static ObjectHandle GetGrabbedObject(AbilitySystem* iSys, ObjectHandle iObj);

    bool CanUse(AbilitySystem& iSys, ObjectHandle iUser, ObjectHandle& ioTarget) override;
    AbilityUseState GetUseState(AbilityStateHandle iId) override;
    AbilityUseState Use(AbilityStateHandle iId, ObjectHandle iTarget) override;
    AbilityUseState StopUsing(AbilityStateHandle iId) override;
  };
}