#pragma once

#include "editortool.hpp"

#include <core/refcobject.hpp>
#include <math/aabb2d.hpp>
#include <math/geometrytraits.hpp>
#include <engine/map/map.hpp>

#include "commonwidgets.hpp"
#include "utils.hpp"
#include "mapitemdata.hpp"


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

    struct PlacedTile
    {
      // -> Position in PIXELS !!!
      
      AABB2Di GetBox() const
      {
        return m_BoxCache;
      }

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
      SmallVector<PlacedTile, 1> m_Tiles;
      const Kind m_Kind;
    };

    static PropertySheetName ToolDataName();

    void Initialize(MapResource const& iMap);

    ObjectHandle GetAt(Vector2i iWorldPos);
    ObjectHandle AddAt(Vector2i iWorldPos, bool iAppend);
    ObjectHandle AddAt(ResourceHandle<Tileset> iTileset, TileName iTile, uint32_t iLayer, TerrainTypeName iTypename, Vector2i iWorldPos, bool iAppend);
    void Remove(ObjectHandle);
    void Cleanup(ObjectHandle);
    void SelectTile(ObjectHandle);

    void AddTileToWorld(ObjectHandle iHandle, TileItemData& iTile);
    uint32_t GetLayer() { return m_LayerWidget->GetCurLayer(); }

    void EnableTool() override;
    void DisableTool() override;

    GameDataView<TileItemData> const& GetTiles() const { return m_TilesView; }
  protected:

    void UpdateObjectBoxAndTile(ObjectHandle iHandle, TileItemData&);

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
    
    GameDataView<TileItemData>& m_TilesView;
    DenseGameDataStorage<PlacedTile> m_PlacedTileData;

    Vector<BoxIndexEntry> m_ResultsCache;
    LayerWidget* m_LayerWidget;
    TerrainWidget* m_TerrainWidget;
    BoxIndex m_TilesIdx;
    
    Vector<Operation> m_TilesHistory;
    uint32_t m_CurrentHistoryPointer = 0;
    
    ObjectHandle m_SelectionHandle;
    TileItemData* m_Selection = nullptr;
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