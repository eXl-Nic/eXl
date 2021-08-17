#include <engine/common/app.hpp>

#include <engine/common/menumanager.hpp>

#include <core/plugin.hpp>
#include <core/input.hpp>
#include <core/clock.hpp>
#include <math/mathtools.hpp>

#include <engine/gfx/gfxsystem.hpp>

#include <engine/physics/physicsys.hpp>
#include <engine/pathfinding/navigator.hpp>
#include <engine/game/character.hpp>
#include <engine/game/ability.hpp>
#include <engine/game/projectile.hpp>
#include <engine/common/transformanim.hpp>
#include <engine/game/archetype.hpp>
#include <engine/script/luascriptsystem.hpp>
#include <engine/game/scripttrigger.hpp>

namespace eXl
{
  struct WorldState::Impl
  {
    Impl(PropertiesManifest const& iManifest);

    World world;

    PropertiesManifest m_Manifest;
    Transforms* transforms;
    GfxSystem* gfxSys;
    PhysicsSystem* phSys;
    CharacterSystem* characters;
    NavigatorSystem* navigator;
    AbilitySystem* abilities;
    ProjectileSystem* projectiles;

    void Tick(float iDelta);

    void Render(GfxSystem::ViewInfo& iView, float iDelta);

    void SetAndInitScenario(Scenario* iScenario)
    {
      scenario = iScenario;
      scenario->Init(world);
    }

    Scenario* scenario = nullptr;

    ProfilingState m_ProfilingState;
  };

  struct DunAtk_Application::Impl : public HeapObject
  {
    MenuManager m_MenuManager;
    InputSystem m_Inputs;

  };

  DunAtk_Application& DunAtk_Application::GetAppl()
  {
    return static_cast<DunAtk_Application&>(Application::GetAppl());
  }

  DunAtk_Application::DunAtk_Application() : m_Impl(eXl_NEW Impl)
  {

  }

  DunAtk_Application::~DunAtk_Application() = default;

  void DunAtk_Application::Start()
  {
    Application::Start();
    //Plugin::LoadLib("eXl_Math");

#ifdef EXL_IMAGESTREAME_ENABLED
    ImageStreamer::Initialize();
#endif

    //Plugin::LoadLib("eXl_OGL");
  }

  WorldState::WorldState()
  {

  }

  WorldState::~WorldState()
  {

  }

  WorldState::WorldState(WorldState&& iOther)
  {
    if (this != &iOther)
    {
      m_Impl = std::move(iOther.m_Impl);
      m_CamState = iOther.m_CamState;
    }
  }

  WorldState& WorldState::operator=(WorldState&& iOther)
  {
    if (this == &iOther)
    {
      return *this;
    }
    this->~WorldState();
    new (this) WorldState(std::move(iOther));

    return *this;
  }

  CameraState& Scenario::GetCamera()
  {
    return m_WorldState->GetCamera();
  }

  WorldState& WorldState::Init(PropertiesManifest const& iProperties)
  {
    m_Impl = std::make_unique<Impl>(iProperties);
    m_CamState.Init(m_Impl->world);

    return *this;
  }

  WorldState& WorldState::WithGfx()
  {
#ifdef EXL_WITH_OGL
    m_Impl->gfxSys = m_Impl->world.AddSystem(std::make_unique<GfxSystem>(*m_Impl->transforms));
#endif
    return *this;
  }

  WorldState& WorldState::WithScenario(Scenario* iScenario)
  {
    iScenario->m_WorldState = this;
    m_Impl->SetAndInitScenario(iScenario);
    return *this;
  }

  ProfilingState const& WorldState::GetProfilingState()
  {
    return m_Impl->m_ProfilingState;
  }

  World& WorldState::GetWorld()
  {
    return m_Impl->world;
  }

  void WorldState::Tick(float iDelta)
  {
    m_Impl->Tick(iDelta);
  }

  void WorldState::Render(GfxSystem::ViewInfo& iView, float iDelta)
  {
    m_Impl->Render(iView, iDelta);
  }

