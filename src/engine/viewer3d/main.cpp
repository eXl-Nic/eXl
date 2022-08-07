
#include <engine/common/world.hpp>
#include <engine/common/app.hpp>
#include <core/input.hpp>
#include <core/path.hpp>
#include <core/image/image.hpp>
#include <core/image/imagestreamer.hpp>
#include <core/type/typemanager.hpp>

#include <engine/gfx/gfxsystem.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/game/character.hpp>
#include <engine/common/debugtool.hpp>

#include <math/mathtools.hpp>

#include <imgui.h>
#include <engine/gfx/gfx3dscene.hpp>
#include <engine/gfx/model_importer.hpp>

namespace eXl
{
  class World;
  class Random;
  class ViewerApp : public Scenario
  {
  public:
    ViewerApp()
    {}


    void PreInit(World& iWorld) override
    {
      m_Scene.Initialize(*iWorld.GetSystem<GfxSystem>());
    }

    void Init(World& iWorld) override;

    void Step(World& iWorld, float iDelta);

    void ProcessInputs(World& iWorld);

    Gfx3DScene m_Scene;
    uint32_t dirMask = 0;
    bool keyChanged = false;
    bool moveCam = false;

  };

  class ViewerAppPanel : public MenuManager::Panel
  {
  public:
    ViewerAppPanel(World& iWorld, ViewerApp& iScenario)
      : m_World(iWorld)
      , m_Scenario(iScenario)
    {

    }

  protected:
    void Display() override
    {
    }
    World& m_World;
    ViewerApp& m_Scenario;
  };



#if 0
  OGLTextureCache::LoadingCallback MakeLoadCB()
  {
    return [](TextureKey iKey)
    {
      Image* img = ImageStreamer::Load(String(iKey.get()));
      auto ret = IntrusivePtr<OGLTexture>(OGLTextureLoader::CreateFromImage(img, false));
      eXl_DELETE(img);
      return ret;
    };
  }
#endif

  struct BuildTask
  {
    uint32_t m_CellId;
    Box3D m_CellBox;
    Vector<uint32_t> m_Objects;
  };

  void ViewerApp::Init(World& iWorld)
  {

    iWorld.AddTick(World::Stage::FrameStart, [this](World& iWorld, float iDelta)
      {
        Step(iWorld, iDelta);
      });
    Engine_Application& app = Engine_Application::GetAppl();

    app.GetMenuManager().AddMenu("Viewer")
      .AddOpenPanelCommand("Menu", [this, &iWorld] {return new ViewerAppPanel(iWorld, *this); })
      .EndMenu();

    Vector<Image*> skyBoxPlanes;
    skyBoxPlanes.push_back(ImageStreamer::Load("D:\\cubeMapb\\posx.jpg"));
    skyBoxPlanes.push_back(ImageStreamer::Load("D:\\cubeMapb\\negx.jpg"));
    skyBoxPlanes.push_back(ImageStreamer::Load("D:\\cubeMapb\\posy.jpg"));
    skyBoxPlanes.push_back(ImageStreamer::Load("D:\\cubeMapb\\negy.jpg"));
    skyBoxPlanes.push_back(ImageStreamer::Load("D:\\cubeMapb\\posz.jpg"));
    skyBoxPlanes.push_back(ImageStreamer::Load("D:\\cubeMapb\\negz.jpg"));

    m_Scene.SetSkybox(skyBoxPlanes);

    Transforms& trans = *iWorld.GetSystem<Transforms>();

    ImporterContext ctx;
    ctx.baseMaterial = MakeRefCounted<Material>();

#if 0
    ctx.bakeTransforms = true;
    Scene mergedScene = ImportModel(ctx, "D:\\eXl_Game\\Ultimate Modular Ruins Pack - Aug 2021\\FBX\\Character_Animated.fbx");

    IntrusivePtr<Model> model = mergedScene.m_Models[0];

    for (uint32_t x = 0; x < 10; ++x)
    {
      for (uint32_t y = 0; y < 10; ++y)
      {
        for (uint32_t z = 0; z < 10; ++z)
        {
          ObjectHandle obj = iWorld.CreateObject();
          trans.AddTransform(obj, glm::translate(Identity<Mat4>(), Vec3(x, y, z) * model->GetBoundingSphere().m_Radius));
          m_Scene.Add3DModel(obj, model);
        }
      }
    }
#endif
    ctx.keepShadowCopy = true;
    ctx.importTransform = glm::rotate(Identity<Mat4>(), glm::radians(-90.f), UnitX<Vec3>());
    Scene splitScene = ImportScene(ctx, "D:\\eXl_Game\\sceneOcc.blend");
    //Scene splitScene = ImportScene(ctx, "D:\\eXl_Game\\Ultimate Modular Ruins Pack - Aug 2021\\FBX\\Character_Animated.fbx");
    //Scene splitScene = ImportScene(ctx, "D:\\eXl_Game\\Ultimate Modular Ruins Pack - Aug 2021\\preview.blend");

    float smallSize = splitScene.m_Models[0]->GetBoundingSphere().m_Radius;
    int smallModel = 0;


    Vector<Box3D> boxes;
    Vec3 sceneSize = splitScene.m_SceneBox.Size();
    for (int32_t i = -5; i < 5; ++i)
    {
      for (int32_t j = -5; j < 5; ++j)
      {
        for (uint32_t k = 0; k < splitScene.m_Transforms.size(); ++k)
        {
          ObjectHandle obj = iWorld.CreateObject();
          Mat4 sceneTrans = translate(Identity<Mat4>(), Vec3(i * sceneSize.x, 0, j * sceneSize.z));
          trans.AddTransform(obj, sceneTrans * splitScene.m_Transforms[k]);
          m_Scene.Add3DModel(obj, splitScene.m_Models[k]);
          //float radius = splitScene.m_Models[k]->GetBoundingSphere().m_Radius;
          //if (radius < smallSize)
          //{
          //  smallSize = radius;
          //  smallModel = i;
          //}
          boxes.push_back(sceneTrans * splitScene.m_Transforms[k] * splitScene.m_Models[k]->GetBoundingBox());
        }
      }
    }


    GetCamera().view.projection = GfxSystem::Perspective;
    GetCamera().view.pos = Vec3(0, 0, 10);
    GetCamera().view.displayedSize = 1.0;

  }

