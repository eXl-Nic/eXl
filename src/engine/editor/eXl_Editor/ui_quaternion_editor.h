/********************************************************************************
** Form generated from reading UI file 'quaternion_editorRJTfBE.ui'
**
** Created by: Qt User Interface Compiler version 5.15.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef QUATERNION_EDITORRJTFBE_H
#define QUATERNION_EDITORRJTFBE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QuatEditor
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QLineEdit *w_Edit;
    QLabel *label_2;
    QLineEdit *x_Edit;
    QLabel *label_4;
    QLineEdit *y_Edit;
    QLabel *label_3;
    QLineEdit *z_Edit;

    void setupUi(QWidget *QuatEditor)
    {
        if (QuatEditor->objectName().isEmpty())
            QuatEditor->setObjectName(QString::fromUtf8("QuatEditor"));
        QuatEditor->resize(320, 38);
        gridLayout = new QGridLayout(QuatEditor);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(QuatEditor);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        w_Edit = new QLineEdit(QuatEditor);
        w_Edit->setObjectName(QString::fromUtf8("w_Edit"));

        gridLayout->addWidget(w_Edit, 0, 1, 1, 1);

        label_2 = new QLabel(QuatEditor);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 0, 2, 1, 1);

        x_Edit = new QLineEdit(QuatEditor);
        x_Edit->setObjectName(QString::fromUtf8("x_Edit"));

        gridLayout->addWidget(x_Edit, 0, 3, 1, 1);

        label_4 = new QLabel(QuatEditor);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gridLayout->addWidget(label_4, 0, 4, 1, 1);

        y_Edit = new QLineEdit(QuatEditor);
        y_Edit->setObjectName(QString::fromUtf8("y_Edit"));

        gridLayout->addWidget(y_Edit, 0, 5, 1, 1);

        label_3 = new QLabel(QuatEditor);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout->addWidget(label_3, 0, 6, 1, 1);

        z_Edit = new QLineEdit(QuatEditor);
        z_Edit->setObjectName(QString::fromUtf8("z_Edit"));

        gridLayout->addWidget(z_Edit, 0, 7, 1, 1);


        retranslateUi(QuatEditor);
        QObject::connect(w_Edit, SIGNAL(editingFinished()), QuatEditor, SLOT(OnEditingFinished()));
        QObject::connect(x_Edit, SIGNAL(editingFinished()), QuatEditor, SLOT(OnEditingFinished()));
        QObject::connect(y_Edit, SIGNAL(editingFinished()), QuatEditor, SLOT(OnEditingFinished()));
        QObject::connect(z_Edit, SIGNAL(editingFinished()), QuatEditor, SLOT(OnEditingFinished()));

        QMetaObject::connectSlotsByName(QuatEditor);
    } // setupUi

    void retranslateUi(QWidget *QuatEditor)
    {
        QuatEditor->setWindowTitle(QCoreApplication::translate("QuatEditor", "Form", nullptr));
        label->setText(QCoreApplication::translate("QuatEditor", "W", nullptr));
        label_2->setText(QCoreApplication::translate("QuatEditor", "X", nullptr));
        label_4->setText(QCoreApplication::translate("QuatEditor", "Y", nullptr));
        label_3->setText(QCoreApplication::translate("QuatEditor", "Z", nullptr));
    } // retranslateUi

};

namespace Ui {
    class QuatEditor: public Ui_QuatEditor {};
} // namespace Ui

QT_END_NAMESPACE

#endif // QUATERNION_EDITORRJTFBE_H