  WorldState::Impl::Impl(PropertiesManifest const& iManifest)
    : world(DunAtk::GetComponents())
    , m_Manifest(iManifest)
  {
    transforms = world.AddSystem(std::make_unique<Transforms>());

    phSys = world.AddSystem(std::make_unique<PhysicsSystem>(*transforms));
    characters = world.AddSystem(std::make_unique<CharacterSystem>());
    navigator = world.AddSystem(std::make_unique<NavigatorSystem>(*transforms));
    abilities = world.AddSystem(std::make_unique<AbilitySystem>());
    projectiles = world.AddSystem(std::make_unique<ProjectileSystem>());
    world.AddSystem(std::make_unique<TransformAnimManager>());
    world.AddSystem(std::make_unique<GameDatabase>(m_Manifest));
#ifdef EXL_LUA
    world.AddSystem(std::make_unique<LuaScriptSystem>());
#endif

    phSys->AddKinematicController(characters);
    phSys->AddKinematicController(projectiles);
    phSys->AddKinematicController(&navigator->GetController());

    world.AddSystem(std::make_unique<ScriptTriggerSystem>());

    //world.AddTick(World::PrePhysics, [this](World&, float iDelta) { navigator->Tick(iDelta, phSys->GetNeighborhoodExtraction()); });
    world.AddTick(World::PostPhysics, [this](World&, float iDelta) { characters->Tick(iDelta); });
    world.AddTick(World::PostPhysics, [this](World&, float iDelta) { projectiles->Tick(iDelta); });
  }


  void CameraState::Init(World& iWorld)
  {
    Transforms& transforms = *iWorld.GetSystem<Transforms>();
    PhysicsSystem& phSys = *iWorld.GetSystem<PhysicsSystem>();
    CharacterSystem& controller = *iWorld.GetSystem<CharacterSystem>();

    //view.projection = GfxSystem::Perspective;
    view.projection = GfxSystem::Orthographic;
    view.displayedSize = 10 * 4.0;

    float const viewAngle = 0.0 * Mathf::PI;

    view.basis[0] = Vector3f::UNIT_X;
    view.basis[1] = Vector3f::UNIT_Y * Mathf::Cos(viewAngle) + Vector3f::UNIT_Z * Mathf::Sin(viewAngle);
    view.basis[2] = Vector3f::UNIT_Y * -Mathf::Sin(viewAngle) + Vector3f::UNIT_Z * Mathf::Cos(viewAngle);

    view.pos = view.basis[2] * 100;

    cameraObj = iWorld.CreateObject();

    Matrix4f camOffset;
    camOffset.MakeIdentity();
    MathTools::GetPosition(camOffset) = view.pos;
    transforms.AddTransform(cameraObj, &camOffset);

    {
      PhysicInitData desc;
      desc.SetFlags(PhysicFlags::NoGravity | PhysicFlags::LockZ | PhysicFlags::Kinematic | PhysicFlags::IsGhost);
      desc.AddSphere(4.0 * 0.25);
      //Matrix4f mainTrans = transforms.GetLocalTransform(cameraObj);
      //MathTools::GetPosition2D(mainTrans) = /*roomCenter - Vector2f::UNIT_Y * 0.25*/ Vector2f::ZERO;
      //transforms.UpdateTransform(cameraObj, mainTrans);
      //
      phSys.CreateComponent(cameraObj, desc);

      //GfxComponent& testComp = gfxSys.CreateComponent(cameraObj);
      //testComp.SetGeometry(characterSprite.get());
      //testComp.AddDraw(material, 6, 0);

      CharacterSystem::Desc cameraDesc;
      cameraDesc.controlKind = CharacterSystem::ControlKind::Predicted;
      cameraDesc.kind = CharacterSystem::PhysicKind::Kinematic;
      //cameraDesc.isSolid = false;
      cameraDesc.size = 1.0;
      cameraDesc.maxSpeed = 100.0;
      
      controller.AddCharacter(cameraObj, cameraDesc);
      controller.SetSpeed(cameraObj, 50.0);
    }
  }

