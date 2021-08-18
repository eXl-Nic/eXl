/*
Copyright 2009-2021 Nicolas Colombe

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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