#include <engine/game/swordability.hpp>
#include <engine/game/commondef.hpp>

#include <engine/gfx/gfxsystem.hpp>
#include <engine/gfx/gfxcomponent.hpp>

#include <math/mathtools.hpp>


namespace eXl
{
  IMPLEMENT_RTTI(SwordAbility);

  SwordAbility::SwordAbility()
    : AbilityDescT<SwordAbilityState>(Name())
  {
    m_RequireTarget = false;
    m_ApplyUserTags.insert(DunAtk::AnimLocked());
    m_ApplyUserTags.insert(DunAtk::ActionLock());
    m_BlockedByTags.insert(DunAtk::ActionLock());
  }

  AbilityStateHandle SwordAbility::AddTo(ObjectHandle iUser)
  {
    AbilityStateHandle stateHandle = AbilityDescT<SwordAbilityState>::AddTo(iUser);
    SwordAbilityState& state = Get(stateHandle);

    World& world = m_System->GetWorld();
    state.m_SwordActor = world.CreateObject();

    Transforms& trans = *world.GetSystem<Transforms>();

#ifdef EXL_WITH_OGL
    GfxSystem& gfxSys = *world.GetSystem<GfxSystem>();
    ResourceHandle<Tileset> tilesetHandle;
    {
      Resource::UUID id({ 3147825353,893968785, 3414459273, 2423460864 });
      tilesetHandle.SetUUID(id);
      tilesetHandle.Load();
    }
    Tileset const* swordSet = tilesetHandle.Get();

    GfxSpriteComponent& gfxComp = gfxSys.CreateSpriteComponent(state.m_SwordActor);
    gfxComp.SetTileset(swordSet);
    gfxComp.SetTileName(TileName(""));
    gfxComp.SetRotateSprite(false);
#endif

    Matrix4f transform;
    transform.MakeIdentity();
    MathTools::GetPosition(transform).Z() = 0.05;
    trans.AddTransform(state.m_SwordActor, &transform);
    trans.Attach(state.m_SwordActor, iUser);

    return stateHandle;
  }

  void SwordAbility::Remove(AbilityStateHandle iToRemove)
  {
    SwordAbilityState& state = Get(iToRemove);
    World& world = m_System->GetWorld();
    world.DeleteObject(state.m_SwordActor);
  }

  void SwordAbility::SetSwingDirection(AbilitySystem* iSys, ObjectHandle iObj, Vector2f const& iDir)
  {
    SwordAbility* self = iSys->GetAbilityDesc<SwordAbility>(Name());
    AbilityStateHandle stateHandle = iSys->GetAbilityState(iObj, Name());
    if (stateHandle.IsAssigned())
    {
      SwordAbilityState& state = self->Get(stateHandle);
      if (state.m_UseState == AbilityUseState::None)
      {
        state.m_SwingDirection = iDir;
      }
    }
  }

  AbilityUseState SwordAbility::GetUseState(AbilityStateHandle iId)
  {
    if (!IsValid(iId))
    {
      return AbilityUseState::CannotUse;
    }
    return Get(iId).m_UseState;
  }

  AbilityUseState SwordAbility::Use(AbilityStateHandle iId, ObjectHandle iTarget)
  {
    SwordAbilityState& state = Get(iId);

    Using_Begin(state, iTarget);

    World& world = m_System->GetWorld();

    
    Transforms& trans = *world.GetSystem<Transforms>();
#ifdef EXL_WITH_OGL
    GfxSystem& gfxSys = *world.GetSystem<GfxSystem>();
    GfxSpriteComponent* gfxComp = gfxSys.GetSpriteComponent(state.m_SwordActor);

    if (state.m_SwingDirection.Dot(Vector2f::UNIT_X) > 0.5)
    {
      gfxComp->SetTileName(TileName("use_right"));
    }
    if (state.m_SwingDirection.Dot(-Vector2f::UNIT_X) > 0.5)
    {
      gfxComp->SetTileName(TileName("use_left"));
    }
    if (state.m_SwingDirection.Dot(Vector2f::UNIT_Y) > 0.5)
    {
      gfxComp->SetTileName(TileName("use_up"));
    }
    if (state.m_SwingDirection.Dot(-Vector2f::UNIT_Y) > 0.5)
    {
      gfxComp->SetTileName(TileName("use_down"));
    }
#endif

    world.AddTimer(0.30, false, [this, user = state.m_User](World& iWorld)
    {
      m_System->StopUsingAbility(user, Name());
    });

    m_System->StartCue(state.m_User, UseSwordCue());

    state.m_UseState = AbilityUseState::Using;

    return AbilityUseState::Using;
  }

  AbilityUseState SwordAbility::StopUsing(AbilityStateHandle iId)
  {
    SwordAbilityState& state = Get(iId);
    World& world = m_System->GetWorld();

    state.m_UseState = AbilityUseState::None;

    Transforms& trans = *world.GetSystem<Transforms>();

#ifdef EXL_WITH_OGL
    GfxSystem& gfxSys = *world.GetSystem<GfxSystem>();
    GfxSpriteComponent* gfxComp = gfxSys.GetSpriteComponent(state.m_SwordActor);
    gfxComp->SetTileName(TileName(""));
#endif
    m_System->EndCue(state.m_User, UseSwordCue());

    StopUsing_End(state);
    return AbilityUseState::None;
  }
}