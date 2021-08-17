#include "enum_editor.h"
#include "ui_enum_editor.h"

#include <core/type/enumtype.hpp>

Enum_Editor::Enum_Editor(QWidget *parent) :
  DynObjectEditor(parent),
  ui(new Ui::Enum_Editor)
{
  ui->setupUi(this);
}

void Enum_Editor::UpdateView()
{
  eXl::EnumType const* enumType = eXl::EnumType::DynamicCast(m_Obj.GetType());
  unsigned int* value = reinterpret_cast<unsigned int*>(m_Obj.GetBuffer());
  unsigned int numValues = enumType->GetNumEnum();

  if(*value >= numValues)
    *value = 0;

  ui->EnumNames->clear();
  for(unsigned int i = 0; i<numValues; ++i)
  {
    eXl::TypeEnumName enumName;
    enumType->GetEnumName(i, enumName);
    ui->EnumNames->addItem(enumName.c_str());
  }
  ui->EnumNames->setCurrentIndex(*value);
}

Enum_Editor::~Enum_Editor()
{
  delete ui;
}

void Enum_Editor::on_EnumNames_currentIndexChanged(int index)
{
  if(index >= 0)
  {
    unsigned int* value = reinterpret_cast<unsigned int*>(m_Obj.GetBuffer());
    *value = index;
  }

  emit editingFinished();
}
