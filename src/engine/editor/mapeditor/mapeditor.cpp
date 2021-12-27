#include "mapeditor.hpp"

#include <editor/editordef.hpp>
#include <editor/objectmodel.hpp>
#include <editor/objectdelegate.hpp>
#include <editor/gamewidget.hpp>
#include <editor/gamewidgetselection.hpp>
#include <editor/aspectratiowidget.hpp>
#include <editor/tileset/tileselectionwidget.hpp>
#include <editor/editoricons.hpp>

#include <core/input.hpp>
#include <core/resource/resourcemanager.hpp>
#include <core/process.hpp>
#include <core/corelib.hpp>

#include "tilestool.hpp"
#include "terraintool.hpp"
#include "pentoolfilter.hpp"
#include "objectstool.hpp"
#include "mcmclearntool.hpp"

#include <math/mathtools.hpp>
#include <math/aabb2dpolygon.hpp>

#include <engine/map/map.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/physics/physicsys.hpp>
#include <engine/common/app.hpp>

#include <boost/geometry/index/rtree.hpp>
#include <math/geometrytraits.hpp>

#include <QTabWidget>
#include <QTreeView>
#include <QTableView>
#include <QListView>
#include <QSplitter>
#include <QVBoxLayout>
#include <QToolbar>
#include <QLabel>
#include <QFileDialog>
#include <QScrollArea>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QSpinBox>
#include <QMouseEvent>
#include <QTimer>

namespace eXl
{
  ResourceEditorHandler& MapEditor::GetEditorHandler()
  {
    static EditorHandler_T<MapResource, MapEditor> s_Handler;
    return s_Handler;
  }

  class MapEditorComponentMgr : public ToolEditorComponentMgr
  {
  public:

    MapEditorComponentMgr(MapEditor::Impl& iEditorImpl)
      : m_EditorImpl(iEditorImpl)
    {}

    void DeleteComponent(ObjectHandle) override;
  protected:
    MapEditor::Impl& m_EditorImpl;
  };

