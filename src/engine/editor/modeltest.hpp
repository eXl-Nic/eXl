#pragma once

#include <QAbstractItemModel>
#include <qstack.h>

class ModelTest : public QObject
{
  Q_OBJECT

public:
  ModelTest( QAbstractItemModel *model, QObject *parent = 0 );

private Q_SLOTS:
  void nonDestructiveBasicTest();
  void rowCount();
  void columnCount();
  void hasIndex();
  void index();
  void parent();
  void data();

protected Q_SLOTS:
  void runAllTests();
  void layoutAboutToBeChanged();
  void layoutChanged();
  void rowsAboutToBeInserted( const QModelIndex &parent, int start, int end );
  void rowsInserted( const QModelIndex & parent, int start, int end );
  void rowsAboutToBeRemoved( const QModelIndex &parent, int start, int end );
  void rowsRemoved( const QModelIndex & parent, int start, int end );

private:
  void checkChildren( const QModelIndex &parent, int currentDepth = 0 );

  QAbstractItemModel *model;

  struct Changing {
    QModelIndex parent;
    int oldSize;
    QVariant last;
    QVariant next;
  };
  QStack<Changing> insert;
  QStack<Changing> remove;

  bool fetchingMore;

  QList<QPersistentModelIndex> changing;
};
