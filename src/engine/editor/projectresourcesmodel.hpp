#pragma once

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <core/name.hpp>
#include <core/containers.hpp>
#include <core/resource/resource.hpp>

namespace eXl
{
  class ProjectResourcesModel : public QAbstractItemModel
  {
    Q_OBJECT
  public:
    ProjectResourcesModel();

    class TypeFilter : public QAbstractProxyModel
    {
    public:
      TypeFilter(QObject* Parent, ProjectResourcesModel* iOrigModel, ResourceLoaderName iResourceType, bool iIncludeNullRsc);

      Resource::UUID const* GetResourceIDFromIndex(QModelIndex const& iIndex) const
      {
        return static_cast<ProjectResourcesModel*>(sourceModel())->GetResourceIDFromIndex(mapToSource(iIndex));
      }
      QModelIndex GetIndexFromUUID(Resource::UUID const& iId)
      {
        return mapFromSource(static_cast<ProjectResourcesModel*>(sourceModel())->GetIndexFromUUID(iId));
      }

      QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
      QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;

      QModelIndex index(int row, int column, const QModelIndex &parent) const override;
      QModelIndex parent(const QModelIndex &child) const override;
      QVariant data(const QModelIndex &index, int role) const override;
      int rowCount(const QModelIndex &parent) const override;
      int columnCount(const QModelIndex &parent) const override;
    protected:
      ResourceLoaderName m_ResourceType;
      QModelIndex m_TypeRoot;
      bool m_HasNull;
    };

    TypeFilter* MakeFilteredModel(QObject* Parent, ResourceLoaderName iName, bool iIncludeNullRsc);

    void Reset();

    Resource::UUID const* GetResourceIDFromIndex(QModelIndex const& iIndex) const;
    QModelIndex GetIndexFromUUID(Resource::UUID const& iId);
    QModelIndex GetIndexFromResourceType(ResourceLoaderName iName);

    void OnResourceCreated(Resource* iRsc);
    void OnResourceDeleted(Resource::UUID const& iRsc, ResourceLoaderName iLoader);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex &child) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent) const override;

    int columnCount(const QModelIndex &parent) const override;

  protected:

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    void BuildMap();

    struct CategoryData
    {
      UnorderedMap<Resource::UUID, uint32_t> m_NameToIndex;
      Vector<Resource::UUID> m_IndexToName;
    };
    
    Vector<CategoryData> m_Data;
    UnorderedMap<ResourceLoaderName, uint32_t> m_LoaderToIndex;
    Vector<ResourceLoaderName> m_IndexToLoader;
  };

  
}