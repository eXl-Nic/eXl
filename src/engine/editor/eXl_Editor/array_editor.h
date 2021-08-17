#pragma once

#include <QWidget>

#include <editor/editordef.hpp>
#include <editor/dynobjecteditor.hpp>

namespace Ui
{
  class ArrayEditor;
}

class Array_Editor : public DynObjectEditor
{
  Q_OBJECT
public:
  Array_Editor(QWidget*);
  ~Array_Editor();

  void SetObject(eXl::ConstDynObject const* iObj) override
  {
    m_ConstObj = iObj;
    UpdateView();
  }

Q_SIGNALS:
  void onAddItem();
  void onEmptyArray();

protected:
  void UpdateView();

  Ui::ArrayEditor *ui;
  eXl::ConstDynObject const* m_ConstObj = nullptr;
};

