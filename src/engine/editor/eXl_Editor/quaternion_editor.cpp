#include "quaternion_editor.h"
#include "ui_quaternion_editor.h"

#include <math/math.hpp>
#include <core/type/type.hpp>
//#include <gametk/gametkcommon.hpp>

Quaternion_Editor::Quaternion_Editor(QWidget *parent)
  :DynObjectEditor(parent)
  ,ui(new Ui::QuatEditor)
{
  ui->setupUi(this);
}

void Quaternion_Editor::OnEditingFinished()
{
  bool valid = false;
  QString value = ui->w_Edit->text();
  float valW = value.toFloat(&valid);
  if(!valid)
    return;
  value = ui->x_Edit->text();
  float valX = value.toFloat(&valid);
  if(!valid)
    return;
  value = ui->y_Edit->text();
  float valY = value.toFloat(&valid);
  if(!valid)
    return;
  value = ui->z_Edit->text();
  float valZ = value.toFloat(&valid);
  if(!valid)
    return;
  *m_Obj.CastBuffer<eXl::Quaternion>() = eXl::Quaternion(valW,valX,valY,valZ);
}

void Quaternion_Editor::UpdateView()
{
  eXl::Quaternion tempQuat = *m_Obj.CastBuffer<eXl::Quaternion>();
  QString text("%f");
  ui->w_Edit->setText(text.arg(tempQuat.w));
  ui->x_Edit->setText(text.arg(tempQuat.x));
  ui->y_Edit->setText(text.arg(tempQuat.y));  
  ui->z_Edit->setText(text.arg(tempQuat.z));
}
