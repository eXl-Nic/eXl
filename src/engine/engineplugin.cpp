/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/plugin.hpp>
#include <core/log.hpp>
#include <core/corelib.hpp>
#include <core/type/typemanager.hpp>

#include <engine/game/commondef.hpp>

#include <engine/gfx/tileset.hpp>
#include <engine/map/tilinggroup.hpp>
#include <engine/map/mcmcmodelrsc.hpp>
#include <engine/common/project.hpp>
#include <engine/game/archetype.hpp>
#include <engine/game/characteranimation.hpp>
#include <engine/pathfinding/navigator.hpp>
#include <engine/gui/fontresource.hpp>

#include <engine/gfx/gfxsystem.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/physics/physicsys.hpp>
#include <engine/physics/physicsdef.hpp>
#include <engine/map/map.hpp>

#include <engine/game/grabability.hpp>
#include <engine/game/pickability.hpp>
#include <engine/game/swordability.hpp>
#include <engine/game/throwability.hpp>
#include <engine/game/walkability.hpp>
#include <engine/game/scripttrigger.hpp>

#ifdef EXL_LUA
#include <engine/script/luascriptsystem.hpp>
#endif

#include <yojimbo.h>

namespace eXl
{

  void CharacterAnimation_StaticInit();
  void CharacterAnimation_StaticDestroy();

  namespace
  {
    struct NameRegistry
    {
      EffectName  m_GrabbedEffectName;
      GameTagName m_GrabbedTagName;
      AbilityName m_GrabAbilityName;
      GameTagName m_GrabbableTagName;
      GameCueName m_GrabCue;

      AbilityName m_PickAbilityName;
      GameTagName m_PickableTagName;
      GameTagName m_CarryingTagName;
      GameCueName m_LiftingCue;
      GameCueName m_CarryingCue;

      AbilityName m_SwordAbilityName;
      GameCueName m_UseSwordCue;

      AbilityName m_ThrowAbilityName;
      GameCueName m_ThrowingCue;

      AbilityName m_WalkAbilityName;
      GameTagName m_XLocked;
      GameTagName m_YLocked;

      GameTagName m_ActionLock;
      GameTagName m_WalkingTag;
      GameTagName m_AnimLocked;

      ComponentName m_GfxSpriteComponentName;
      ComponentName m_PhysicsComponentName;
      ComponentName m_TriggerComponentName;
      PropertySheetName m_VelocityName;

      PropertySheetName m_GrabDataName;
      PropertySheetName m_HealthDataName;
      PropertySheetName m_TurretDataName;
      PropertySheetName m_SpriteDescDataName;
      PropertySheetName m_PhysicsInitDataName;
      PropertySheetName m_TerrainCarverName;

      ImageName m_EmptyImage;

      NameRegistry()
      {
        m_GrabbedEffectName = "GrabbedEffect";
        m_GrabbedTagName = "GrabbedTag";
        m_GrabAbilityName = "GrabAbility";
        m_GrabbableTagName = "GrabbableTag";
        m_GrabCue = "GrabCue";

        m_PickAbilityName = "PickAbility";
        m_PickableTagName = "PickableTag";
        m_CarryingTagName = "CarryingTag";
        m_LiftingCue = "LiftingCue";
        m_CarryingCue = "CarryingCue";

        m_SwordAbilityName = "SwordAbility";
        m_UseSwordCue = "UseSwordCue";

        m_ThrowAbilityName = "ThrowAbility";
        m_ThrowingCue = "ThrowingCue";

        m_WalkAbilityName = "WalkAbility";
        m_XLocked = "WalkXLocked";
        m_YLocked = "WalkYLocked";

        m_ActionLock = "ActionLockedTag";
        m_WalkingTag = "WalkingTag";
        m_AnimLocked = "AnimLockedTag";

        m_GfxSpriteComponentName = "GfxSprite";
        m_PhysicsComponentName = "Physics";
        m_TriggerComponentName = "Trigger";

        m_VelocityName = "Velocity";

        m_GrabDataName = "GrabData";
        m_HealthDataName = "Health";
        m_TurretDataName = "Turret";
        m_TerrainCarverName = "TerrainCarver";

        m_EmptyImage = "<empty>";

        m_SpriteDescDataName = "GfxSpriteComponentDesc";
        m_PhysicsInitDataName = "PhysicsInitData";
      }
    };

