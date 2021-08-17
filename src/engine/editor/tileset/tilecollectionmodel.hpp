#pragma once

#include <editor/collectionmodel.hpp>
#include <QImage>
#include <engine/gfx/tileset.hpp>


namespace eXl
{
  struct Tile;
  class Tileset;

  class TileCollectionModel : public CollectionModel<TileName, Tile, Tileset>
  {
    Q_OBJECT
  public:
    static TileCollectionModel* Create(QObject* iParent, Tileset* iTileset);

    TileName const* GetTileNameFromIndex(QModelIndex const& iIndex) const
    {
      return GetNameFromIndex(iIndex);
    }
    Tile const* GetTileFromIndex(QModelIndex const& iIndex) const
    {
      return GetObjectFromIndex(iIndex);
    }
    bool AddTile(TileName iName, Tile const& iTile)
    {
      return AddObject(iName, iTile);
    }

    QVariant data(const QModelIndex &index, int role) const override;

  protected:
    TileCollectionModel(QObject* iParent, Tileset* iTileset);
    bool AddToResource(TileName const& iName, Tile const& iObject) override;
    bool RemoveFromResource(TileName const& iName) override;
    Tile const* FindInResource(TileName const& iName) const override;

    UnorderedMap<ImageName, QImage> m_CacheTileset;
  };
}