#include "archetypecustomization.hpp"
#include <editor/customizationmodel.hpp>

#include <core/type/tupletype.hpp>

#include <QAbstractItemView>

namespace eXl
{
  void ArchetypeCustomizationModel::TreeItem::Deleter::operator()(TreeItem* iItem) const
  {
    if (!iItem->srcIndex.isValid())
    {
      return;
    }

    auto iter = m_Model.m_SourceIndexMap.find(iItem->srcIndex);
    if (iter != m_Model.m_SourceIndexMap.end())
    {
      m_Model.m_SourceIndexMap.erase(iter);
    }
  }

  void ArchetypeCustomizationModel::ClearModelFromView(QAbstractItemView* iView)
  {
    ArchetypeCustomizationModel* model = qobject_cast<ArchetypeCustomizationModel*>(iView->model());
    if (model)
    {
      iView->setModel(nullptr);
      delete model;
    }
  }

  ArchetypeCustomizationModel* ArchetypeCustomizationModel::CreateOrUpdateModel(QAbstractItemView* iView, Archetype const* iArchetype, CustomizationData const& iData)
  {
    ArchetypeCustomizationModel* model = qobject_cast<ArchetypeCustomizationModel*>(iView->model());
    if (iView->model() == nullptr)
    {
      model = new ArchetypeCustomizationModel(iView, iArchetype, iData);
      iView->setModel(model);
    }
    else if(model)
    {
      model->UpdateModel(iArchetype, iData);
    }
    else
    {
      eXl_ASSERT(false);
    }
    return model;
  }

  ArchetypeCustomizationModel::ArchetypeCustomizationModel(QObject* iParent, Archetype const* iArchetype, CustomizationData const& iData)
    : QAbstractItemModel(iParent)
    , m_Root(nullptr)
  {
    std::unique_ptr< TreeItem, TreeItem::Deleter> compRoot(new TreeItem(&m_Root), TreeItem::Deleter(*this));
    std::unique_ptr< TreeItem, TreeItem::Deleter> propRoot(new TreeItem(&m_Root), TreeItem::Deleter(*this));
    m_Root.children.emplace_back(std::move(compRoot));
    m_Root.children.emplace_back(std::move(propRoot));
    m_ComponentRoot = m_Root.children[0].get();
    m_PropertiesRoot = m_Root.children[1].get();
    UpdateModel(iArchetype, iData);
  }

  CustomizationData ArchetypeCustomizationModel::GetCustomization()
  {
    auto makeCustoMap = [&](CustomizationModel* iModel) -> UnorderedMap<TypeFieldName, DynObject>
    {
      UnorderedMap<TypeFieldName, DynObject> custoMap;
      ConstDynObject const& rootObj = iModel->GetObjectFromIndex(QModelIndex());
      TupleType const* structType = rootObj.GetType()->IsTuple();
      for (uint32_t i = 0; i < structType->GetNumField(); ++i)
      {
        if (iModel->IsFieldCustomized(i))
        {
          ConstDynObject const& field = iModel->GetObjectFromIndex(iModel->index(i, 0));
          TypeFieldName fieldName;
          Type const* fieldType = structType->GetFieldDetails(i, fieldName);
          custoMap.insert(std::make_pair(fieldName, DynObject(&field)));
        }
      }

      return custoMap;
    };

    CustomizationData custoData;
    for (auto const& compEntry : m_Components)
    {
      UnorderedMap<TypeFieldName, DynObject> custoMap = makeCustoMap(compEntry.second.get());
      if (!custoMap.empty())
      {
        custoData.m_ComponentCustomization.insert(std::make_pair(compEntry.first, std::move(custoMap)));
      }
    }

    for (auto const& propEntry : m_Properties)
    {
      UnorderedMap<TypeFieldName, DynObject> custoMap = makeCustoMap(propEntry.second.get());
      if (!custoMap.empty())
      {
        custoData.m_PropertyCustomization.insert(std::make_pair(propEntry.first, std::move(custoMap)));
      }
    }

    return custoData;
  }