    Optional<NameRegistry> s_NameRegistry;
  }

  ImageName Tile::EmptyName() { return s_NameRegistry->m_EmptyImage; }

  GameTagName GrabbedEffect::GrabbedTagName() { return s_NameRegistry->m_GrabbedTagName; }
  EffectName GrabbedEffect::Name() { return s_NameRegistry->m_GrabbedEffectName; }

  GameCueName GrabAbility::GrabCue() { return s_NameRegistry->m_GrabCue; }
  GameTagName GrabAbility::GrabbableTagName() { return s_NameRegistry->m_GrabbableTagName; }
  AbilityName GrabAbility::Name() { return s_NameRegistry->m_GrabAbilityName; }

  //{ return s_NameRegistry->; }

  GameTagName PickAbility::PickableTagName() { return s_NameRegistry->m_PickableTagName; }
  GameTagName PickAbility::CarryingTagName() { return s_NameRegistry->m_CarryingTagName; }
  GameCueName PickAbility::LiftingCue() { return s_NameRegistry->m_LiftingCue; }
  GameCueName PickAbility::CarryingCue() { return s_NameRegistry->m_CarryingCue; }
  AbilityName PickAbility::Name() { return s_NameRegistry->m_PickAbilityName; }

  AbilityName SwordAbility::Name() { return s_NameRegistry->m_SwordAbilityName; }
  GameCueName SwordAbility::UseSwordCue() { return s_NameRegistry->m_UseSwordCue; }

  GameCueName ThrowAbility::ThrowingCue() { return s_NameRegistry->m_ThrowingCue; }
  AbilityName ThrowAbility::Name() { return s_NameRegistry->m_ThrowAbilityName; }

  GameTagName WalkAbility::XLocked() { return s_NameRegistry->m_XLocked; }
  GameTagName WalkAbility::YLocked() { return s_NameRegistry->m_YLocked; }
  AbilityName WalkAbility::Name() { return s_NameRegistry->m_WalkAbilityName; }

  GameTagName EngineCommon::ActionLock() { return s_NameRegistry->m_ActionLock; }
  GameTagName EngineCommon::WalkingTag() { return s_NameRegistry->m_WalkingTag; }
  GameTagName EngineCommon::AnimLocked() { return s_NameRegistry->m_AnimLocked; }

  ComponentName EngineCommon::GfxSpriteComponentName() { return s_NameRegistry->m_GfxSpriteComponentName; }
  ComponentName EngineCommon::PhysicsComponentName() { return s_NameRegistry->m_PhysicsComponentName; }
  ComponentName EngineCommon::TriggerComponentName() { return s_NameRegistry->m_TriggerComponentName; }
  ComponentName EngineCommon::CharacterComponentName()
  {
    static ComponentName s_Name("Character");
    return s_Name;
  }
  PropertySheetName EngineCommon::VelocityName() { return s_NameRegistry->m_VelocityName; }

  PropertySheetName EngineCommon::GrabData::PropertyName() { return s_NameRegistry->m_GrabDataName; }
  PropertySheetName EngineCommon::HealthData::PropertyName() { return s_NameRegistry->m_HealthDataName; }
  PropertySheetName EngineCommon::TurretData::PropertyName() { return s_NameRegistry->m_TurretDataName; }
  PropertySheetName EngineCommon::TerrainCarver::PropertyName() { return s_NameRegistry->m_TerrainCarverName; }
  PropertySheetName EngineCommon::GfxSpriteDescName() { return s_NameRegistry->m_SpriteDescDataName; }

  PropertySheetName EngineCommon::ObjectShapeData::PropertyName()
  {
    static PropertySheetName s_Name("ObjectShape");
    return s_Name;
  }

  PropertySheetName EngineCommon::PhysicBodyData::PropertyName()
  {
    static PropertySheetName s_Name("PhysicBody");
    return s_Name;
  }

  PropertySheetName EngineCommon::TriggerComponentDesc::PropertyName()
  {
    static PropertySheetName s_Name("TriggerDescriprion");
    return s_Name;
  }

  PropertySheetName EngineCommon::CharacterDesc::PropertyName()
  {
    static PropertySheetName s_Name("CharacterDescription");
    return s_Name;
  }


