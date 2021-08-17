/********************************************************************************
** Form generated from reading UI file 'array_editorRYnDOK.ui'
**
** Created by: Qt User Interface Compiler version 5.15.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef ARRAY_EDITORRYNDOK_H
#define ARRAY_EDITORRYNDOK_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ArrayEditor
{
public:
    QVBoxLayout *verticalLayout;
    QFrame *frame;
    QHBoxLayout *horizontalLayout;
    QLabel *ArraySize;
    QPushButton *AddItemButton;
    QPushButton *EmptyButton;

    void setupUi(QWidget *ArrayEditor)
    {
        if (ArrayEditor->objectName().isEmpty())
            ArrayEditor->setObjectName(QString::fromUtf8("ArrayEditor"));
        ArrayEditor->resize(206, 26);
        ArrayEditor->setAutoFillBackground(true);
        verticalLayout = new QVBoxLayout(ArrayEditor);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setSizeConstraint(QLayout::SetMinimumSize);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        frame = new QFrame(ArrayEditor);
        frame->setObjectName(QString::fromUtf8("frame"));
        frame->setFrameShape(QFrame::StyledPanel);
        frame->setFrameShadow(QFrame::Raised);
        horizontalLayout = new QHBoxLayout(frame);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        ArraySize = new QLabel(frame);
        ArraySize->setObjectName(QString::fromUtf8("ArraySize"));

        horizontalLayout->addWidget(ArraySize);

        AddItemButton = new QPushButton(frame);
        AddItemButton->setObjectName(QString::fromUtf8("AddItemButton"));

        horizontalLayout->addWidget(AddItemButton);

        EmptyButton = new QPushButton(frame);
        EmptyButton->setObjectName(QString::fromUtf8("EmptyButton"));

        horizontalLayout->addWidget(EmptyButton);


        verticalLayout->addWidget(frame);


        retranslateUi(ArrayEditor);

        QMetaObject::connectSlotsByName(ArrayEditor);
    } // setupUi

    void retranslateUi(QWidget *ArrayEditor)
    {
        ArrayEditor->setWindowTitle(QCoreApplication::translate("ArrayEditor", "Form", nullptr));
        ArraySize->setText(QCoreApplication::translate("ArrayEditor", "0", nullptr));
        AddItemButton->setText(QString());
        EmptyButton->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class ArrayEditor: public Ui_ArrayEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // ARRAY_EDITORRYNDOK_H
