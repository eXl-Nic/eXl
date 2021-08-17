#include "arrayitem_editor.h"
#include "ui_arrayitem_editor.h"

#include <QStyle>

ArrayItem_Editor::ArrayItem_Editor(uint32_t iItemIndex, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ArrayItemEditor)
{
  ui->setupUi(this);

  ui->InsertButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
  ui->RemoveItemButton->setIcon(style()->standardIcon(QStyle::SP_MessageBoxCritical));

  ui->ItemIndexLabel->setText(QString::number(iItemIndex));
  QObject::connect(ui->RemoveItemButton, &QAbstractButton::pressed, [this]()
  {
    emit onRemoveItem();
  });

  QObject::connect(ui->InsertButton, &QAbstractButton::pressed, [this]()
  {
    emit onInsertItem();
  });
}

ArrayItem_Editor::~ArrayItem_Editor()
{
  delete ui;
}