	struct MapEditor::Impl
	{
		Impl(MapEditor* iEditor)
      : m_Player((Path(GetAppPath().data()).parent_path() / "eXl_Player.exe").string().c_str())
		{
      PropertiesManifest mapEditorManifest = EditorState::GetProjectProperties();
      mapEditorManifest.RegisterPropertySheet<TileItemData>(TilesTool::ToolDataName(), false);
      mapEditorManifest.RegisterPropertySheet<TerrainIslandItemData>(TerrainTool::ToolDataName(), false);
      mapEditorManifest.RegisterPropertySheet<MapResource::ObjectHeader>(ObjectsTool::ToolDataName(), false);

      m_World.Init(mapEditorManifest).WithGfx();

      World& world = m_World.GetWorld();

      world.AddSystem(std::make_unique<MapEditorComponentMgr>(*this));

      GfxSystem& gfx = *world.GetSystem<GfxSystem>();
      PhysicsSystem& ph = *world.GetSystem<PhysicsSystem>();
      
      //ph.EnableDebugDraw(*gfx.GetDebugDrawer());

      m_Editor = iEditor;
      m_Map = MapResource::DynamicCast(iEditor->GetDocument()->GetResource());

      QSplitter* rootSplitter = new QSplitter(Qt::Horizontal, iEditor);

      QTabWidget* tools = new QTabWidget(iEditor);

      m_Selection = new MouseSelectionFilter(iEditor);
      m_PenTool = new PenToolFilter(iEditor, m_World.GetWorld());

      {
        TilesTool::Tools subTools;
        subTools.m_Pen = m_PenTool;
        subTools.m_Selection = m_Selection;

        m_TilesTool = new TilesTool(iEditor, subTools, m_World.GetWorld());
      }
      {
        TerrainTool::Tools subTools;
        subTools.m_Selection = m_Selection;

        m_IslandsTool = new TerrainTool(iEditor, subTools, m_World.GetWorld());
      }
      {
        ObjectsTool::Tools subTools;
        subTools.m_Pen = m_PenTool;
        subTools.m_Selection = m_Selection;

        m_ObjectsTool = new ObjectsTool(iEditor, subTools, m_World.GetWorld());
      }

      m_MCMCTool = new MCMCLearnTool(m_Editor, m_World.GetWorld(), m_Editor);

      int tilesTabIdx = tools->addTab(m_TilesTool, "Tiles");
      m_Tools[tilesTabIdx] = m_TilesTool;
      int islandsTabIdx = tools->addTab(m_IslandsTool, "Terrain");
      m_Tools[islandsTabIdx] = m_IslandsTool;
      int objectsTabIdx = tools->addTab(m_ObjectsTool, "Objects");
      m_Tools[objectsTabIdx] = m_ObjectsTool;
      int mcmcTabIdx = tools->addTab(m_MCMCTool, "MCMC");
      m_Tools[mcmcTabIdx] = m_MCMCTool;

      m_CurTool = m_TilesTool;

      for (auto toolEntry : m_Tools)
      {
        EditorTool* tool = toolEntry.second;
        if (tool == nullptr)
        {
          continue;
        }
        QObject::connect(tool, &EditorTool::onToolChanged, [this](QObject* iFilter, GameWidget::PainterInterface* iPainter)
        {
          m_GameWidget->installEventFilter(iFilter);
          m_GameWidget->SetPainterInterface(iPainter);
        });

        QObject::connect(tool, &EditorTool::onEditDone, [this]()
        {
          m_Editor->GetDocument()->Touch();
        });
      }

      QObject::connect(tools, &QTabWidget::currentChanged, [this](int iCurTab)
      {
        if (m_CurTool)
        {
          m_CurTool->DisableTool();
        }
        m_CurTool = m_Tools[iCurTab];
        
        if (m_CurTool)
        {
          m_CurTool->EnableTool();
        }
      });

      rootSplitter->addWidget(tools);

      QWidget* mapTool = new QWidget(iEditor);
      rootSplitter->addWidget(mapTool);

      QVBoxLayout* mapToolLayout = new QVBoxLayout(iEditor);

      QToolBar* mapDisplayTools = new QToolBar(iEditor);
      mapToolLayout->addWidget(mapDisplayTools);


      m_PollClosePlayerTimer = new QTimer(iEditor);
      m_PlayAction = mapDisplayTools->addAction(EditorIcons::GetPlayIcon(), "Play", [&]
        {
          if (!m_Player.IsRunning())
          {
            Path projectPath = ResourceManager::GetPath(EditorState::GetCurrentProject()->GetResource()->GetHeader().m_ResourceId);
            Path projectDir = projectPath.parent_path();
            m_Player.AddArgument("--project");
            m_Player.AddArgument(projectPath.string().c_str());
            Path mapPath = ResourceManager::GetPath(m_Map->GetHeader().m_ResourceId);
            Path localMapPath = Filesystem::relative(mapPath, projectDir);
            m_Player.AddArgument("--map");
            m_Player.AddArgument(localMapPath.string().c_str());

            if (m_Player.Start())
            {
              m_PlayAction->setDisabled(true);
              QObject::connect(m_PollClosePlayerTimer, &QTimer::timeout, [&]
                {
                  if (!m_Player.WaitForEnd(0))
                  {
                    return;
                  }
                  m_PollClosePlayerTimer->stop();
                  m_Player.Clear();
                  m_PlayAction->setDisabled(false);
                });
              m_PollClosePlayerTimer->start(1000);
            }
          }
        });

      mapTool->setLayout(mapToolLayout);
      {
        m_GameWidget = new GameWidget(iEditor);
        m_GameWidget->SetInputSystem(&m_Inputs);
        m_GameWidget->SetGfxSystem(m_World.GetWorld().GetSystem<GfxSystem>());

        GfxSystem::ViewInfo& view = m_GameWidget->GetViewInfo();
        view.pos = Vector3f::UNIT_Z + Vector3f::UNIT_X * 2;
        view.projection = GfxSystem::Orthographic;
        m_World.GetCamera().view.displayedSize = view.displayedSize = EngineCommon::s_WorldToPixel * 20;
        view.backgroundColor = Vector4f::ONE;

        m_World.GetCamera().view = view;

        //auto ratioWidget = new AspectRatioWidget(gameWidget, 1.0, 1.0, this);

        m_GameWidget->SetTickCallback([this](float iDelta)
        {
          GfxSystem& gfx = *m_World.GetWorld().GetSystem<GfxSystem>();
          if (!m_ToolsInitialized
            && gfx.GetRenderNode(gfx.GetSpriteHandle())->IsInitialized())
          {
            m_TilesTool->Initialize(*m_Map);
            m_IslandsTool->Initialize(*m_Map);
            m_ObjectsTool->Initialize(*m_Map);
            m_ToolsInitialized = true;
          }

          m_World.GetCamera().ProcessInputs(m_World.GetWorld(), m_Inputs, CameraState::WheelZoom |CameraState::RightClickPan);
          m_World.GetCamera().UpdateView(m_World.GetWorld());
          m_GameWidget->GetViewInfo().pos = m_World.GetCamera().view.pos;
          m_GameWidget->GetViewInfo().basis[0] = m_World.GetCamera().view.basis[0];
          m_GameWidget->GetViewInfo().basis[1] = m_World.GetCamera().view.basis[1];
          m_GameWidget->GetViewInfo().basis[2] = m_World.GetCamera().view.basis[2];
          m_GameWidget->GetViewInfo().displayedSize = m_World.GetCamera().view.displayedSize;
          m_GameWidget->ViewInfoUpdated();

          m_Inputs.Clear();

          m_World.Tick();
        });

        //mapToolLayout->addWidget(ratioWidget);
        mapToolLayout->addWidget(m_GameWidget);
        m_GameWidget->SetAnimated(true);
      }

      QVBoxLayout* layout = new QVBoxLayout(iEditor);

      layout->addWidget(rootSplitter);

      iEditor->setLayout(layout);
		}

