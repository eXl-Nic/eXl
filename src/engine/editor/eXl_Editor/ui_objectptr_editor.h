/********************************************************************************
** Form generated from reading UI file 'objectptr_editorKCzoCT.ui'
**
** Created by: Qt User Interface Compiler version 5.15.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef OBJECTPTR_EDITORKCZOCT_H
#define OBJECTPTR_EDITORKCZOCT_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ObjectPtr_Editor
{
public:
    QVBoxLayout *verticalLayout;
    QComboBox *RttiList;

    void setupUi(QWidget *ObjectPtr_Editor)
    {
        if (ObjectPtr_Editor->objectName().isEmpty())
            ObjectPtr_Editor->setObjectName(QString::fromUtf8("ObjectPtr_Editor"));
        ObjectPtr_Editor->resize(214, 39);
        verticalLayout = new QVBoxLayout(ObjectPtr_Editor);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        RttiList = new QComboBox(ObjectPtr_Editor);
        RttiList->setObjectName(QString::fromUtf8("RttiList"));

        verticalLayout->addWidget(RttiList);


        retranslateUi(ObjectPtr_Editor);

        QMetaObject::connectSlotsByName(ObjectPtr_Editor);
    } // setupUi

    void retranslateUi(QWidget *ObjectPtr_Editor)
    {
        ObjectPtr_Editor->setWindowTitle(QCoreApplication::translate("ObjectPtr_Editor", "Form", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ObjectPtr_Editor: public Ui_ObjectPtr_Editor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // OBJECTPTR_EDITORKCZOCT_H
