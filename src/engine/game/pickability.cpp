/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/pickability.hpp>
#include <engine/game/commondef.hpp>
#include <engine/game/grabability.hpp>
#include <engine/common/transforms.hpp>
#include <engine/physics/physicsys.hpp>
#include <math/mathtools.hpp>
#include <engine/common/transformanim.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(PickAbility);

  PickAbility::PickAbility()
    : AbilityDescT<PickAbilityState>(Name())
  {
    m_RequireTarget = true;
    m_RequireTargetTags.insert(GrabbedEffect::GrabbedTagName());
    m_RequireTargetTags.insert(PickableTagName());
    m_ApplyUserTags.insert(EngineCommon::ActionLock());
  }

  ObjectHandle PickAbility::GetCarriedObject(AbilitySystem* iSys, ObjectHandle iObj)
  {
    PickAbility* self = iSys->GetAbilityDesc<PickAbility>(Name());
    AbilityStateHandle stateHandle = iSys->GetAbilityState(iObj, Name());
    if (stateHandle.IsAssigned())
    {
      PickAbilityState& state = self->Get(stateHandle);
      if (!state.m_LiftingTimer.IsAssigned())
      {
        return state.m_PickedObject;
      }
    }
    return ObjectHandle();
  }

  LinearPositionAnimation& GetLiftAnimation()
  {
    static LinearPositionAnimation s_Anim = []
    {
      LinearPositionAnimation newAnim;
      uint32_t numPos = 16;
      float durationIncrement = 0.6 / numPos;
      float angleIncrement = Mathf::Pi() * 0.5 / numPos;
      for (uint32_t i = 0; i < 16; ++i)
      {
        newAnim.Add(Vec3(Mathf::Cos(i * angleIncrement), 0.0, Mathf::Sin(i * angleIncrement)) * 2.0, durationIncrement * i);
      }
      return newAnim;
    }();

    return s_Anim;
  }

  AbilityUseState PickAbility::GetUseState(AbilityStateHandle iId)
  {
    if(!IsValid(iId))
    {
      return AbilityUseState::CannotUse;
    }
    return Get(iId).m_PickedObject.IsAssigned() ? AbilityUseState::Using : AbilityUseState::None;
  }

  AbilityUseState PickAbility::Use(AbilityStateHandle iId, ObjectHandle iTarget)
  {
    PickAbilityState& state = Get(iId);
    Using_Begin(state, iTarget);
    
    World& world = m_System->GetWorld();

    Vec2 grabDirection = GrabAbility::GetGrabDirection(m_System, state.m_User);

    m_System->StopUsingAbility(state.m_User, GrabAbility::Name());

    Transforms& transforms = *world.GetSystem<Transforms>();
    PhysicsSystem& phSys = *world.GetSystem<PhysicsSystem>();

    state.m_PickedObject = state.m_Target;
    phSys.SetComponentEnabled(state.m_Target, false);

    transforms.UpdateTransform(state.m_Target, Identity<Mat4>());
    transforms.Attach(state.m_Target, state.m_User, Transforms::AttachType::Position);

    auto& animMgr = *world.GetSystem<TransformAnimManager>();

    state.m_AnimHandle = animMgr.Start(state.m_PickedObject, GetLiftAnimation(), EngineCommon::GetProjectionMatrix() * EngineCommon::GetRotationMatrix(grabDirection));

    state.m_LiftingTimer = world.AddTimer(0.60, false, [this, iId](World&)
    {
      LiftingDone(iId);
    });
    m_System->AddTag(state.m_User, EngineCommon::AnimLocked());
    m_System->StartCue(state.m_User, LiftingCue());

    return AbilityUseState::Using;
  }

  void PickAbility::LiftingDone(AbilityStateHandle iId)
  {
    PickAbilityState& state = Get(iId);
    
    World& world = m_System->GetWorld();
    Transforms& transforms = *world.GetSystem<Transforms>();

    Mat4 offset = translate(Identity<Mat4>(), Vec3(0.0, 2.0, 2.0 / Mathf::Sqrt(2.0)));

    transforms.Attach(state.m_Target, state.m_User, Transforms::AttachType::Position);
    transforms.UpdateTransform(state.m_Target, offset);

    LiftingCleanup(state);

    m_System->StartCue(state.m_User, CarryingCue());
    m_System->AddTag(state.m_User, CarryingTagName());
  }

  void PickAbility::LiftingCleanup(PickAbilityState& iState)
  {
    auto& animMgr = *m_System->GetWorld().GetSystem<TransformAnimManager>();
    animMgr.Stop(iState.m_AnimHandle);
    iState.m_LiftingTimer = TimerHandle();
    m_System->RemoveTag(iState.m_User, EngineCommon::AnimLocked());
    m_System->EndCue(iState.m_User, LiftingCue());
  }

  AbilityUseState PickAbility::StopUsing(AbilityStateHandle iId)
  {
    // Break the object ? Should watch events to throw it instead.

    PickAbilityState& state = Get(iId);
    if (state.m_LiftingTimer.IsAssigned())
    {
      // Should throw the object.
      LiftingCleanup(state);
    }
    else
    {
      World& world = m_System->GetWorld();
      if (state.m_PickedObject.IsAssigned())
      {
        Transforms& transforms = *world.GetSystem<Transforms>();
        PhysicsSystem& phSys = *world.GetSystem<PhysicsSystem>();

        transforms.Detach(state.m_Target);
      }
    }

    state.m_PickedObject = ObjectHandle();

    m_System->EndCue(state.m_User, CarryingCue());
    m_System->AddTag(state.m_User, CarryingTagName());

    StopUsing_End(state);
    return AbilityUseState::None;
  }
}