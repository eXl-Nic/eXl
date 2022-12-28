#include "functionlibwidget.hpp"
#include <editor/resourceselectionwidget.hpp>
#include <editor/editordef.hpp>

#include <core/resource/resourcemanager.hpp>

#include <QBoxLayout>
#include <QToolbar>
#include <QLabel>
#include <QComboBox>

namespace eXl
{
  LuaFunctionLibrarySelector::LuaFunctionLibrarySelector(QWidget* iParent, Vector<ResourceHandle<LuaFunctionLibrary>> const& iInit)
    : QWidget(iParent)
  {
    QVBoxLayout* layout = new QVBoxLayout(this);
    m_List = new QListWidget(this);
    
    QToolBar* compCollectionTool = new QToolBar(this);
    m_List->setSelectionModel(new QItemSelectionModel(m_List->model()));

    layout->addWidget(new QLabel(QString::fromUtf8("Libraries"), this));
    //QComboBox* libSelector = new QComboBox(this);
    ResourceSelectionWidget* libSelector = new ResourceSelectionWidget(this, LuaFunctionLibrary::StaticLoaderName(), ResourceSelectionWidget::Combo);
    layout->addWidget(libSelector);
    for (auto lib : iInit)
    {
      Resource::Header const* header = ResourceManager::GetHeader(lib.GetUUID());
      if(header != nullptr )
      {
        QListWidgetItem* newItem = new QListWidgetItem(QString::fromUtf8(header->m_ResourceName.c_str()));
        newItem->setData(Qt::UserRole, QVariant::fromValue(lib.GetUUID()));
        m_List->addItem(newItem);
      }
    }

    QObject::connect(m_List->model(), &QAbstractItemModel::dataChanged, [this](QModelIndex const& iIndex, QModelIndex const&)
      {
        emit onListChanged();
      });

    QObject::connect(m_List->model(), &QAbstractItemModel::rowsInserted, [this]()
      {
        emit onListChanged();
      });
    QObject::connect(m_List->model(), &QAbstractItemModel::rowsRemoved, [this]()
      {
        emit onListChanged();
      });

    layout->addWidget(compCollectionTool);
    layout->addWidget(m_List);

    compCollectionTool->addAction(style()->standardIcon(QStyle::SP_FileIcon), "Add New Entry", [this, libSelector]
      {
        uint32_t curRowCount = m_List->count();
        Resource::UUID selId = libSelector->GetSelectedResourceId();
        if (!selId.IsValid()) 
        {
          return;
        }

        for(uint32_t i = 0 ; i < m_List->count(); ++i)
        {
          Resource::UUID curId = m_List->item(i)->data(Qt::UserRole).value < Resource::UUID >() ;
          if(selId == curId)
          {
            return;
          }
        }
        Resource::Header const* header = ResourceManager::GetHeader(selId);
        if (header != nullptr)
        {
          QListWidgetItem* newItem = new QListWidgetItem(QString::fromUtf8(header->m_ResourceName.c_str()));
          newItem->setData(Qt::UserRole, QVariant::fromValue(selId));
          m_List->addItem(newItem);
        }

      });

    compCollectionTool->addAction(style()->standardIcon(QStyle::SP_DialogCancelButton), "Remove Entry", [this]
      {
        QModelIndex currentSelection = m_List->selectionModel()->currentIndex();
        if (currentSelection.isValid())
        {
          delete m_List->takeItem(currentSelection.row());
        }
      });
  }

  Vector<ResourceHandle<LuaFunctionLibrary>> LuaFunctionLibrarySelector::GetList()
  {
    Vector<ResourceHandle<LuaFunctionLibrary>> list(m_List->count());
    for (uint32_t i = 0; i < m_List->count(); ++i)
    {
      list[i].SetUUID(m_List->item(i)->data(Qt::UserRole).value < Resource::UUID >());
    }

    return list;
  }
}