  void Register_ENGINE_Types();
#ifdef EXL_LUA
  LUA_REG_FUN(BindEngine);
#endif

  Optional<ComponentManifest> s_EngineCommonManifest;

  using namespace EngineCommon;

  ComponentManifest const& EngineCommon::GetComponents() { return *s_EngineCommonManifest; }
  PropertiesManifest EngineCommon::GetBaseProperties()
  {
    PropertiesManifest baseManifest;
    baseManifest.RegisterPropertySheet<HealthData>(HealthData::PropertyName());
    baseManifest.RegisterPropertySheet<GrabData>(GrabData::PropertyName());
    baseManifest.RegisterPropertySheet<TurretData>(TurretData::PropertyName());
    baseManifest.RegisterPropertySheet<TerrainCarver>(TerrainCarver::PropertyName());

    baseManifest.RegisterPropertySheet<GfxSpriteComponent::Desc>(GfxSpriteDescName());
    baseManifest.RegisterPropertySheet<ObjectShapeData>(ObjectShapeData::PropertyName());
    baseManifest.RegisterPropertySheet<PhysicBodyData>(PhysicBodyData::PropertyName());
    baseManifest.RegisterPropertySheet<TriggerComponentDesc>(TriggerComponentDesc::PropertyName());
    baseManifest.RegisterPropertySheet<CharacterDesc>(CharacterDesc::PropertyName());

    baseManifest.RegisterPropertySheet<Vec3>(VelocityName(), false);

    return baseManifest;
  }

  EventsManifest& EngineCommon::GetBaseEvents()
  {
    static EventsManifest s_BaseEvents = []
    {
      EventsManifest baseEvents;
      EventsManifest::FunctionsMap triggerFunctions;

      triggerFunctions.insert(std::make_pair("Enter", FunDesc::Create<void(ObjectHandle, ObjectHandle)>()));
      triggerFunctions.insert(std::make_pair("Leave", FunDesc::Create<void(ObjectHandle, ObjectHandle)>()));
      baseEvents.m_Interfaces.insert(std::make_pair("Trigger", std::move(triggerFunctions)));

      return baseEvents;
    }();

    return s_BaseEvents;
  }

  class EnginePlugin : public Plugin
  {
  public:
    EnginePlugin() 
      : Plugin("EnginePlugin")
    {
      m_Dependencies.push_back("eXl_Math");
#ifdef EXL_WITH_OGL
      m_Dependencies.push_back("eXl_OGL");
#endif
      //m_Dependencies.push_back("eXl_Gen");
    }

