#include "customizationmodel.hpp"
#include "objectmodel.hpp"
#include "editordef.hpp"

#include <core/type/tupletype.hpp>

#include <QAbstractItemView>

namespace eXl
{
  void CustomizationModel::ClearModelFromView(QAbstractItemView* iView)
  {
    CustomizationModel* model = qobject_cast<CustomizationModel*>(iView->model());
    if (model)
    {
      iView->setModel(nullptr);
      delete model;
    }
  }

  CustomizationModel* CustomizationModel::CreateOrUpdateModel(QAbstractItemView* iView, ConstDynObject const& iObj)
  {
    CustomizationModel* model = qobject_cast<CustomizationModel*>(iView->model());
    if (iView->model() == nullptr)
    {
      model = new CustomizationModel(iView, iObj);
      iView->setModel(model);
    }
    else if(model)
    {
      model->UpdateModel(iObj);
    }
    else
    {
      eXl_ASSERT(false);
    }
    return model;
  }

  CustomizationModel::CustomizationModel(QObject* iParent, ConstDynObject const& iObj)
    : ObjectModel(iParent, false, DynObject())
    , m_Root(nullptr)
  {
    UpdateModel(iObj);
  }

  void CustomizationModel::UpdateModel(ConstDynObject const& iObj)
  {
    emit beginResetModel();

    m_CustomizedObject = DynObject(&iObj);
    m_DefaultObject.SetTypeConst(iObj.GetType(), iObj.GetBuffer());

    if (m_Model == nullptr)
    {
      m_Model = new ObjectModel(this, false, m_CustomizedObject);
    }
    else
    {
      m_Model->UpdateModel(m_CustomizedObject);
    }
    m_Root.children.clear();
    m_Customized.clear();
    BuildTree(QModelIndex(), nullptr);
    m_Customized.resize(m_Root.children.size(), false);

    emit endResetModel();
  }

  std::unique_ptr<CustomizationModel::TreeItem> CustomizationModel::BuildTree(QModelIndex iIndex, TreeItem* iParent)
  {
    TreeItem* newItem = iIndex.isValid() ? new TreeItem(iParent) : &m_Root;

    newItem->srcIndex = iIndex;

    int32_t numChildren = m_Model->rowCount(iIndex);
    for (int32_t i = 0; i < numChildren; ++i)
    {
      newItem->children.push_back(BuildTree(m_Model->index(i, 0, iIndex), newItem));
    }
    
    return std::unique_ptr<CustomizationModel::TreeItem>(iIndex.isValid() ? newItem : nullptr);
  }

  QModelIndex CustomizationModel::GetField(QModelIndex const& iParent, TypeFieldName iName)
  {
    QModelIndex src = SourceIndex(iParent);
    if (src.isValid())
    {
      return m_Model->GetField(src, iName);
    }
    return QModelIndex();
  }

  QModelIndex CustomizationModel::GetSlot(QModelIndex const& iParent, uint32_t iIndex)
  {
    QModelIndex src = SourceIndex(iParent);
    if (src.isValid())
    {
      return m_Model->GetSlot(src, iIndex);
    }
    return QModelIndex();
  }

  ConstDynObject const& CustomizationModel::GetObjectFromIndex(QModelIndex const& iIndex) const
  {
    QModelIndex src = SourceIndex(iIndex);
    return m_Model->GetObjectFromIndex(src);
  }

  void CustomizationModel::ApplyCustomization(uint32_t iIndex, ConstDynObject const& iFieldData)
  {
    if (iIndex < m_Customized.size())
    {
      //emit beginResetModel();
      emit beginRemoveRows(QModelIndex(), iIndex, iIndex);
      m_Root.children.erase(m_Root.children.begin() + iIndex);
      emit endRemoveRows();

      m_Model->ForceFieldValue(m_Model->index(iIndex, 0), iFieldData);

      emit beginInsertRows(QModelIndex(), iIndex, iIndex);
      m_Root.children.insert(m_Root.children.begin() + iIndex, BuildTree(m_Model->index(iIndex, 0), &m_Root));
      m_Customized[iIndex] = true;
      emit endInsertRows();
      //emit endResetModel();
    }
  }

  bool CustomizationModel::IsFieldCustomized(uint32_t iIndex) const
  {
    if (iIndex < m_Customized.size())
    {
      return m_Customized[iIndex];
    }

    return false;
  }

