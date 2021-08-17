/********************************************************************************
** Form generated from reading UI file 'enum_editorMweAOJ.ui'
**
** Created by: Qt User Interface Compiler version 5.15.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef ENUM_EDITORMWEAOJ_H
#define ENUM_EDITORMWEAOJ_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Enum_Editor
{
public:
    QVBoxLayout *verticalLayout;
    QComboBox *EnumNames;

    void setupUi(QWidget *Enum_Editor)
    {
        if (Enum_Editor->objectName().isEmpty())
            Enum_Editor->setObjectName(QString::fromUtf8("Enum_Editor"));
        Enum_Editor->resize(239, 24);
        verticalLayout = new QVBoxLayout(Enum_Editor);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        EnumNames = new QComboBox(Enum_Editor);
        EnumNames->setObjectName(QString::fromUtf8("EnumNames"));

        verticalLayout->addWidget(EnumNames);


        retranslateUi(Enum_Editor);

        QMetaObject::connectSlotsByName(Enum_Editor);
    } // setupUi

    void retranslateUi(QWidget *Enum_Editor)
    {
        Enum_Editor->setWindowTitle(QCoreApplication::translate("Enum_Editor", "Form", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Enum_Editor: public Ui_Enum_Editor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // ENUM_EDITORMWEAOJ_H
