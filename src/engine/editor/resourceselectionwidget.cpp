#include "resourceselectionwidget.hpp"

#include "editorstate.hpp"

#include <QVBoxLayout>
#include <QComboBox>
#include <QListView>

namespace eXl
{
  ResourceSelectionWidget::ResourceSelectionWidget(QWidget* iParent, ResourceLoaderName iName, Widget iConf, Resource::UUID const& iInitRsc)
    : QWidget(iParent)
    , m_Selected(iInitRsc)
  {
    QVBoxLayout* layout = new QVBoxLayout(this);
    setLayout(layout);

    m_Collection = EditorState::GetState()->GetProjectResourcesModel()->MakeFilteredModel(m_Selector, iName, iConf == Combo);
    m_FilteredModel = new QSortFilterProxyModel(m_Collection);
    m_FilteredModel->setSourceModel(m_Collection);
    m_FilteredModel->sort(0);

    if (iConf == Combo)
    {
      m_Selector = new QComboBox;
      layout->addWidget(m_Selector);
      m_Selector->setModel(m_FilteredModel);

      QObject::connect(m_Selector, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int iIndex)
      {
        QModelIndex srcIdx = m_FilteredModel->mapToSource(m_FilteredModel->index(iIndex, 0, QModelIndex()));
        Resource::UUID const* resourceId = m_Collection->GetResourceIDFromIndex(srcIdx);

        if (resourceId != nullptr && *resourceId != m_Selected)
        {
          m_Selected = *resourceId;
          emit onResourceChanged();
        }
      });


    }

    if (iConf == List)
    {
      m_List = new QListView;
      layout->addWidget(m_List);

      m_List->setModel(m_FilteredModel);
      m_List->setSelectionModel(new QItemSelectionModel(m_FilteredModel));
      QObject::connect(m_List->selectionModel(), &QItemSelectionModel::selectionChanged, [this](const QItemSelection& iSelected, const QItemSelection& iDeselected)
      {
        if (iSelected.isEmpty())
        {
          m_Selected = Resource::UUID();
          emit onResourceChanged();
        }
        else
        {
          if (iSelected.indexes().size() == 1)
          {
            QModelIndex index = *iSelected.indexes().begin();

            QModelIndex srcIdx = m_FilteredModel->mapToSource(index);
            Resource::UUID const* resourceId = m_Collection->GetResourceIDFromIndex(srcIdx);

            if (resourceId != nullptr && *resourceId != m_Selected)
            {
              m_Selected = *resourceId;
              emit onResourceChanged();
            }
          }
        }
      });
    }

  }

  void ResourceSelectionWidget::ForceSelection(const Resource::UUID& iId)
  {
    if (iId != m_Selected)
    {
      QModelIndex index = m_Collection->GetIndexFromUUID(iId);
      QModelIndex filteredIdx = m_FilteredModel->mapFromSource(index);
      if (filteredIdx.isValid())
      {
        m_Selected = iId;
        if (m_Selector)
        {
          m_Selector->setCurrentIndex(filteredIdx.row());
        }
        if (m_List)
        {
          m_List->selectionModel()->setCurrentIndex(filteredIdx, QItemSelectionModel::ClearAndSelect);
        }
      }
    }
  }
}