  QModelIndex CustomizationModel::index(int row, int column, const QModelIndex &iParent) const
  {
    if (!iParent.isValid())
    {
      if (row < m_Root.children.size() && (column == 0 || column == 1 || column == 2))
        return createIndex(row, column, (void*)&m_Root);
    }
    else if (iParent.isValid())
    {
      TreeItem const* parentNode = reinterpret_cast<TreeItem const*>(iParent.internalPointer());
      TreeItem const* node = parentNode->children[iParent.row()].get();

      if (row < node->children.size())
      {
        if (column == 0 || column == 1)
        {
          return createIndex(row, column, (void*)node);
        }
      }
    }
    return QModelIndex();
  }

  QModelIndex CustomizationModel::IndexOf(TreeItem* iNode) const
  {
    if(iNode)
    {
      if(iNode->parent != nullptr)
      {
        for(unsigned int i = 0; i< iNode->parent->children.size(); ++i)
        {
          if(iNode->parent->children[i].get() == iNode)
					{
            return createIndex(i,0,iNode->parent);
					}
        }
      }
    }
    return QModelIndex();
  }

  QModelIndex CustomizationModel::parent(const QModelIndex &child) const
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

  QVariant CustomizationModel::data(const QModelIndex &iIndex, int role) const
  {
    if (role == Qt::CheckStateRole)
    {
      if (iIndex.internalPointer() == &m_Root && iIndex.column() == 2)
      {
        return QVariant::fromValue(m_Customized[iIndex.row()] ? Qt::Checked : Qt::Unchecked);
      }
    }
    if (iIndex.column() < 2)
    {
      bool const isRootRow = iIndex.internalPointer() == &m_Root;

      QModelIndex srcIndex = SourceIndex(iIndex);
      int32_t column = /*isRootRow ? iIndex.column() - 1 :*/ iIndex.column();

      return m_Model->data(m_Model->index(srcIndex.row(), column, srcIndex.parent()), role);
    }
    return QVariant();
  }

  bool CustomizationModel::setData(const QModelIndex &iIndex, const QVariant &value, int role)
  {
    if (role == Qt::CheckStateRole
      && iIndex.internalPointer() == &m_Root 
      && iIndex.column() == 2)
    {
      auto checkState = value.value<Qt::CheckState>();
      if (checkState == Qt::Unchecked
        && m_Customized[iIndex.row()])
      {
        TupleType const* objType = TupleType::DynamicCast(m_DefaultObject.GetType());
        eXl_ASSERT_REPAIR_RET(objType != nullptr, false);

        uint32_t rowReset = iIndex.row();

        emit beginRemoveRows(QModelIndex(), rowReset, rowReset);
        //removeRowsRecurse(index(rowReset, 0, iIndex.parent()));
        m_Root.children.erase(m_Root.children.begin() + rowReset);
        emit endRemoveRows();

        Type const* fieldType = nullptr;
        void const* defaultField = objType->GetField(m_DefaultObject.GetBuffer(), iIndex.row(), fieldType);

        m_Model->ForceFieldValue(m_Model->index(rowReset, 0), ConstDynObject(fieldType, defaultField));

        emit beginInsertRows(QModelIndex(), rowReset, rowReset);
        m_Root.children.insert(m_Root.children.begin() + rowReset, BuildTree(m_Model->index(rowReset, 0), &m_Root));
        m_Customized[rowReset] = false;
        emit endInsertRows();

        //emit beginResetModel();
        //
        //Type const* fieldType = nullptr;
        //void const* defaultField = objType->GetField(m_DefaultObject.GetBuffer(), iIndex.row(), fieldType);
        //void* destField = objType->GetField(m_CustomizedObject.GetBuffer(), iIndex.row(), fieldType);
        //eXl_ASSERT_REPAIR_RET(fieldType != nullptr, false);
        //fieldType->Copy(defaultField, destField);
        //
        //m_Model->UpdateModel(m_CustomizedObject);
        ////m_Root.children.clear();
        ////BuildTree(QModelIndex(), nullptr);
        //  
        //m_Customized[iIndex.row()] = false;
        //
        //emit endResetModel();

        return true;
      }
      return false;
    }

    bool const isRootRow = iIndex.internalPointer() == &m_Root;

    QModelIndex srcIndex = SourceIndex(iIndex);
    int32_t column = /*isRootRow ? iIndex.column() - 1 :*/ iIndex.column();

    if (m_Model->setData(m_Model->index(srcIndex.row(), column, srcIndex.parent()), value, role))
    {
      emit dataChanged(iIndex, iIndex); 

      FieldModified(iIndex);

      return true;
    }

    return false;
  }

  void CustomizationModel::FieldModified(QModelIndex iIndex)
  {
    TreeItem const* parentNode = reinterpret_cast<TreeItem const*>(iIndex.internalPointer());
    TreeItem const* node = parentNode->children[iIndex.row()].get();
    while (parentNode != &m_Root)
    {
      node = parentNode;
      parentNode = node->parent;
    }

    uint32_t modifiedField = node->srcIndex.row();
    m_Customized[modifiedField] = true;

    QModelIndex checkBoxIndex = index(modifiedField, 0, QModelIndex());

    emit dataChanged(checkBoxIndex, checkBoxIndex);
  }

