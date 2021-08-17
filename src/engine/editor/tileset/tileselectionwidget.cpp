#include "tileselectionwidget.hpp"
#include "tilecollectionmodel.hpp"

#include <editor/editorstate.hpp>

#include <QVBoxLayout>
#include <QComboBox>
#include <QListView>

namespace eXl
{
  TileSelectionWidget::TileSelectionWidget(QWidget* iParent, Conf const& iConf, ResourceHandle<Tileset> iInitTileset, TileName iInitTile)
    : QWidget(iParent)
    , m_SelectedTileset(iInitTileset)
    , m_SelectedTile(iInitTile)
  {
    QVBoxLayout* layout = new QVBoxLayout(this);
    setLayout(layout);

    if(iConf.m_SelectTileset)
    { 
      m_TilesetSelector = new QComboBox;
      layout->addWidget(m_TilesetSelector);

      auto* tilesetModel = EditorState::GetState()->GetProjectResourcesModel()->MakeFilteredModel(m_TilesetSelector, Tileset::StaticLoaderName(), true);
      m_TilesetSelector->setModel(tilesetModel);

      QObject::connect(m_TilesetSelector, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this, tilesetModel](int iIndex)
      {
        Resource::UUID const* resourceId = tilesetModel->GetResourceIDFromIndex(tilesetModel->index(iIndex, 0, QModelIndex()));

        if (resourceId != nullptr && *resourceId != m_SelectedTileset.GetUUID())
        {
          m_SelectedTileset.SetUUID(*resourceId);
          ResetTileCollectionModel();
          emit onTilesetChanged();
          emit onTileChanged();
        }
      });

      {
        Resource::UUID const& tilesetUUID = m_SelectedTileset.GetUUID();
        if (tilesetUUID.IsValid())
        {
          QModelIndex index = tilesetModel->GetIndexFromUUID(tilesetUUID);
          if (index.isValid())
          {
            m_TilesetSelector->setCurrentIndex(index.row());
          }
        }
      }
    }
    else
    {
      m_TilesetSelector = nullptr;
    }

    if (iConf.m_ComboForTiles)
    {
      m_TileSelector = new QComboBox;
      layout->addWidget(m_TileSelector);

      QObject::connect(m_TileSelector, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int iIndex)
      {
        if (m_TileCollection)
        {
          QModelIndex index = m_FilteredModel->index(iIndex, 0, QModelIndex());
          TileName const* selTile = m_TileCollection->GetTileNameFromIndex(m_FilteredModel->mapToSource(index));
          if (selTile != nullptr && *selTile != m_SelectedTile)
          {
            m_SelectedTile = *selTile;
            emit onTileChanged();
          }
        }
      });
    }
    else
    {
      m_TileList = new QListView(this);
      layout->addWidget(m_TileList);

    }

    ResetTileCollectionModel();

  }

  void TileSelectionWidget::ForceSelection(ResourceHandle<Tileset> iTileset, TileName iTile)
  {
    if (iTileset.GetUUID() != m_SelectedTileset.GetUUID())
    {
      m_SelectedTileset = iTileset;
      if (m_TilesetSelector)
      {
        eXl::ProjectResourcesModel::TypeFilter* resModel = static_cast<eXl::ProjectResourcesModel::TypeFilter*>(m_TilesetSelector->model());
        {
          Resource::UUID const& tilesetUUID = m_SelectedTileset.GetUUID();
          if (tilesetUUID.IsValid())
          {
            QModelIndex index = resModel->GetIndexFromUUID(tilesetUUID);
            if (index.isValid())
            {
              m_TilesetSelector->setCurrentIndex(index.row());
            }
          }
        }
      }
      ResetTileCollectionModel();
    }

    if (iTile != m_SelectedTile)
    {
      if (m_TileCollection)
      {
        QModelIndex index = m_TileCollection->GetIndexFromName(iTile);
        if (index.isValid())
        {
          if (m_TileSelector)
          {
            m_TileSelector->setCurrentIndex(m_FilteredModel->mapFromSource(index).row());
          }
          else
          {
            m_TileList->selectionModel()->setCurrentIndex(m_FilteredModel->mapFromSource(index), QItemSelectionModel::SelectCurrent);
          }
        }
      }
    }
  }

  void TileSelectionWidget::ResetTileCollectionModel()
  {
    if (m_TileCollection != nullptr)
    {
      if (m_TileSelector)
      {
        m_TileSelector->setModel(nullptr);
      }
      if(m_TileList)
      {
        m_TileList->setModel(nullptr);
      }
      delete m_TileCollection;
      m_TileCollection = nullptr;
      m_FilteredModel = nullptr;
    }

    if (Tileset const* tileset = m_SelectedTileset.GetOrLoad())
    {
      TileName currentName = m_SelectedTile;
      m_TileCollection = TileCollectionModel::Create(this, const_cast<Tileset*>(tileset));

      m_FilteredModel = new QSortFilterProxyModel(m_TileCollection);
      m_FilteredModel->setSourceModel(m_TileCollection);
      m_FilteredModel->sort(0);

      if (m_TileSelector)
      {
        m_TileSelector->setModel(m_FilteredModel);

        QModelIndex index = m_TileCollection->GetIndexFromName(currentName);
        if (index.isValid())
        {
          m_TileSelector->setCurrentIndex(m_FilteredModel->mapFromSource(index).row());
        }
        else
        {
          if (m_TileCollection->rowCount(QModelIndex()) > 0)
          {
            m_TileSelector->setCurrentIndex(0);
          }
        }
      }
      if (m_TileList)
      {
        m_TileList->setModel(m_FilteredModel);
        m_TileList->setSelectionModel(new QItemSelectionModel(m_FilteredModel));
        QObject::connect(m_TileList->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& iSelected, const QItemSelection& iDeselected)
        {
          if (iSelected.isEmpty())
          {
            m_SelectedTile = TileName();
            emit onTileChanged();
          }
          else
          {
            if (iSelected.indexes().size() == 1)
            {
              QModelIndex index = *iSelected.indexes().begin();
              if (TileName const* selTile = m_TileCollection->GetNameFromIndex(m_FilteredModel->mapToSource(index)))
              {
                m_SelectedTile = *selTile;
                emit onTileChanged();
              }
            }
          }
        });
      }
    }
  }
}