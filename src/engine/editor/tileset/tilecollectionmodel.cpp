#include "tilecollectionmodel.hpp"

#include <editor/editordef.hpp>

#include <QMessageBox>

#include <engine/gfx/tileset.hpp>

namespace eXl
{
  QVariant TileCollectionModel::data(const QModelIndex &index, int role) const
  {
    TileName const* name;
    Tile const* tile;
    if (index.isValid() && index.row() < m_IndexToName.size())
    {
      name = &m_IndexToName[index.row()];
      tile = GetTileFromIndex(index);
    }

    if (role == Qt::DecorationRole)
    {
      if (index.column() == 0)
      {
        Vector2i size = tile->m_Size;
        Vector2i orig = tile->m_Frames.size() > 0 ? tile->m_Frames[0] : Vector2i::ZERO;

        //QRect tileDim(QPoint(orig.X(), m_CacheTileset.height() - orig.Y()),QSize(size.X(), size.Y()));
        QRect tileDim(QPoint(orig.X(), orig.Y()), QSize(size.X(), size.Y()));

        auto iter = m_CacheTileset.find(tile->m_ImageName);
        if (iter == m_CacheTileset.end())
        {
          if (Image const* img = m_Resource->GetImage(tile->m_ImageName))
          {
            QImage temp;
            eXlImageToQImage(*img, temp);
            iter = const_cast<TileCollectionModel*>(this)->m_CacheTileset.emplace(std::make_pair(tile->m_ImageName, std::move(temp))).first;
          }
          else
          {
            return QVariant();
          }
          
        }

        return QPixmap::fromImage(iter->second.copy(tileDim))/*.mirrored()*/;
      }
    }

    return CollectionModel::data(index, role);
  }

  bool TileCollectionModel::AddToResource(TileName const& iName, Tile const& iObject)
  {
    return m_Resource->AddTile(iName, iObject) == Err::Success;
  }

  bool TileCollectionModel::RemoveFromResource(TileName const& iName)
  {
    m_Resource->RemoveTile(iName);
    return true;
  }

  Tile const* TileCollectionModel::FindInResource(TileName const& iName) const
  {
    return m_Resource->Find(iName);
  }

  TileCollectionModel::TileCollectionModel(QObject* iParent, Tileset* iTileset)
    : CollectionModel(iParent, iTileset)
  {

  }

  TileCollectionModel* TileCollectionModel::Create(QObject* iParent, Tileset* iTileset)
  {
    TileCollectionModel* model = new TileCollectionModel(iParent, iTileset);
    model->BuildMap(*iTileset);

    return model;
  }
}
