#include "terraintool.hpp"

#include <core/type/tagtype.hpp>

#include <editor/editordef.hpp>
#include <editor/gamewidgetselection.hpp>
#include <editor/editoricons.hpp>
#include <editor/eXl_Editor/resourcehandle_editor.h>

#include <math/mathtools.hpp>
#include <math/aabb2dpolygon.hpp>

#include <engine/map/map.hpp>
#include <engine/map/maptiler.hpp>
#include <engine/gfx/gfxcomponent.hpp>

#include <QListView>
#include <QSplitter>
#include <QVBoxLayout>
#include <QToolbar>
#include <QActionGroup>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QSpinBox>
#include <QPainter>
#include <QPainterPath>

namespace eXl
{
  IMPLEMENT_TAG_TYPE_EX(TerrainTool::Island, TerrainTool__Island);

  class TerrainToolDrawer : public GameWidget::PainterInterface
  {
  public:
    TerrainToolDrawer(QObject* iParent) : PainterInterface(iParent)
    {}

    UnorderedMap<ObjectHandle, QPainterPath*> m_CurIslands;
    UnorderedMap<ObjectHandle, QPainterPath*> m_OtherIslands;
    AABB2Di m_CurSelection;

  protected:
    void paint(QPainter& iPainter) const override
    {
      QPen pen;
      
      pen.setWidth(2);
      pen.setStyle(Qt::SolidLine);
      pen.setColor(QColor(0, 0, 255));
      iPainter.setPen(pen);

      QBrush brush;
      brush.setColor(QColor(0, 0, 128));
      brush.setStyle(Qt::SolidPattern);
      
      iPainter.setTransform(GetWorldToScreenTransform());

      for (auto const& island : m_CurIslands)
      {
        iPainter.drawPath(*island.second);
        iPainter.fillPath(*island.second, brush);
      }

      pen.setColor(QColor(64, 64, 64));
      brush.setColor(QColor(96, 96, 96));
      iPainter.setPen(pen);
      for (auto const& island : m_OtherIslands)
      {
        iPainter.drawPath(*island.second);
        iPainter.fillPath(*island.second, brush);
      }

      iPainter.setTransform(QTransform());


      iPainter.setBrush(QBrush());
      if (!m_CurSelection.Empty())
      {
        pen.setStyle(Qt::DotLine);
        pen.setColor(QColor(0, 0, 0));
        iPainter.setPen(pen);

        Vector2i size = m_CurSelection.GetSize();
        iPainter.drawRect(m_CurSelection.m_Data[0].X(), m_CurSelection.m_Data[0].Y(), size.X(), size.Y());
      }
    }
  };

  PropertySheetName TerrainTool::ToolDataName()
  {
    static PropertySheetName s_Name("TerrainIsland");
    return s_Name;
  }

