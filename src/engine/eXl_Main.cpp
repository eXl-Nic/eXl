#include <engine/eXl_Main.hpp>

#include <core/corelib.hpp>
#include <core/log.hpp>

#include <core/plugin.hpp>
#include <core/random.hpp>
#include <core/clock.hpp>
#include <core/coretest.hpp>
#include <core/image/imagestreamer.hpp>
#include <core/resource/resourcemanager.hpp>

#include <core/utils/filetextreader.hpp>
#include <core/stream/inputstream.hpp>

#define IMGUI_API __declspec(dllimport)
#include <imgui.h>
#undef IMGUI_API
#undef IMGUI_IMPL_API
#define IMGUI_API
#define IMGUI_IMPL_API

#include <backends/imgui_impl_sdl.h>

#include <backends/imgui_impl_opengl3.h>

#include <SDL.h>

#include <core/stream/jsonstreamer.hpp>
#include <core/stream/jsonunstreamer.hpp>
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>

//#define USE_BAKED

#ifndef __ANDROID__
extern "C"
FILE* __iob_func()
{
  static FILE locPtr[] = { *stdin, *stdout, *stderr };
  return locPtr;
}

extern "C"
{
	__declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
}

#endif


#include <core/name.hpp>
#include <core/input.hpp>
#include <core/type/typemanager.hpp>
#include <math/mathtools.hpp>


#include "console.hpp"
#include "sdlkeytranslator.hpp"
#include "debugpanel.hpp"
#include "debugviews.hpp"
#include <engine/common/debugtool.hpp>
#include <engine/common/menumanager.hpp>
#include <engine/common/app.hpp>

#include <engine/common/transforms.hpp>
#include <engine/gfx/gfxsystem.hpp>
#include <engine/physics/physicsys.hpp>
#include <engine/pathfinding/navigator.hpp>
#include <engine/common/project.hpp>
#include <engine/game/scenariobase.hpp>

#ifndef EXL_SHARED_LIBRARY

namespace eXl
{
  Plugin& MathPlugin_GetPlugin();
  Plugin& OGLPlugin_GetPlugin();
  Plugin& EnginePlugin_GetPlugin();

  PluginLoadMap s_StaticPluginMap =
  {
    {"eXl_OGL", &OGLPlugin_GetPlugin},
    {"eXl_Math", &MathPlugin_GetPlugin},
    {"eXl_Engine", &EnginePlugin_GetPlugin}
  };
}
#endif

using namespace eXl;

class SDLFileInputStream : public InputStream
{
public:

  SDLFileInputStream(char const* iPath)
  {
    m_RWStruct = SDL_RWFromFile(iPath, "rb");
    if (m_RWStruct)
    {
      SDL_RWseek(m_RWStruct, 0, RW_SEEK_END);
      m_Size = SDL_RWtell(m_RWStruct);
      SDL_RWseek(m_RWStruct, 0, RW_SEEK_SET);
    }
    else
    {
      LOG_ERROR << "Could not open " << iPath << "\n";
    }
  }

  ~SDLFileInputStream()
  {
    if (m_RWStruct)
    {
      SDL_RWclose(m_RWStruct);
    }
  }

  size_t Read(size_t iOffset, size_t iSize, void* oData) override
  {
    if (m_CurPos != iOffset)
    {
      SDL_RWseek(m_RWStruct, iOffset, RW_SEEK_SET);
      m_CurPos = iOffset;
    }
    size_t numRead = SDL_RWread(m_RWStruct, oData, iSize, 1);
    if (numRead != 0)
    {
      m_CurPos += iSize;
    }

    return numRead * iSize;
  }

  size_t GetSize() override
  {
    return m_Size;
  }
protected:
  SDL_RWops* m_RWStruct;
  size_t m_Size = 0;
  size_t m_CurPos = 0;
};

class LogPanel : public MenuManager::Panel
{
public:

  LogPanel(ImGuiLogState& iLogState) : m_State(iLogState)
  {}

private:
  void Display() override
  {
    m_State.Display();
  }

  ImGuiLogState& m_State;
};

class LuaConsolePanel : public MenuManager::Panel
{
public:

  LuaConsolePanel(LuaConsole& iConsole) : m_Console(iConsole)
  {}

private:
  void Display() override
  {
    m_Console.Draw();
  }

  LuaConsole& m_Console;
};

struct DebugVisualizerState
{
  float m_TimeScaling = 1.0;
  bool m_ViewNavMesh = false;
  bool m_ViewPhysic = false;
  bool m_ViewNeighbours = false;
  bool m_ViewAgents = false;
};

