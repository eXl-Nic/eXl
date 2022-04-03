#include "tilestool.hpp"
#include "pentoolfilter.hpp"

#include <editor/editordef.hpp>
#include <editor/gamewidgetselection.hpp>
#include <editor/tileset/tileselectionwidget.hpp>
#include <editor/tileset/tilecollectionmodel.hpp>
#include <editor/editoricons.hpp>

#include <math/mathtools.hpp>
#include <math/aabb2dpolygon.hpp>

#include <engine/map/map.hpp>
#include <engine/gfx/gfxcomponent.hpp>
#include <engine/physics/physicsys.hpp>
#include <engine/common/app.hpp>

#include <math/geometrytraits.hpp>

#include <QListView>
#include <QSplitter>
#include <QVBoxLayout>
#include <QToolbar>
#include <QActionGroup>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QSpinBox>

namespace eXl
{
  PropertySheetName TilesTool::ToolDataName()
  {
    static PropertySheetName s_Name("PlacedTile");
    return s_Name;
  }

  TilesTool::TilesTool(QWidget* parent, Tools iTools, World& iWorld)
    : EditorTool(parent, iWorld)
    , m_World(iWorld)
    , m_TilesView(*iWorld.GetSystem<GameDatabase>()->GetView<TileItemData>(ToolDataName()))
    , m_PlacedTileData(iWorld)
    , m_Tools(iTools)
  {
    m_Icons[None] = EditorIcons::GetMoveToolIcon();
    m_Icons[PenTool] = EditorIcons::GetDrawToolIcon();
    m_Icons[EraserTool] = EditorIcons::GetEraseToolIcon();
    m_HIcons[None] = EditorIcons::GetHighlightedMoveToolIcon();
    m_HIcons[PenTool] = EditorIcons::GetHighlightedDrawToolIcon();
    m_HIcons[EraserTool] = EditorIcons::GetHighlightedEraseToolIcon();

    QVBoxLayout* toolLayout = new QVBoxLayout(this);
    setLayout(toolLayout);
    QSplitter* toolSplitter = new QSplitter(Qt::Vertical);

    {
      QToolBar* tilesTool = new QToolBar(this);

      m_Actions[None] = tilesTool->addAction(m_Icons[None], "Move", [this]
      {
        ChangeTool(None);
      });

      m_Actions[PenTool] = tilesTool->addAction(m_Icons[PenTool], "Draw", [this]
      {
        ChangeTool(PenTool);
      });

      m_Actions[EraserTool] = tilesTool->addAction(m_Icons[EraserTool], "Erase", [this]
      {
        ChangeTool(EraserTool);
      });

      QActionGroup* grp = new QActionGroup(this);
      grp->addAction(m_Actions[None]);
      grp->addAction(m_Actions[PenTool]);
      grp->addAction(m_Actions[EraserTool]);

      toolSplitter->addWidget(tilesTool);
      ChangeTool(None);
    }

    {
      QWidget* penSettings = new QWidget(this);
      QVBoxLayout* layout = new QVBoxLayout(penSettings);
      penSettings->setLayout(layout);

      m_TerrainWidget = new TerrainWidget(this);

      layout->addWidget(m_TerrainWidget);

      {
        TileSelectionWidget::Conf widgetConf;
        widgetConf.m_ComboForTiles = false;
        m_TileSelection = new TileSelectionWidget(this, widgetConf, ResourceHandle<Tileset>(), TileName());
        layout->addWidget(m_TileSelection);

        QObject::connect(m_TileSelection, &TileSelectionWidget::onTilesetChanged, [this]
        {
          if (m_Tool == PenTool)
          {
            SetPenToolTile();
          }
        });
        QObject::connect(m_TileSelection, &TileSelectionWidget::onTileChanged, [this]
        {
          if (m_Tool == PenTool)
          {
            SetPenToolTile();
          }
        });
      }

      m_LayerWidget = new LayerWidget(this);

      QObject::connect(m_LayerWidget, &LayerWidget::onLayerChanged, [this](int32_t)
      {
        ChangeLowLight(Layer);
      });

      layout->addWidget(m_LayerWidget);
      toolSplitter->addWidget(penSettings);
    }

    //m_TilesList = new QListWidget;
    //toolSplitter->addWidget(m_TilesList);

    {
      m_SelectionSettings = new QWidget(this);
      QVBoxLayout* selectionLayout = new QVBoxLayout(m_SelectionSettings);
      m_SelectionSettings->setLayout(selectionLayout);

      TileSelectionWidget::Conf widgetConf;
      m_SelectionTile = new TileSelectionWidget(m_SelectionSettings, widgetConf, ResourceHandle<Tileset>(), TileName());
      selectionLayout->addWidget(m_SelectionTile);

      m_SelectionLayer = new LayerWidget(m_SelectionSettings);
      selectionLayout->addWidget(m_SelectionLayer);

      m_SelectionX = new QSpinBox(m_SelectionSettings);
      m_SelectionX->setMinimum(INT32_MIN);
      m_SelectionX->setMaximum(INT32_MAX);

      m_SelectionY = new QSpinBox(m_SelectionSettings);
      m_SelectionY->setMinimum(INT32_MIN);
      m_SelectionY->setMaximum(INT32_MAX);

      ConnectToValueChanged(m_SelectionX, [this](int iNewValue)
      {
        if (m_Selection)
        {
          m_Selection->m_Position.x = iNewValue;
          UpdateObjectBoxAndTile(m_SelectionHandle, *m_Selection);
          emit onEditDone();
        }
      });

      ConnectToValueChanged(m_SelectionY, [this](int iNewValue)
      {
        if (m_Selection)
        {
          m_Selection->m_Position.y = iNewValue;
          UpdateObjectBoxAndTile(m_SelectionHandle, *m_Selection);
          emit onEditDone();
        }
      });

      QObject::connect(m_SelectionLayer, &LayerWidget::onLayerChanged, [this](int32_t iValue)
        {
          if (m_Selection == nullptr)
          {
            return;
          }
          m_Selection->m_Layer = iValue;
          emit onEditDone();
        });

      QWidget* posWidget = new QWidget(m_SelectionSettings);
      QHBoxLayout* posWidgetLayout = new QHBoxLayout(posWidget);
      posWidget->setLayout(posWidgetLayout);

      posWidgetLayout->addWidget(new QLabel("X : ", this));
      posWidgetLayout->addWidget(m_SelectionX);
      posWidgetLayout->addWidget(new QLabel("Y : ", this));
      posWidgetLayout->addWidget(m_SelectionY);

      selectionLayout->addWidget(posWidget);

      m_SelectionSettings->setEnabled(false);

      toolSplitter->addWidget(m_SelectionSettings);
    }
    toolLayout->addWidget(toolSplitter);
  }

