
#include <core/coredef.hpp>
#include <core/name.hpp>

namespace eXl
{
  namespace
  {
    template<typename KeyType, typename std::enable_if<std::is_base_of<Name, KeyType>::value>::type = true>
    inline QString KeyTypeToString(KeyType const& iKey)
    {
      return QString(iKey.c_str());
    }

    inline QString KeyTypeToString(String const& iKey)
    {
      return QString(iKey.c_str());
    }
  }

  template <typename Key, typename Val, typename Resource>
  CollectionModel<Key, Val, Resource>::CollectionModel(QObject* iParent, Resource* iResource)
    : QAbstractItemModel(iParent)
    , m_Resource(iResource)
  {
  }

  template <typename Key, typename Val, typename Resource>
  template <typename Container>
  void CollectionModel<Key, Val, Resource>::Reset(Container const& iContainer)
  {
    beginResetModel();

    m_IndexToName.clear();
    m_NameToIndex.clear();
    BuildMap(iContainer);

    endResetModel();
  }

  template <typename Key, typename Val, typename Resource>
  template <typename Container>
  void CollectionModel<Key, Val, Resource>::BuildMap(Container const& iContainer)
  {
    for (auto const& entry : iContainer)
    {
      auto iter = m_NameToIndex.insert(std::make_pair(entry.first, m_IndexToName.size())).first;
      m_IndexToName.push_back(iter->first);
    }
  }

  template <typename Key, typename Val, typename Resource>
  Key const* CollectionModel<Key, Val, Resource>::GetNameFromIndex(QModelIndex const& iIndex) const
  {
    if(iIndex.isValid())
    {
      if (iIndex.row() < m_IndexToName.size())
      {
        return &m_IndexToName[iIndex.row()];
      }
    }

    return nullptr;
  }

  template <typename Key, typename Val, typename Resource>
  QModelIndex CollectionModel<Key, Val, Resource>::GetIndexFromName(Key const& iKey) const
  {
    auto iter = m_NameToIndex.find(iKey);
    if (iter == m_NameToIndex.end())
    {
      return QModelIndex();
    }
    return index(iter->second, 0, QModelIndex());
  }

  template <typename Key, typename Val, typename Resource>
  Val const* CollectionModel<Key, Val, Resource>::GetObjectFromIndex(QModelIndex const& iIndex) const
  {
    if (iIndex.isValid())
    {
      if (iIndex.row() < m_IndexToName.size())
      {
        return FindInResource(m_IndexToName[iIndex.row()]);
      }
    }

    return nullptr;
  }

  template <typename Key, typename Val, typename Resource>
  QModelIndex CollectionModel<Key, Val, Resource>::index(int row, int column, const QModelIndex &parent) const
  {
    if(!parent.isValid())
    {
      if(row < m_IndexToName.size() && ( column == 0 || column == 1 ))
        return createIndex(row,column,nullptr);
    }
    return QModelIndex();
  }

  template <typename Key, typename Val, typename Resource>
  QModelIndex CollectionModel<Key, Val, Resource>::parent(const QModelIndex &child) const
  {
    return QModelIndex();
  }

  template <typename Key, typename Val, typename Resource>
  QVariant CollectionModel<Key, Val, Resource>::data(const QModelIndex &index, int role) const
  {
    Key const* name;
    Val const* value;
    if (index.isValid() && index.row() < m_IndexToName.size())
    {
      name = &m_IndexToName[index.row()];
      value = GetObjectFromIndex(index);
    }

    eXl_ASSERT_REPAIR_RET(name != nullptr && value != nullptr, QVariant());

    if(role == Qt::DisplayRole)
    {
      if(index.column() == 0)
      {
        return name->c_str();
      }
    }
    else if(role == Qt::EditRole && index.column() == 0)
    {
      return name->c_str();
    }
    return QVariant();
  }

  namespace
  {
    template <typename Obj, typename std::enable_if<std::is_copy_constructible<Obj>::value, bool>::type = true>
    Obj MakeCopy(Obj const& iVal)
    {
      return Obj(iVal);
    }

    template <typename Obj, typename std::enable_if<!std::is_copy_constructible<Obj>::value, bool>::type = true>
    Obj MakeCopy(Obj const& iVal)
    {
      return Obj();
    }
  }

  template <typename Key, typename Val, typename Resource>
  Val CollectionModel<Key, Val, Resource>::MakeObjCopy(Val const& iVal)
  {
    return MakeCopy<Val>(iVal);
  }