class DebugVisualizerPanel : public MenuManager::Panel
{
public:
  DebugVisualizerPanel(DebugVisualizerState& iState) : m_State(iState)
  {

  }
private:
  void Display() override
  {
    ImGui::DragFloat("Time Scaling", &m_State.m_TimeScaling, 0.1, 0.001, 100.0, "%.3f", ImGuiSliderFlags_Logarithmic);
    ImGui::Checkbox("Nav Mesh", &m_State.m_ViewNavMesh);
    ImGui::Checkbox("Physic", &m_State.m_ViewPhysic);
    ImGui::Checkbox("Neighbours", &m_State.m_ViewNeighbours);
    ImGui::Checkbox("Nav Agents", &m_State.m_ViewAgents);
  }

  DebugVisualizerState& m_State;
};


class ProfilingPanel : public MenuManager::Panel
{
public:
  ProfilingPanel(ProfilingState const& iState) : m_State(iState)
  {}
private:
  void Display() override
  {
    ImGui::Text("Last Frame Time : %f ms", m_State.m_LastFrameTime);
    ImGui::Text("NeighExtraction Time : %f ms", m_State.m_NeighETime);
    //ImGui::Text("Navigator Time : %f ms", m_State.m_NavigatorTime);
    ImGui::Text("Physic Time : %f ms", m_State.m_PhysicTime);
    ImGui::Text("Renderer Time : %f ms", m_State.m_RendererTime);
    ImGui::Text("Transform tick Time : %f ms", m_State.m_TransformsTickTime);
  }

  ProfilingState const& m_State;
};

namespace eXl
{
  class SDL_Application : public Engine_Application
  {
  public:
    SDL_Application()
    {
      SDLKeyTranslator::Init();
    }

    SDL_Window* win = 0;
    SDL_GLContext context = 0;
    SDL_Renderer* renderer = 0;

    WorldState* m_World;
    PropertiesManifest* m_Manifest;

    void InitOGL()
    {
      if (context)
      {
        return;
      }

      context = SDL_GL_CreateContext(win);

      if (!context)
      {
        LOG_ERROR << "Failed to create OGL context" << "\n";
      }

      if (SDL_GL_SetSwapInterval(-1) != 0)
      {
        SDL_GL_SetSwapInterval(1);
      }

      SDL_GL_MakeCurrent(win, context);

      GfxSystem::StaticInit();

      m_World->Init(*m_Manifest).WithGfx();
      if (GetScenario())
      {
        m_World->WithScenario(GetScenario());
      }

      GfxSystem* gfxSys = m_World->GetWorld().GetSystem<GfxSystem>();
      DebugTool::SetDrawer(gfxSys->GetDebugDrawer());
    }

    void Start_SDLApp()
    {
      SDL_SetMainReady();

      SDL_Init(SDL_INIT_VIDEO);

      Vector2i viewportSize(m_Width, m_Height);
      uint32_t windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
#ifdef __ANDROID__
      SDL_DisplayMode mode;
      SDL_GetDisplayMode(0, 0, &mode);
      viewportSize.X() = mode.w;
      viewportSize.Y() = mode.h;
      //windowFlags |= SDL_WINDOW_FULLSCREEN;

      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
      SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
      SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

#endif
      win = SDL_CreateWindow("eXl", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, viewportSize.X(), viewportSize.Y(), windowFlags);

      InitOGL();

      for (uint32_t logLevel = INFO_STREAM; logLevel <= ERROR_STREAM; ++logLevel)
      {
        Log_Manager::AddOutput(eXl_NEW ImGuiLogOutput(m_LogState, logLevel), 1 << logLevel);
      }

#ifndef __ANDROID__
      IMGUI_CHECKVERSION();
      ImGui::CreateContext();
      ImGuiIO& io = ImGui::GetIO();
      io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

      ImGui_ImplSDL2_InitForOpenGL(win, context);
      ImGui_ImplOpenGL3_Init();
      
      // Setup style
      ImGui::StyleColorsDark();

      MenuManager& menuMgr = GetMenuManager();
      menuMgr.AddMenu("View")
        .AddOpenPanelCommand("Log", [this] {return eXl_NEW LogPanel(m_LogState); })
        .AddOpenPanelCommand("Profiling", [this] {return eXl_NEW ProfilingPanel(m_World->GetProfilingState()); })
        .AddOpenPanelCommand("Debug", [this] {return eXl_NEW DebugPanel(m_World->GetWorld()); })
        .AddOpenPanelCommand("Debug Visualizer", [this] {return eXl_NEW DebugVisualizerPanel(m_DebugVisState); })
        //.AddOpenPanelCommand("Console", [this] {return eXl_NEW LuaConsolePanel(m_Console); })
        .EndMenu();
#endif

      m_World->GetCamera().view.viewportSize = viewportSize;
    }

