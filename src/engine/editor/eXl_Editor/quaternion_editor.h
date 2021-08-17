#pragma once

#include "../dynobjecteditor.hpp"

namespace Ui
{
class QuatEditor;
}

class Quaternion_Editor : public DynObjectEditor
{
  Q_OBJECT
public:
  static DynObjectEditor* Construct(QWidget *parent)
  {return new Quaternion_Editor(parent);}

  Quaternion_Editor(QWidget *parent);

protected slots:
  void OnEditingFinished();

protected:

  virtual void UpdateView();

  Ui::QuatEditor* ui;
};

