#pragma once

#include "typedecleditor.hpp"

class QComboBox;

namespace eXl
{
  class FunDeclEditor : public TypeDeclEditor
  {
    Q_OBJECT
  public:

    static FunDeclEditor* Create(QWidget* iParent, TypeName const& iRetType);

    TypeName GetRetType() const;
    void SetRetType(TypeName);

  Q_SIGNALS:
    void OnRetTypeChanged();

  protected:

    FunDeclEditor(QWidget* iParent);
    void Build(QLayout* iLayout, TypeName const& iRetType);

    QComboBox* m_RetTypeSelector;
  };
}