/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <core/plugin.hpp>
#include <core/log.hpp>
#include <core/corelib.hpp>


#include <engine/game/commondef.hpp>

#include <engine/gfx/tileset.hpp>
#include <engine/map/tilinggroup.hpp>
#include <engine/map/mcmcmodelrsc.hpp>
#include <engine/common/project.hpp>
#include <engine/game/archetype.hpp>
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

namespace eXl
{
  void CharacterAnimation_StaticInit();
  void CharacterAnimation_StaticDestroy();
  void Script_StaticInit();
  void Script_StaticDestroy();

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

    boost::optional<NameRegistry> s_NameRegistry;
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
  PropertySheetName EngineCommon::VelocityName() { return s_NameRegistry->m_VelocityName; }

  PropertySheetName EngineCommon::GrabData::PropertyName() { return s_NameRegistry->m_GrabDataName; }
  PropertySheetName EngineCommon::HealthData::PropertyName() { return s_NameRegistry->m_HealthDataName; }
  PropertySheetName EngineCommon::TurretData::PropertyName() { return s_NameRegistry->m_TurretDataName; }
  PropertySheetName EngineCommon::TerrainCarver::PropertyName() { return s_NameRegistry->m_TerrainCarverName; }
  PropertySheetName EngineCommon::GfxSpriteDescName() { return s_NameRegistry->m_SpriteDescDataName; }
  PropertySheetName EngineCommon::PhysicsInitDataName() { return s_NameRegistry->m_PhysicsInitDataName; }

  void Register_ENGINE_Types();
  LUA_REG_FUN(BindDunatk);

  boost::optional<ComponentManifest> s_EngineCommonManifest;

  ComponentManifest const& EngineCommon::GetComponents() { return s_EngineCommonManifest.get(); }
  PropertiesManifest EngineCommon::GetBaseProperties()
  {
    PropertiesManifest baseManifest;
    baseManifest.RegisterPropertySheet<HealthData>(HealthData::PropertyName());
    baseManifest.RegisterPropertySheet<GrabData>(GrabData::PropertyName());
    baseManifest.RegisterPropertySheet<TurretData>(TurretData::PropertyName());
    baseManifest.RegisterPropertySheet<TerrainCarver>(TerrainCarver::PropertyName());

    baseManifest.RegisterPropertySheet<Vector3f>(VelocityName(), false);
    baseManifest.RegisterPropertySheet<GfxSpriteComponent::Desc>(EngineCommon::GfxSpriteDescName(), false);
    baseManifest.RegisterPropertySheet<PhysicInitData>(EngineCommon::PhysicsInitDataName(), false);

    return baseManifest;
  }

  class EnginePlugin : public Plugin
  {
  public:
    EnginePlugin() 
      : Plugin("EbninePlugin")
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
      Script_StaticInit();

      Tileset::Init();
      TilingGroup::Init();
      Project::Init();
      Archetype::Init();
      MapResource::Init();
      MCMCModelRsc::Init();
      FontResource::Init();
      //CharacterAnimation::Init();
#ifdef EXL_LUA
      LuaScriptBehaviour::Init();
#endif

      Register_ENGINE_Types();
#ifdef EXL_LUA
      LuaManager::AddRegFun(&BindDunatk);
#endif

      TypeManager::RegisterCoreType<ObjectHandle>();

      auto createGfxSpriteFactory = [](World& iWorld, ObjectHandle iObject, ConstDynObject const& iComponentData)
      {
#ifdef EXL_WITH_OGL
        if (GfxSystem* gfx = iWorld.GetSystem<GfxSystem>())
        {
          GfxSpriteComponent::Desc const* spriteDesc = iComponentData.CastBuffer<GfxSpriteComponent::Desc>();
          GfxSpriteComponent& spriteComp = gfx->CreateSpriteComponent(iObject);
          spriteComp.SetDesc(*spriteDesc);
        }
#endif
      };

