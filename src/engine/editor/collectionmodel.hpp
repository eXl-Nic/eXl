#pragma once

#include <QAbstractItemModel>
#include <core/containers.hpp>

namespace eXl
{
  template <typename Key, typename Val, typename Resource>
  class CollectionModel : public QAbstractItemModel
  {
  public:

    using key_type = Key;
    using value_type = Val;

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

  template <typename Key, typename Val, typename Resource, UnorderedMap<Key, Val> Resource::* MapPtr>
  class CollectionModelMapAdaptor : public CollectionModel<Key, Val, Resource>
  {
  public:
    static CollectionModelMapAdaptor* Create(QObject* iParent, Resource* iResource);

    // Bypasses model, only use on modification callbacks.
    bool SetOnResource(Key const& iName, Val iObject);
  protected:
    CollectionModelMapAdaptor(QObject* iParent, Resource* iResource);
    bool AddToResource(Key const& iName, Val const& iValue) override;
    bool RemoveFromResource(Key const& iName) override;
    Val const* FindInResource(Key const& iName) const override;
  };

  template<typename Key, typename Value, typename ResourceType>
  class MapCollectionModel : public CollectionModel<Key, Value, ResourceType>
  {
  public:
    static MapCollectionModel* Create(QObject* iParent, ResourceType* iRes, UnorderedMap<Key, Value> ResourceType::* iMapPtr);

  protected:
    MapCollectionModel(QObject* iParent, ResourceType* iSys);
    bool AddToResource(Key const& iName, Value const& iObject) override;
    bool RemoveFromResource(Key const& iName) override;
    Value const* FindInResource(Key const& iName) const override;

    UnorderedMap<Key, Value> ResourceType::* m_MapPtr;
  };

  template<typename Key, typename Value, typename ResourceType>
  MapCollectionModel<Key,Value, ResourceType>* MapCollectionModel<Key, Value, ResourceType>::Create(QObject* iParent, ResourceType* iRes, UnorderedMap<Key, Value> ResourceType::* iMapPtr)
  {
    MapCollectionModel<Key, Value, ResourceType>* newModel = new MapCollectionModel<Key, Value, ResourceType>(iParent, iRes);
    newModel->m_MapPtr = iMapPtr;
    newModel->BuildMap(iRes->*iMapPtr);

    return newModel;
  }

  template<typename Key, typename Value, typename ResourceType>
  MapCollectionModel<Key, Value, ResourceType>::MapCollectionModel(QObject* iParent, ResourceType* iRes)
    : CollectionModel<Key,Value,ResourceType>(iParent, iRes)
  {
  }

  template<typename Key, typename Value, typename ResourceType>
  bool MapCollectionModel<Key, Value, ResourceType>::AddToResource(Key const& iName, Value const& iObject)
  {
    return (this->m_Resource->*m_MapPtr).insert(std::make_pair(iName, iObject)).second;
  }

  template<typename Key, typename Value, typename ResourceType>
  bool MapCollectionModel<Key, Value, ResourceType>::RemoveFromResource(Key const& iName)
  {
    (this->m_Resource->*m_MapPtr).erase(iName);
    return true;
  }

  template<typename Key, typename Value, typename ResourceType>
  Value const* MapCollectionModel<Key, Value, ResourceType>::FindInResource(Key const& iName) const
  {
    auto iter = (this->m_Resource->*m_MapPtr).find(iName);
    if (iter != (this->m_Resource->*m_MapPtr).end())
    {
      return &iter->second;
    }
    return nullptr;
  }

  template<typename Key, typename Value, typename ResourceType>
  constexpr MapCollectionModel<Key, Value, ResourceType>* MakeMapCollectionModel(QObject* iParent, ResourceType* iRes, UnorderedMap<Key, Value> ResourceType::* iMapPtr)
  {
    return MapCollectionModel<Key, Value, ResourceType>::Create(iParent, iRes, iMapPtr);
  }
}

#include "collectionmodel.cxx"