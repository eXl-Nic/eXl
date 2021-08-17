#pragma once
#include <QWidget>

#include "editordef.hpp"
#include <core/type/dynobject.hpp>

class DynObjectEditor : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(eXl::ConstDynObject const* eXlObject READ Object WRITE SetObject USER true)
public:
  DynObjectEditor(QWidget*);

  inline eXl::ConstDynObject const* Object() const { return &m_Obj; }

  virtual void SetObject(eXl::ConstDynObject const* iObj)
  {
    m_Obj = eXl::DynObject(iObj);
    UpdateView();
  }

Q_SIGNALS:
  void editingFinished();

protected:

  virtual void UpdateView() = 0;

  eXl::DynObject m_Obj;
};

