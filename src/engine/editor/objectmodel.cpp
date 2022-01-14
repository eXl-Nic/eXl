#include "objectmodel.hpp"
#include "editordef.hpp"

#include <core/type/tupletype.hpp>
#include <core/type/arraytype.hpp>
//#include <core/type/objectptrtype.hpp>
#include <core/type/classtype.hpp>
//#include <core/type/typemanager.hpp>

#include <QAbstractItemView>

namespace eXl
{
  inline bool IsLeafType(Type const* iType)
  {
    return iType->IsTuple() == nullptr && ArrayType::DynamicCast(iType) == nullptr;
  }

  inline bool IsArrayType(Type const* iType)
  {
    return ArrayType::DynamicCast(iType) != nullptr;
  }

  void ObjectModel::ClearModelFromView(QAbstractItemView* iView)
  {
    ObjectModel* model = qobject_cast<ObjectModel*>(iView->model());
    if (QAbstractItemDelegate* delegate = iView->itemDelegate())
    {
      iView->setItemDelegate(nullptr);
      delete delegate;
    }
    if (model)
    {
      iView->setModel(nullptr);
      delete model;
    }
  }

  ObjectModel* ObjectModel::CreateOrUpdateModel(QAbstractItemView* iView, bool iReadOnly, DynObject& iObj)
  {
    ObjectModel* model = qobject_cast<ObjectModel*>(iView->model());
    if (iView->model() == nullptr)
    {
      model = new ObjectModel(iView, false, iObj);
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

  ObjectModel::ObjectModel(QObject* iParent, bool iReadOnly, DynObject& iObj)
    : QAbstractItemModel(iParent)
    , m_ReadOnly(iReadOnly)
  {
    UpdateModel(iObj);
  }

  void ObjectModel::UpdateModel(DynObject& iObj)
  {
    //emit layoutAboutToBeChanged();
    emit beginResetModel();

    m_TreeView.children.clear();

    m_TreeView.parent = nullptr;
    m_TreeView.object = iObj.Ref();

    BuildTree(&m_TreeView, &m_TreeView);
    emit endResetModel();
    //emit layoutChanged();
  }

  void ObjectModel::RemapTree(ObjTreeView* iRoot)
  {
    Type const* type = iRoot->object.GetType();
    TupleType const* tupleType = type->IsTuple();
    if(tupleType != nullptr)
    {
      unsigned int numFields = tupleType->GetNumField();
      for(unsigned int i = 0; i< numFields; ++i)
      {
        iRoot->object.GetField(i,iRoot->children[i]->object);
        RemapTree(iRoot->children[i].get());
      }
    }
    else
    {
      ArrayType const* arrayType = ArrayType::DynamicCast(iRoot->object.GetType());
      if(arrayType!= nullptr )
      {
        unsigned int numFields = arrayType->GetArraySize(iRoot->object.GetBuffer());
        for(unsigned int i = 0; i< numFields; ++i)
        {
          iRoot->object.GetElement(i,iRoot->children[i]->object);
          RemapTree(iRoot->children[i].get());
        }
      }
      //else
      //{
      //  ObjectPtrType const* objPtrType = ObjectPtrType::DynamicCast(iRoot->object.GetType());
      //  if(objPtrType != nullptr)
      //  {
      //    ClassType const* classType = objPtrType->GetObjectType(iRoot->object.GetBuffer());
      //    if(classType != nullptr)
      //    {
      //      unsigned int numFields = classType->GetNumProps();
      //      for(unsigned int i = 0; i< numFields; ++i)
      //      {
      //        Type const* propType = classType->GetPropertyType(i);
      //        iRoot->children[i]->object.SetType(propType,propType->Build(),true);
      //        RttiObject* obj = iRoot->object.CastBuffer<RttiOPtr>()->get();
      //        classType->GetProperty(i,obj,iRoot->children[i]->object.GetBuffer());
      //        RemapTree(iRoot->children[i]);
      //      }
      //    }
      //  }
      //}
    }
  }

  void ObjectModel::BuildTree(ObjTreeView* iRoot, ObjTreeView* iParentObj)
  {
    Type const* type = iRoot->object.GetType();

    if (type == nullptr)
    {
      return;
    }

    TupleType const* tupleType = type->IsTuple();
    if(tupleType != nullptr)
    {
      unsigned int numFields = tupleType->GetNumField();
      for(unsigned int i = 0; i< numFields; ++i)
      {
        iRoot->children.push_back(std::make_unique<ObjTreeView>());
        iRoot->children.back()->parent = iRoot;
        iRoot->children.back()->parentObj = iParentObj;
        iRoot->children.back()->field = i;
        iRoot->object.GetField(i,iRoot->children.back()->object);
        BuildTree(iRoot->children.back().get(), iParentObj);
      }
    }
    else
    {
      eXl::ArrayType const* arrayType = ArrayType::DynamicCast(iRoot->object.GetType());
      if(arrayType!= nullptr )
      {
        unsigned int numFields = arrayType->GetArraySize(iRoot->object.GetBuffer());
        for(unsigned int i = 0; i< numFields; ++i)
        {
          iRoot->children.push_back(std::make_unique<ObjTreeView>());
          iRoot->children.back()->parent = iRoot;
          iRoot->children.back()->parentObj = iParentObj;
          iRoot->children.back()->field = i;
          iRoot->object.GetElement(i,iRoot->children.back()->object);
          BuildTree(iRoot->children.back().get(), iParentObj);
        }
      }
      //else
      //{
      //  ObjectPtrType const* objPtrType = ObjectPtrType::DynamicCast(iRoot->object.GetType());
      //  if(objPtrType != nullptr)
      //  {
      //    ClassType const* classType = objPtrType->GetObjectType(iRoot->object.GetBuffer());
      //    if(classType != nullptr)
      //    {
      //      unsigned int numFields = classType->GetNumProps();
      //      for(unsigned int i = 0; i< numFields; ++i)
      //      {
      //        Type const* propType = classType->GetPropertyType(i);
      //        iRoot->children.push_back(new ObjTreeView());
      //        iRoot->children.back()->parent = iRoot;
      //        iRoot->children.back()->object.SetType(propType,propType->Build(),true);
      //        iRoot->children.back()->parentObj = iRoot;
      //        iRoot->children.back()->property = i;
      //        RttiObject* obj = iRoot->object.CastBuffer<RttiOPtr>()->get();
      //        classType->GetProperty(i,obj,iRoot->children.back()->object.GetBuffer());
      //        BuildTree(iRoot->children.back(), iRoot, i);
      //      }
      //    }
      //  }
      //}
    }
  }

  void ObjectModel::ForceFieldValue(QModelIndex iIndex, ConstDynObject const& iValue)
  {
    if (!iIndex.isValid())
    {
      return;
    }

    ObjTreeView* parentNode = reinterpret_cast<ObjTreeView*>(iIndex.internalPointer());
    ObjTreeView* node = parentNode->children[iIndex.row()].get();

    DynObject fieldToUpdate = node->object.Ref();

    eXl_ASSERT_REPAIR_RET(fieldToUpdate.GetType()->CanAssignFrom(iValue.GetType()), );

    uint32_t rowToUpdate = iIndex.row();

    beginRemoveRows(iIndex.parent(), rowToUpdate, rowToUpdate);

    //removeRowsRecurse(iIndex);

    parentNode->children.erase(parentNode->children.begin() + rowToUpdate);
    node = nullptr;
    endRemoveRows();

    fieldToUpdate.GetType()->Assign(iValue.GetType(), iValue.GetBuffer(), fieldToUpdate.GetBuffer());
    
    beginInsertRows(iIndex.parent(), rowToUpdate, rowToUpdate);

    std::unique_ptr<ObjTreeView> newObj = std::make_unique<ObjTreeView>();
    newObj->field = rowToUpdate;
    newObj->parent = parentNode;
    newObj->parentObj = parentNode;
    newObj->object = fieldToUpdate.Ref();
    node = newObj.get();
    parentNode->children.insert(parentNode->children.begin() + rowToUpdate, std::move(newObj));
    BuildTree(node, parentNode->parentObj);
    endInsertRows();
  }

  QModelIndex ObjectModel::GetField(QModelIndex const& iParent, TypeFieldName iName)
  {
    TupleType const* parentType = nullptr;
    if (!iParent.isValid())
    {
      parentType = TupleType::DynamicCast(m_TreeView.object.GetType());
    }
    else if (iParent.isValid())
    {
      ObjTreeView const* parentNode = reinterpret_cast<ObjTreeView const*>(iParent.internalPointer());
      ObjTreeView const* node = parentNode->children[iParent.row()].get();

      parentType = TupleType::DynamicCast(node->object.GetType());
    }

    if (parentType == nullptr)
    {
      return QModelIndex();
    }
    uint32_t fieldIdx;
    if (parentType->GetFieldDetails(iName, fieldIdx) == nullptr)
    {
      return QModelIndex();
    }

    return index(fieldIdx, 0, iParent);
  }

  QModelIndex ObjectModel::GetSlot(QModelIndex const& iParent, uint32_t iIndex)
  {
    ArrayType const* parentType = nullptr;
    ConstDynObject const* arrayObj = nullptr;
    if (!iParent.isValid())
    {
      parentType = ArrayType::DynamicCast(m_TreeView.object.GetType());
      if (parentType != nullptr)
      {
        arrayObj = &m_TreeView.object;
      }
    }
    else if (iParent.isValid())
    {
      ObjTreeView const* parentNode = reinterpret_cast<ObjTreeView const*>(iParent.internalPointer());
      ObjTreeView const* node = parentNode->children[iParent.row()].get();

      parentType = ArrayType::DynamicCast(node->object.GetType());
      if (parentType != nullptr)
      {
        arrayObj = &node->object;
      }
    }

    if (arrayObj == nullptr)
    {
      return QModelIndex();
    }
    ConstDynObject element;
    if (!arrayObj->GetElement(iIndex, element))
    {
      return QModelIndex();
    }

    return index(iIndex, 0, iParent);
  }

  ConstDynObject const& ObjectModel::GetObjectFromIndex(QModelIndex const& iIndex) const
  {
    if(iIndex.isValid())
    {
      ObjTreeView const* parentNode = reinterpret_cast<ObjTreeView const*>(iIndex.internalPointer());
      ObjTreeView const* node = parentNode->children[iIndex.row()].get();
      return node->object;
    }
    else
      return m_TreeView.object;
  }

  QModelIndex ObjectModel::index(int row, int column, const QModelIndex &parent) const
  {
    if(!parent.isValid())
    {
      if(row < m_TreeView.children.size() && ( column == 0 || column == 1 ))
        return createIndex(row,column,(void*)&m_TreeView);
    }
    else if(parent.isValid())
    {
      ObjTreeView const* grandParentNode = reinterpret_cast<ObjTreeView const*>(parent.internalPointer());
      ObjTreeView const* parentNode = grandParentNode->children[parent.row()].get();

      if(row < parentNode->children.size())
      {
        if(column == 0 || column == 1)
				{
          return createIndex(row,column,(void*)parentNode);
				}
      }
    }
    return QModelIndex();
  }

  QModelIndex ObjectModel::IndexOf(ObjTreeView* iNode)
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

  QModelIndex ObjectModel::parent(const QModelIndex &child) const
  {
    if(child.isValid())
    {
      ObjTreeView const* parentNode = reinterpret_cast<ObjTreeView const*>(child.internalPointer());

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

  QVariant ObjectModel::data(const QModelIndex &index, int role) const
  {
    if (!index.isValid())
    {
      return QVariant();
    }

    if (role == Qt::CheckStateRole)
    {
      if (index.column() == 1)
      {
        ConstDynObject const& obj = GetObjectFromIndex(index);
        eXl_ASSERT_MSG(obj.IsValid(), "Could not get object");
        Type const* fieldType = obj.GetType();
        if (obj.GetType() == TypeManager::GetType<bool>())
        {
          return QVariant::fromValue(*obj.CastBuffer<bool>() ? Qt::Checked : Qt::Unchecked);
        }
      }
    }
    if(role == Qt::DisplayRole)
    {
      ObjTreeView const* parentNode = reinterpret_cast<ObjTreeView const*>(index.internalPointer());
      ObjTreeView const* node = parentNode->children[index.row()].get();
      Type const* parentType = parentNode->object.GetType();
      
      if(index.column() == 0)
      {
        TupleType const* tupleType = parentType->IsTuple();
        if(tupleType)
        {
          String fieldName;
          tupleType->GetFieldDetails(index.row(),fieldName);
          return QString(fieldName.c_str());
        }
        else
        {
          ArrayType const* arrayType = ArrayType::DynamicCast(parentType);
          if(arrayType != nullptr)
          {
            return QString("%1").arg(index.row());
          }
          //else
          //{
          //  ObjectPtrType const* objPtrType = ObjectPtrType::DynamicCast(rootType);
          //  if(objPtrType != nullptr)
          //  {
          //    //RttiObject* obj = parentNode->object.ConstDynObject::CastBuffer<RttiOPtr>()->get();
          //    ClassType const* classType = objPtrType->GetObjectType(parentNode->object.GetBuffer());
          //    if(classType != nullptr)
          //    {
          //      std::string const* propName = classType->GetPropertyName(index.row());
          //      return QString(propName->c_str());
          //    }
          //  }
          //}
        }
      }
      else if(index.column() == 1)
      {
        ConstDynObject const& object = node->object;
        
        Type const* fieldType;
        TupleType const* tupleType = parentType->IsTuple();
        if(tupleType)
        {
          fieldType = tupleType->GetFieldDetails(index.row());
        }
        else
        {
          ArrayType const* arrayType = ArrayType::DynamicCast(parentType);
          if(arrayType != nullptr)
          {
            fieldType = arrayType->GetElementType();
          }
          //else
          //{
          //  ObjectPtrType const* objPtrType = ObjectPtrType::DynamicCast(rootType);
          //  if(objPtrType != nullptr)
          //  {
          //    //RttiObject* obj = parentNode->object.ConstDynObject::CastBuffer<RttiOPtr>()->get();
          //    ClassType const* classType = objPtrType->GetObjectType(parentNode->object.GetBuffer());
          //    if(classType != nullptr)
          //    {
          //      fieldType = classType->GetPropertyType(index.row());
          //    }
          //  }
          //}
        }
        if(IsLeafType(fieldType))
        {
          QVariant variantData;
          DynObjToQVariant(object,variantData);
          if(!variantData.isValid())
          {
            return qVariantFromValue(&object);
          }
          else
          {
            return variantData;
          }
        }
        else if(IsArrayType(fieldType))
        {
          ArrayType const* arrayType = static_cast<ArrayType const*>(fieldType);
          ConstDynObject const& obj = GetObjectFromIndex(index);
          unsigned int size = arrayType->GetArraySize(obj.GetBuffer());
          return size;
        }
        //else
        //{
        //  ObjectPtrType const* objPtrType = ObjectPtrType::DynamicCast(fieldType);
        //  if(objPtrType != nullptr)
        //  {
        //    //RttiObject* obj = object->CastBuffer<RttiOPtr>()->get();
        //    ClassType const* classType = objPtrType->GetObjectType(object->GetBuffer());
        //    if(classType != nullptr)
        //    {
        //      return QString(classType->GetClassRtti().GetName().c_str());
        //    }
        //    else
        //    {
        //      return QString("nullptr");
        //    }
        //  }
        //}
      }
    }
    else if(role == Qt::EditRole)
    {
      if (index.column() == 1)
      {
        ConstDynObject const& obj = GetObjectFromIndex(index);
        eXl_ASSERT_MSG(obj.IsValid(), "Could not get object");
        Type const* fieldType = obj.GetType();
        if (IsLeafType(fieldType) || IsArrayType(fieldType))
        {
          return QVariant::fromValue(&obj);
        }
      }
      else
      {
        QModelIndex parent = index.parent();
        if (index.column() == 0 && parent.isValid())
        {
          ConstDynObject const& object = this->GetObjectFromIndex(parent);
          if (IsArrayType(object.GetType()))
          {
            ArrayItem item;
            item.index = index.row();

            return QVariant::fromValue(item);
          }
        }
      }
      //else if(IsArrayType(fieldType))
      //{
      //  //ArrayType const* arrayType = static_cast<ArrayType const*>(fieldType);
      //  //unsigned int size = arrayType->GetArraySize(obj.GetBuffer());
      //  //return size;
      //}
      //else
      //{
      //  ObjectPtrType const* objPtrType = ObjectPtrType::DynamicCast(fieldType);
      //  if(objPtrType != nullptr)
      //  {
      //    return qVariantFromValue(&obj);
      //  }
      //}
    }
    return QVariant();
  }

  bool ObjectModel::setData(const QModelIndex &iIndex, const QVariant &value, int role)
  {
    if (!iIndex.isValid() || iIndex.column() != 1)
    {
      return false;
    }

    ObjTreeView const* parentNode = reinterpret_cast<ObjTreeView const*>(iIndex.internalPointer());
    ObjTreeView* node = parentNode->children[iIndex.row()].get();
    
    DynObject* object = &node->object;
    Type const* fieldType = object->GetType();

    if (role == Qt::CheckStateRole)
    {
      if (fieldType == TypeManager::GetType<bool>())
      {
        auto checkState = value.value<Qt::CheckState>();

        *object->CastBuffer<bool>() = checkState == Qt::Checked ? true : false;

        emit dataChanged(iIndex, iIndex);
        return true;
      }
    }

    if(role == Qt::EditRole)
    {
      if(IsLeafType(fieldType))
      {
        DynObjFromQVariant(*object, value);
        //if(node->parentObj != nullptr)
        //{
        //  UpdateObject(node->parentObj,node->field);
        //}
        emit dataChanged(iIndex,iIndex);
        return true;
      }
      else if(IsArrayType(fieldType))
      {
        ArrayType const* arrayType = static_cast<ArrayType const*>(fieldType);
        unsigned int oldSize = arrayType->GetArraySize(object->GetBuffer());
        unsigned int size = value.toUInt();
        if(oldSize == size)
          return true;

        if(size > oldSize)
        {
          insertRows(oldSize,size - oldSize,iIndex);
        }
        else if(size < oldSize)
        {
          removeRows(size, oldSize - size, iIndex);
        }
        //if(node->parentObj != nullptr)
        //{
        //  UpdateObject(node->parentObj,node->field);
        //}
        emit dataChanged(iIndex,iIndex);

        return true;
      }
      //else
      //{
      //  ObjectPtrType const* objPtrType = ObjectPtrType::DynamicCast(fieldType);
      //  if(objPtrType != nullptr)
      //  {
      //    DynObject tempObj;
      //    DynObjFromQVariant(tempObj, value);
			//
      //    RttiObjectRefC* otherObj = tempObj.CastBuffer<RttiOPtr>()->get();
      //    Rtti const* newRttiVal = otherObj == nullptr ? nullptr : &otherObj->GetRtti();
			//
      //    ClassType const* classType = objPtrType->GetObjectType(object->GetBuffer());
      //    Rtti const* oldRttiVal = classType != nullptr ? &classType->GetClassRtti() : nullptr;
      //    if(newRttiVal != oldRttiVal)
      //    {
      //      removeRowsRecurse(iIndex);
      //      if(newRttiVal != nullptr)
      //      {
      //        //ClassType const* newClassType = TypeManager::GetClassForRtti(*newRttiVal);
      //        ClassType const* newClassType = otherObj->GetClassType();
      //        //if(newClassType && !newClassType->IsAbstract())
      //        {
      //          RttiObject* newObj = nullptr;
      //          //ErrCode err = newClassType->Build((void*&)newObj,nullptr,nullptr);
      //          //if(EC_SUCCESS(err))
      //          {
      //            *object->CastBuffer<RttiOPtr>() = otherObj;
      //            beginInsertRows(iIndex, 0, newClassType->GetNumProps() - 1);
      //            BuildTree(parentNode->children[iIndex.row()],node->parentObj,node->property);
      //            endInsertRows();
      //            emit layoutChanged();
      //          }
      //        }
      //      }
      //      else
      //      {
      //        *object->CastBuffer<RttiOPtr>() = nullptr;
      //      }
      //    }
      //    if(node->parentObj != nullptr)
      //    {
      //      UpdateObject(node->parentObj,node->property);
      //    }
      //    emit dataChanged(iIndex,iIndex);
      //    return true;
      //  }
      //}
    }
    return false;
  }

  void ObjectModel::UpdateObject(ObjTreeView* iNode, unsigned int iProperty)
  {
    //if(iNode != nullptr && IsObjPtrType(iNode->object.GetType()))
    //{
    //  ObjectPtrType const* objPtrType = static_cast<ObjectPtrType const*>(iNode->object.GetType());
    //  ClassType const* classType = objPtrType->GetObjectType(iNode->object.GetBuffer());
    //  if(classType != nullptr)
    //  {
    //    RttiObject* obj = iNode->object.CastBuffer<RttiOPtr>()->get();
    //    classType->SetProperty(iProperty,obj,iNode->children[iProperty]->object.GetBuffer());
    //  }
    //}
  }

  bool ObjectModel::insertRows(int row, int count, const QModelIndex &iParent)
  {
    if(iParent.isValid())
    {
      if(count == 0)
        return true;

      ObjTreeView* parentNode = reinterpret_cast<ObjTreeView*>(iParent.internalPointer());
      ObjTreeView* node = parentNode->children[iParent.row()].get();
      
      DynObject* object = &node->object;
      Type const* rootType;
      rootType = object->GetType();
      
      if(IsArrayType(rootType))
      {
        ArrayType const* arrayType = static_cast<ArrayType const*>(rootType);
        unsigned int oldSize = arrayType->GetArraySize(object->GetBuffer());
        if(row <= oldSize)
        {
          unsigned int newSize = oldSize + count;
          unsigned int toMove = (oldSize - row);

          beginInsertRows(iParent,row,row + count - 1);
          arrayType->SetArraySize(object->GetBuffer(),newSize);
          for(unsigned int i = 0; i < row; ++i)
          {
            object->GetElement(i,node->children[i]->object);
            RemapTree(node->children[i].get());
          }
          for (unsigned int i = row; i < row + count; ++i)
          {
            std::unique_ptr<ObjTreeView> newChild = std::make_unique<ObjTreeView>();
            newChild->parent = node;
            newChild->parentObj = node->parentObj;
            newChild->field = i;
            Err err = object->GetElement(i, newChild->object);
            eXl_ASSERT_MSG(!(!err), "Could not get array obj");
            node->children.emplace(node->children.begin() + i, std::move(newChild));
          }
          for (int i = newSize - 1; i >= row + count; --i)
          {
            unsigned int origIdx = i - count;
            unsigned int destIdx = i;

            DynObject& orig = node->children[origIdx]->object;
            DynObject& dest = node->children[destIdx]->object;
            object->DynObject::GetElement(origIdx, orig);
            object->GetElement(destIdx, dest);
            arrayType->GetElementType()->Assign(arrayType->GetElementType(), orig.GetBuffer(), dest.GetBuffer());
            arrayType->GetElementType()->Destruct(orig.GetBuffer());
            arrayType->GetElementType()->Construct(orig.GetBuffer());
            RemapTree(node->children[destIdx].get());
          }
          for(unsigned int i = row; i < row + count; ++i)
          {
            BuildTree(node->children[i].get(), node->parentObj);
          }

          endInsertRows();
          emit layoutChanged();
          return true;
        }
      }
    }
    return false;
  }

  void ObjectModel::removeRowsRecurse(const QModelIndex &iParent)
  {
    if(iParent.isValid())
    {
      int numRows = rowCount(iParent);
      if(numRows > 0)
      {
        for(unsigned int i = 0; i< numRows; ++i)
        {
          QModelIndex childIndex = index(i,0,iParent);
          removeRowsRecurse(childIndex);
          changePersistentIndex(childIndex,QModelIndex());
        }
        beginRemoveRows(iParent, 0, numRows - 1);
        ObjTreeView* parentNode = reinterpret_cast<ObjTreeView*>(iParent.internalPointer());
        ObjTreeView* node = parentNode->children[iParent.row()].get();
        node->children.clear();
        endRemoveRows();
      }
    }
  }

  bool ObjectModel::removeRows(int row, int count, const QModelIndex &iParent)
  {
    if(iParent.isValid())
    {
      if(count == 0)
        return true;

      ObjTreeView* parentNode = reinterpret_cast<ObjTreeView*>(iParent.internalPointer());
      ObjTreeView* node = parentNode->children[iParent.row()].get();
      DynObject* object = &node->object;
      Type const* rootType;
      rootType = object->GetType();
      if(IsArrayType(rootType))
      {
        ArrayType const* arrayType = static_cast<ArrayType const*>(rootType);
        unsigned int oldSize = arrayType->GetArraySize(object->GetBuffer());
        if(row < oldSize)
        {
          unsigned int newSize = oldSize - count;
          
          for(unsigned int i = 0; i<count; ++i)
          {
            QModelIndex childIndex = index(row + i, 0, iParent);
            //removeRowsRecurse(childIndex);
            changePersistentIndex(childIndex,QModelIndex());
          }

          beginRemoveRows(iParent,row,row + count - 1);

          unsigned int toMove = (oldSize - (row + count));
          
          for(unsigned int i = row + count; i < oldSize; ++i)
          {
            DynObject dest;
            DynObject orig;
            object->DynObject::GetElement(i,orig);
            object->GetElement(i - count,dest);
            arrayType->GetElementType()->Assign(arrayType->GetElementType(),orig.GetBuffer(),dest.GetBuffer());
          }

          for(unsigned int i = row; i < row+count; ++i)
          {
            node->children.erase(node->children.begin() + row);
          }
          arrayType->SetArraySize(object->GetBuffer(),newSize);

          for(unsigned int i = 0; i < newSize; ++i)
          {
            Err err = object->GetElement(i,node->children[i]->object);
            eXl_ASSERT_MSG(!(!err),"Could not get array obj");
            RemapTree(node->children[i].get());
          }

          endRemoveRows();
          emit layoutChanged();
          return true;
        }
      }
    }
    return false;
  }

  QVariant ObjectModel::headerData(int section, Qt::Orientation orientation, int role) const
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
      }
    }
    return QVariant();
  }

  Qt::ItemFlags ObjectModel::flags(const QModelIndex &index) const
  {
    if(index.isValid())
    {
      Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
      ConstDynObject const& obj = GetObjectFromIndex(index);

      if((IsLeafType(obj.GetType()) || IsArrayType(obj.GetType())) && index.column() == 1 && !m_ReadOnly)
      {
        if (obj.GetType() == TypeManager::GetType<bool>())
        {
          flags |= Qt::ItemIsUserCheckable;
        }
        else
        {
          flags |= Qt::ItemIsEditable;
        }
      }
      return flags;
    }
    return Qt::ItemFlag();
  }

  int ObjectModel::rowCount(const QModelIndex &parent) const
  {
    if(!parent.isValid())
    {
      return m_TreeView.children.size();
    }
    else
    {
      if(parent.column() == 0)
      {
        ObjTreeView const* parentNode = reinterpret_cast<ObjTreeView const*>(parent.internalPointer());  
        return parentNode->children[parent.row()]->children.size();
      }
    }
    return 0;
  }

  int ObjectModel::columnCount(const QModelIndex &parent) const
  {
    return 2 ;
  }
}