  TerrainTool::TerrainTool(QWidget* parent, Tools iTools, World& iWorld)
    : EditorTool(parent)
    , m_World(iWorld)
    , m_IslandsView(*iWorld.GetSystem<GameDatabase>()->GetView<Island>(ToolDataName()))
    , m_Tools(iTools)
  {

    m_Drawer = new TerrainToolDrawer(this);

    m_Icons[MapView] = EditorIcons::GetMapToolIcon();
    m_Icons[BoxTool] = EditorIcons::GetSelectionIcon();
    m_Icons[BoxRemovalTool] = EditorIcons::GetEraseToolIcon();
    m_HIcons[MapView] = EditorIcons::GetHighlightedMapToolIcon();
    m_HIcons[BoxTool] = EditorIcons::GetHighlightedSelectionIcon();
    m_HIcons[BoxRemovalTool] = EditorIcons::GetHighlightedEraseToolIcon();

    QVBoxLayout* toolLayout = new QVBoxLayout(this);
    QSplitter* toolSplitter = new QSplitter(Qt::Vertical);

    {
      QToolBar* tilesTool = new QToolBar(this);

      m_Actions[MapView] = tilesTool->addAction(m_Icons[MapView], "View", [this]
      {
        ChangeTool(MapView);
      });

      m_Actions[BoxTool] = tilesTool->addAction(m_Icons[BoxTool], "Draw", [this]
      {
        ChangeTool(BoxTool);
      });

      m_Actions[BoxRemovalTool] = tilesTool->addAction(m_Icons[BoxRemovalTool], "Erase", [this]
      {
        ChangeTool(BoxRemovalTool);
      });

      QActionGroup* grp = new QActionGroup(this);
      grp->addAction(m_Actions[MapView]);
      grp->addAction(m_Actions[BoxTool]);
      grp->addAction(m_Actions[BoxRemovalTool]);

      toolSplitter->addWidget(tilesTool);
      ChangeTool(MapView);
    }

    {
      QWidget* penSettings = new QWidget(this);
      QVBoxLayout* layout = new QVBoxLayout(penSettings);
      penSettings->setLayout(layout);

      m_TerrainWidget = new TerrainWidget(penSettings);

      QObject::connect(m_TerrainWidget, &TerrainWidget::onTerrainChanged, [this]()
      {
        if (m_Tool != MapView)
        {
          ChangeDisplayedTerrain(false);
        }
      });

      layout->addWidget(m_TerrainWidget);
      

      m_LayerWidget = new LayerWidget(this);

      QObject::connect(m_LayerWidget, &LayerWidget::onLayerChanged, [this](int)
      {
        if (m_Tool != MapView)
        {
          UpdateLayerView(m_CurSelectedLayer);
          ChangeDisplayedTerrain(false);
        }
          
        m_CurSelectedLayer = GetCurLayer();

        if (m_Tool != MapView)
        {
          ClearLayerView(m_CurSelectedLayer);
        }
      });

      layout->addWidget(m_LayerWidget);
      toolSplitter->addWidget(penSettings);

      DynObject dummy;
      dummy.SetType(ResourceManager::GetHandleType(TilingGroup::StaticRtti()), &m_TilingGroup, false);
      m_TilingGroupSelector = new ResourceHandle_Editor(this);
      m_TilingGroupSelector->SetObject(&dummy);

      QObject::connect(m_TilingGroupSelector, &ResourceHandle_Editor::editingFinished, [this]
      {
        ResourceHandle<TilingGroup> const& handle = *m_TilingGroupSelector->Object()->CastBuffer<ResourceHandle<TilingGroup>>();
        SetTilingGroup(handle);
      });
      
      layout->addWidget(m_TilingGroupSelector);
    }

    toolLayout->addWidget(toolSplitter);
  }

  void TerrainTool::Initialize(MapResource const& iMap)
  {
    for (auto const& terrainGroup : iMap.m_Terrains)
    {
      Vector2i tilingSize = SafeGetTilingSize(terrainGroup.m_TilingGroup.GetOrLoad());
      for (auto const& block : terrainGroup.m_Blocks)
      {
        Island newIsland;
        newIsland.m_Layer = block.m_Layer;
        newIsland.m_Terrain = terrainGroup.m_Type;
        newIsland.m_TilingGroup = terrainGroup.m_TilingGroup;
        newIsland.m_IslandPoly = block.m_Shape;
        newIsland.m_IslandPoly.ScaleComponents(tilingSize.X(), tilingSize.Y(), 1, 1);

        AddIsland(std::move(newIsland));
      }
    }
  }

  void TerrainTool::EnableTool()
  {
    ChangeTool(m_Tool, true);
  }

  void TerrainTool::DisableTool()
  {
    //ChangeTool(MapView, true);

    ClearToolConnections();
  }

  void TerrainTool::ClearToolConnections()
  {
    for (auto connection : m_ToolConnections)
    {
      QObject::disconnect(connection);
    }
    m_ToolConnections.clear();
  }

  void TerrainTool::ChangeDisplayedTerrain(bool iDisableAll)
  {
    m_Drawer->m_CurIslands.clear();
    m_Drawer->m_OtherIslands.clear();
    
    m_IslandsView.Iterate([this](ObjectHandle iObject, Island& iIsland)
      {    
        if (iIsland.m_Layer == GetCurLayer())
        {
          auto drawPair = std::make_pair(iObject, &iIsland.m_PolyPath);
          if (iIsland.m_Terrain == GetCurTerrain())
          {
            m_Drawer->m_CurIslands.insert(drawPair);
          }
          else
          {
            m_Drawer->m_OtherIslands.insert(drawPair);
          }
        }
      });
  }

  AABB2Di TerrainTool::ScreenBoxFromPixelWorldBox(AABB2Di const& iBox)
  {
    GfxSystem& gfxSys = *m_World.GetSystem<GfxSystem>();

    AABB2Di screenBox;
    screenBox.m_Data[0] = gfxSys.WorldToScreen(MathTools::To3DVec(MathTools::ToFVec(iBox.m_Data[0]) / EngineCommon::s_WorldToPixel));
    screenBox.m_Data[1] = gfxSys.WorldToScreen(MathTools::To3DVec(MathTools::ToFVec(iBox.m_Data[1]) / EngineCommon::s_WorldToPixel));

    // World and Screen space have opposite Y directions
    std::swap(screenBox.m_Data[0].Y(), screenBox.m_Data[1].Y());

    return screenBox;
  }

