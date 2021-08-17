#pragma once

#include "editortool.hpp"
#include <math/aabb2dpolygon.hpp>
#include <engine/map/tilinggroup.hpp>

#include "commonwidgets.hpp"
#include "utils.hpp"

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

    class Island : public HeapObject
    {
      DECLARE_RefC;
    public:

      void ComputePainterPath();

      AABB2DPolygoni m_IslandPoly;
      ResourceHandle<TilingGroup> m_TilingGroup;
      TerrainTypeName m_Terrain;
      QPainterPath m_PolyPath;
      uint8_t m_Layer;
    };

    void Initialize(MapResource const& iMap);
    void EnableTool() override;
    void DisableTool() override;

    static Vector2i SafeGetTilingSize(TilingGroup const*);

    UnorderedMap<uint32_t, IntrusivePtr<Island>> const& GetIslands() const { return m_Islands; }

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

    void UpdatePolygon(uint32_t iIsland, AABB2DPolygoni&& iNewPoly);
    void RemoveIsland(uint32_t iIdx);
    void AddIsland(IntrusivePtr<Island> iIslandPtr);

    void SetTilingGroup(ResourceHandle<TilingGroup> const& iHandle);

    AABB2Di SnappedPixelWorldBoxFromScreenBox(AABB2Di const& iBox);
    AABB2Di ScreenBoxFromPixelWorldBox(AABB2Di const& iBox);

    uint8_t GetCurLayer();
    TerrainTypeName GetCurTerrain();

    Tools m_Tools;
    Vector<QMetaObject::Connection> m_ToolConnections;

    World& m_World;
    Vector<BoxIndexEntry> m_ResultsCache;
    LayerWidget* m_LayerWidget;
    TerrainWidget* m_TerrainWidget;
    TerrainToolDrawer* m_Drawer;
    BoxIndex m_IslandsIdx;
    UnorderedMap<uint32_t, IntrusivePtr<Island>> m_Islands;
    uint32_t m_Counter = 0;
    ResourceHandle_Editor* m_TilingGroupSelector;
    ResourceHandle<TilingGroup> m_TilingGroup;
    Vector2i m_TilingSize;
    uint8_t m_CurSelectedLayer = 0;
    //Vector<Operation> m_TilesHistory;
    //uint32_t m_CurrentHistoryPointer = 0;
    //PlacedTile* m_Selection = nullptr;

    Map<uint8_t, Vector<ObjectHandle>> m_LayerViews;

    QAction* m_Actions[3];
    QIcon m_Icons[3];
    QIcon m_HIcons[3];
  };
}