  bool CustomizationModel::insertRows(int row, int count, const QModelIndex &iParent)
  {
    if (count == 1)
    {
      if (IsValidField(iParent))
      {
        QModelIndex sourceParent = SourceIndex(iParent);
        if (m_Model->insertRows(row, count, sourceParent))
        {
          TreeItem* grandParentNode = reinterpret_cast<TreeItem*>(iParent.internalPointer());
          TreeItem* parentNode = grandParentNode->children[iParent.row()].get();
          beginInsertRows(iParent, row, row);
          parentNode->children.insert(parentNode->children.begin() + row, BuildTree(m_Model->index(row, 0, sourceParent), parentNode));
          endInsertRows();

          FieldModified(iParent);

        }
      }
    }
    return false;
  }

  void CustomizationModel::removeRowsRecurse(const QModelIndex &iParent)
  {
    if (iParent.isValid())
    {
      int numRows = rowCount(iParent);
      if (numRows > 0)
      {
        for (unsigned int i = 0; i < numRows; ++i)
        {
          QModelIndex childIndex = index(i, 0, iParent);
          removeRowsRecurse(childIndex);
          changePersistentIndex(childIndex, QModelIndex());
        }
        beginRemoveRows(iParent, 0, numRows - 1);
        TreeItem* parentNode = reinterpret_cast<TreeItem*>(iParent.internalPointer());
        TreeItem* node = parentNode->children[iParent.row()].get();
        node->children.clear();
        endRemoveRows();

      }
    }
  }

  bool CustomizationModel::removeRows(int row, int count, const QModelIndex &iParent)
  {
    if (count == 1)
    {
      if (IsValidField(iParent))
      {
        QModelIndex sourceParent = SourceIndex(iParent);
        if (m_Model->removeRows(row, count, sourceParent))
        {
          TreeItem* grandParentNode = reinterpret_cast<TreeItem*>(iParent.internalPointer());
          TreeItem* parentNode = grandParentNode->children[iParent.row()].get();
          beginRemoveRows(iParent, row, row);
          parentNode->children.erase(parentNode->children.begin() + row);
          endRemoveRows();

          FieldModified(iParent);
        }
      }
    }
    return false;
  }

  QVariant CustomizationModel::headerData(int section, Qt::Orientation orientation, int role) const
  {
    if (role == Qt::DisplayRole)
    {
      if (orientation == Qt::Horizontal)
      {
        if (section == 2)
        {
          return QString("Customized");
        }
        else if(section == 0)
        {
          return QString("Name");
        }
        else if(section == 1)
        {
          return QString("Value");
        }
      }
    }
    return QVariant();
  }
  bool CustomizationModel::IsValidField(QModelIndex iIndex) const
  {
    if (iIndex.isValid())
    {
      TreeItem const* parentNode = reinterpret_cast<TreeItem const*>(iIndex.internalPointer());
      return iIndex.column() < 2 /*(parentNode == &m_Root && iIndex.column() == 1)
        || (parentNode != &m_Root && iIndex.column() == 0)*/;
    }
    return false;
  }

  QModelIndex CustomizationModel::SourceIndex(QModelIndex iIndex) const
  {
    if (IsValidField(iIndex))
    {
      TreeItem const* parentNode = reinterpret_cast<TreeItem const*>(iIndex.internalPointer());
      TreeItem const* node = parentNode->children[iIndex.row()].get();
      return m_Model->index(node->srcIndex.row(), iIndex.column(), node->srcIndex.parent());
    }
    return QModelIndex();
  }

  Qt::ItemFlags CustomizationModel::flags(const QModelIndex &iIndex) const
  {
    if(iIndex.isValid())
    {
      if (IsValidField(iIndex))
      {
        return m_Model->flags(SourceIndex(iIndex));
      }
      else if(iIndex.internalPointer() == &m_Root
        && iIndex.column() == 2)
      {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
        return flags;
      }
    }
    return Qt::ItemFlag();
  }

  int CustomizationModel::rowCount(const QModelIndex &parent) const
  {
    if(!parent.isValid())
    {
      return m_Root.children.size();
    }
    else
    {
      if(IsValidField(parent))
      {
        TreeItem const* parentNode = reinterpret_cast<TreeItem const*>(parent.internalPointer());
        return parentNode->children[parent.row()]->children.size();
      }
    }
    return 0;
  }

  int CustomizationModel::columnCount(const QModelIndex &parent) const
  {
    return 3;
  }
}
