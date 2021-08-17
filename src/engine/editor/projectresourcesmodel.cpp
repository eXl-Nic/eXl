#include "projectresourcesmodel.hpp"
#include "editordef.hpp"
#include "editorstate.hpp"

#include <QApplication>
#include <QStyle>

#include <core/resource/resourcemanager.hpp>
#include <engine/common/project.hpp>

namespace eXl
{
  ProjectResourcesModel::ProjectResourcesModel()
  {
    BuildMap();
  }

  void ProjectResourcesModel::Reset()
  {
    beginResetModel();

    m_Data.clear();
    m_IndexToLoader.clear();
    m_LoaderToIndex.clear();
    BuildMap();

    endResetModel();
  }

  void ProjectResourcesModel::BuildMap()
  {
    Vector<ResourceLoaderName> loaders = EditorState::GetAllRegisteredHandlers();
    std::sort(loaders.begin(), loaders.end(), [](ResourceLoaderName const& iName1, ResourceLoaderName const& iName2)
    {
      return iName1.get() < iName2.get();
    });
    for (uint32_t i = 0; i < loaders.size(); ++i)
    {
      if (loaders[i] == Project::StaticLoaderName())
      {
        continue;
      }
      m_IndexToLoader.push_back(loaders[i]);
      m_LoaderToIndex.insert(std::make_pair(loaders[i], m_IndexToLoader.size() - 1));
    }
    m_Data.resize(loaders.size());

    Vector<Resource::Header> resources = ResourceManager::ListResources();
    for (auto const& resource : resources)
    {
      auto iterCat = m_LoaderToIndex.find(resource.m_LoaderName);
      if (iterCat != m_LoaderToIndex.end())
      {
        CategoryData& cat = m_Data[iterCat->second];

        cat.m_NameToIndex.insert(std::make_pair(resource.m_ResourceId, cat.m_IndexToName.size()));
        cat.m_IndexToName.push_back(resource.m_ResourceId);
      }
    }
  }

  Resource::UUID const* ProjectResourcesModel::GetResourceIDFromIndex(QModelIndex const& iIndex) const
  {
    QModelIndex parent = iIndex.parent();
    if (!parent.isValid())
    {
      return nullptr;
    }

    eXl_ASSERT(parent.row() < m_Data.size());

    CategoryData const& cat = m_Data[parent.row()];

    eXl_ASSERT(iIndex.row() < cat.m_IndexToName.size());

    return &cat.m_IndexToName[iIndex.row()];
  }

  QModelIndex ProjectResourcesModel::GetIndexFromUUID(Resource::UUID const& iId)
  {
    Resource::Header const* header = ResourceManager::GetHeader(iId);
    eXl_ASSERT(header != nullptr);

    auto iterCat = m_LoaderToIndex.find(header->m_LoaderName);
    eXl_ASSERT(iterCat != m_LoaderToIndex.end());

    CategoryData const& cat = m_Data[iterCat->second];

    auto iterRsc = cat.m_NameToIndex.find(iId);
    eXl_ASSERT(iterRsc != cat.m_NameToIndex.end());

    return index(iterRsc->second, 0, index(iterCat->second, 0));
  }

  QModelIndex ProjectResourcesModel::GetIndexFromResourceType(ResourceLoaderName iName)
  {
    auto iterCat = m_LoaderToIndex.find(iName);
    if (iterCat == m_LoaderToIndex.end())
    {
      return QModelIndex();
    }

    return index(iterCat->second, 0, QModelIndex());
  }

  QModelIndex ProjectResourcesModel::index(int row, int column, const QModelIndex &parent) const
  {
    if (parent.isValid())
    {
      quintptr catId = parent.row();
      eXl_ASSERT(catId < m_IndexToLoader.size());

      CategoryData const& cat = m_Data[catId];
      if (row < cat.m_IndexToName.size() && column == 0)
      {
        return createIndex(row, column, catId);
      }
    }

    if (!parent.isValid())
    {
      if (row < m_IndexToLoader.size() && (column == 0))
      {
        return createIndex(row, column, quintptr(-1));
      }
    }

    return QModelIndex();
  }

