/********************************************************************************
** Form generated from reading UI file 'tileset_toolboxeFuVok.ui'
**
** Created by: Qt User Interface Compiler version 5.15.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef TILESET_TOOLBOXEFUVOK_H
#define TILESET_TOOLBOXEFUVOK_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TilesetToolbox
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QSpinBox *gridSizeX;
    QSpinBox *gridSizeY;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label_2;
    QSpinBox *offsetX;
    QSpinBox *offsetY;
    QPushButton *bootstrapButton;

    void setupUi(QWidget *TilesetToolbox)
    {
        if (TilesetToolbox->objectName().isEmpty())
            TilesetToolbox->setObjectName(QString::fromUtf8("TilesetToolbox"));
        TilesetToolbox->resize(129, 97);
        QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(TilesetToolbox->sizePolicy().hasHeightForWidth());
        TilesetToolbox->setSizePolicy(sizePolicy);
        verticalLayout = new QVBoxLayout(TilesetToolbox);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(TilesetToolbox);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout->addWidget(label);

        gridSizeX = new QSpinBox(TilesetToolbox);
        gridSizeX->setObjectName(QString::fromUtf8("gridSizeX"));
        gridSizeX->setMinimum(1);
        gridSizeX->setMaximum(65536);

        horizontalLayout->addWidget(gridSizeX);

        gridSizeY = new QSpinBox(TilesetToolbox);
        gridSizeY->setObjectName(QString::fromUtf8("gridSizeY"));
        gridSizeY->setMinimum(1);
        gridSizeY->setMaximum(65536);

        horizontalLayout->addWidget(gridSizeY);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        label_2 = new QLabel(TilesetToolbox);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        horizontalLayout_2->addWidget(label_2);

        offsetX = new QSpinBox(TilesetToolbox);
        offsetX->setObjectName(QString::fromUtf8("offsetX"));
        offsetX->setMaximum(65536);

        horizontalLayout_2->addWidget(offsetX);

        offsetY = new QSpinBox(TilesetToolbox);
        offsetY->setObjectName(QString::fromUtf8("offsetY"));
        offsetY->setMaximum(65536);

        horizontalLayout_2->addWidget(offsetY);


        verticalLayout->addLayout(horizontalLayout_2);

        bootstrapButton = new QPushButton(TilesetToolbox);
        bootstrapButton->setObjectName(QString::fromUtf8("bootstrapButton"));

        verticalLayout->addWidget(bootstrapButton);


        retranslateUi(TilesetToolbox);

        QMetaObject::connectSlotsByName(TilesetToolbox);
    } // setupUi

    void retranslateUi(QWidget *TilesetToolbox)
    {
        TilesetToolbox->setWindowTitle(QCoreApplication::translate("TilesetToolbox", "Form", nullptr));
        label->setText(QCoreApplication::translate("TilesetToolbox", "Grid", nullptr));
        label_2->setText(QCoreApplication::translate("TilesetToolbox", "Offset", nullptr));
        bootstrapButton->setText(QCoreApplication::translate("TilesetToolbox", "Bootstrap", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TilesetToolbox: public Ui_TilesetToolbox {};
} // namespace Ui

QT_END_NAMESPACE

#endif // TILESET_TOOLBOXEFUVOK_H
