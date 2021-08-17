#include "quaternion_editor.h"
#include "ui_quaternion_editor.h"

#include <math/quaternion.hpp>
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
  *m_Obj.CastBuffer<eXl::Quaternionf>() = eXl::Quaternionf(valW,valX,valY,valZ);
}

void Quaternion_Editor::UpdateView()
{
  eXl::Quaternionf tempQuat = *m_Obj.CastBuffer<eXl::Quaternionf>();
  QString text("%f");
  text.arg(tempQuat.W());
  ui->w_Edit->setText(text);
  text = "%f";
  text.arg(tempQuat.X());
  ui->x_Edit->setText(text);
  text = "%f";
  text.arg(tempQuat.Y());
  ui->y_Edit->setText(text);
  text = "%f";
  text.arg(tempQuat.Z());
  ui->z_Edit->setText(text);
}