  void ArchetypeCustomizationModel::AddCallbacks(TreeItem* iRoot, Name iName, CustomizationModel* iModel)
  {
    QObject::connect(iModel, &QAbstractItemModel::modelReset, [this, iRoot, iName, iModel]()
    {
      for (uint32_t i = 0; i < iRoot->children.size(); ++i)
      {
        TreeItem* row = iRoot->children[i].get();
        if (row->objectName == iName)
        {
          beginRemoveRows(index(0, 0), i, i);
          row = nullptr;
          iRoot->children.erase(iRoot->children.begin() + i);
          endRemoveRows();

          beginInsertRows(index(0, 0), i, i);
          iRoot->children.insert(iRoot->children.begin() + i, BuildTree(QModelIndex(), iRoot, iName, iModel));
          endInsertRows();
        }
      }
    });

    QObject::connect(iModel, &QAbstractItemModel::rowsAboutToBeRemoved, [this, iName, iModel, iRoot](QModelIndex iParent, int first, int last)
    {
      TreeItem* parent;
      if (!iParent.isValid())
      {
        for (uint32_t i = 0; i < iRoot->children.size(); ++i)
        {
          TreeItem* row = iRoot->children[i].get();
          if (row->objectName == iName)
          {
            parent = row;
            break;
          }
        }
      }
      else
      {
        auto iter = m_SourceIndexMap.find(iParent);
        eXl_ASSERT_REPAIR_RET(iter != m_SourceIndexMap.end(), );
        parent = iter.value();
      }
      
      TreeItem* grandParent = parent->parent;
      uint32_t row = -1;
      for (uint32_t i = 0; i < grandParent->children.size(); ++i)
      {
        if (grandParent->children[i].get() == parent)
        {
          row = i;
          break;
        }
      }
      eXl_ASSERT_REPAIR_RET(row != -1, );

      QModelIndex dstParent = createIndex(row, 0, grandParent);
      beginRemoveRows(dstParent, first, last);
      //changePersistentIndex(index(first, 0, dstParent), QModelIndex());
      parent->children.erase(parent->children.begin() + first, parent->children.begin() + last + 1);
      endRemoveRows();

    });

    QObject::connect(iModel, &QAbstractItemModel::rowsInserted, [this, iName, iModel, iRoot](QModelIndex iParent, int first, int last)
    {
      eXl_ASSERT(first == last);
      TreeItem* parent;
      if (!iParent.isValid())
      {
        for (uint32_t i = 0; i < iRoot->children.size(); ++i)
        {
          TreeItem* row = iRoot->children[i].get();
          if (row->objectName == iName)
          {
            parent = row;
            break;
          }
        }
      }
      else
      {
        auto iter = m_SourceIndexMap.find(iParent);
        eXl_ASSERT_REPAIR_RET(iter != m_SourceIndexMap.end(), );
        parent = iter.value();
      }
      TreeItem* grandParent = parent->parent;
      uint32_t row = -1;
      for (uint32_t i = 0; i < grandParent->children.size(); ++i)
      {
        if (grandParent->children[i].get() == parent)
        {
          row = i;
          break;
        }
      }
      eXl_ASSERT_REPAIR_RET(row != -1, );

      QModelIndex dstParent = createIndex(row, 0, grandParent);
      beginInsertRows(dstParent, first, last);
      parent->children.insert(parent->children.begin() + first, BuildTree(iModel->index(first, 0, iParent), parent, iName, iModel));
      endInsertRows();

    });
  };

