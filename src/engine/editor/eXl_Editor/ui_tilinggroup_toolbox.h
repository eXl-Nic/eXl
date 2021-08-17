/********************************************************************************
** Form generated from reading UI file 'tilinggroup_toolboxxrQADp.ui'
**
** Created by: Qt User Interface Compiler version 5.15.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TILINGGROUP_TOOLBOXXRQADP_H
#define TILINGGROUP_TOOLBOXXRQADP_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TilingGroupToolbox
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QSpinBox *patternSizeX;
    QSpinBox *patternSizeY;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    QSpinBox *anchorX;
    QSpinBox *anchorY;

    void setupUi(QWidget *TilingGroupToolbox)
    {
        if (TilingGroupToolbox->objectName().isEmpty())
            TilingGroupToolbox->setObjectName(QString::fromUtf8("TilingGroupToolbox"));
        TilingGroupToolbox->resize(210, 100);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(TilingGroupToolbox->sizePolicy().hasHeightForWidth());
        TilingGroupToolbox->setSizePolicy(sizePolicy);
        verticalLayout = new QVBoxLayout(TilingGroupToolbox);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setSizeConstraint(QLayout::SetMaximumSize);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetMinimumSize);
        label = new QLabel(TilingGroupToolbox);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout->addWidget(label);

        patternSizeX = new QSpinBox(TilingGroupToolbox);
        patternSizeX->setObjectName(QString::fromUtf8("patternSizeX"));
        patternSizeX->setMinimum(1);
        patternSizeX->setMaximum(65536);

        horizontalLayout->addWidget(patternSizeX);

        patternSizeY = new QSpinBox(TilingGroupToolbox);
        patternSizeY->setObjectName(QString::fromUtf8("patternSizeY"));
        patternSizeY->setMinimum(1);
        patternSizeY->setMaximum(65536);

        horizontalLayout->addWidget(patternSizeY);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setSizeConstraint(QLayout::SetMinimumSize);
        label_2 = new QLabel(TilingGroupToolbox);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        horizontalLayout_2->addWidget(label_2);

        anchorX = new QSpinBox(TilingGroupToolbox);
        anchorX->setObjectName(QString::fromUtf8("anchorX"));
        anchorX->setMinimum(-1);
        anchorX->setMaximum(65536);

        horizontalLayout_2->addWidget(anchorX);

        anchorY = new QSpinBox(TilingGroupToolbox);
        anchorY->setObjectName(QString::fromUtf8("anchorY"));
        anchorY->setMinimum(-1);
        anchorY->setMaximum(65536);

        horizontalLayout_2->addWidget(anchorY);


        verticalLayout->addLayout(horizontalLayout_2);


        retranslateUi(TilingGroupToolbox);

        QMetaObject::connectSlotsByName(TilingGroupToolbox);
    } // setupUi

    void retranslateUi(QWidget *TilingGroupToolbox)
    {
        TilingGroupToolbox->setWindowTitle(QCoreApplication::translate("TilingGroupToolbox", "Form", nullptr));
        label->setText(QCoreApplication::translate("TilingGroupToolbox", "Pattern Size", nullptr));
        label_2->setText(QCoreApplication::translate("TilingGroupToolbox", "Anchor Position", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TilingGroupToolbox: public Ui_TilingGroupToolbox {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TILINGGROUP_TOOLBOXXRQADP_H
