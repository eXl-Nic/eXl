#include "array_editor.h"
#include "ui_array_editor.h"
#include <QStyle>

#include <core/type/arraytype.hpp>

Array_Editor::Array_Editor(QWidget *parent) :
  DynObjectEditor(parent),
  ui(new Ui::ArrayEditor)
{
  ui->setupUi(this);
  ui->AddItemButton->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
  ui->EmptyButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));

  QObject::connect(ui->AddItemButton, &QAbstractButton::pressed, [this]()
  { 
    emit onAddItem();
  });

  QObject::connect(ui->EmptyButton, &QAbstractButton::pressed, [this]()
  {
    if (m_ConstObj == nullptr)
    {
      return;
    }

    if (eXl::ArrayType const* arrayType = eXl::ArrayType::DynamicCast(m_ConstObj->GetType()))
    {
      void const* value = m_ConstObj->GetBuffer();
      unsigned int numEntries = arrayType->GetArraySize(value);
      if (numEntries > 0)
      {
        emit onEmptyArray();
      }
    }
  });
}

void Array_Editor::UpdateView()
{
  if (m_ConstObj == nullptr)
  {
    return ;
  }

  if (eXl::ArrayType const* arrayType = eXl::ArrayType::DynamicCast(m_ConstObj->GetType()))
  {
    void const* value = m_ConstObj->GetBuffer();
    unsigned int numEntries = arrayType->GetArraySize(value);

    ui->ArraySize->setText(QString::number(numEntries));
  }
}

Array_Editor::~Array_Editor()
{
  delete ui;
}