      auto createPhysicsFactory = [](World& iWorld, ObjectHandle iObject, ConstDynObject const& iComponentData)
      {
        PhysicsSystem& ph = *iWorld.GetSystem<PhysicsSystem>();
        EngineCommon::PhysicsCompTestData const* phDesc = iComponentData.CastBuffer<EngineCommon::PhysicsCompTestData>();

        PhysicInitData initData;
        initData.SetFlags(EngineCommon::s_BasePhFlags);
        switch (phDesc->m_Type)
        {
        case EngineCommon::PhysicsType::Static:
          initData.SetFlags(initData.GetFlags() | PhysicFlags::Static);
          break;
        case EngineCommon::PhysicsType::Kinematic:
          initData.SetFlags(initData.GetFlags() | PhysicFlags::Kinematic);
          break;
        }

        switch (phDesc->m_Category)
        {
        case EngineCommon::PhysicsCollisionCategory::Default:
          initData.SetCategory(EngineCommon::s_DefaultCategory, ~1);
          break;
        case EngineCommon::PhysicsCollisionCategory::Wall:
          initData.SetCategory(EngineCommon::s_WallCategory, EngineCommon::s_WallMask);
          break;
        case EngineCommon::PhysicsCollisionCategory::Character:
          initData.SetCategory(EngineCommon::s_CharacterCategory, EngineCommon::s_CharacterMask);
          break;
        case EngineCommon::PhysicsCollisionCategory::Trigger:
          initData.SetCategory(EngineCommon::s_TriggerCategory, EngineCommon::s_TriggerMask);
          break;
        case EngineCommon::PhysicsCollisionCategory::Projectile:
          initData.SetCategory(EngineCommon::s_ProjectileCategory, EngineCommon::s_ProjectileMask);
          break;
        case EngineCommon::PhysicsCollisionCategory::None:
          initData.SetCategory(0, 0);
          break;
        }

        for (auto const& shape : phDesc->m_Shapes)
        {
          switch (shape.m_Type)
          {
          case EngineCommon::PhysicsShapeType::Sphere:
            initData.AddSphere(shape.m_Dims.X(), shape.m_Offset);
            break;
          case EngineCommon::PhysicsShapeType::Box:
            initData.AddBox(shape.m_Dims, shape.m_Offset);
            break;
          }
        }

        ph.CreateComponent(iObject, initData);
      };

      auto createTriggerFactory = [](World& iWorld, ObjectHandle iObject, ConstDynObject const& iComponentData)
      {
        ScriptTriggerSystem* triggerSys = iWorld.GetSystem<ScriptTriggerSystem>();
        PhysicsSystem* phSys = iWorld.GetSystem<PhysicsSystem>();
        LuaScriptSystem* scriptSys = iWorld.GetSystem<LuaScriptSystem>();
        if (!triggerSys || !phSys || !scriptSys)
        {
          return;
        }

        EngineCommon::TriggerComponentDesc const* desc = iComponentData.CastBuffer<EngineCommon::TriggerComponentDesc>();

        LuaScriptBehaviour const* script = desc->m_Script.GetOrLoad();
        if (!script)
        {
          return;
        }

        TriggerDef def;

        switch (desc->m_Shape.m_Type)
        {
        case EngineCommon::PhysicsShapeType::Sphere:
          def.m_Geom = GeomDef::MakeSphere(desc->m_Shape.m_Dims.X());
          break;
        case EngineCommon::PhysicsShapeType::Box:
          def.m_Geom = GeomDef::MakeBox(desc->m_Shape.m_Dims);
          break;
        }

        def.m_Category = EngineCommon::s_TriggerCategory;
        def.m_Filter = EngineCommon::s_TriggerMask;

        phSys->AddTrigger(iObject, def, triggerSys->GetScriptCallbackhandle());
        scriptSys->AddBehaviour(iObject, *script);
      };

      s_EngineCommonManifest.emplace();

      s_EngineCommonManifest->RegisterComponent(EngineCommon::GfxSpriteComponentName(), GfxSpriteComponent::Desc::GetType(), createGfxSpriteFactory);
      s_EngineCommonManifest->RegisterComponent(EngineCommon::PhysicsComponentName(), EngineCommon::PhysicsCompTestData::GetType(), createPhysicsFactory);
      s_EngineCommonManifest->RegisterComponent(EngineCommon::TriggerComponentName(), EngineCommon::TriggerComponentDesc::GetType(), createTriggerFactory);
#if 0
      String appPath(GetAppPath());
      Path appDir(appPath.c_str());
      appDir = appDir.parent_path();

      String homeArg("--home=");
      Path pl_homeFile = appDir / "swipl.home";
      std::ifstream homeFile(pl_homeFile.string());
      if (homeFile.is_open())
      {
        char pl_homePath[256] = { 0 };
        homeFile.read(pl_homePath, sizeof(pl_homePath) - 1);
        homeArg += pl_homePath;
        while (std::isspace(homeArg.back()))
        {
          homeArg.pop_back();
        }
      }
      else
      {
        homeArg += appDir.string().c_str();
      }
      
      char* pl_argv[3] = { const_cast<char*>(appPath.c_str()), const_cast<char*>(homeArg.c_str()), "--quiet" };
      if (!PL_initialise(3, pl_argv))
      {
        eXl_ASSERT(false);
      }
#endif

      
#ifdef EXL_LUA
      BehaviourDesc desc;
      desc.behaviourName = "Trigger";
      desc.functions.insert(std::make_pair("Enter", FunDesc::Create<void(ObjectHandle, ObjectHandle)>()));
      desc.functions.insert(std::make_pair("Leave", FunDesc::Create<void(ObjectHandle, ObjectHandle)>()));

      LuaScriptSystem::AddBehaviourDesc(desc);
#endif
    }

    void _Unload()
    {
      Script_StaticDestroy();
      CharacterAnimation_StaticDestroy();
      s_NameRegistry.reset();
      s_EngineCommonManifest.reset();
    }
  };

  PLUGIN_FACTORY(EnginePlugin)
}
