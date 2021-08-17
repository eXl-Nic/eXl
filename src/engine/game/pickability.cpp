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
    m_ApplyUserTags.insert(DunAtk::ActionLock());
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
      float angleIncrement = Mathf::PI * 0.5 / numPos;
      for (uint32_t i = 0; i < 16; ++i)
      {
        newAnim.Add(Vector3f(Mathf::Cos(i * angleIncrement), 0.0, Mathf::Sin(i * angleIncrement)) * 2.0, durationIncrement * i);
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

    Vector2f grabDirection = GrabAbility::GetGrabDirection(m_System, state.m_User);

    m_System->StopUsingAbility(state.m_User, GrabAbility::Name());

    Transforms& transforms = *world.GetSystem<Transforms>();
    PhysicsSystem& phSys = *world.GetSystem<PhysicsSystem>();

    state.m_PickedObject = state.m_Target;
    phSys.SetComponentEnabled(state.m_Target, false);

    Matrix4f offset;
    MathTools::GetPosition(offset) = Vector3f::ZERO;

    transforms.UpdateTransform(state.m_Target, offset);
    transforms.Attach(state.m_Target, state.m_User, Transforms::AttachType::Position);

    auto& animMgr = *world.GetSystem<TransformAnimManager>();

    state.m_AnimHandle = animMgr.Start(state.m_PickedObject, GetLiftAnimation(), DunAtk::GetProjectionMatrix() * DunAtk::GetRotationMatrix(grabDirection));

    state.m_LiftingTimer = world.AddTimer(0.60, false, [this, iId](World&)
    {
      LiftingDone(iId);
    });
    m_System->AddTag(state.m_User, DunAtk::AnimLocked());
    m_System->StartCue(state.m_User, LiftingCue());

    return AbilityUseState::Using;
  }

  void PickAbility::LiftingDone(AbilityStateHandle iId)
  {
    PickAbilityState& state = Get(iId);
    
    World& world = m_System->GetWorld();
    Transforms& transforms = *world.GetSystem<Transforms>();

    Matrix4f offset;
    MathTools::GetPosition(offset) = Vector3f(0.0, 2.0, 2.0 / Mathf::Sqrt(2.0));

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
    m_System->RemoveTag(iState.m_User, DunAtk::AnimLocked());
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

        //phSys.SetComponentEnabled(state.m_Target, true);

        Matrix4f worldPos = transforms.GetWorldTransform(state.m_PickedObject);
        MathTools::GetPosition2D(worldPos) -= Vector2f(0.0, 1.0);

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