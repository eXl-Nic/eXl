#pragma once

#include <QAbstractItemModel>
#include <vector>
#include <core/type/dynobject.hpp>

#include "objectmodel.hpp"

class QAbstractItemView;

namespace eXl
{
  class DynObject;
  class ObjectModel;

  class CustomizationModel : public ObjectModel
  {
    Q_OBJECT
  public:

    static void ClearModelFromView(QAbstractItemView* iView);
    static CustomizationModel* CreateOrUpdateModel(QAbstractItemView* iView, ConstDynObject const& iObj);

    CustomizationModel(QObject* iParent, ConstDynObject const& iObj);

    void UpdateModel(ConstDynObject const& iObj);
    ConstDynObject const& GetObjectFromIndex(QModelIndex const& iIndex) const override;
    QModelIndex GetField(QModelIndex const& iParent, TypeFieldName iName) override;
    QModelIndex GetSlot(QModelIndex const& iParent, uint32_t iIndex) override;

    void ApplyCustomization(uint32_t iIndex, ConstDynObject const& iFieldData);
    bool IsFieldCustomized(uint32_t iIndex) const;

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

    struct TreeItem
    {
      TreeItem(TreeItem* iParent) : parent(iParent)
      {}
      TreeItem(TreeItem const& ) = delete;
      TreeItem& operator=(TreeItem const&) = delete;
      TreeItem* parent;
      QModelIndex srcIndex;
      Vector<std::unique_ptr<TreeItem>> children;
    };

    QModelIndex IndexOf(TreeItem* iNode) const;
    QModelIndex SourceIndex(QModelIndex iIndex) const;

    void FieldModified(QModelIndex iIndex);

    void removeRowsRecurse(const QModelIndex &iParent);

    std::unique_ptr<TreeItem> BuildTree(QModelIndex iIndex, TreeItem* iParent);

    bool IsValidField(QModelIndex iIndex) const;

    ObjectModel* m_Model = nullptr;
    // Non owning
    ConstDynObject m_DefaultObject;
    // Owning
    DynObject m_CustomizedObject;
    TreeItem m_Root;
    Vector<bool> m_Customized;
  };
}