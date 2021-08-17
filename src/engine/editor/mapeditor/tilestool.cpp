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
  IMPLEMENT_RefC(TilesTool::PlacedTile);

  TilesTool::TilesTool(QWidget* parent, Tools iTools, World& iWorld)
    : EditorTool(parent)
    , m_World(iWorld)
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
          m_Selection->m_Position.X() = iNewValue;
          UpdateObjectBoxAndTile(*m_Selection);
        }
      });

      ConnectToValueChanged(m_SelectionY, [this](int iNewValue)
      {
        if (m_Selection)
        {
          m_Selection->m_Position.Y() = iNewValue;
          UpdateObjectBoxAndTile(*m_Selection);
        }
      });

      QWidget* posWidget = new QWidget(m_SelectionSettings);
      QHBoxLayout* posWidgetLayout = new QHBoxLayout(posWidget);
      posWidget->setLayout(posWidgetLayout);

      posWidgetLayout->addWidget(new QLabel("X : ", this));
      posWidgetLayout->addWidget(m_SelectionX);
      posWidgetLayout->addWidget(new QLabel("Y : ", this));
      posWidgetLayout->addWidget(m_SelectionY);

      selectionLayout->addWidget(posWidget);

      QObject::connect(m_SelectionLayer, &LayerWidget::onLayerChanged, [this](int32_t iValue)
      {
        if (m_Selection == nullptr)
        {
          return;
        }
      });

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
        IntrusivePtr<PlacedTile> newTile = eXl_NEW PlacedTile;
        newTile->m_Position = tile.m_Position;
        newTile->m_Layer = tile.m_Layer;
        newTile->m_Tileset = tileGroup.m_Tileset;
        newTile->m_Tile = tile.m_Name;
        newTile->m_Type = tileGroup.m_Type;
        newTile->m_Index = m_Counter++;

        UpdateObjectBoxAndTile(*newTile);

        m_Tiles.insert(std::make_pair(newTile->m_Index, newTile));
        AddTileToWorld(*newTile);
      }
    }
  }

  void TilesTool::ChangeLowLight(LowLightSettings iLowLight)
  {
    GfxSystem& sys = *m_World.GetSystem<GfxSystem>();
    for (auto const& tile : m_Tiles)
    {
      if (iLowLight == EnableAll || (iLowLight == Layer
        && tile.second->m_Layer == m_LayerWidget->GetCurLayer()))
      {
        sys.GetSpriteComponent(tile.second->m_WorldTile)->SetTint(Vector4f::ONE);
      }
      if (iLowLight == DisableAll || (iLowLight == Layer
        && tile.second->m_Layer != m_LayerWidget->GetCurLayer()))
      {
        sys.GetSpriteComponent(tile.second->m_WorldTile)->SetTint(Vector4f(0.8, 0.8, 0.8, 0.5));
      }
    }
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

    TilesTool::PlacedTile dummyTile;
    dummyTile.m_Tileset = m_TileSelection->GetTileset();
    dummyTile.m_Tile = tile;
    dummyTile.m_Layer = m_LayerWidget->GetCurLayer();
    AddTileToWorld(dummyTile);
    m_Tools.m_Pen->SetPenObject(dummyTile.m_WorldTile);
    m_Tools.m_Pen->SetSnapSize(tileData->m_Size);
  }

  void TilesTool::SetupPenTool()
  {
    SetPenToolTile();

    auto newConnection = QObject::connect(m_Tools.m_Pen, &PenToolFilter::onAddPoint, [this](Vector2i iPos, bool iWasDrawing)
    {
      TilesTool::PlacedTile* curTile = GetAt(iPos);
      if (curTile && curTile->m_Layer == GetLayer())
      {
        return ;
      }
      if (m_TileSelection->GetTileset().GetUUID().IsValid()
        && m_TileSelection->GetTileName() != TileName())
      {
        TilesTool::PlacedTile* newTile = AddAt(iPos, iWasDrawing);
        AddTileToWorld(*newTile);
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
    m_Tools.m_Pen->SetSnapSize(Vector2i::ONE);
    auto newConnection = QObject::connect(m_Tools.m_Pen, &PenToolFilter::onAddPoint, [this](Vector2i iPos, bool iWasDrawing)
    {
      PlacedTile* tile = GetAt(iPos);
      if (tile == nullptr)
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
      Vector3f worldPos;
      Vector3f viewDir;
      gfxSys.ScreenToWorld(iSelBox.m_Data[0], worldPos, viewDir);
      queryBox.m_Data[0] = MathTools::ToIVec(MathTools::As2DVec(worldPos)) * DunAtk::s_WorldToPixel;
      gfxSys.ScreenToWorld(iSelBox.m_Data[1], worldPos, viewDir);
      queryBox.m_Data[1] = MathTools::ToIVec(MathTools::As2DVec(worldPos)) * DunAtk::s_WorldToPixel;

      // World and Screen space have opposite Y directions
      std::swap(queryBox.m_Data[0].Y(), queryBox.m_Data[1].Y());

      queryBox.m_Data[1].X() = Mathi::Max(queryBox.m_Data[0].X() + 1, queryBox.m_Data[1].X());
      queryBox.m_Data[1].Y() = Mathi::Max(queryBox.m_Data[0].Y() + 1, queryBox.m_Data[1].Y());

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

      auto iter = m_Tiles.find(m_ResultsCache[0].second);
      eXl_ASSERT(iter != m_Tiles.end());

      SelectTile(iter->second.get());

    });

    m_ToolConnections.push_back(newConnection);
  }

  void TilesTool::SelectTile(PlacedTile* iTile)
  {
    if (iTile == m_Selection)
    {
      return;
    }

    if (iTile == nullptr)
    {
      m_SelectionSettings->setEnabled(false);
    }
    
    m_Selection = nullptr;
    if (iTile)
    {
      m_SelectionSettings->setEnabled(true);

      m_SelectionTile->ForceSelection(iTile->m_Tileset, iTile->m_Tile);
      m_SelectionLayer->SetLayer(iTile->m_Layer);
      m_SelectionX->setValue(iTile->m_Position.X());
      m_SelectionY->setValue(iTile->m_Position.Y());

      m_Selection = iTile;
    }
  }

  TilesTool::PlacedTile* TilesTool::GetAt(Vector2i iWorldPos)
  {
    AABB2Di queryBox(iWorldPos, Vector2i::ONE);

    QueryResult results(m_ResultsCache);
    m_TilesIdx.query(boost::geometry::index::intersects(queryBox), results.Inserter());

    if (results.empty())
    {
      return nullptr;
    }

    auto iter = m_Tiles.find(m_ResultsCache[0].second);
    eXl_ASSERT(iter != m_Tiles.end());

    return iter->second.get();
  }

  TilesTool::PlacedTile* TilesTool::AddAt(Vector2i iPixelPos, bool iAppend)
  {
    IntrusivePtr<PlacedTile> newTile = eXl_NEW PlacedTile;
    newTile->m_Position = iPixelPos;
    newTile->m_Layer = m_LayerWidget->GetCurLayer();
    newTile->m_Tileset = m_TileSelection->GetTileset();
    newTile->m_Tile = m_TileSelection->GetTileName();
    newTile->m_Type = m_TerrainWidget->GetCurTerrain();
    newTile->m_Index = m_Counter++;

    m_Tiles.insert(std::make_pair(newTile->m_Index, newTile));

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

    UpdateObjectBoxAndTile(*newTile);

    emit onEditDone();

    return newTile.get();
  }

  void TilesTool::AddTileToWorld(PlacedTile& iTile)
  {
    if (iTile.m_WorldTile.IsAssigned())
    {
      return;
    }

    iTile.m_WorldTile = m_World.CreateObject();
    Transforms& trans = *m_World.GetSystem<Transforms>();
    GfxSystem& gfx = *m_World.GetSystem<GfxSystem>();

    Matrix4f worldTrans;
    worldTrans.MakeIdentity();
    MathTools::GetPosition2D(worldTrans) = Vector2f(iTile.m_Position.X(), iTile.m_Position.Y()) / DunAtk::s_WorldToPixel;
    trans.AddTransform(iTile.m_WorldTile, &worldTrans);

    GfxSpriteComponent& gfxComp = gfx.CreateSpriteComponent(iTile.m_WorldTile);
    gfxComp.SetTileset(iTile.m_Tileset.GetOrLoad());
    gfxComp.SetTileName(iTile.m_Tile);
    gfxComp.SetLayer(iTile.m_Layer);
  }

  void TilesTool::RemoveFromWorld(PlacedTile& iTile)
  {
    if (!iTile.m_WorldTile.IsAssigned())
    {
      return;
    }
    m_World.DeleteObject(iTile.m_WorldTile);
    iTile.m_WorldTile = ObjectHandle();
  }

  void TilesTool::Remove(PlacedTile* iTile)
  {
    RemoveFromWorld(*iTile);

    m_TilesIdx.remove(std::make_pair(iTile->GetBox(), iTile->m_Index));

    auto iter = m_Tiles.find(iTile->m_Index);
    if (iter != m_Tiles.end())
    {
      m_Tiles.erase(iter);
    }
    emit onEditDone();
  }

  void TilesTool::UpdateObjectBoxAndTile(PlacedTile& iTile)
  {
    BoxIndexEntry oldBoxEntry(iTile.m_BoxCache, iTile.m_Index);
   
    Vector2i size = SafeGetTileSize(iTile.m_Tileset, iTile.m_Tile);
    iTile.m_BoxCache = AABB2Di(iTile.m_Position - size / 2, size);

    if (iTile.m_WorldTile.IsAssigned())
    {
      m_World.AddTimer(0.0, false, [&iTile](World& iWorld)
      {
        Transforms& trans = *iWorld.GetSystem<Transforms>();
        Matrix4f mat = trans.GetLocalTransform(iTile.m_WorldTile);
        MathTools::GetPosition2D(mat) = Vector2f(iTile.m_Position.X(), iTile.m_Position.Y()) / DunAtk::s_WorldToPixel;
        trans.UpdateTransform(iTile.m_WorldTile, mat);
        trans.GetWorldTransform(iTile.m_WorldTile);
      });
    }

    m_TilesIdx.remove(oldBoxEntry);
    m_TilesIdx.insert(std::make_pair(iTile.m_BoxCache, iTile.m_Index));
  }
}