  void TilesTool::Initialize(MapResource const& iMap)
  {
    for (auto const& tileGroup : iMap.m_Tiles)
    {
      for (auto const& tile : tileGroup.m_Tiles)
      {
        ObjectHandle tileHandle = m_World.CreateObject();
        TileItemData& newTile = m_TilesView.GetOrCreate(tileHandle);
        newTile.m_Position = tile.m_Position;
        newTile.m_Layer = tile.m_Layer;
        newTile.m_Tileset = tileGroup.m_Tileset;
        newTile.m_Tile = tile.m_Name;
        newTile.m_Type = tileGroup.m_Type;
        m_EditorMgr.CreateComponent(tileHandle);
        UpdateObjectBoxAndTile(tileHandle, newTile);

        AddTileToWorld(tileHandle, newTile);
      }
    }
  }

  void TilesTool::ChangeLowLight(LowLightSettings iLowLight)
  {
    GfxSystem& sys = *m_World.GetSystem<GfxSystem>();
    m_TilesView.Iterate([this, iLowLight, &sys](ObjectHandle iObject, TileItemData& iTile)
      {
        if (iLowLight == EnableAll || (iLowLight == Layer
          && iTile.m_Layer == m_LayerWidget->GetCurLayer()))
        {
          sys.GetSpriteComponent(iObject)->SetTint(One<Vec4>());
        }
        if (iLowLight == DisableAll || (iLowLight == Layer
          && iTile.m_Layer != m_LayerWidget->GetCurLayer()))
        {
          sys.GetSpriteComponent(iObject)->SetTint(Vec4(0.8, 0.8, 0.8, 0.5));
        }
      });
  }