  template <typename Key, typename Val, typename Resource>
  bool CollectionModel<Key, Val, Resource>::setData(const QModelIndex &iIndex, const QVariant &value, int role)
  {
    if(iIndex.isValid() && role == Qt::EditRole && iIndex.column() == 0)
    {
      Key name(value.toString().toUtf8().data());
      if (m_NameToIndex.find(name) == m_NameToIndex.end())
      {
        Key oldName = m_IndexToName[iIndex.row()];
        Val const* obj = FindInResource(oldName);
        eXl_ASSERT(obj != nullptr);

        Val objSave = MakeObjCopy(*obj);

        RemoveFromResource(oldName);
        AddToResource(name, objSave);

        m_IndexToName[iIndex.row()] = name;
        m_NameToIndex.erase(oldName);
        m_NameToIndex.insert(std::make_pair(name, iIndex.row()));

        emit dataChanged(iIndex, iIndex);
        return true;
      }
    }
    return false;
  }

  template <typename Key, typename Val, typename Resource>
  bool CollectionModel<Key, Val, Resource>::insertRows(int row, int count, const QModelIndex &iParent)
  {
    eXl_ASSERT_REPAIR_RET(!iParent.isValid(), false);
    eXl_ASSERT_REPAIR_RET(row == rowCount(iParent), false);
    eXl_ASSERT_REPAIR_RET(count == 1, false);
    
    if (count == 0)
    {
      return true;
    }

    QString newTileNameTst = QString::fromUtf8("NewObject");
    while (m_NameToIndex.count(Key(newTileNameTst.toUtf8().data())) != 0)
    {
      newTileNameTst.append("_0");
    }

    Key newTileName(newTileNameTst.toUtf8().data());

    if (AddObject(newTileName, Val()))
    {
      return true;
    }

    return false;
  }

  template <typename Key, typename Val, typename Resource>
  bool CollectionModel<Key, Val, Resource>::AddObject(Key const& iName, Val const& iValue)
  {
    uint32_t curRowCount = m_IndexToName.size();

    if (AddToResource(iName, iValue))
    {
      beginInsertRows(QModelIndex(), curRowCount, curRowCount);

      m_NameToIndex.insert(std::make_pair(iName, m_IndexToName.size()));
      m_IndexToName.push_back(iName);

      endInsertRows();

      return true;
    }
    return false;
  }

  template <typename Key, typename Val, typename Resource>
  bool CollectionModel<Key, Val, Resource>::removeRows(int row, int count, const QModelIndex &iParent)
  {
    eXl_ASSERT_REPAIR_RET(!iParent.isValid(), false);
    eXl_ASSERT_REPAIR_RET(count == 1, false);
    
    if (count == 0)
    {
      return true;
    }

    QModelIndex removed = index(row, 0, iParent);

    eXl_ASSERT_REPAIR_RET(removed.isValid() && removed.row() < m_IndexToName.size(), false);
    
    Key name = m_IndexToName[removed.row()];

    beginRemoveRows(iParent, row, row);

    m_IndexToName.erase(m_IndexToName.begin() + row);
    m_NameToIndex.erase(name);
    for (auto& entry : m_NameToIndex)
    {
      if (static_cast<int32_t>(entry.second) > row)
      {
        --entry.second;
      }
    }
    RemoveFromResource(name);

    endRemoveRows();

    return true;
    //emit layoutChanged();
  }

  template <typename Key, typename Val, typename Resource>
  QVariant CollectionModel<Key, Val, Resource>::headerData(int section, Qt::Orientation orientation, int role) const
  {
    //if (role == Qt::DisplayRole)
    //{
    //  if (orientation == Qt::Horizontal)
    //  {
    //    if(section == 0)
    //    {
    //      return QString("Icon");
    //    }
    //    else if(section == 1)
    //    {
    //      return QString("Name");
    //    }
    //  }
    //}
    return QVariant();
  }

  template <typename Key, typename Val, typename Resource>
  Qt::ItemFlags CollectionModel<Key, Val, Resource>::flags(const QModelIndex &index) const
  {
    if(index.isValid())
    {
      Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

      if(GetNameFromIndex(index) != nullptr && index.column() == 0)
      {
        flags |= Qt::ItemIsEditable;
      }
      return flags;
    }
    return Qt::ItemFlag();
  }

  template <typename Key, typename Val, typename Resource>
  int CollectionModel<Key, Val, Resource>::rowCount(const QModelIndex &parent) const
  {
    if (parent.isValid())
    {
      return 0;
    }
    return m_IndexToName.size();
  }

  template <typename Key, typename Val, typename Resource>
  int CollectionModel<Key, Val, Resource>::columnCount(const QModelIndex &parent) const
  {
    return 1 ;
  }
}
