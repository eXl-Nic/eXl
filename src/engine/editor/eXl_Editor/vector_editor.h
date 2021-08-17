#pragma once

#include "../dynobjecteditor.hpp"

class QAbstractSpinBox;

class Vector_Editor : public DynObjectEditor
{
  Q_OBJECT
public:
  static DynObjectEditor* Construct(QWidget *parent)
  {return new Vector_Editor(parent);}

  Vector_Editor(QWidget *parent);

protected slots:
  void OnEditingFinished();

protected:

  void UpdateView();

  bool m_Integer;
  QAbstractSpinBox* m_Boxes[4];

};
