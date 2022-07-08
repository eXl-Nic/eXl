#include "fundecleditor.hpp"

#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

namespace eXl
{

  FunDeclEditor::FunDeclEditor(QWidget* iParent)
    : TypeDeclEditor(iParent)
    , m_RetTypeSelector(nullptr)
  {
  }

  TypeName FunDeclEditor::GetRetType() const
  {
    if (m_RetTypeSelector->currentIndex() == 0)
    {
      return "void";
    }
    else
    {
      return TypeName(m_TypeNames[m_RetTypeSelector->currentIndex() - 1].toStdString());
    }
  }

  void FunDeclEditor::SetRetType(TypeName iName)
  {
    int idx = m_TypeNames.indexOf(QString(iName.c_str()));
    if (idx < 0)
    {
      m_RetTypeSelector->setCurrentIndex(0);
    }
    else
    {
      m_RetTypeSelector->setCurrentIndex(idx + 1);
    }
  }

  FunDeclEditor* FunDeclEditor::Create(QWidget* iParent, TypeName const& iType)
  {
    FunDeclEditor* editor = new FunDeclEditor(iParent);
    QVBoxLayout* propDataLayout = new QVBoxLayout(editor);
    editor->setLayout(propDataLayout);
    editor->Build(propDataLayout, iType);

    return editor;
  }

  void FunDeclEditor::Build(QLayout* iLayout, TypeName const& iType)
  {
    {
      QWidget* retWidget = new QWidget(this);
      QHBoxLayout* retLayout = new QHBoxLayout(retWidget);
      retWidget->setLayout(retLayout);
      iLayout->addWidget(retWidget);

      retLayout->addWidget(new QLabel("Return type : "));
      m_RetTypeSelector = new QComboBox(retWidget);
      int selIndex = 0;
      m_RetTypeSelector->addItem("void");
      for (int32_t i = 0; i < m_TypeNames.size(); ++i)
      {
        m_RetTypeSelector->addItem(m_TypeDisplayNames[i]);
        if (m_TypeNames[i].toStdString() == iType)
        {
          selIndex = i;
        }
      }
      m_RetTypeSelector->setCurrentIndex(selIndex);

      QObject::connect(m_RetTypeSelector, (void (QComboBox::*)(int)) &QComboBox::currentIndexChanged, [this]()
        {
          OnRetTypeChanged();
        });
      retLayout->addWidget(m_RetTypeSelector);
    }

    TypeDeclEditor::Build(iLayout);
  }
}