#pragma once

#include <editor/objectdelegate.hpp>
#include "tilecollectionmodel.hpp"


class QSortFilterProxyModel;

namespace eXl
{
  class Tileset;
  class ObjectModel;

  class TileItemDelegate : public ObjectDelegate
  {
    Q_OBJECT

  public:

    TileItemDelegate(QWidget *parent);

    void Setup(ObjectModel *iModel, TileCollectionModel* iTileset, Type const* iParentType, uint32_t iFieldNum);
    void Clear();

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

  protected:

    bool IsOverridenField(QModelIndex iIndex) const;

    ObjectModel* m_Model = nullptr;
    TileCollectionModel* m_Tileset = nullptr;
    QSortFilterProxyModel* m_FilteredModel = nullptr;
    Type const* m_ParentType = nullptr;
    uint32_t m_FieldNum = 0;
  };
}