  void ViewerApp::Step(World& iWorld, float iDelta)
  {
    ProcessInputs(iWorld);

    GfxSystem& gfx = *iWorld.GetSystem<GfxSystem>();

    DebugTool::Drawer* drawer = gfx.GetDebugDrawer();
  }

  void ViewerApp::ProcessInputs(World& iWorld)
  {
    Engine_Application& app = Engine_Application::GetAppl();

    InputSystem& iInputs = app.GetInputSystem();
    CharacterSystem& controller = *iWorld.GetSystem<CharacterSystem>();

    GfxSystem& gfxSys = *iWorld.GetSystem<GfxSystem>();
    Vec2i vptSize = gfxSys.GetViewportSize();

    boost::optional<bool> actionKeyUsed;

    Vec3(&basis)[3] = GetCamera().view.basis;

    for (int i = 0; i < (int)iInputs.m_KeyEvts.size(); ++i)
    {
      KeyboardEvent& evt = iInputs.m_KeyEvts[i];
      if (!evt.pressed)
      {
        if (evt.key == K_SPACE)
        {
          dirMask &= ~(1 << 2);
          keyChanged = true;
        }
        if (evt.key == K_LCTRL)
        {
          dirMask &= ~(1 << 3);
          keyChanged = true;
        }
        if (evt.key == K_UP)
        {
          dirMask &= ~(1 << 5);
          keyChanged = true;
        }
        if (evt.key == K_DOWN)
        {
          dirMask &= ~(1 << 4);
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
        if (evt.key == K_SPACE)
        {
          dirMask |= 1 << 2;
          keyChanged = true;
        }
        if (evt.key == K_LCTRL)
        {
          dirMask |= 1 << 3;
          keyChanged = true;
        }
        if (evt.key == K_UP)
        {
          dirMask |= 1 << 5;
          keyChanged = true;
        }
        if (evt.key == K_DOWN)
        {
          dirMask |= 1 << 4;
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

      if (keyChanged)
      {
        iInputs.m_KeyEvts.erase(iInputs.m_KeyEvts.begin() + i);
        --i;
      }
    }

    uint32_t mask = dirMask;

    for (auto const& evt : iInputs.m_MouseEvts)
    {
      if (evt.button == MouseButton::Left)
      {
        moveCam = evt.pressed;
      }
    }

    for (auto const& evt : iInputs.m_MouseMoveEvts)
    {
      if (evt.wheel)
      {
        //  if (evt.relY < 0)
        //  {
        //    mask |= 1 << 4;
        //    keyChanged = true;
        //  }
        //  else
        //  {
        //    mask |= 1 << 5;
        //    keyChanged = true;
        //  }
      }


      if (moveCam)
      {
        basis[2] = basis[2] + basis[0] * Mathf::Clamp(-0.005 * evt.relX, -1.0, 1.0) + basis[1] * Mathf::Clamp(0.005 * evt.relY, -1.0, 1.0);
        basis[2] = normalize(basis[2]);

        Vec3 upRef;
        if (Mathf::Abs(dot(basis[2], UnitY<Vec3>())) < (1.0 - Mathf::ZeroTolerance()))
          upRef = UnitY<Vec3>();
        else
          upRef = basis[1];

        basis[0] = cross(upRef, basis[2]);
        basis[0] = normalize(basis[0]);
        basis[1] = cross(basis[2], basis[0]);
        basis[1] = normalize(basis[1]);
      }
    }

    if (keyChanged)
    {
      static const Vec3 dirs[] =
      {
        UnitX<Vec3>() * 1.0,
        UnitX<Vec3>() * -1.0,
        UnitY<Vec3>() * 1.0,
        UnitY<Vec3>() * -1.0,
        UnitZ<Vec3>() * 1.0,
        UnitZ<Vec3>() * -1.0,
      };
      Vec3 dir = Zero<Vec3>();
      for (unsigned int i = 0; i < 6; ++i)
      {
        if (mask & (1 << i))
        {
          dir += dirs[i];
        }
      }

      controller.SetSpeed(GetCamera().cameraObj, 10.0);
      dir = dir.x * basis[0] + dir.y * basis[1] + dir.z * basis[2];
      controller.SetCurDir(GetCamera().cameraObj, dir);
    }
  }
}

#include <engine/eXl_Main.hpp>

EXL_MAIN_WITH_SCENARIO_AND_PROJECT(ViewerApp, "eXl_ForestProject/ForestProject.eXlProject")