    void _Load()
    {
      s_NameRegistry.emplace();
      CharacterAnimation_StaticInit();

      Tileset::Init();
      TilingGroup::Init();
      Project::Init();
      Archetype::Init();
      MapResource::Init();
      MCMCModelRsc::Init();
      FontResource::Init();
      //CharacterAnimation::Init();
      LuaScriptBehaviour::Init();

      Register_ENGINE_Types();
#ifdef EXL_LUA
      LuaManager::AddRegFun(&BindEngine);
#endif

      TypeManager::RegisterCoreType<ObjectHandle>();

      auto createGfxSpriteFactory = [](World& iWorld, ObjectHandle iObject)
      {
#ifdef EXL_WITH_OGL
        if (GfxSystem* gfx = iWorld.GetSystem<GfxSystem>())
        {
          if (gfx->GetSpriteComponent(iObject) != nullptr)
          {
            return;
          }
          GfxSpriteComponent& spriteComp = gfx->CreateSpriteComponent(iObject);
        }
#endif
      };

      auto createPhysicsFactory = [](World& iWorld, ObjectHandle iObject)
      {
        PhysicsSystem& ph = *iWorld.GetSystem<PhysicsSystem>();

        if (ph.GetCompImpl(iObject) != nullptr)
        {
          return;
        }

        GameDatabase& gameDb = *iWorld.GetSystem<GameDatabase>();
        ObjectShapeData const* shapeDesc = gameDb.GetData<ObjectShapeData>(iObject, ObjectShapeData::PropertyName());
        PhysicBodyData const* phDesc = gameDb.GetData<PhysicBodyData>(iObject, PhysicBodyData::PropertyName());

        eXl_ASSERT_REPAIR_RET(shapeDesc != nullptr, void());
        eXl_ASSERT_REPAIR_RET(phDesc != nullptr, void());

        PhysicInitData initData;
        initData.SetFlags(EngineCommon::s_BasePhFlags);
        switch (phDesc->m_Type)
        {
        case PhysicsType::Static:
          initData.SetFlags(initData.GetFlags() | PhysicFlags::Static);
          break;
        case PhysicsType::Kinematic:
          initData.SetFlags(initData.GetFlags() | PhysicFlags::Kinematic);
          break;
        }

        switch (phDesc->m_Category)
        {
        case PhysicsCollisionCategory::Default:
          initData.SetCategory(s_DefaultCategory, ~1);
          break;
        case PhysicsCollisionCategory::Wall:
          initData.SetCategory(s_WallCategory, s_WallMask);
          break;
        case PhysicsCollisionCategory::Character:
          initData.SetCategory(s_CharacterCategory, s_CharacterMask);
          break;
        case PhysicsCollisionCategory::Trigger:
          initData.SetCategory(s_TriggerCategory, s_TriggerMask);
          break;
        case PhysicsCollisionCategory::Projectile:
          initData.SetCategory(s_ProjectileCategory, s_ProjectileMask);
          break;
        case PhysicsCollisionCategory::None:
          initData.SetCategory(0, 0);
          break;
        }

        for (auto const& shape : shapeDesc->m_Shapes)
        {
          switch (shape.m_Type)
          {
          case PhysicsShapeType::Sphere:
            initData.AddSphere(shape.m_Dims.x, shape.m_Offset);
            break;
          case PhysicsShapeType::Box:
            initData.AddBox(shape.m_Dims, shape.m_Offset);
            break;
          }
        }

        ph.CreateComponent(iObject, initData);
      };

#ifdef EXL_LUA
      auto createTriggerFactory = [](World& iWorld, ObjectHandle iObject)
      {
        ScriptTriggerSystem* triggerSys = iWorld.GetSystem<ScriptTriggerSystem>();

        PhysicsSystem* phSys = iWorld.GetSystem<PhysicsSystem>();
        LuaScriptSystem* scriptSys = iWorld.GetSystem<LuaScriptSystem>();
        if (!triggerSys || !phSys || !scriptSys)
        {
          return;
        }

        GameDatabase& gameDb = *iWorld.GetSystem<GameDatabase>();
        ObjectShapeData const* shapeDesc = gameDb.GetData<ObjectShapeData>(iObject, ObjectShapeData::PropertyName());
        TriggerComponentDesc const* desc = gameDb.GetData<TriggerComponentDesc>(iObject, TriggerComponentDesc::PropertyName());

        eXl_ASSERT_REPAIR_RET(shapeDesc != nullptr, void());
        eXl_ASSERT_REPAIR_RET(desc != nullptr, void());

        LuaScriptBehaviour const* script = desc->m_Script.GetOrLoad();
        if (!script)
        {
          return;
        }
        if (shapeDesc->m_Shapes.empty())
        {
          return;
        }

        TriggerDef def;

        if (shapeDesc->IsSimpleSphere())
        {
          def.m_Geom = GeomDef::MakeSphere(shapeDesc->m_Shapes[0].m_Dims.x);
        }
        else if (shapeDesc->IsSimpleBox())
        {
          def.m_Geom = GeomDef::MakeBox(shapeDesc->m_Shapes[0].m_Dims);
        }
        else
        {
          eXl_FAIL_MSG_RET("Only simple shapes are supported for triggers", void());
        }
        
        def.m_Category = s_TriggerCategory;
        def.m_Filter = s_TriggerMask;

        phSys->AddTrigger(iObject, def, triggerSys->GetScriptCallbackhandle());
        scriptSys->AddBehaviour(iObject, *script);
      };
#endif

      auto createCharacterFactory = [](World& iWorld, ObjectHandle iObject)
      {
        static CharacterAnimation s_Anim;
        s_Anim.Register(iWorld);

        CharacterSystem& charSys = *iWorld.GetSystem<CharacterSystem>();

        if (charSys.GetDesc(iObject) != nullptr)
        {
          return;
        }

        GameDatabase& gameDb = *iWorld.GetSystem<GameDatabase>();
        ObjectShapeData const* shapeDesc = gameDb.GetData<ObjectShapeData>(iObject, ObjectShapeData::PropertyName());
        CharacterDesc const* charDesc = gameDb.GetData<CharacterDesc>(iObject, CharacterDesc::PropertyName());

        AbilitySystem& abilitySys = *iWorld.GetSystem<AbilitySystem>();

        PhysicsSystem* phSys = iWorld.GetSystem<PhysicsSystem>();
        NavigatorSystem* navSys = iWorld.GetSystem<NavigatorSystem>();

        CharacterSystem::Desc systemDesc;
        systemDesc.kind = CharacterSystem::PhysicKind::Kinematic;
        systemDesc.animation = &s_Anim;
        systemDesc.size = shapeDesc->ComputeBoundingCircle2DRadius();
        systemDesc.maxSpeed = charDesc->m_MaxSpeed;

        switch (charDesc->m_Control)
        {
        case CharacterControlKind::Navigation:
          systemDesc.controlKind = CharacterSystem::ControlKind::Navigation;
          systemDesc.kind = CharacterSystem::PhysicKind::Simulated;
          break;
        case CharacterControlKind::PlayerControl:
          systemDesc.controlKind = CharacterSystem::ControlKind::Predicted;
          systemDesc.kind = CharacterSystem::PhysicKind::Kinematic;
          break;
        case CharacterControlKind::Remote:
          systemDesc.controlKind = CharacterSystem::ControlKind::Remote;
          systemDesc.kind = CharacterSystem::PhysicKind::KinematicAbsolute;
          break;
        }

        PhysicInitData desc;
        uint32_t flags = PhysicFlags::NoGravity | PhysicFlags::LockZ | PhysicFlags::LockRotation | PhysicFlags::AlignRotToVelocity | PhysicFlags::AddSensor;

        if (systemDesc.controlKind == CharacterSystem::ControlKind::Remote)
        {
          flags |= (PhysicFlags::IsGhost | PhysicFlags::Kinematic);
        }
        else
        {
          if (systemDesc.kind == CharacterSystem::PhysicKind::Ghost
            || systemDesc.kind == CharacterSystem::PhysicKind::GhostAbsolute)
          {
            flags |= PhysicFlags::IsGhost;
          }
          if (systemDesc.kind == CharacterSystem::PhysicKind::Kinematic
            || systemDesc.kind == CharacterSystem::PhysicKind::KinematicAbsolute)
          {
            flags |= PhysicFlags::Kinematic;
          }
        }

        desc.SetFlags(flags);
        desc.AddSphere(systemDesc.size);
        desc.SetCategory(EngineCommon::s_CharacterCategory, EngineCommon::s_CharacterMask);

        phSys->CreateComponent(iObject, desc);

        if (navSys)
        {
          phSys->GetNeighborhoodExtraction().AddObject(iObject, systemDesc.size, true);
          if (systemDesc.controlKind == CharacterSystem::ControlKind::Navigation)
          {
            navSys->AddNavigator(iObject, systemDesc.size, systemDesc.maxSpeed);
          }
          else
          {
            navSys->AddObstacle(iObject, systemDesc.size);
          }
        }
        charSys.AddCharacter(iObject, systemDesc);

        abilitySys.CreateComponent(iObject);
        abilitySys.AddAbility(iObject, WalkAbility::Name());
      };

      s_EngineCommonManifest.emplace();

      s_EngineCommonManifest->RegisterComponent(GfxSpriteComponentName(), createGfxSpriteFactory, { GfxSpriteDescName() });
      s_EngineCommonManifest->RegisterComponent(PhysicsComponentName(), createPhysicsFactory, {ObjectShapeData::PropertyName(), PhysicBodyData::PropertyName()});
#ifdef EXL_LUA
      s_EngineCommonManifest->RegisterComponent(TriggerComponentName(), createTriggerFactory, {ObjectShapeData::PropertyName(), TriggerComponentDesc::PropertyName() });
#endif
      s_EngineCommonManifest->RegisterComponent(CharacterComponentName(), createCharacterFactory, { ObjectShapeData::PropertyName(), CharacterDesc::PropertyName() });
      

      InitializeYojimbo();
    }

    void _Unload()
    {
      ShutdownYojimbo();
      CharacterAnimation_StaticDestroy();
      s_NameRegistry.reset();
      s_EngineCommonManifest.reset();
    }
  };

  PLUGIN_FACTORY(EnginePlugin)
}
