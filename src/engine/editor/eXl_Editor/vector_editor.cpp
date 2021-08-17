#include "vector_editor.h"

#include <math/math.hpp>
#include <core/type/type.hpp>

#include <QHBoxLayout>

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>

Vector_Editor::Vector_Editor(QWidget *parent)
  :DynObjectEditor(parent)
{
  memset(m_Boxes,0,sizeof(m_Boxes));
  QHBoxLayout* hLayout = new QHBoxLayout;
  hLayout->setContentsMargins(0,0,0,0);
  hLayout->setAlignment(Qt::AlignLeft);
  setLayout(hLayout);
}

void Vector_Editor::OnEditingFinished()
{
  if(m_Integer)
  {
    int* data = reinterpret_cast<int*>(m_Obj.GetBuffer());
    QSpinBox** curBox = reinterpret_cast<QSpinBox**>(m_Boxes);
    for(uint32_t i = 0; i<4; ++i, ++curBox, ++data)
    {
      if (*curBox == nullptr)
      {
        break;
      }
      *data = (*curBox)->value();
    }
  }
  else
  {
    float* data = reinterpret_cast<float*>(m_Obj.GetBuffer());
    QDoubleSpinBox** curBox = reinterpret_cast<QDoubleSpinBox**>(m_Boxes);
    for (uint32_t i = 0; i < 4; ++i, ++curBox, ++data)
    {
      if (*curBox == nullptr)
      {
        break;
      }
      *data = (*curBox)->value();
    }
  }

  emit editingFinished();
}

static eXl::Type const* v2iId =  eXl::TypeManager::GetType<eXl::Vector2i>();
static eXl::Type const* v3iId =  eXl::TypeManager::GetType<eXl::Vector3i>();
static eXl::Type const* v4iId =  eXl::TypeManager::GetType<eXl::Vector4i>();                            
static eXl::Type const* v2fId =  eXl::TypeManager::GetType<eXl::Vector2f>();
static eXl::Type const* v3fId =  eXl::TypeManager::GetType<eXl::Vector3f>();
static eXl::Type const* v4fId =  eXl::TypeManager::GetType<eXl::Vector4f>();

void Vector_Editor::UpdateView()
{
  unsigned int numComp = 0;
  eXl::Type const* typeId = m_Obj.GetType();
  if(typeId == v2iId || typeId == v3iId ||typeId == v4iId)
  {
    m_Integer = true;
  }
  else
  {
    m_Integer = false;
  }
  if(typeId == v2iId || typeId == v2fId)
    numComp = 2;
  if(typeId == v3iId || typeId == v3fId)
    numComp = 3;
  if(typeId == v4iId || typeId == v4fId)
    numComp = 4;

  static const char* labels[ 4 ] = {"X","Y","Z","W"};

  if(m_Integer)
  {
    int* data = reinterpret_cast<int*>(m_Obj.GetBuffer());
    for(unsigned int i = 0; i<numComp; ++i)
    {
      QLabel* newLabel = new QLabel(labels[i], this);
      layout()->addWidget(newLabel);
      QSpinBox* newSpinBox = new QSpinBox(this);
      newSpinBox->setMinimum(INT32_MIN);
      newSpinBox->setMaximum(INT32_MAX);
      newSpinBox->setValue(*data);
      connect(newSpinBox,SIGNAL(editingFinished()),SLOT(OnEditingFinished()));
      m_Boxes[i] = newSpinBox;
      layout()->addWidget(newSpinBox);

      ++data;
    }
  }
  else
  {
    float* data = reinterpret_cast<float*>(m_Obj.GetBuffer());
    for(unsigned int i = 0; i<numComp; ++i)
    {
      QLabel* newLabel = new QLabel(labels[i], this);
      layout()->addWidget(newLabel);
      QDoubleSpinBox* newSpinBox = new QDoubleSpinBox(this);
      newSpinBox->setDecimals(4);
      newSpinBox->setStepType(QDoubleSpinBox::AdaptiveDecimalStepType);
      newSpinBox->setMaximum(FLT_MAX);
      newSpinBox->setMinimum(-FLT_MAX);
      newSpinBox->setValue(*data);
      connect(newSpinBox,SIGNAL(editingFinished()),SLOT(OnEditingFinished()));
      m_Boxes[i] = newSpinBox;
      layout()->addWidget(newSpinBox);

      ++data;
    }
  }
}