  void CameraState::ProcessInputs(World& iWorld, InputSystem& iInputs, uint32_t iControlScheme)
  {
    CharacterSystem& controller = *iWorld.GetSystem<CharacterSystem>();

    if (iControlScheme & KeyMove)
    {
      for (auto const& evt : iInputs.m_KeyEvts)
      {
        if (!evt.pressed)
        {
          if (evt.key == K_UP)
          {
            dirMask &= ~(1 << 2);
            keyChanged = true;
          }
          if (evt.key == K_DOWN)
          {
            dirMask &= ~(1 << 3);
            keyChanged = true;
          }
          if (evt.key == K_LEFT)
          {
            dirMask &= ~(1 << 1);
            keyChanged = true;
          }
          if (evt.key == K_RIGHT)
          {
            dirMask &= ~(1 << 0);
            keyChanged = true;
          }
        }
        else
        {
          if (evt.key == K_UP)
          {
            dirMask |= 1 << 2;
            keyChanged = true;
          }
          if (evt.key == K_DOWN)
          {
            dirMask |= 1 << 3;
            keyChanged = true;
          }
          if (evt.key == K_LEFT)
          {
            dirMask |= 1 << 1;
            keyChanged = true;
          }
          if (evt.key == K_RIGHT)
          {
            dirMask |= 1 << 0;
            keyChanged = true;
          }
        }
      }
    }

    for (auto const& evt : iInputs.m_MouseMoveEvts)
    {
      if (iControlScheme & WheelZoom)
      {
        if (evt.wheel)
        {
          if (evt.relY < 0)
          {
            view.displayedSize *= 1.1;
          }
          else
          {
            view.displayedSize *= 0.90;
          }
          view.displayedSize = Mathf::Clamp(view.displayedSize, 1, 10000);
        }
      }

      if (iControlScheme & RightClickPan)
      {
        Vector2i curMousePos(evt.absX, evt.absY);
        if (rightClickMoving)
        {
          Transforms& trans = *iWorld.GetSystem<Transforms>();
          GfxSystem& gfx = *iWorld.GetSystem<GfxSystem>();
          Vector3f prevWorldPos;
          Vector3f curWorldPos;
          Vector3f view;
          gfx.ScreenToWorld(lastMousePos, prevWorldPos, view);
          gfx.ScreenToWorld(curMousePos, curWorldPos, view);
          Vector3f moveVector = curWorldPos - prevWorldPos;
          Matrix4f cameraTrans = trans.GetLocalTransform(cameraObj);
          MathTools::GetPosition(cameraTrans) -= moveVector;
          trans.UpdateTransform(cameraObj, cameraTrans);
        }
        lastMousePos = curMousePos;
      }
    }

    for (auto const& evt : iInputs.m_MouseEvts)
    {
      if (iControlScheme & RightClickPan)
      {
        if (evt.button == MouseButton::Right)
        {
          rightClickMoving = evt.pressed;
        }
      }
    }

    if (keyChanged)
    {
      static const Vector3f dirs[] =
      {
        Vector3f::UNIT_X *  1.0,
        Vector3f::UNIT_X * -1.0,
        Vector3f::UNIT_Y *  1.0,
        Vector3f::UNIT_Y * -1.0,
      };
      Vector3f dir;
      for (unsigned int i = 0; i < 4; ++i)
      {
        if (dirMask & (1 << i))
        {
          dir += dirs[i];
        }
      }
      controller.SetCurDir(cameraObj, dir);
      keyChanged = false;
    }
  }

  void CameraState::UpdateView(World& iWorld)
  {
    Transforms& transforms = *iWorld.GetSystem<Transforms>();

    Matrix4f camPos = transforms.GetWorldTransform(cameraObj);
    
    view.pos.X() = camPos.m_Data[12];
    view.pos.Y() = camPos.m_Data[13];
    view.pos.Z() = camPos.m_Data[14];
  }

  void WorldState::Impl::Tick(float iDelta)
  {
    DunAtk_Application& app = DunAtk_Application::GetAppl();
    InputSystem& inputs = app.GetInputSystem();

    world.Tick(iDelta, m_ProfilingState);

    inputs.Clear();
  }

  void WorldState::Impl::Render(GfxSystem::ViewInfo& iView, float iDelta)
  {
#ifdef EXL_WITH_OGL
    Clock profiler;

    gfxSys->SynchronizeTransforms();

    gfxSys->SetView(iView);

    gfxSys->RenderFrame(iDelta);
    //view.pos.Z() -= 1;

    m_ProfilingState.m_RendererTime = profiler.GetTime() * 1000.0;
#endif
  }

  MenuManager& DunAtk_Application::GetMenuManager() 
  { 
    return m_Impl->m_MenuManager;
  }

  InputSystem& DunAtk_Application::GetInputSystem()
  {
    return m_Impl->m_Inputs;
  }
}