  void ArchetypeCustomizationModel::UpdateModel(Archetype const* iArchetype, CustomizationData const& iData)
  {
    emit beginResetModel();

    m_Archetype = iArchetype;
    auto const& components = m_Archetype->GetComponents();
    auto const& properties = m_Archetype->GetProperties();
    
    m_ComponentRoot->children.clear();
    m_PropertiesRoot->children.clear();

    m_Components.clear();
    m_Properties.clear();

    auto applyCusto = [](CustomizationModel& iModel, CustomizationData::FieldsMap const& iCusto)
    {
      TupleType const* objType = iModel.GetObjectFromIndex(QModelIndex()).GetType()->IsTuple();
      for (auto fieldEntry : iCusto)
      {
        TypeFieldName fieldName = fieldEntry.first;
        uint32_t fieldIdx;
        if (objType->GetFieldDetails(fieldName, fieldIdx) != nullptr)
        {
          iModel.ApplyCustomization(fieldIdx, fieldEntry.second);
        }
      }
    };

    for (auto const& compEntry : components)
    {
      auto model = std::make_unique<CustomizationModel>(nullptr, compEntry.second);

      ComponentName name = compEntry.first;
      auto compCusto = iData.m_ComponentCustomization.find(name);
      if (compCusto != iData.m_ComponentCustomization.end())
      {
        applyCusto(*model, compCusto->second);
      }

      m_ComponentRoot->children.emplace_back(BuildTree(QModelIndex(), m_ComponentRoot, name, model.get()));
      AddCallbacks(m_ComponentRoot, name, model.get());
      m_Components.insert(std::make_pair(name, std::move(model)));
    }

    for (auto const& propEntry : properties)
    {
      PropertySheetName name = propEntry.first;

      if (!propEntry.second.m_Instanced)
      {
        continue;
      }

      auto propCusto = iData.m_PropertyCustomization.find(name);

      auto model = std::make_unique<CustomizationModel>(nullptr, propEntry.second.m_Data);
      if (propCusto != iData.m_PropertyCustomization.end())
      {
        applyCusto(*model, propCusto->second);
      }

      m_PropertiesRoot->children.emplace_back(BuildTree(QModelIndex(), m_PropertiesRoot, name, model.get()));
      AddCallbacks(m_PropertiesRoot, name, model.get());
      m_Properties.insert(std::make_pair(name, std::move(model)));
    }

    emit endResetModel();
  }

  std::unique_ptr<ArchetypeCustomizationModel::TreeItem, ArchetypeCustomizationModel::TreeItem::Deleter> ArchetypeCustomizationModel::BuildTree(QModelIndex iIndex, TreeItem* iParent, Name iName, CustomizationModel* iModel)
  {
    std::unique_ptr<TreeItem, TreeItem::Deleter> newItem(new TreeItem(iParent), TreeItem::Deleter(*this));
    newItem->objectName = iName;
    newItem->srcIndex = iIndex;

    int32_t numChildren = iModel->rowCount(iIndex);
    for (int32_t i = 0; i < numChildren; ++i)
    {
      newItem->children.push_back(BuildTree(iModel->index(i, 0, iIndex), newItem.get(), iName, iModel));
    }
    if (iIndex.isValid())
    {
      m_SourceIndexMap.insert(iIndex, newItem.get());
    }
    return std::move(newItem);
  }

  QModelIndex ArchetypeCustomizationModel::index(int row, int column, const QModelIndex &iParent) const
  {
    if (!iParent.isValid())
    {
      if (row < 2 && column == 0)
        return createIndex(row, column, (void*)&m_Root);
    }
    else if (iParent.isValid())
    {
      TreeItem const* parentNode = reinterpret_cast<TreeItem const*>(iParent.internalPointer());
      TreeItem const* node = parentNode->children[iParent.row()].get();

      if (row < node->children.size())
      {
        if (column < 3)
        {
          return createIndex(row, column, (void*)node);
        }
      }
    }
    return QModelIndex();
  }

  QModelIndex ArchetypeCustomizationModel::parent(const QModelIndex &child) const
  {
    if(child.isValid())
    {
      TreeItem const* parentNode = reinterpret_cast<TreeItem const*>(child.internalPointer());
      if(parentNode)
      {
        if(parentNode->parent)
        {
          unsigned int i;
          for(i = 0; i< parentNode->parent->children.size(); ++i)
          {
            if(parentNode->parent->children[i].get() == parentNode)
						{
              break;
						}
          }
          return createIndex(i,0,(void*)parentNode->parent);
        }
      }
    }
    return QModelIndex();
  }

  QVariant ArchetypeCustomizationModel::data(const QModelIndex &iIndex, int role) const
  {
    if (IsValidField(iIndex))
    {
      CustomizationModel* model;
      QModelIndex srcIndex = SourceIndex(iIndex, model);

      return model->data(srcIndex, role);
    }
    if (role == Qt::DisplayRole && iIndex.column() == 0)
    {
      TreeItem const* parentNode = reinterpret_cast<TreeItem const*>(iIndex.internalPointer());
      if (parentNode == m_ComponentRoot
        || parentNode == m_PropertiesRoot)
      {
        TreeItem const* node = parentNode->children[iIndex.row()].get();

        return QString::fromUtf8(node->objectName.get().c_str());
      }
      else if(parentNode == &m_Root)
      {
        if (iIndex.row() == 0)
        {
          return QString::fromUtf8("Components");
        }
        if (iIndex.row() == 1)
        {
          return QString::fromUtf8("Properties");
        }
      }
    }
    return QVariant();
  }

