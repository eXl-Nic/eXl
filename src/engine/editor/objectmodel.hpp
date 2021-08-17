#pragma once

#include <QAbstractItemModel>
#include <vector>
#include <core/type/dynobject.hpp>

class QAbstractItemView;

namespace eXl
{
  class DynObject;

  class ObjectModel : public QAbstractItemModel
  {
    Q_OBJECT
  public:

    static void ClearModelFromView(QAbstractItemView* iView);
    static ObjectModel* CreateOrUpdateModel(QAbstractItemView* iView, bool iReadOnly, DynObject& iObj);

    ObjectModel(QObject* iParent, bool iReadOnly, DynObject& iObj);

    void UpdateModel(DynObject& iObj);

    virtual ConstDynObject const& GetObjectFromIndex(QModelIndex const& iIndex) const;
    virtual QModelIndex GetField(QModelIndex const& iParent, TypeFieldName iName);
    virtual QModelIndex GetSlot(QModelIndex const& iParent, uint32_t iIndex);
    void ForceFieldValue(QModelIndex iIndex, ConstDynObject const& iValue);

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

    struct ObjTreeView
    {
      ObjTreeView() : parentObj(NULL){}
      ObjTreeView(ObjTreeView const&) = delete;
      ObjTreeView& operator=(ObjTreeView const&) = delete;
      ObjTreeView* parent;
      DynObject object;
      ObjTreeView* parentObj;
      uint32_t field;
      std::vector<std::unique_ptr<ObjTreeView>> children;
    };

  protected:

    QModelIndex IndexOf(ObjTreeView* iNode);

    void removeRowsRecurse(const QModelIndex &parent);

    void UpdateObject(ObjTreeView* iNode, unsigned int iProperty);

    void BuildTree(ObjTreeView* iRoot, ObjTreeView* iCurrentObj);

    void RemapTree(ObjTreeView* iRoot);

    ObjTreeView m_TreeView;
    bool m_ReadOnly;
    //ContainersMap m_Map;
  };
}