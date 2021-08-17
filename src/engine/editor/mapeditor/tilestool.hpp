#pragma once

#include "editortool.hpp"

#include <core/refcobject.hpp>
#include <math/aabb2d.hpp>
#include <math/geometrytraits.hpp>
#include <engine/map/map.hpp>

#include "commonwidgets.hpp"
#include "utils.hpp"


#include <QIcon>

class QListWidget;
class QAction;
class QSpinBox;
class QComboBox;

namespace eXl
{
  class MouseSelectionFilter;
  class PenToolFilter;
  class TileSelectionWidget;

  class TilesTool : public EditorTool
  {
    Q_OBJECT
  public:
    enum CurTool
    {
      None, // Selection/Move
      PenTool,
      EraserTool
    };

    struct Tools
    {
      MouseSelectionFilter* m_Selection;
      PenToolFilter* m_Pen;
    };

    TilesTool(QWidget* parent, Tools iTools, World& iWorld);

    CurTool m_Tool = None;
    
    TileSelectionWidget* m_TileSelection;

    class PlacedTile : public HeapObject
    {
      DECLARE_RefC;
    public:

      // -> Position in PIXELS !!!
      Vector2i m_Position;
      uint32_t m_Layer;
      ResourceHandle<Tileset> m_Tileset;
      TileName m_Tile;
      TerrainTypeName m_Type;
      uint32_t m_Index;

      AABB2Di GetBox()
      {
        return m_BoxCache;
      }

      ObjectHandle m_WorldTile;
      AABB2Di m_BoxCache;
    };

    struct Operation
    {
      enum Kind
      {
        Invalid,
        Added,
        Removed,
        Moved,
        ChangedTileData
      };
      Operation(Kind iKind) : m_Kind(iKind)
      {}
      SmallVector<IntrusivePtr<PlacedTile>, 1> m_Tiles;
      const Kind m_Kind;
    };

    void Initialize(MapResource const& iMap);
    PlacedTile* GetAt(Vector2i iWorldPos);
    PlacedTile* AddAt(Vector2i iWorldPos, bool iAppend);
    void Remove(PlacedTile* iTile);
    void SelectTile(PlacedTile* iTile);

    void AddTileToWorld(PlacedTile& iTile);
    void RemoveFromWorld(PlacedTile& iTile);
    uint32_t GetLayer() { return m_LayerWidget->GetCurLayer(); }

    void EnableTool() override;
    void DisableTool() override;

    UnorderedMap<uint32_t, IntrusivePtr<PlacedTile>> const& GetTiles() const { return m_Tiles; }

  protected:

    void UpdateObjectBoxAndTile(PlacedTile&);

    void CleanupToolConnections();

    enum LowLightSettings
    {
      Layer,
      DisableAll,
      EnableAll,
    };
    void ChangeLowLight(LowLightSettings iLowLight);

    void ChangeTool(CurTool iTool, bool iForceRefresh = false);

    void ClearPenToolTile();
    void SetupPenTool();
    void SetPenToolTile();
    void SetupEraserTool();

    void SetupSelectionTool();

    Tools m_Tools;
    Vector<QMetaObject::Connection> m_ToolConnections;

    World& m_World;
    Vector<BoxIndexEntry> m_ResultsCache;
    LayerWidget* m_LayerWidget;
    TerrainWidget* m_TerrainWidget;
    BoxIndex m_TilesIdx;
    UnorderedMap<uint32_t, IntrusivePtr<PlacedTile>> m_Tiles;
    Vector<Operation> m_TilesHistory;
    uint32_t m_CurrentHistoryPointer = 0;
    uint32_t m_Counter = 0;
    PlacedTile* m_Selection = nullptr;
    QAction* m_Actions[3];
    QIcon m_Icons[3];
    QIcon m_HIcons[3];

    QWidget* m_SelectionSettings;
    TileSelectionWidget* m_SelectionTile;
    LayerWidget* m_SelectionLayer;
    QSpinBox* m_SelectionX;
    QSpinBox* m_SelectionY;
  };
}