    void PumpMessages(float iDelta)
    {
      SDL_Event curEvent;
      while (SDL_PollEvent(&curEvent))
      {
#ifndef __ANDROID__
        ImGui_ImplSDL2_ProcessEvent(&curEvent);
#endif

        if (curEvent.type == SDL_MOUSEMOTION)
        {
          SDL_MouseMotionEvent& mouseEvt = reinterpret_cast<SDL_MouseMotionEvent&>(curEvent);

          m_MousePos.X() = mouseEvt.x;
          m_MousePos.Y() = mouseEvt.y;

          GetInputSystem().InjectMouseMoveEvent(mouseEvt.x, mouseEvt.y, mouseEvt.xrel, mouseEvt.yrel, false);
        }

        if (curEvent.type == SDL_MOUSEWHEEL)
        {
          SDL_MouseWheelEvent& wheelEvt = reinterpret_cast<SDL_MouseWheelEvent&>(curEvent);
          GetInputSystem().InjectMouseMoveEvent(wheelEvt.y, wheelEvt.y, wheelEvt.y, wheelEvt.y, true);
        }

        if (curEvent.type == SDL_MOUSEBUTTONDOWN
          || curEvent.type == SDL_MOUSEBUTTONUP)
        {
          SDL_MouseButtonEvent& mouseEvt = reinterpret_cast<SDL_MouseButtonEvent&>(curEvent);

          MouseButton button;
          switch (mouseEvt.button)
          {
          case SDL_BUTTON_LEFT:
            button = MouseButton::Left;
            break;
          case SDL_BUTTON_RIGHT:
            button = MouseButton::Right;
            break;
          case SDL_BUTTON_MIDDLE:
            button = MouseButton::Middle;
            break;
          default:
            continue;
            break;
          }

          if (mouseEvt.button == SDL_BUTTON_RIGHT)
          {
            PhysicsSystem& phSys = *m_World->GetWorld().GetSystem<PhysicsSystem>();
            GfxSystem& gfxSys = *m_World->GetWorld().GetSystem<GfxSystem>();

            Vector3f worldPos;
            Vector3f viewDir;
            gfxSys.ScreenToWorld(m_MousePos, worldPos, viewDir);

            List<CollisionData> colResult;

            phSys.RayQuery(colResult, worldPos, worldPos - viewDir * 10000);

            if (!colResult.empty())
            {
              DebugTool::SelectObject(colResult.front().obj1);
            }
          }

          GetInputSystem().InjectMouseEvent(button, curEvent.type == SDL_MOUSEBUTTONDOWN);
        }

        bool keyChanged = false;
        if (curEvent.type == SDL_KEYUP
          || curEvent.type == SDL_KEYDOWN)
        {
          SDL_KeyboardEvent& keyEvt = reinterpret_cast<SDL_KeyboardEvent&>(curEvent);
          if (keyEvt.keysym.sym == SDLK_BACKQUOTE)
          {
            if (curEvent.type == SDL_KEYUP)
            {
              m_ConsoleOpen = !m_ConsoleOpen;
            }
          }
          else
          {
            GetInputSystem().InjectKeyEvent(0, SDLKeyTranslator::Translate(keyEvt.keysym.scancode), curEvent.type == SDL_KEYDOWN);
          }
        }

        if (curEvent.type == SDL_WINDOWEVENT)
        {
          SDL_WindowEvent& winEvt = reinterpret_cast<SDL_WindowEvent&>(curEvent);
          if (winEvt.event == SDL_WINDOWEVENT_CLOSE)
          {
            exit(0);
          }
          if (winEvt.event == SDL_WINDOWEVENT_RESIZED)
          {
						Vector2i viewportSize;
            viewportSize.X() = winEvt.data1;
            viewportSize.Y() = winEvt.data2;

            m_World->GetCamera().view.viewportSize = viewportSize;
          }
          if (winEvt.event == SDL_WINDOWEVENT_SHOWN)
          {
            InitOGL();
          }

        }
        if (curEvent.type == SDL_QUIT)
        {
          exit(0);
        }
      }
    }