    GameWidget* m_GameWidget;
    ObjectHandle m_Handle;
    WorldState m_World;
    InputSystem m_Inputs;

    MouseSelectionFilter* m_Selection;
    PenToolFilter* m_PenTool;

    TilesTool* m_TilesTool;
    TerrainTool* m_IslandsTool;
    ObjectsTool* m_ObjectsTool;
    MCMCLearnTool* m_MCMCTool;
    bool m_ToolsInitialized = false;

    Map<uint32_t, EditorTool*> m_Tools;

    EditorTool* m_CurTool;

    MapEditor* m_Editor;
    MapResource* m_Map;

    Process m_Player;
    QAction* m_PlayAction;
    QTimer* m_PollClosePlayerTimer;
	};

  void MapEditor::Cleanup()
  {
    m_Impl.reset();
    ResourceEditor::Cleanup();
  }

  MapEditor::MapEditor(QWidget* iParent, DocumentState* iDoc)
    : ResourceEditor(iParent, iDoc)
  {
    m_Impl.reset(new Impl(this));
	}

  void MapEditor::CommitDocument()
  {
    MapResource* map = m_Impl->m_Map;

    map->m_Tiles.clear();
    map->m_Terrains.clear();
    map->m_Objects.clear();

    auto const& tiles = m_Impl->m_TilesTool->GetTiles();
    UnorderedMap<std::pair<ResourceHandle<Tileset>, Name>, uint32_t> tileGroups;
    tiles.Iterate([&](ObjectHandle, TileItemData const& iTile)
      {
        auto key = std::make_pair(iTile.m_Tileset, iTile.m_Type);
        auto insertRes = tileGroups.insert(std::make_pair(key, map->m_Tiles.size()));
        if (insertRes.second)
        {
          map->m_Tiles.push_back(MapResource::PlacedTiles());
          map->m_Tiles.back().m_Tileset = key.first;
          map->m_Tiles.back().m_Type = iTile.m_Type;
        }
        MapResource::PlacedTiles::Tile newTile;
        newTile.m_Name = iTile.m_Tile;
        newTile.m_Layer = iTile.m_Layer;
        newTile.m_Position = iTile.m_Position;
        map->m_Tiles[insertRes.first->second].m_Tiles.push_back(newTile);
      });

    auto const& islands = m_Impl->m_IslandsTool->GetIslands();
    UnorderedMap<std::pair<ResourceHandle<TilingGroup>, Name>, uint32_t> terrainGroups;
    islands.Iterate([&](ObjectHandle, TerrainIslandItemData const& iIsland)
      {
        auto key = std::make_pair(iIsland.m_TilingGroup, iIsland.m_Terrain);
        auto insertRes = terrainGroups.insert(std::make_pair(key, map->m_Terrains.size()));
        if (insertRes.second)
        {
          map->m_Terrains.push_back(MapResource::Terrain());
          map->m_Terrains.back().m_TilingGroup = key.first;
          map->m_Terrains.back().m_Type = iIsland.m_Terrain;
        }
        MapResource::Terrain::Block newBlock;
        newBlock.m_Layer = iIsland.m_Layer;
        newBlock.m_Shape = iIsland.m_IslandPoly;
        MapResource::Terrain& terrainGroup = map->m_Terrains[insertRes.first->second];
        Vector2i tilingSize = TerrainTool::SafeGetTilingSize(terrainGroup.m_TilingGroup.GetOrLoad());
        newBlock.m_Shape.ScaleComponents(1, 1, tilingSize.X(), tilingSize.Y());
        terrainGroup.m_Blocks.push_back(std::move(newBlock));
      });

    auto const& objects = m_Impl->m_ObjectsTool->GetObjects();

    objects.Iterate([&](ObjectHandle iHandle, MapResource::ObjectHeader const& iObject)
      {
        map->m_Objects.push_back(MapResource::Object());
        MapResource::Object& newObject = map->m_Objects.back();
        newObject.m_Header = iObject;
        ObjectsTool::PlacedObject const* additionalData = m_Impl->m_ObjectsTool->GetObjectsAdditionalData().Get(iHandle);
        newObject.m_Data = additionalData->m_CustoData;
      });
  }

