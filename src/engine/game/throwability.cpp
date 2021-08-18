/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <engine/game/throwability.hpp>
#include <engine/game/pickability.hpp>
#include <engine/game/character.hpp>
#include <engine/game/projectile.hpp>
#include <engine/game/commondef.hpp>

#include <engine/common/transforms.hpp>
#include <engine/physics/physicsys.hpp>
#include <math/mathtools.hpp>

namespace eXl
{
  IMPLEMENT_RTTI(ThrowAbility);

  ThrowAbility::ThrowAbility()
    : AbilityDescT<ThrowAbilityState>(Name())
  {
    m_RequireTarget = false;
    m_RequireUserTags.insert(PickAbility::CarryingTagName());
    m_ApplyUserTags.insert(DunAtk::ActionLock());
    m_ApplyUserTags.insert(DunAtk::AnimLocked());
  }

  AbilityUseState ThrowAbility::GetUseState(AbilityStateHandle iId)
  {
    if(!IsValid(iId))
    {
      return AbilityUseState::CannotUse;
    }
    return Get(iId).m_UseState;
  }

  static float const s_ThrowDuration = 0.6;

  LinearPositionAnimation& GetFallAnimation()
  {
    static LinearPositionAnimation s_Anim = []
    {
      LinearPositionAnimation newAnim;
      uint32_t numPos = 16;
      float durationIncrement = s_ThrowDuration / numPos;
      float fallAltitude = 2.0;
      for (uint32_t i = 0; i <= numPos; ++i)
      {
        float ratio = (1.0 - float(numPos - i) / numPos);
        newAnim.Add(Vector3f(0.0, 0.0, (1.0 - ratio * ratio) * fallAltitude), durationIncrement * i);
      }
      return newAnim;
    }();

    return s_Anim;
  }

  AbilityUseState ThrowAbility::Use(AbilityStateHandle iId, ObjectHandle iTarget)
  {
    ThrowAbilityState& state = Get(iId);
    Using_Begin(state, iTarget);

    World& world = m_System->GetWorld();

    state.m_ThrownObject = PickAbility::GetCarriedObject(m_System, state.m_User);

    m_System->StopUsingAbility(state.m_User, PickAbility::Name());

    CharacterSystem& characters = *world.GetSystem<CharacterSystem>();
    ProjectileSystem& projectiles = *world.GetSystem<ProjectileSystem>();
    PhysicsSystem& phSys = *world.GetSystem<PhysicsSystem>();
    Transforms& transforms = *world.GetSystem<Transforms>();
    Vector3f throwDir = characters.GetCurrentFacingDirection(state.m_User);

    transforms.Detach(state.m_ThrownObject);

    ProjectileSystem::Desc desc;
    //desc.kind = ProjectileSystem::PhysicKind::Ghost;
    desc.kind = ProjectileSystem::PhysicKind::Simulated;
    desc.contactCb = [user = state.m_User, this](ObjectHandle iProj, ObjectHandle iTouched)
    {
      if (iTouched == user)
      {
        if (m_System->GetAbilityUseState(user, Name()) == AbilityUseState::Using)
        {
          return false;
        }
      }
      return true;
    };
    desc.registerNav = false;
    desc.rotateSprite = false;
    desc.size = 1.0;

    state.m_ProjectileProxy = world.CreateObject();
    transforms.AddTransform(state.m_ProjectileProxy, &transforms.GetWorldTransform(state.m_User));

    PhysicInitData phData;
    phData.SetShapeObj(state.m_ThrownObject);
    phData.SetFlags(DunAtk::s_BasePhFlags);
    phSys.CreateComponent(state.m_ProjectileProxy, phData);

    transforms.Attach(state.m_ThrownObject, state.m_ProjectileProxy, Transforms::AttachType::Position);
    transforms.UpdateTransform(state.m_ThrownObject, Matrix4f::IDENTITY);
    projectiles.AddProjectile(state.m_ProjectileProxy, desc, throwDir * 20.0);

    auto& animMgr = *world.GetSystem<TransformAnimManager>();
    /*state.m_AnimHandle =*/ animMgr.Start(state.m_ThrownObject, GetFallAnimation(), DunAtk::GetProjectionMatrix() * DunAtk::GetRotationMatrix(MathTools::As2DVec(throwDir)));

    world.AddTimer(0.3, false, [this, user = state.m_User](World& iWorld)
    {
      m_System->StopUsingAbility(user, Name());
    });

    world.AddTimer(s_ThrowDuration, false, [this, projectile = state.m_ProjectileProxy, thrownObject = state.m_ThrownObject](World& iWorld)
    {
      Transforms& transforms = *iWorld.GetSystem<Transforms>();
      PhysicsSystem& phSys = *iWorld.GetSystem<PhysicsSystem>();
      transforms.Detach(thrownObject);
      Matrix4f newPos = Matrix4f::IDENTITY;
      MathTools::GetPosition(newPos) = MathTools::GetPosition(transforms.GetWorldTransform(projectile));
      transforms.UpdateTransform(thrownObject, newPos);
      iWorld.DeleteObject(projectile);
      phSys.SetComponentEnabled(thrownObject, true);
    });

    state.m_UseState = AbilityUseState::Using;

    m_System->StartCue(state.m_User, ThrowingCue());
    return AbilityUseState::Using;
  }

  AbilityUseState ThrowAbility::StopUsing(AbilityStateHandle iId)
  {
    ThrowAbilityState& state = Get(iId);
    
    m_System->EndCue(state.m_User, ThrowingCue());
    StopUsing_End(state);
    state.m_UseState = AbilityUseState::None;
    return AbilityUseState::None;
  }
}