  void TilesTool::ClearPenToolTile()
  {
    ObjectHandle prevObject = m_Tools.m_Pen->GetPenObject();
    if (prevObject.IsAssigned())
    {
      m_World.DeleteObject(prevObject);
      m_Tools.m_Pen->SetPenObject(ObjectHandle());
    }
  }

  void TilesTool::SetPenToolTile()
  {
    ClearPenToolTile();

    TileName tile = m_TileSelection->GetTileName();
    if (tile.get().empty())
    {
      return;
    }
    Tileset const* tileset = m_TileSelection->GetTileset().GetOrLoad();
    if (tileset == nullptr)
    {
      return;
    }

    Tile const* tileData = tileset->Find(tile);
    if (tileData == nullptr)
    {
      return;
    }

    TileItemData dummyTile;
    dummyTile.m_Tileset = m_TileSelection->GetTileset();
    dummyTile.m_Tile = tile;
    dummyTile.m_Layer = m_LayerWidget->GetCurLayer();
    ObjectHandle penObject = m_World.CreateObject();
    AddTileToWorld(penObject, dummyTile);
    m_Tools.m_Pen->SetPenObject(penObject);
    m_Tools.m_Pen->SetSnapSize(tileData->m_Size);
  }

  void TilesTool::SetupPenTool()
  {
    SetPenToolTile();

    auto newConnection = QObject::connect(m_Tools.m_Pen, &PenToolFilter::onAddPoint, [this](Vec2i iPos, bool iWasDrawing)
    {
      ObjectHandle curTileHandle = GetAt(iPos);
      TileItemData* curTile = m_TilesView.Get(curTileHandle);
      if (curTile && curTile->m_Layer == GetLayer())
      {
        return ;
      }
      if (m_TileSelection->GetTileset().GetUUID().IsValid()
        && m_TileSelection->GetTileName() != TileName())
      {
        ObjectHandle newTile = AddAt(iPos, iWasDrawing);
      }
    });

    m_ToolConnections.push_back(newConnection);
  }

  void TilesTool::EnableTool()
  {
    ChangeTool(m_Tool, true);
    ChangeLowLight(Layer);
  }

  void TilesTool::DisableTool()
  {
    ClearPenToolTile();
    CleanupToolConnections();
    ChangeLowLight(DisableAll);
  }

  void TilesTool::CleanupToolConnections()
  {
    for (auto connection : m_ToolConnections)
    {
      QObject::disconnect(connection);
    }
    m_ToolConnections.clear();
  }

  void TilesTool::ChangeTool(CurTool iTool, bool iForceRefresh)
  {
    if(m_Tool != iTool || iForceRefresh)
    {
      m_Actions[m_Tool]->setIcon(m_Icons[m_Tool]);
      m_Actions[iTool]->setIcon(m_HIcons[iTool]);
      m_Tool = iTool;

      if (m_Tool != PenTool)
      {
        ClearPenToolTile();
      }

      CleanupToolConnections();

      QObject* newFilter = nullptr;
      switch (iTool)
      {
      case None:
        SetupSelectionTool();
        newFilter = m_Tools.m_Selection;
        break;
      case PenTool:
        SetupPenTool();
        newFilter = m_Tools.m_Pen;

        break;
      case EraserTool:
        SetupEraserTool();
        newFilter = m_Tools.m_Pen;

        break;
      }

      emit onToolChanged(newFilter, nullptr);
    }
  }

  void TilesTool::SetupEraserTool()
  {
    m_Tools.m_Pen->SetSnapSize(One<Vec2i>());
    auto newConnection = QObject::connect(m_Tools.m_Pen, &PenToolFilter::onAddPoint, [this](Vec2i iPos, bool iWasDrawing)
    {
      ObjectHandle tile = GetAt(iPos);
      if (!m_World.IsObjectValid(tile))
      {
        return;
      }
      Remove(tile);
    });

    m_ToolConnections.push_back(newConnection);
  }