  ObjectHandle MapEditor::Place(Resource::UUID const& iUUID, Name iSubobject, Vector2i const& iPos)
  {
    Resource::Header const* header = ResourceManager::GetHeader(iUUID);
    if (header == nullptr)
    {
      return ObjectHandle();
    }
    if (header->m_LoaderName == Tileset::StaticLoaderName())
    {
      ResourceHandle<Tileset> handle;
      handle.SetUUID(iUUID);
      return m_Impl->m_TilesTool->AddAt(handle, TileName(iSubobject.get()), 0, TerrainTypeName("Wall"), iPos, false);
    }
    else if (header->m_LoaderName == Archetype::StaticLoaderName())
    {
      ResourceHandle<Archetype> handle;
      handle.SetUUID(iUUID);
      if (Archetype const* archetype = handle.GetOrLoad())
      {
        return m_Impl->m_ObjectsTool->AddAt(archetype, iPos);
      }
    }
    return ObjectHandle();
  }

  void MapEditorComponentMgr::DeleteComponent(ObjectHandle iObject)
  {
    GameDatabase& database = *GetWorld().GetSystem<GameDatabase>();
    if (database.GetView<TileItemData>(TilesTool::ToolDataName())->GetDataForDeletion(iObject) != nullptr)
    {
      m_EditorImpl.m_TilesTool->Cleanup(iObject);
    }
    if (database.GetView<MapResource::ObjectHeader>(ObjectsTool::ToolDataName())->GetDataForDeletion(iObject) != nullptr)
    {
      m_EditorImpl.m_ObjectsTool->Cleanup(iObject);
    }
  }
}