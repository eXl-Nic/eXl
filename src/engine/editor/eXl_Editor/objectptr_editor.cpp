#if 0

#include "objectptr_editor.h"
#include "ui_objectptr_editor.h"

#include <core/type/classtype.hpp>
#include <core/type/objectptrtype.hpp>
#include <core/type/typemanager.hpp>

ObjectPtr_Editor::ObjectPtr_Editor(QWidget *parent) :
  DynObjectEditor(parent),
  ui(new Ui::ObjectPtr_Editor)
{
  ui->setupUi(this);
}

void ObjectPtr_Editor::UpdateView()
{
  eXl::ObjectPtrType const* ptrType = eXl::ObjectPtrType::DynamicCast(m_Obj.GetType());
  eXl::RttiObjectRefC* object = m_Obj.CastBuffer<eXl::RttiOPtr>()->get();
  
  std::list<eXl::Rtti const*> derivedList;
  eXl::TypeManager::ListDerivedClassesForRtti(ptrType->GetMinimalRtti(),derivedList);

  derivedList.push_front(&ptrType->GetMinimalRtti());

  ui->RttiList->clear();
  ui->RttiList->addItem("NULL",qVariantFromValue((eXl::Rtti const*)0));
  std::list<eXl::Rtti const*>::iterator iter = derivedList.begin();
  std::list<eXl::Rtti const*>::iterator iterEnd = derivedList.end();

  unsigned int curValue = 0;
  unsigned int i = 1;
  for(;iter != iterEnd; ++iter)
  {
    eXl::ClassType const* classType = eXl::TypeManager::GetClassForRtti(*(*iter));
    if(classType != NULL && !classType->IsAbstract())
    {
      ui->RttiList->addItem((*iter)->GetName().c_str(),qVariantFromValue(*iter));
      if(*iter == &object->GetRtti())
      {
        curValue = i;
      }
      ++i;
    }
  }
  ui->RttiList->setCurrentIndex(curValue);
}

ObjectPtr_Editor::~ObjectPtr_Editor()
{
  delete ui;
}

void ObjectPtr_Editor::on_RttiList_currentIndexChanged(int index)
{
  QVariant data = ui->RttiList->itemData(index);
  eXl::Rtti const* rtti = data.value<eXl::Rtti const*>();
  if(rtti != NULL)
  {
    eXl::ClassType const* classType = eXl::TypeManager::GetClassForRtti(*rtti);
    void* object = NULL;
    *m_Obj.CastBuffer<eXl::RttiOPtr>() = reinterpret_cast<eXl::RttiObjectRefC*>(classType->Build(object,NULL,NULL));
  }
  else
  {
    *m_Obj.CastBuffer<eXl::RttiOPtr>() = NULL;
  }
}

#endif