  void TilesTool::SetupSelectionTool()
  {
    auto newConnection = QObject::connect(m_Tools.m_Selection, &MouseSelectionFilter::onSelectionEnded, [this](AABB2Di const& iSelBox)
    {
      GfxSystem& gfxSys = *m_World.GetSystem<GfxSystem>();

      AABB2Di queryBox;
      Vec3 worldPos;
      Vec3 viewDir;
      gfxSys.ScreenToWorld(iSelBox.m_Data[0], worldPos, viewDir);
      queryBox.m_Data[0] = MathTools::ToIVec(MathTools::As2DVec(worldPos)) * EngineCommon::s_WorldToPixel;
      gfxSys.ScreenToWorld(iSelBox.m_Data[1], worldPos, viewDir);
      queryBox.m_Data[1] = MathTools::ToIVec(MathTools::As2DVec(worldPos)) * EngineCommon::s_WorldToPixel;

      // World and Screen space have opposite Y directions
      std::swap(queryBox.m_Data[0].y, queryBox.m_Data[1].y);

      queryBox.m_Data[1].x = Mathi::Max(queryBox.m_Data[0].x + 1, queryBox.m_Data[1].x);
      queryBox.m_Data[1].y = Mathi::Max(queryBox.m_Data[0].y + 1, queryBox.m_Data[1].y);

      QueryResult results(m_ResultsCache);

      m_TilesIdx.query(boost::geometry::index::intersects(queryBox), results.Inserter());

      if (results.empty())
      {
        return ;
      }
      if (results.size() > 1)
      {
        LOG_WARNING << "Multiple selection not yet supported" << "\n";
        return;
      }

      SelectTile(m_ResultsCache[0].second);

    });

    m_ToolConnections.push_back(newConnection);
  }

  void TilesTool::SelectTile(ObjectHandle iTile)
  {
    if (iTile == m_SelectionHandle)
    {
      return;
    }

    if (!m_World.IsObjectValid(iTile))
    {
      m_SelectionSettings->setEnabled(false);
    }

    m_SelectionHandle = ObjectHandle();
    m_Selection = nullptr;

    if (m_World.IsObjectValid(iTile))
    {
      m_SelectionHandle = iTile;
      m_SelectionSettings->setEnabled(true);
      TileItemData* tileDesc = m_TilesView.Get(iTile);

      m_SelectionTile->ForceSelection(tileDesc->m_Tileset, tileDesc->m_Tile);
      m_SelectionLayer->SetLayer(tileDesc->m_Layer);
      m_SelectionX->setValue(tileDesc->m_Position.x);
      m_SelectionY->setValue(tileDesc->m_Position.y);

      m_Selection = tileDesc;
    }
  }

  ObjectHandle TilesTool::GetAt(Vec2i iWorldPos)
  {
    AABB2Di queryBox(iWorldPos, One<Vec2i>());

    QueryResult results(m_ResultsCache);
    m_TilesIdx.query(boost::geometry::index::intersects(queryBox), results.Inserter());

    if (!results.empty())
    {
      eXl_ASSERT(m_TilesView.Get(m_ResultsCache[0].second) != nullptr);
      return m_ResultsCache[0].second;
    }
    return ObjectHandle();
  }

  ObjectHandle TilesTool::AddAt(Vec2i iPixelPos, bool iAppend)
  {
    return AddAt(m_TileSelection->GetTileset(), 
      m_TileSelection->GetTileName(), 
      m_LayerWidget->GetCurLayer(), 
      m_TerrainWidget->GetCurTerrain(), 
      iPixelPos, iAppend);
  }

