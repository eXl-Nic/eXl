#include "modeleditor.hpp"

#include <core/resource/resourcemanager.hpp>
#include <core/input.hpp>
#include <core/image/imagestreamer.hpp>

#include <engine/common/app.hpp>
#include <engine/common/transforms.hpp>

#include <engine/gfx/model.hpp>
#include <engine/gfx/gfx3dscene.hpp>
#include <engine/gfx/modelrsc.hpp>
#include <engine/gfx/gfxsystem.hpp>
#include <engine/gfx/model_importer.hpp>

#include <editor/gamewidget.hpp>

#include <QBoxLayout>
#include <QSplitter>
#include <QLineEdit>

namespace eXl
{
  ResourceEditorHandler& ModelEditor::GetEditorHandler()
  {
    static EditorHandler_T<ModelResource, ModelEditor> s_Handler;
    return s_Handler;
  }

  struct ModelEditor::Impl
  {
    Impl(ModelEditor& iEdit);

    bool InitDisplay();
    void UpdateObject();
    void HandleInput();

    WorldConfig m_Config;
    World m_World;
    InputSystem m_Input;

    Transforms* m_Transforms;
    GfxSystem* m_Gfx;
    Gfx3DScene m_Scene;
    OrbitCamera m_Cam;

    ModelEditor* m_Editor;
    ModelResource* m_Model;

    GameWidget* m_ModelPreview;
    ObjectHandle m_ModelObj;
    bool m_Initialized = false;
    bool m_RightMouseClicked = false;
  };

  void ModelEditor::Cleanup()
  {
    m_Impl.reset();
    ResourceEditor::Cleanup();
  }

  ModelEditor::ModelEditor(QWidget* iParent, DocumentState* iDoc)
    : ResourceEditor(iParent, iDoc)
    , m_Impl(new Impl(*this))
  {

  }

  ModelEditor::Impl::Impl(ModelEditor& iEdit)
    : m_Config(EditorState::BuildWorldConfig())
    , m_World(m_Config)
    , m_Editor(&iEdit)
    , m_Model(ModelResource::DynamicCast(iEdit.GetDocument()->GetResource()))
  {
    m_Transforms = m_World.AddSystem(std::make_unique<Transforms>());
    m_Gfx = m_World.AddSystem(std::make_unique<GfxSystem>(*m_Transforms));
    m_Scene.Initialize(*m_Gfx);
    m_World.AddSystem(std::make_unique<GameDatabase>(m_Config.m_Properties));

    QSplitter* rootSplitter = new QSplitter(Qt::Horizontal, m_Editor);

    {
      QWidget* dataTab = new QWidget(m_Editor);
      QVBoxLayout* layout = new QVBoxLayout(dataTab);

      QLineEdit* modelEdit = new QLineEdit(dataTab);
      modelEdit->setText(m_Model->GetModelName().c_str());
      QObject::connect(modelEdit, &QLineEdit::textChanged, [this, modelEdit]() {
        m_Model->SetModelName(String(modelEdit->text().toUtf8().data()));
        UpdateObject();
        m_Editor->ModifyResource();
        });
      layout->addWidget(modelEdit);

      rootSplitter->addWidget(dataTab);
    }

    {
      m_ModelPreview = new GameWidget(m_Editor);
      m_ModelPreview->SetGfxSystem(m_Gfx);

      rootSplitter->addWidget(m_ModelPreview);
      m_ModelPreview->SetAnimated(true);
      m_ModelPreview->SetInputSystem(&m_Input);
      m_ModelPreview->SetTickCallback([this](float)
        {
          if (InitDisplay())
          {
            HandleInput();
          }
          
          m_Input.Clear();
        });
    }

    QVBoxLayout* layout = new QVBoxLayout(m_Editor);

    layout->addWidget(rootSplitter);

    m_Editor->setLayout(layout);
  }

  void ModelEditor::Impl::HandleInput()
  {
    for (auto const& mouseEvt : m_Input.m_MouseEvts)
    {
      if (mouseEvt.button == MouseButton::Right)
      {
        m_RightMouseClicked = mouseEvt.pressed;
      }
    }

    Vec3 motion = Zero<Vec3>();

    for (auto const& moveEvt : m_Input.m_MouseMoveEvts)
    {
      if (moveEvt.wheel)
      {
        motion.z = moveEvt.relY;
      }
      else if (m_RightMouseClicked)
      {
        motion.x = -moveEvt.relX;
        motion.y = moveEvt.relY;
      }
    }

    if (motion != Zero<Vec3>() )
    {
      m_Cam.Update(m_ModelPreview->GetViewInfo(), motion);
      m_ModelPreview->ViewInfoUpdated();
    }
  }

  bool ModelEditor::Impl::InitDisplay()
  {
    if (!m_Gfx->GetRenderNode(m_Gfx->GetSpriteHandle())->IsInitialized())
    {
      return false;
    }

    if (m_Initialized)
    {
      return true;
    }

    {
      GfxSystem::ViewInfo& view = m_ModelPreview->GetViewInfo();
      view.projection = GfxSystem::Perspective;
      view.displayedSize = 1.0;
      view.backgroundColor = One<Vec4>() * 0.5;
      view.pos = Vec3(0, 0, -1);
      m_ModelPreview->ViewInfoUpdated();
    }

    Path appDir = eXl::Application::GetAppl().GetAppPath().parent_path();
    static Vector<Image*> skyBoxPlanes;
    if(skyBoxPlanes.empty())
    {
      Path skyboxDir = appDir / "editor_rsc" / "skybox";
      char const* files[] = { 
        "clouds1_east.png",
        "clouds1_west.png",
        "clouds1_up.png",
        "clouds1_down.png",
        "clouds1_north.png",
        "clouds1_south.png",
         };

      for (auto file : files) {
        Path filePath = skyboxDir / file;
        Image* plane = ImageStreamer::Load(ToString(filePath));
        if (plane != nullptr) {
          skyBoxPlanes.push_back(plane);
        }
      }
      if (skyBoxPlanes.size() != 6) {
        skyBoxPlanes.clear();
        skyBoxPlanes.push_back(nullptr);
      }
    }

    if (skyBoxPlanes.size() == 6) {
      m_Scene.SetSkybox(skyBoxPlanes);
    }

    UpdateObject();
    m_Initialized = true;
    return true;
  }
  void ModelEditor::Impl::UpdateObject()
  {
    if (m_ModelObj.IsAssigned())
    {
      m_World.DeleteObject(m_ModelObj);
      m_ModelObj = ObjectHandle();
    }
    
    String const& localName = m_Model->GetModelName();
    Path rscPath = ResourceManager::GetPath(m_Model->GetHeader().m_ResourceId);
    Path modelPath = rscPath.parent_path() / localName;

    ImporterContext ctx;
    ctx.bakeTransforms = true;
    Scene imported = ImportScene(ctx, String(modelPath.string().c_str()));
    if (imported.m_Models.empty())
    {
      return;
    }

    IntrusivePtr<Model> mdl = imported.m_Models[0];

    m_ModelObj = m_World.CreateObject();
    m_Transforms->AddTransform(m_ModelObj);
    m_Scene.Add3DModel(m_ModelObj, mdl);

    GfxSystem::ViewInfo& view = m_ModelPreview->GetViewInfo();
    m_Cam.Reframe(view, mdl->GetBoundingSphere());

    m_ModelPreview->ViewInfoUpdated();
  }
}