  QModelIndex ProjectResourcesModel::parent(const QModelIndex &child) const
  {
    quintptr catId = child.internalId();
    if (catId == quintptr(-1))
    {
      return QModelIndex();
    }
    eXl_ASSERT(catId < m_IndexToLoader.size());

    return index(catId, 0);
  }

  QVariant ProjectResourcesModel::data(const QModelIndex &index, int role) const
  {
    if (index.column() != 0)
    {
      return QVariant();
    }

    if (role == Qt::DecorationRole)
    {
      QModelIndex parent = index.parent();
      if (!parent.isValid())
      {
        return QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton);
      }
    }

    if (role == Qt::DisplayRole)
    {
      QModelIndex parent = index.parent();
      if (!parent.isValid())
      {
        return QString::fromUtf8(m_IndexToLoader[index.row()].get().c_str());
      }
      else
      {
        CategoryData const& cat = m_Data[parent.row()];
        Resource::UUID const& id = cat.m_IndexToName[index.row()];
        Resource::Header const* header = ResourceManager::GetHeader(id);
        return QString::fromUtf8(header->m_ResourceName.c_str());
      }
    }
    return QVariant();
  }

  bool ProjectResourcesModel::setData(const QModelIndex &iIndex, const QVariant &value, int role)
  {
    return false;
  }

  void ProjectResourcesModel::OnResourceCreated(Resource* iRsc)
  {
    if (!iRsc)
    {
      return;
    }

    ResourceLoaderName loader = iRsc->GetHeader().m_LoaderName;
    auto iter = m_LoaderToIndex.find(loader);
    if (iter == m_LoaderToIndex.end())
    {
      LOG_WARNING << "Unhandled resource type " << loader.get() << "\n";
      return;
    }

    QModelIndex categoryIndex = index(iter->second, 0);

    CategoryData& data = m_Data[iter->second];
    data.m_NameToIndex.insert(std::make_pair(iRsc->GetHeader().m_ResourceId, data.m_IndexToName.size()));
    data.m_IndexToName.push_back(iRsc->GetHeader().m_ResourceId);
    insertRow(rowCount(categoryIndex), categoryIndex);
  }

  void ProjectResourcesModel::OnResourceDeleted(Resource::UUID const& iRsc, ResourceLoaderName iLoader)
  {
    if (!iRsc.IsValid())
    {
      return;
    }

    auto iter = m_LoaderToIndex.find(iLoader);
    if (iter == m_LoaderToIndex.end())
    {
      LOG_WARNING << "Unhandled resource type " << iLoader.get() << "\n";
      return;
    }

    CategoryData& cat = m_Data[iter->second];
    auto rscIter = cat.m_NameToIndex.find(iRsc);
    if (rscIter == cat.m_NameToIndex.end())
    {
      LOG_WARNING << "Unregistered resource <UUID>" /*<< iRsc*/ << "\n";
      return;
    }

    removeRow(rscIter->second, index(iter->second, 0));
  }

  bool ProjectResourcesModel::insertRows(int row, int count, const QModelIndex &iParent)
  {
    eXl_ASSERT_REPAIR_RET(iParent.isValid(), false);
    eXl_ASSERT_REPAIR_RET(row == rowCount(iParent), false);
    eXl_ASSERT_REPAIR_RET(count == 1, false);


    //emit layoutAboutToBeChanged();

    beginInsertRows(iParent, row, row + count - 1);

    endInsertRows();
    //emit layoutChanged();

    return true;
  }

  bool ProjectResourcesModel::removeRows(int row, int count, const QModelIndex &iParent)
  {
    eXl_ASSERT_REPAIR_RET(iParent.isValid(), false);
    eXl_ASSERT_REPAIR_RET(count == 1, false);

    if (count == 0)
    {
      return true;
    }

    QModelIndex removed = index(row, 0, iParent);

    quintptr catId = iParent.row();
    eXl_ASSERT(catId < m_IndexToLoader.size());

    CategoryData& cat = m_Data[catId];

    eXl_ASSERT(row < cat.m_IndexToName.size());

    beginRemoveRows(iParent, row, row);

    cat.m_NameToIndex.erase(cat.m_IndexToName[row]);
    cat.m_IndexToName.erase(cat.m_IndexToName.begin() + row);
    
    endRemoveRows();

    return true;
  }

  QVariant ProjectResourcesModel::headerData(int section, Qt::Orientation orientation, int role) const
  {
    return QVariant();
  }

  Qt::ItemFlags ProjectResourcesModel::flags(const QModelIndex &index) const
  {
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
  }

  int ProjectResourcesModel::rowCount(const QModelIndex &parent) const
  {
    if (parent.isValid())
    {
      QModelIndex parentTest = parent.parent();
      if (parentTest.isValid())
      {
        return 0;
      }
      return m_Data[parent.row()].m_IndexToName.size();
    }
    return m_IndexToLoader.size();
  }

  int ProjectResourcesModel::columnCount(const QModelIndex &parent) const
  {
    return 1;
  }

  ProjectResourcesModel::TypeFilter* ProjectResourcesModel::MakeFilteredModel(QObject* Parent, ResourceLoaderName iType, bool iIncludeNullRsc)
  {
    return new TypeFilter(Parent, this, iType, iIncludeNullRsc);
  }

  ProjectResourcesModel::TypeFilter::TypeFilter(QObject* Parent, ProjectResourcesModel* iOrigModel, ResourceLoaderName iResourceType, bool iIncludeNullRsc)
    : QAbstractProxyModel(Parent)
    , m_ResourceType(iResourceType)
    , m_HasNull(iIncludeNullRsc)
  {
    setSourceModel(iOrigModel);
    auto iter = iOrigModel->m_LoaderToIndex.find(iResourceType);
    eXl_ASSERT(iter != iOrigModel->m_LoaderToIndex.end());
    m_TypeRoot = iOrigModel->index(iter->second, 0);
  }

  QModelIndex ProjectResourcesModel::TypeFilter::mapToSource(const QModelIndex &proxyIndex) const
  {
    if (!proxyIndex.isValid())
    {
      return m_TypeRoot;
    }
    else
    {
      if (m_HasNull)
      {
        if (proxyIndex.row() == 0)
        {
          return QModelIndex();
        }
        return sourceModel()->index(proxyIndex.row() - 1, 0, m_TypeRoot);
      }
      return sourceModel()->index(proxyIndex.row(), 0, m_TypeRoot);
    }
  }

  QModelIndex ProjectResourcesModel::TypeFilter::mapFromSource(const QModelIndex &sourceIndex) const
  {
    QModelIndex parent = sourceIndex.parent();
    if (!sourceIndex.isValid()
      || !parent.isValid())
    {
      return QModelIndex();
    }
    if (m_HasNull)
    {
      return index(sourceIndex.row() + 1, 0, QModelIndex());
    }
    return index(sourceIndex.row(), 0, QModelIndex());
  }

  QModelIndex ProjectResourcesModel::TypeFilter::index(int row, int column, const QModelIndex &parent) const
  {
    if (parent.isValid())
    {
      return QModelIndex();
    }
    return createIndex(row, 0, nullptr);
  }

  QModelIndex ProjectResourcesModel::TypeFilter::parent(const QModelIndex &child) const
  {
    return QModelIndex();
  }

  int ProjectResourcesModel::TypeFilter::rowCount(const QModelIndex &parent) const
  {
    return sourceModel()->rowCount(m_TypeRoot) + (m_HasNull ? 1 : 0);
  }

  int ProjectResourcesModel::TypeFilter::columnCount(const QModelIndex &parent) const
  {
    return sourceModel()->columnCount(m_TypeRoot);
  }

  QVariant ProjectResourcesModel::TypeFilter::data(const QModelIndex &index, int role) const
  {
    if (m_HasNull && index.row() == 0)
    {
      if (role == Qt::DisplayRole)
      {
        return QString::fromUtf8("(null)");
      }
      return QVariant();
    }
    else
    {
      return QAbstractProxyModel::data(index, role);
    }
  }


  //bool ProjectResourcesModel::TypeFilter::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
  //{
  //  ProjectResourcesModel const* model = static_cast<ProjectResourcesModel const*>(sourceModel());
  //  if (source_parent.isValid())
  //  {
  //    return true;
  //  }
  //
  //  eXl_ASSERT(source_row < model->m_IndexToLoader.size());
  //  if (model->m_IndexToLoader[source_row] == m_ResourceType)
  //  {
  //    return true;
  //  }
  //  return false;
  //}
}