  ObjectHandle TilesTool::AddAt(ResourceHandle<Tileset> iTileset, TileName iTile, uint32_t iLayer, TerrainTypeName iTerrain, Vec2i iPixelPos, bool iAppend)
  {
    ObjectHandle tileHandle = m_World.CreateObject();
    TileItemData& newTile = m_TilesView.GetOrCreate(tileHandle);
    newTile.m_Position = iPixelPos;
    newTile.m_Layer = iLayer;
    newTile.m_Tileset = iTileset;
    newTile.m_Tile = iTile;
    newTile.m_Type = iTerrain;

    m_EditorMgr.CreateComponent(tileHandle);

#if 0
    if (iAppend)
    {
      eXl_ASSERT(m_TilesHistory.back().m_Kind == Operation::Added);
      m_TilesHistory.back().m_Tiles.push_back(newTile);
    }
    else
    {
      Operation op(Operation::Added);
      op.m_Tiles.push_back(newTile);
      if (m_CurrentHistoryPointer < m_TilesHistory.size())
      {
        // Branch history
        // (we do not have to, could insert, but let's not be too original)
        m_TilesHistory.resize(m_CurrentHistoryPointer, Operation(Operation::Invalid));
      }
      m_TilesHistory.push_back(std::move(op));
      ++m_CurrentHistoryPointer;
    }
#endif
    AddTileToWorld(tileHandle, newTile);
    UpdateObjectBoxAndTile(tileHandle, newTile);

    emit onEditDone();

    return tileHandle;
  }

  void TilesTool::AddTileToWorld(ObjectHandle iHandle, TileItemData& iTile)
  {
    Transforms& trans = *m_World.GetSystem<Transforms>();
    GfxSystem& gfx = *m_World.GetSystem<GfxSystem>();

    Vec2 tilePos(iTile.m_Position.x, iTile.m_Position.y);
    tilePos /= EngineCommon::s_WorldToPixel;
    trans.AddTransform(iHandle, translate(Identity<Mat4>(), Vec3(tilePos, 0)));

    GfxSpriteComponent& gfxComp = gfx.CreateSpriteComponent(iHandle);
    gfxComp.SetTileset(iTile.m_Tileset.GetOrLoad());
    gfxComp.SetTileName(iTile.m_Tile);
    gfxComp.SetLayer(iTile.m_Layer);
  }
  
  void TilesTool::Remove(ObjectHandle iHandle)
  {
    TileItemData* tileDesc = m_TilesView.Get(iHandle);
    if (tileDesc == nullptr)
    {
      return;
    }
    tileDesc->m_Tileset = ResourceHandle<Tileset>();

    PlacedTile* placedTile = m_PlacedTileData.Get(iHandle);
    eXl_ASSERT_REPAIR_BEGIN(placedTile != nullptr) {}
    else
    {
      m_TilesIdx.remove(std::make_pair(placedTile->GetBox(), iHandle));
      m_PlacedTileData.Erase(iHandle);
    }

    m_World.DeleteObject(iHandle);

    emit onEditDone();
  }

  void TilesTool::Cleanup(ObjectHandle iHandle)
  {
    PlacedTile const* placedTile = m_PlacedTileData.GetDataForDeletion(iHandle);
    if(placedTile == nullptr) 
    {
      return;
    }
    
    m_TilesIdx.remove(std::make_pair(placedTile->GetBox(), iHandle));
    m_PlacedTileData.Erase(iHandle);
  }

  void TilesTool::UpdateObjectBoxAndTile(ObjectHandle iHandle, TileItemData& iTile)
  {
    AABB2Di& tileBox = m_PlacedTileData.GetOrCreate(iHandle).m_BoxCache;
    BoxIndexEntry oldBoxEntry(tileBox, iHandle);
   
    Vec2i size = SafeGetTileSize(iTile.m_Tileset, iTile.m_Tile);
    tileBox = AABB2Di(iTile.m_Position - size / 2, size);

    m_World.AddTimer(0.0, false, [&iTile, iHandle](World& iWorld)
    {
        if (iWorld.IsObjectValid(iHandle))
        {
          Transforms& trans = *iWorld.GetSystem<Transforms>();
          Mat4 mat = trans.GetLocalTransform(iHandle);
          mat[3] = Vec4(Vec2(iTile.m_Position.x, iTile.m_Position.y) / EngineCommon::s_WorldToPixel, 0, 1);
          trans.UpdateTransform(iHandle, mat);
          trans.GetWorldTransform(iHandle);
        }
    });

    m_TilesIdx.remove(oldBoxEntry);
    m_TilesIdx.insert(std::make_pair(tileBox, iHandle));
  }
}