  AABB2Di TerrainTool::SnappedPixelWorldBoxFromScreenBox(AABB2Di const& iBox)
  {
    GfxSystem& gfxSys = *m_World.GetSystem<GfxSystem>();

    AABB2Di worldBox;
    Vector3f worldPos;
    Vector3f viewDir;
    gfxSys.ScreenToWorld(iBox.m_Data[0], worldPos, viewDir);
    worldBox.m_Data[0] = MathTools::ToIVec(MathTools::As2DVec(worldPos)) * EngineCommon::s_WorldToPixel;
    gfxSys.ScreenToWorld(iBox.m_Data[1], worldPos, viewDir);
    worldBox.m_Data[1] = MathTools::ToIVec(MathTools::As2DVec(worldPos)) * EngineCommon::s_WorldToPixel;

    // World and Screen space have opposite Y directions
    std::swap(worldBox.m_Data[0].Y(), worldBox.m_Data[1].Y());

    //queryBox.m_Data[1].X() = Mathi::Max(queryBox.m_Data[0].X() + 1, queryBox.m_Data[1].X());
    //queryBox.m_Data[1].Y() = Mathi::Max(queryBox.m_Data[0].Y() + 1, queryBox.m_Data[1].Y());

    AABB2Di snappedBox = worldBox;

    snappedBox.m_Data[1] += m_TilingSize / 2;
    snappedBox.m_Data[0] -= m_TilingSize / 2;

    snappedBox.m_Data[0].X() /= m_TilingSize.X();
    snappedBox.m_Data[0].Y() /= m_TilingSize.Y();
    snappedBox.m_Data[1].X() /= m_TilingSize.X();
    snappedBox.m_Data[1].Y() /= m_TilingSize.Y();

    snappedBox.m_Data[0].X() *= m_TilingSize.X();
    snappedBox.m_Data[0].Y() *= m_TilingSize.Y();
    snappedBox.m_Data[1].X() *= m_TilingSize.X();
    snappedBox.m_Data[1].Y() *= m_TilingSize.Y();

    return snappedBox;
  }
  
  void TerrainTool::SetupBoxTool()
  {
    auto connectionMoved = QObject::connect(m_Tools.m_Selection, &MouseSelectionFilter::onSelectionChanged, [this](AABB2Di const& iSelBox)
    {
      m_Drawer->m_CurSelection = AABB2Di();
      if (m_TilingGroup.Get() == nullptr)
      {
        return;
      }

      AABB2Di worldBox = SnappedPixelWorldBoxFromScreenBox(iSelBox);
      AABB2Di selectionBox = ScreenBoxFromPixelWorldBox(worldBox);

      m_Drawer->m_CurSelection = selectionBox;

    });

    auto connectionEnded = QObject::connect(m_Tools.m_Selection, &MouseSelectionFilter::onSelectionEnded, [this](AABB2Di const& iSelBox)
    {
      m_Drawer->m_CurSelection = AABB2Di();
      if (m_TilingGroup.Get() == nullptr)
      {
        return;
      }

      AABB2Di worldBox = SnappedPixelWorldBoxFromScreenBox(iSelBox);

      AddBox(worldBox);

    });

    m_ToolConnections.push_back(connectionMoved);
    m_ToolConnections.push_back(connectionEnded);
  }

  void TerrainTool::SetupBoxRemovalTool()
  {
    auto connectionMoved = QObject::connect(m_Tools.m_Selection, &MouseSelectionFilter::onSelectionChanged, [this](AABB2Di const& iSelBox)
    {
      m_Drawer->m_CurSelection = AABB2Di();
      if (m_TilingGroup.Get() == nullptr)
      {
        return;
      }

      AABB2Di worldBox = SnappedPixelWorldBoxFromScreenBox(iSelBox);
      AABB2Di selectionBox = ScreenBoxFromPixelWorldBox(worldBox);

      m_Drawer->m_CurSelection = selectionBox;

    });

    auto connectionEnded = QObject::connect(m_Tools.m_Selection, &MouseSelectionFilter::onSelectionEnded, [this](AABB2Di const& iSelBox)
    {
      m_Drawer->m_CurSelection = AABB2Di();
      if (m_TilingGroup.Get() == nullptr)
      {
        return;
      }

      AABB2Di worldBox = SnappedPixelWorldBoxFromScreenBox(iSelBox);

      RemoveBox(worldBox);

    });

    m_ToolConnections.push_back(connectionMoved);
    m_ToolConnections.push_back(connectionEnded);
  }