    void Tick(float iDelta) override
    {
      PumpMessages(iDelta);

      if (context == 0)
      {
        return;
      }

      Engine_Application::Tick(iDelta);

      MenuManager& menuMgr = GetMenuManager();
#ifndef __ANDROID__
      
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplSDL2_NewFrame(win);

      ImGui::NewFrame();

      if (ImGui::BeginMainMenuBar())
      {
        menuMgr.DisplayMenus();
        ImGui::EndMainMenuBar();
      }
#endif

      Transforms& transforms = *m_World->GetWorld().GetSystem<Transforms>();
      PhysicsSystem& phSys = *m_World->GetWorld().GetSystem<PhysicsSystem>();
      NavigatorSystem& navigator = *m_World->GetWorld().GetSystem<NavigatorSystem>();
      {
        bool displayPhysSaveState = m_DebugVisState.m_ViewPhysic;
#ifndef __ANDROID__
        menuMgr.DisplayPanels();
#endif

        if (displayPhysSaveState != m_DebugVisState.m_ViewPhysic)
        {
          if (m_DebugVisState.m_ViewPhysic)
          {
            phSys.EnableDebugDraw(*DebugTool::GetDrawer());
          }
          else
          {
            phSys.DisableDebugDraw();
          }
        }
      }

      World& world = m_World->GetWorld();

      float scaledTime = iDelta * m_DebugVisState.m_TimeScaling;

      InputSystem& inputs = GetInputSystem();

      //m_CamState.ProcessInputs(world, inputs);
      m_World->Tick(scaledTime);

      inputs.Clear();

      m_World->GetCamera().UpdateView(world);
      m_World->Render(m_World->GetCamera().view, scaledTime);

      if (m_DebugVisState.m_ViewNavMesh)
      {
        if (auto const* mesh = navigator.GetNavMesh())
        {
          DrawNavMesh(*mesh, *DebugTool::GetDrawer());
        }
      }

      //DrawRooms(rooms[0], gfxSys.GetDebugDrawer());

      if (m_DebugVisState.m_ViewNeighbours)
      {
        DrawNeigh(phSys.GetNeighborhoodExtraction(), transforms, *DebugTool::GetDrawer());
      }

      if (m_DebugVisState.m_ViewAgents)
      {
        DrawNavigator(navigator, transforms, *DebugTool::GetDrawer());
      }

#ifndef __ANDROID__
      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

      SDL_GL_SwapWindow(win);
    }

    ImGuiLogState m_LogState;

    bool m_ConsoleOpen = true;
    LuaConsole m_Console;
    DebugVisualizerState m_DebugVisState;
    Vector2i m_MousePos;
  };
}

namespace
{
  SDL_Application& GetApp()
  {
    static SDL_Application s_App;
    return s_App;
  }
}

eXl_Main::eXl_Main()
{
  GetApp();
}

int eXl_Main::Start(int argc, char const* const argv[])
{
  SDL_Application& app = GetApp();
  InitConsoleLog();

  app.SetWindowSize(1024, 768);
  app.SetArgs(argc, argv);

  WorldState world;
  app.m_World = &world;

  app.Start();

  Path projectPath = app.GetProjectPath();

#if defined(WIN32) && !defined(USE_BAKED)

  ResourceManager::SetTextFileReadFactory([](char const* iFilePath)
  {
    return std::unique_ptr<TextReader>(FileTextReader::Create(iFilePath));
  });
#endif

#if defined(__ANDROID__) || defined(USE_BAKED)
  ResourceManager::SetTextFileReadFactory([](char const* iFilePath) -> std::unique_ptr<TextReader>
    {
      std::unique_ptr<SDLFileInputStream> stream = std::make_unique<SDLFileInputStream>(iFilePath);
      if (stream->GetSize() > 0)
      {
        return std::make_unique<InputStreamTextReader>(std::move(stream));
      }
      return std::unique_ptr<TextReader>();
    });
#endif

  Path appPath(GetAppPath().begin(), GetAppPath().end());

  Project::ProjectTypes types;
  PropertiesManifest appManifest = EngineCommon::GetBaseProperties();
  app.m_Manifest = &appManifest;

  if (!projectPath.empty())
  {
    Path projectDir = projectPath.parent_path();
#if defined(USE_BAKED) && defined(WIN32)
    ResourceManager::BootstrapAssetsFromManifest(projectDir);
#else
    ResourceManager::BootstrapDirectory(projectDir, true);
#endif

    Project* project = ResourceManager::Load<Project>(projectPath);
    if (!project)
    {
      return -1;
    }

    project->FillProperties(types, appManifest);

    ResourceManager::AddManifest(EngineCommon::GetComponents());
    ResourceManager::AddManifest(appManifest);
  }

#if defined(__ANDROID__)
  ResourceManager::BootstrapAssetsFromManifest(String());
#endif

  //ResourceManager::Bake(Path("D:/eXlBakedProject"));

  if (!app.GetScenario() && !app.GetMapPath().empty())
  {
    std::unique_ptr<Scenario_Base> scenario = std::make_unique<Scenario_Base>();
    
    MapResource const* mapRsc = ResourceManager::Load<MapResource>(app.GetMapPath());
    if (mapRsc)
    {
      ResourceHandle<MapResource> mapRef;
      mapRef.Set(mapRsc);
      scenario->SetMap(mapRef);
    }
    app.SetScenario(std::move(scenario));
  }

  app.Start_SDLApp();

  app.DefaultLoop();
  
  return 0;
}
