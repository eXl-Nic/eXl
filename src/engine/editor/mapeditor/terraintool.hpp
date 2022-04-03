#pragma once

#include "editortool.hpp"

#include <math/aabb2dpolygon.hpp>
#include <engine/map/tilinggroup.hpp>

#include "commonwidgets.hpp"
#include "utils.hpp"
#include "mapitemdata.hpp"

#include <QIcon>
#include <QPainterPath>

class ResourceHandle_Editor;

namespace eXl
{
  class TerrainToolDrawer;
  class MouseSelectionFilter;

  class TerrainTool : public EditorTool
  {
    Q_OBJECT
  public:
    enum CurTool
    {
      MapView,
      BoxTool,
      BoxRemovalTool
    };

    struct Tools
    {
      MouseSelectionFilter* m_Selection;
    };

    TerrainTool(QWidget* parent, Tools iTools, World& iWorld);

    CurTool m_Tool = BoxTool;

    struct Island
    {
    public:

      void ComputePainterPath(TerrainIslandItemData const& iData);

      QPainterPath m_PolyPath;
    };

    static PropertySheetName ToolDataName();

    void Initialize(MapResource const& iMap);
    void EnableTool() override;
    void DisableTool() override;

    static Vec2i SafeGetTilingSize(TilingGroup const*);

    GameDataView<TerrainIslandItemData> const& GetIslands() const { return m_IslandsView; }

  protected:

    void UpdateMapView();
    void UpdateLayerView(uint8_t);
    void ClearLayerView(uint8_t);

    void ClearToolConnections();

    void ChangeTool(CurTool iTool, bool iForceRefresh = false);
    void ChangeDisplayedTerrain(bool iDisableAll);

    void SetupBoxTool();
    void SetupBoxRemovalTool();

    void AddBox(AABB2Di const&);
    void RemoveBox(AABB2Di const&);

    void UpdatePolygon(ObjectHandle iIsland, AABB2DPolygoni&& iNewPoly);
    void RemoveIsland(ObjectHandle iIsland);
    void AddIsland(TerrainIslandItemData iIsland);

    void SetTilingGroup(ResourceHandle<TilingGroup> const& iHandle);

    AABB2Di SnappedPixelWorldBoxFromScreenBox(AABB2Di const& iBox);
    AABB2Di ScreenBoxFromPixelWorldBox(AABB2Di const& iBox);

    uint8_t GetCurLayer();
    TerrainTypeName GetCurTerrain();

    Tools m_Tools;
    Vector<QMetaObject::Connection> m_ToolConnections;

    World& m_World;
    GameDataView<TerrainIslandItemData>& m_IslandsView;
    DenseGameDataStorage<Island> m_PolyView;
    Vector<BoxIndexEntry> m_ResultsCache;
    LayerWidget* m_LayerWidget;
    TerrainWidget* m_TerrainWidget;
    TerrainToolDrawer* m_Drawer;
    BoxIndex m_IslandsIdx;
    ResourceHandle_Editor* m_TilingGroupSelector;
    ResourceHandle<TilingGroup> m_TilingGroup;
    Vec2i m_TilingSize;
    uint8_t m_CurSelectedLayer = 0;

    Map<uint8_t, Vector<ObjectHandle>> m_LayerViews;

    QAction* m_Actions[3];
    QIcon m_Icons[3];
    QIcon m_HIcons[3];
  };

  DEFINE_TYPE_EX(TerrainTool::Island, TerrainTool__Island, );
}