  void TerrainTool::ChangeTool(CurTool iTool, bool iForceRefresh)
  {
    if (m_Tool != iTool || iForceRefresh)
    {
      m_Actions[m_Tool]->setIcon(m_Icons[m_Tool]);
      m_Actions[iTool]->setIcon(m_HIcons[iTool]);
      m_Tool = iTool;

      ClearToolConnections();

      QObject* newFilter = nullptr;
      switch (iTool)
      {
      case MapView:
        UpdateMapView();
        m_Drawer->m_CurIslands.clear();
        m_Drawer->m_OtherIslands.clear();
        break;
      case BoxTool:
        SetupBoxTool();
        ClearLayerView(m_CurSelectedLayer);
        ChangeDisplayedTerrain(false);
        newFilter = m_Tools.m_Selection;

        break;
      case BoxRemovalTool:
        SetupBoxRemovalTool();
        ClearLayerView(m_CurSelectedLayer);
        ChangeDisplayedTerrain(false);
        newFilter = m_Tools.m_Selection;

        break;
      }

      emit onToolChanged(newFilter, m_Drawer);
    }
  }

  uint8_t TerrainTool::GetCurLayer()
  {
    return m_LayerWidget->GetCurLayer();
  }

  TerrainTypeName TerrainTool::GetCurTerrain()
  {
    return m_TerrainWidget->GetCurTerrain();
  }

  void TerrainTool::Island::ComputePainterPath()
  {
    QPainterPath polyPath;
    {
      QPolygonF outer;
      for (auto const& point : m_IslandPoly.Border())
      {
        outer << QPointF(point.X(), point.Y()) / EngineCommon::s_WorldToPixel;
      }

      polyPath.addPolygon(std::move(outer));
    }
    
    for (auto const& hole : m_IslandPoly.Holes())
    {
      QPolygonF holePoly;
      for (auto const& point : hole)
      {
        holePoly << QPointF(point.X(), point.Y()) / EngineCommon::s_WorldToPixel;
      }
      QPainterPath holePath;
      holePath.addPolygon(std::move(holePoly));
      polyPath = polyPath.subtracted(holePath);
    }

    m_PolyPath = std::move(polyPath);
  }

  void TerrainTool::UpdatePolygon(ObjectHandle iHandle, AABB2DPolygoni&& iNewPoly)
  {
    Island* island = m_IslandsView.Get(iHandle);
    if (island != nullptr)
    {
      m_IslandsIdx.remove(BoxIndexEntry(island->m_IslandPoly.GetAABB(), iHandle));

      island->m_IslandPoly = std::move(iNewPoly);
      island->ComputePainterPath();

      m_IslandsIdx.insert(BoxIndexEntry(island->m_IslandPoly.GetAABB(), iHandle));
    }
  }

  void TerrainTool::RemoveIsland(ObjectHandle iHandle)
  {
    Island* island = m_IslandsView.Get(iHandle);
    if (island != nullptr)
    {
      if (island->m_Layer == GetCurLayer())
      {
        if (island->m_Terrain == GetCurTerrain())
        {
          m_Drawer->m_CurIslands.erase(iHandle);
        }
        else
        {
          m_Drawer->m_OtherIslands.erase(iHandle);
        }
      }
      m_IslandsIdx.remove(BoxIndexEntry(island->m_IslandPoly.GetAABB(), iHandle));
      island->m_IslandPoly.Clear();
      island->m_PolyPath.clear();
      island->m_TilingGroup = ResourceHandle<TilingGroup>();
      m_World.DeleteObject(iHandle);
    }
  }

  void TerrainTool::AddIsland(Island iIsland)
  {
    ObjectHandle newIslandHandle = m_World.CreateObject();
    Island& newIsland = m_IslandsView.GetOrCreate(newIslandHandle);
    newIsland = std::move(iIsland);

    newIsland.ComputePainterPath();

    m_IslandsIdx.insert(BoxIndexEntry(newIsland.m_IslandPoly.GetAABB(), newIslandHandle));

    auto drawPair = std::make_pair(newIslandHandle, &newIsland.m_PolyPath);

    if (newIsland.m_Layer == GetCurLayer())
    {
      if (newIsland.m_Terrain == GetCurTerrain())
      {
        m_Drawer->m_CurIslands.insert(drawPair);
      }
      else
      {
        m_Drawer->m_OtherIslands.insert(drawPair);
      }
    }
  }

