/********************************************************************************
** Form generated from reading UI file 'arrayitem_editorNawqgf.ui'
**
** Created by: Qt User Interface Compiler version 5.15.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef ARRAYITEM_EDITORNAWQGF_H
#define ARRAYITEM_EDITORNAWQGF_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ArrayItemEditor
{
public:
    QVBoxLayout *verticalLayout;
    QFrame *frame;
    QHBoxLayout *horizontalLayout;
    QLabel *ItemIndexLabel;
    QPushButton *InsertButton;
    QPushButton *RemoveItemButton;

    void setupUi(QWidget *ArrayItemEditor)
    {
        if (ArrayItemEditor->objectName().isEmpty())
            ArrayItemEditor->setObjectName(QString::fromUtf8("ArrayItemEditor"));
        ArrayItemEditor->resize(94, 26);
        ArrayItemEditor->setAutoFillBackground(true);
        verticalLayout = new QVBoxLayout(ArrayItemEditor);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        frame = new QFrame(ArrayItemEditor);
        frame->setObjectName(QString::fromUtf8("frame"));
        frame->setAutoFillBackground(true);
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        horizontalLayout = new QHBoxLayout(frame);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        ItemIndexLabel = new QLabel(frame);
        ItemIndexLabel->setObjectName(QString::fromUtf8("ItemIndexLabel"));

        horizontalLayout->addWidget(ItemIndexLabel);

        InsertButton = new QPushButton(frame);
        InsertButton->setObjectName(QString::fromUtf8("InsertButton"));

        horizontalLayout->addWidget(InsertButton);

        RemoveItemButton = new QPushButton(frame);
        RemoveItemButton->setObjectName(QString::fromUtf8("RemoveItemButton"));
        RemoveItemButton->setAutoFillBackground(true);
        QIcon icon;
        QString iconThemeName = QString::fromUtf8("SP_DialogCancelButton");
        if (QIcon::hasThemeIcon(iconThemeName)) {
            icon = QIcon::fromTheme(iconThemeName);
        } else {
            icon.addFile(QString::fromUtf8("."), QSize(), QIcon::Normal, QIcon::Off);
        }
        RemoveItemButton->setIcon(icon);

        horizontalLayout->addWidget(RemoveItemButton);


        verticalLayout->addWidget(frame);


        retranslateUi(ArrayItemEditor);

        QMetaObject::connectSlotsByName(ArrayItemEditor);
    } // setupUi

    void retranslateUi(QWidget *ArrayItemEditor)
    {
        ArrayItemEditor->setWindowTitle(QCoreApplication::translate("ArrayItemEditor", "Form", nullptr));
        ItemIndexLabel->setText(QCoreApplication::translate("ArrayItemEditor", "0", nullptr));
        InsertButton->setText(QString());
        RemoveItemButton->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class ArrayItemEditor: public Ui_ArrayItemEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // ARRAYITEM_EDITORNAWQGF_H
