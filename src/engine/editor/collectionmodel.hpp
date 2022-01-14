#pragma once

#include <QAbstractItemModel>
#include <core/containers.hpp>

namespace eXl
{
  template <typename Key, typename Val, typename Resource>
  class CollectionModel : public QAbstractItemModel
  {
  public:

    template <typename Container>
    void Reset(Container const&);

    Key const* GetNameFromIndex(QModelIndex const& iIndex) const;
    QModelIndex GetIndexFromName(Key const& iKey) const;
    Val const* GetObjectFromIndex(QModelIndex const& iIndex) const;
    bool AddObject(Key const& iName, Val const& iObject);

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex &child) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent) const override;

    int columnCount(const QModelIndex &parent) const override;

  protected:
    CollectionModel(QObject* iParent, Resource*);
    virtual bool AddToResource(Key const& iName, Val const& iObject) = 0;
    virtual bool RemoveFromResource(Key const& iName) = 0;
    virtual Val const* FindInResource(Key const& iName) const = 0;
    virtual Val MakeObjCopy(Val const& iVal);

    template <typename Container>
    void BuildMap(Container const&);

    Resource* m_Resource;
    UnorderedMap<Key, uint32_t> m_NameToIndex;
    Vector<Key> m_IndexToName;
  };
}

#include "collectionmodel.cxx"