  void TerrainTool::AddBox(AABB2Di const& iBox)
  {
    QueryResult results(m_ResultsCache);
    m_IslandsIdx.query(
      boost::geometry::index::intersects(iBox), results.Inserter());

    UnorderedSet<ObjectHandle> islandsToRemove;

    UnorderedSet<ObjectHandle> islandsToMerge;

    Vector<Island> newIslands;

    Vector<AABB2DPolygoni> temp;
    AABB2DPolygoni additionalBox(iBox);
    for (auto res : results)
    {
      ObjectHandle islandHandle = res.second;
      Island* island = m_IslandsView.Get(islandHandle);
      if (island != nullptr)
      {
        if (island->m_Layer == GetCurLayer())
        {
          temp.clear();
          if (island->m_Terrain == GetCurTerrain())
          {
            if (boost::geometry::within(additionalBox, island->m_IslandPoly))
            {
              return;
            }

            if(boost::geometry::overlaps(additionalBox, island->m_IslandPoly)
              || boost::geometry::touches(additionalBox, island->m_IslandPoly))
            {
              islandsToMerge.insert(islandHandle);
            }
          }
          else
          {
            island->m_IslandPoly.Difference(additionalBox, temp);
            m_Drawer->m_OtherIslands.find(islandHandle);
            if (temp.empty())
            {
              islandsToRemove.insert(islandHandle);
            }
            else if (temp.size() == 1)
            {
              UpdatePolygon(islandHandle, std::move(temp[0]));
            }
            else
            {
              islandsToRemove.insert(islandHandle);

              for (auto& poly : temp)
              {
                Island newIsland;
                newIsland.m_Layer = island->m_Layer;
                newIsland.m_Terrain = island->m_Terrain;
                newIsland.m_TilingGroup = island->m_TilingGroup;
                newIsland.m_IslandPoly = std::move(poly);

                newIslands.push_back(std::move(newIsland));
              }
            }
          }
        }
      }
    }

    for (ObjectHandle handle : islandsToRemove)
    {
      RemoveIsland(handle);
    }

    Vector<AABB2DPolygoni> polys;
    polys.push_back(std::move(additionalBox));
    for (ObjectHandle handle : islandsToMerge)
    {
      Island* island = m_IslandsView.Get(handle);
      if (island != nullptr)
      {
        polys.push_back(std::move(island->m_IslandPoly));
      }
      RemoveIsland(handle);
    }

    AABB2DPolygoni::Merge(polys);
    for (auto& poly : polys)
    {
      Island newIsland;
      newIsland.m_Layer = GetCurLayer();
      newIsland.m_Terrain = GetCurTerrain();
      newIsland.m_TilingGroup = m_TilingGroup;
      newIsland.m_IslandPoly = std::move(poly);
      AddIsland(std::move(newIsland));
    }
    for (auto& newIsland : newIslands)
    {
      AddIsland(std::move(newIsland));
    }

    emit onEditDone();
  }

  void TerrainTool::RemoveBox(AABB2Di const& iBox)
  {
    QueryResult results(m_ResultsCache);
    m_IslandsIdx.query(
      boost::geometry::index::intersects(iBox), results.Inserter());

    UnorderedSet<ObjectHandle> islandsToRemove;

    Vector<Island> newIslands;

    Vector<AABB2DPolygoni> temp;
    AABB2DPolygoni additionalBox(iBox);
    for (auto islandIdx : results)
    {
      ObjectHandle islandHandle = islandIdx.second;
      Island* island = m_IslandsView.Get(islandHandle);
      if (island != nullptr)
      {
        if (island->m_Layer == GetCurLayer())
        {
          temp.clear();
          if (island->m_Terrain == GetCurTerrain())
          {
            island->m_IslandPoly.Difference(additionalBox, temp);
            m_Drawer->m_OtherIslands.find(islandHandle);
            if (temp.empty())
            {
              islandsToRemove.insert(islandHandle);
            }
            else if (temp.size() == 1)
            {
              UpdatePolygon(islandHandle, std::move(temp[0]));
            }
            else
            {
              islandsToRemove.insert(islandHandle);

              for (auto& poly : temp)
              {
                Island newIsland;
                newIsland.m_Layer = island->m_Layer;
                newIsland.m_Terrain = island->m_Terrain;
                newIsland.m_TilingGroup = island->m_TilingGroup;
                newIsland.m_IslandPoly = std::move(poly);

                newIslands.push_back(std::move(newIsland));
              }
            }
          }
        }
      }
    }

    for (ObjectHandle idx : islandsToRemove)
    {
      RemoveIsland(idx);
    }

    for (auto& newIsland : newIslands)
    {
      AddIsland(std::move(newIsland));
    }

    emit onEditDone();
  }

