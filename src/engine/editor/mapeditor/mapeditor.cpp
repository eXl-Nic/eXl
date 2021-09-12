#include "mapeditor.hpp"

#include <editor/editordef.hpp>
#include <editor/objectmodel.hpp>
#include <editor/objectdelegate.hpp>
#include <editor/gamewidget.hpp>
#include <editor/gamewidgetselection.hpp>
#include <editor/aspectratiowidget.hpp>
#include <editor/tileset/tileselectionwidget.hpp>

#include <core/input.hpp>

#include "tilestool.hpp"
#include "terraintool.hpp"
#include "pentoolfilter.hpp"
#include "objectstool.hpp"

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

namespace eXl
{
  ResourceEditorHandler& MapEditor::GetEditorHandler()
  {
    static EditorHandler_T<MapResource, MapEditor> s_Handler;
    return s_Handler;
  }

	struct MapEditor::Impl
	{
		Impl(MapEditor* iEditor)
		{
      PropertiesManifest mapEditorManifest = EditorState::GetProjectProperties();
      mapEditorManifest.RegisterPropertySheet<TilesTool::PlacedTile>(TilesTool::ToolDataName(), false);
      mapEditorManifest.RegisterPropertySheet<TerrainTool::Island>(TerrainTool::ToolDataName(), false);
      mapEditorManifest.RegisterPropertySheet<ObjectsTool::PlacedObject>(ObjectsTool::ToolDataName(), false);

      m_World.Init(mapEditorManifest).WithGfx();

      World& world = m_World.GetWorld();
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
        m_TilesTool->Initialize(*m_Map);
      }
      {
        TerrainTool::Tools subTools;
        subTools.m_Selection = m_Selection;

        m_IslandsTool = new TerrainTool(iEditor, subTools, m_World.GetWorld());
        m_IslandsTool->Initialize(*m_Map);
      }
      {
        ObjectsTool::Tools subTools;
        subTools.m_Pen = m_PenTool;
        subTools.m_Selection = m_Selection;

        m_ObjectsTool = new ObjectsTool(iEditor, subTools, m_World.GetWorld());
        m_ObjectsTool->Initialize(*m_Map);
      }

      int tilesTabIdx = tools->addTab(m_TilesTool, "Tiles");
      m_Tools[tilesTabIdx] = m_TilesTool;
      int islandsTabIdx = tools->addTab(m_IslandsTool, "Terrain");
      m_Tools[islandsTabIdx] = m_IslandsTool;
      int objectsTabIdx = tools->addTab(m_ObjectsTool, "Objects");
      m_Tools[objectsTabIdx] = m_ObjectsTool;

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
          m_World.GetCamera().ProcessInputs(m_World.GetWorld(), m_Inputs, CameraState::WheelZoom |CameraState::RightClickPan);
          m_World.GetCamera().UpdateView(m_World.GetWorld());
          m_GameWidget->GetViewInfo().pos = m_World.GetCamera().view.pos;
          m_GameWidget->GetViewInfo().basis[0] = m_World.GetCamera().view.basis[0];
          m_GameWidget->GetViewInfo().basis[1] = m_World.GetCamera().view.basis[1];
          m_GameWidget->GetViewInfo().basis[2] = m_World.GetCamera().view.basis[2];
          m_GameWidget->GetViewInfo().displayedSize = m_World.GetCamera().view.displayedSize;
          m_GameWidget->ViewInfoUpdated();

          m_Inputs.Clear();

          m_World.Tick(iDelta);
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

    Map<uint32_t, EditorTool*> m_Tools;

    EditorTool* m_CurTool;

    MapEditor* m_Editor;
    MapResource* m_Map;
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
    tiles.Iterate([&](ObjectHandle, TilesTool::PlacedTile const& iTile)
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
    islands.Iterate([&](ObjectHandle, TerrainTool::Island const& iIsland)
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
    
    objects.Iterate([&](ObjectHandle, ObjectsTool::PlacedObject const& iObject)
      {
        map->m_Objects.push_back(MapResource::Object());
        MapResource::Object& newObject = map->m_Objects.back();
        newObject.m_Header.m_Archetype = iObject.m_Archetype;
        newObject.m_Header.m_ObjectId = iObject.m_UUID;
        newObject.m_Header.m_Position = MathTools::To3DVec(MathTools::ToFVec(iObject.m_Position));
        newObject.m_Header.m_Position.X() /= EngineCommon::s_WorldToPixel;
        newObject.m_Header.m_Position.Y() /= EngineCommon::s_WorldToPixel;
        newObject.m_Data = iObject.m_CustoData;
      });

  }
}