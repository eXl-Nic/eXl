#pragma once

#include <engine/enginelib.hpp>
#include <engine/common/menumanager.hpp>

#include <core/application.hpp>
#include <engine/gfx/gfxsystem.hpp>


namespace eXl
{
  class PropertiesManifest;
  class MenuManager;
  class InputSystem;

  class World;

  class WorldState;
  struct CameraState;

  class EXL_ENGINE_API Scenario
  {
  public:
    virtual void Init(World& iWorld) = 0;

    //virtual void Step(World& iWorld, float iDelta) = 0;

    CameraState& GetCamera();

  private:
    friend WorldState;
    WorldState* m_WorldState = nullptr;
  };

  struct EXL_ENGINE_API CameraState
  {
    void Init(World& iWorld);

    enum InputToProcess
    {
      WheelZoom = 1<<0,
      RightClickPan = 1<<1,
      KeyMove = 1<<2,
    };

    void ProcessInputs(World& iWorld, InputSystem& iInputs, uint32_t iControlScheme = WheelZoom | KeyMove);

    void UpdateView(World& iWorld);

    GfxSystem::ViewInfo view;
    ObjectHandle cameraObj;

    uint32_t dirMask = 0;
    bool keyChanged = false;

    Vector2i lastMousePos;
    bool rightClickMoving = false;
  };

  class EXL_ENGINE_API WorldState
  {
  public:

    WorldState();
    ~WorldState();
    WorldState(WorldState&&);
    WorldState& operator=(WorldState&&);

    WorldState& Init(PropertiesManifest const& iProperties);

    WorldState& WithGfx();

    WorldState& WithScenario(Scenario* iScenario);

    void Tick(float iDelta);

    void Render(GfxSystem::ViewInfo& iView, float iDelta);

    ProfilingState const& GetProfilingState();

    World& GetWorld();

    CameraState& GetCamera() { return m_CamState; }

  protected:

    WorldState(WorldState const&) = delete;
    WorldState& operator=(WorldState const&) = delete;

    struct Impl;
    std::unique_ptr<Impl> m_Impl;

    CameraState m_CamState;
  };

  class EXL_ENGINE_API DunAtk_Application : public Application
  {
    struct Impl;
  public:

    static DunAtk_Application& GetAppl();

    DunAtk_Application();
    ~DunAtk_Application();

    void Start() override;

    MenuManager& GetMenuManager();
    InputSystem& GetInputSystem();

  private:
    std::unique_ptr<Impl> m_Impl;
  };
}