  bool ArchetypeCustomizationModel::setData(const QModelIndex &iIndex, const QVariant &value, int role)
  {
    if (!IsValidField(iIndex))
    {
      return false;
    }

    CustomizationModel* model;
    QModelIndex srcIndex = SourceIndex(iIndex, model);
    
    if (model->setData(srcIndex, value, role))
    {
      emit dataChanged(iIndex, iIndex);

      return true;
    }

    return false;
  }

  bool ArchetypeCustomizationModel::insertRows(int row, int count, const QModelIndex &iParent)
  {
    if (!IsValidField(iParent))
    {
      return false;
    }

    CustomizationModel* model;
    QModelIndex srcIndex = SourceIndex(iParent, model);

    if (model->insertRows(row, count, srcIndex))
    {
      return true;
    }

    return false;
  }

  bool ArchetypeCustomizationModel::removeRows(int row, int count, const QModelIndex &iParent)
  {
    if (!IsValidField(iParent))
    {
      return false;
    }

    CustomizationModel* model;
    QModelIndex srcIndex = SourceIndex(iParent, model);

    if (model->removeRows(row, count, srcIndex))
    {
      return true;
    }

    return false;
  }

  QVariant ArchetypeCustomizationModel::headerData(int section, Qt::Orientation orientation, int role) const
  {
    if (role == Qt::DisplayRole)
    {
      if (orientation == Qt::Horizontal)
      {
        if(section == 0)
        {
          return QString("Name");
        }
        else if(section == 1)
        {
          return QString("Value");
        }
        else if (section == 2)
        {
          return QString("Customized");
        }
      }
    }
    return QVariant();
  }
  bool ArchetypeCustomizationModel::IsValidField(QModelIndex iIndex) const
  {
    if (iIndex.isValid())
    {
      TreeItem const* parentNode = reinterpret_cast<TreeItem const*>(iIndex.internalPointer());
      if (parentNode == m_ComponentRoot)
      {
        return false;
      }
      if (parentNode == m_PropertiesRoot)
      {
        return false;
      }
      if (parentNode == &m_Root)
      {
        return false;
      }
      return true;
    }
    return false;
  }

  QModelIndex ArchetypeCustomizationModel::SourceIndex(QModelIndex iIndex, CustomizationModel*& oModel) const
  {
    if (IsValidField(iIndex))
    {
      TreeItem const* parentNode = reinterpret_cast<TreeItem const*>(iIndex.internalPointer());
      TreeItem const* node = parentNode->children[iIndex.row()].get();
      while (parentNode->parent != &m_Root)
      {
        parentNode = parentNode->parent;
      }

      oModel = parentNode == m_ComponentRoot 
        ? m_Components.find(ComponentName(node->objectName))->second.get()
        : m_Properties.find(PropertySheetName(node->objectName))->second.get();
      return oModel->index(node->srcIndex.row(), iIndex.column(), node->srcIndex.parent());
    }
    return QModelIndex();
  }

  Qt::ItemFlags ArchetypeCustomizationModel::flags(const QModelIndex &iIndex) const
  {
    if(iIndex.isValid())
    {
      CustomizationModel* model;
      QModelIndex srcIdx = SourceIndex(iIndex, model);
      if (srcIdx.isValid())
      {
        return model->flags(srcIdx);
      }
    }
    return Qt::ItemFlag();
  }

  int ArchetypeCustomizationModel::rowCount(const QModelIndex &parent) const
  {
    if (!parent.isValid())
    {
      return 2;
    }
    else
    {
      TreeItem const* parentNode = reinterpret_cast<TreeItem const*>(parent.internalPointer());
      return parentNode->children[parent.row()]->children.size();
    }
    return 0;
  }

  int ArchetypeCustomizationModel::columnCount(const QModelIndex &parent) const
  {
    return 3;
  }
}
