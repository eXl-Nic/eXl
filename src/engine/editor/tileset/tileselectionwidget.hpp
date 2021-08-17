#pragma once

#include <QWidget>

#include <core/resource/resource.hpp>
#include <engine/gfx/tileset.hpp>

class QComboBox;
class QListView;
class QSortFilterProxyModel;

namespace eXl
{
  class TileCollectionModel;

  class TileSelectionWidget : public QWidget
  {
    Q_OBJECT
  public:

    struct Conf
    {
      bool m_SelectTileset = true;
      bool m_ComboForTiles = true;
    };

    TileSelectionWidget(QWidget* iParent, Conf const& iConf, ResourceHandle<Tileset>, TileName);

    void ForceSelection(ResourceHandle<Tileset>, TileName);
    void ResetTileCollectionModel();

    ResourceHandle<Tileset> GetTileset() const { return m_SelectedTileset; }
    TileCollectionModel* GetTileCollection() const { return m_TileCollection; }
    //QSortFilterProxyModel* GetSortedTileCollection() const { return m_FilteredModel; }
    TileName GetTileName() const { return m_SelectedTile; }

  Q_SIGNALS:
    void onTilesetChanged();
    void onTileChanged();

  protected:
    ResourceHandle<Tileset> m_SelectedTileset;
    TileName m_SelectedTile;
    TileCollectionModel* m_TileCollection = nullptr;
    QSortFilterProxyModel* m_FilteredModel = nullptr;
    QComboBox* m_TilesetSelector = nullptr;
    QComboBox* m_TileSelector = nullptr;
    QListView* m_TileList = nullptr;
  };
}