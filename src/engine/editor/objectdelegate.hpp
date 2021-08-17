#pragma once

#include <QStyledItemDelegate>

class DynObjectEditor;

namespace eXl
{
  class Type;
  class ConstDynObject;

  class ObjectDelegate : public QStyledItemDelegate
  {
    Q_OBJECT

  public:

    struct ObjectHandler
    {
      virtual QWidget* CreateEditor(QWidget* iParent) = 0;
      virtual void paint(QPainter* painter, const QStyleOptionViewItem &option, ConstDynObject const&) = 0;
    };

    static void RegisterType(ObjectHandler*, Type const* iType);

    ObjectDelegate(QWidget *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setEditorData(QWidget *editor, const QModelIndex &index) const override;

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index ) const override;

  private slots:
    void commitAndCloseEditor();
  };
}
