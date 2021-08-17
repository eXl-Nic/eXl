#pragma once

#include "../dynobjecteditor.hpp"

class QComboBox;

class ResourceHandle_Editor : public DynObjectEditor
{
  Q_OBJECT
public:
  static DynObjectEditor* Construct(QWidget *parent)
  {return new ResourceHandle_Editor(parent);}

  ResourceHandle_Editor(QWidget *parent);

protected slots:
  void OnEditingFinished();

protected:

  void UpdateView();

  bool m_Integer;
  QComboBox* m_ResourceCombo;

};
