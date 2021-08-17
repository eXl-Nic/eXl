#pragma once

#include <QWidget>

#include <editor/editordef.hpp>

namespace Ui
{
  class ArrayItemEditor;
}

class ArrayItem_Editor : public QWidget
{
  Q_OBJECT
public:
  ArrayItem_Editor(uint32_t iItemIndex, QWidget*);
  ~ArrayItem_Editor();

Q_SIGNALS:
  void onRemoveItem();
  void onInsertItem();

protected:

  Ui::ArrayItemEditor *ui;
};