  void TerrainTool::SetTilingGroup(ResourceHandle<TilingGroup> const& iHandle)
  {
    TilingGroup const* group = iHandle.GetOrLoad();
    Tileset const* tileset = group ? group->GetTileset().GetOrLoad() : nullptr;
    Tile const* defaultTile = tileset ? tileset->Find(group->m_DefaultTile) : nullptr;
    if (defaultTile)
    {
      m_TilingGroup.Set(group);
      m_TilingSize = defaultTile->m_Size;
    }
    else
    {
      m_TilingGroup.Set(nullptr);
    }
  }

  Vector2i SafeGetTileSize(ResourceHandle<Tileset> const& iHandle, TileName iName)
  {
    Tileset const* tileset = iHandle.GetOrLoad();
    eXl_ASSERT_REPAIR_RET(tileset != nullptr, Vector2i::ONE);
    Tile const* tile = tileset->Find(iName);
    eXl_ASSERT_REPAIR_RET(tile != nullptr, Vector2i::ONE);
    return tile->m_Size;
  }

  Vector2i TerrainTool::SafeGetTilingSize(TilingGroup const* iGroup)
  {
    eXl_ASSERT_REPAIR_RET(iGroup != nullptr, Vector2i::ONE);
    return SafeGetTileSize(iGroup->GetTileset(), iGroup->m_DefaultTile);
  }

  void TerrainTool::UpdateLayerView(uint8_t iLayer)
  {
    ClearLayerView(iLayer);
    MapTiler::Batcher batcher;
    Map<TilingGroup const*, MapTiler::Blocks> blocks;
    AABB2Di fullSize;
    m_IslandsView.Iterate([this, iLayer, &blocks, &fullSize](ObjectHandle iHandle, Island& iIsland)
      {
        if (iIsland.m_Layer == iLayer)
        {
          auto iter = blocks.insert(std::make_pair(iIsland.m_TilingGroup.GetOrLoad(), MapTiler::Blocks())).first;
          MapTiler::Blocks& block = iter->second;
          block.group = iIsland.m_TilingGroup.Get();
          Tileset const* groupTileset = block.group->GetTileset().GetOrLoad();
          Vector2i tilingSize = SafeGetTilingSize(block.group);
          block.islands.push_back(iIsland.m_IslandPoly);
          block.islands.back().ScaleComponents(1, 1, tilingSize.X(), tilingSize.Y());
          if (fullSize.Empty())
          {
            fullSize = block.islands.back().GetAABB();
          }
          else
          {
            fullSize.Absorb(block.islands.back().GetAABB());
          }
        }
      });

    if (!fullSize.Empty())
    {
      fullSize.m_Data[0] -= Vector2i::ONE;
      fullSize.m_Data[1] += Vector2i::ONE;
      for (auto& blockEntry : blocks)
      {
        MapTiler::ComputeGfxForBlock(batcher, fullSize, blockEntry.second);
      }
      ObjectHandle layerView = m_World.CreateObject();
      batcher.Finalize(*m_World.GetSystem<GfxSystem>(), layerView, iLayer);

      m_LayerViews.insert(std::make_pair(iLayer, Vector<ObjectHandle>())).first->second.push_back(layerView);
    }
  }

  void TerrainTool::ClearLayerView(uint8_t iLayer)
  {
    auto iter = m_LayerViews.find(iLayer);
    if (iter != m_LayerViews.end())
    {
      for (auto object : iter->second)
      {
        m_World.DeleteObject(object);
      }
      m_LayerViews.erase(iter);
    }
  }

  void TerrainTool::UpdateMapView()
  {
    m_IslandsView.Iterate([this](ObjectHandle iHandle, Island& iIsland)
      {
        if (m_LayerViews.count(iIsland.m_Layer) == 0)
        {
          UpdateLayerView(iIsland.m_Layer);
        }
      });
  }
}