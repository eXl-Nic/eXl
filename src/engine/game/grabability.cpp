/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/grabability.hpp>
#include <engine/game/commondef.hpp>
#include <engine/game/walkability.hpp>
#include <engine/game/character.hpp>

#include <engine/common/transforms.hpp>
#include <engine/physics/physicsys.hpp>

#include <math/mathtools.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(GrabbedEffect);
  IMPLEMENT_RTTI(GrabAbility);

  void GrabAbility::SetGrabDirection(AbilitySystem* iSys, ObjectHandle iObj, Vector2f const& iDir)
  {
    GrabAbility* self = iSys->GetAbilityDesc<GrabAbility>(Name());
    AbilityStateHandle stateHandle = iSys->GetAbilityState(iObj, Name());
    if (stateHandle.IsAssigned())
    {
      GrabAbilityState& state = self->Get(stateHandle);
      if (!state.m_GrabbedObject.IsAssigned())
      {
        state.m_GrabDirection = iDir;
      }
    }
  }

  Vector2f const& GrabAbility::GetGrabDirection(AbilitySystem* iSys, ObjectHandle iObj)
  {
    GrabAbility* self = iSys->GetAbilityDesc<GrabAbility>(Name());
    AbilityStateHandle stateHandle = iSys->GetAbilityState(iObj, Name());
    if (stateHandle.IsAssigned())
    {
      GrabAbilityState const& state = self->Get(stateHandle);
      if (state.m_GrabbedObject.IsAssigned())
      {
        return state.m_GrabDirection;
      }
    }

    return Vector2f::ZERO;
  }

  ObjectHandle GrabAbility::GetGrabbedObject(AbilitySystem* iSys, ObjectHandle iObj)
  {
    GrabAbility* self = iSys->GetAbilityDesc<GrabAbility>(Name());
    AbilityStateHandle stateHandle = iSys->GetAbilityState(iObj, Name());
    if (stateHandle.IsAssigned())
    {
      GrabAbilityState const& state = self->Get(stateHandle);
      return state.m_GrabbedObject;
    }

    return ObjectHandle();
  }

  GrabAbility::GrabAbility()
    : AbilityDescT<GrabAbilityState>(Name())
  {
    m_RequireTarget = true;
    m_RequireTargetTags.insert(GrabbableTagName());
    m_ApplyUserTags.insert(EngineCommon::ActionLock());
    m_BlockedByTags.insert(EngineCommon::ActionLock());
    m_BlockedByTargetTags.insert(GrabbedEffect::GrabbedTagName());
  }

  AbilityUseState GrabAbility::GetUseState(AbilityStateHandle iId)
  {
    if(!IsValid(iId))
    { 
      return AbilityUseState::CannotUse;
    }
    return Get(iId).m_GrabbedObject.IsAssigned() ? AbilityUseState::Using : AbilityUseState::None;
  }

  bool GrabAbility::CanUse(AbilitySystem& iSys, ObjectHandle iUser, ObjectHandle& ioTarget)
  {
    World& iWorld = iSys.GetWorld();

    AbilityStateHandle stateHandle = iSys.GetAbilityState(iUser, Name());
    if (stateHandle.IsAssigned())
    {
      if (GetUseState(stateHandle) == AbilityUseState::None)
      {
        GrabAbilityState& state = Get(stateHandle);
        ioTarget = ObjectHandle();

        Transforms& trans = *iWorld.GetSystem<Transforms>();
        PhysicsSystem& phSys = *iWorld.GetSystem<PhysicsSystem>();
        List<CollisionData> res;
        Vector3f charPos = MathTools::GetPosition(trans.GetWorldTransform(iUser));
        Vector3f posEnd = charPos + MathTools::To3DVec(state.m_GrabDirection) * 1.05;
        phSys.RayQuery(res, charPos, posEnd, 2);

        for (auto obj : res)
        {
          if (obj.obj1 != iUser)
          {
            if(AbilityDesc::IsTargetValid(obj.obj1))
            {
              ioTarget = obj.obj1;
            }
            break;
          }
        }
        if (ioTarget.IsAssigned())
        {
          return true;
        }
      }
    }
    return false;
  }

  AbilityUseState GrabAbility::Use(AbilityStateHandle iId, ObjectHandle iTarget)
  {
    GrabAbilityState& state = Get(iId);
    Using_Begin(state, iTarget);

    World& world = m_System->GetWorld();

    CharacterSystem* charSys = world.GetSystem<CharacterSystem>();
    if (charSys->GrabObject(state.m_User, state.m_Target))
    {
      state.m_GrabbedObject = state.m_Target;
      state.m_GrabbedEffect = m_System->CreateEffect<GrabbedEffect>(state.m_Target, GrabbedEffect::Name(), [](ObjectHandle iTarget, GrabbedEffect& iEffect)
      {
        return iEffect.Create(iTarget);
      });
    }
    else
    {
      return AbilityUseState::CannotUse;
    }

    uint32_t dirState = charSys->GetCurrentState(state.m_User) & (uint32_t)CharacterSystem::StateFlags::DirMask;
    if (dirState == (uint32_t)CharacterSystem::StateFlags::DirLeft 
      || dirState == (uint32_t)CharacterSystem::StateFlags::DirRight)
    {
      m_System->AddTag(state.m_User, WalkAbility::XLocked());
      state.m_XLocked = true;
    }
    else
    {
      m_System->AddTag(state.m_User, WalkAbility::YLocked());
      state.m_XLocked = false;
    }
    m_System->StartCue(state.m_User, GrabCue());
    return AbilityUseState::Using;
  }

  AbilityUseState GrabAbility::StopUsing(AbilityStateHandle iId)
  {
    GrabAbilityState& state = Get(iId);
    World& world = m_System->GetWorld();

    if (state.m_XLocked)
    {
      m_System->RemoveTag(state.m_User, WalkAbility::XLocked());
    }
    else
    {
      m_System->RemoveTag(state.m_User, WalkAbility::YLocked());
    }

    if (state.m_GrabbedObject.IsAssigned())
    {
      CharacterSystem* charSys = world.GetSystem<CharacterSystem>();
      eXl_ASSERT_REPAIR_RET(charSys->ReleaseObject(state.m_User, state.m_GrabbedObject), AbilityUseState::Using);
      
      m_System->RemoveEffect(state.m_GrabbedObject, GrabbedEffect::Name(), state.m_GrabbedEffect);
      state.m_GrabbedObject = ObjectHandle();
      state.m_GrabbedEffect = EffectHandle();
    }
    m_System->EndCue(state.m_User, GrabCue());
    StopUsing_End(state);
    return AbilityUseState::None;
  }
}