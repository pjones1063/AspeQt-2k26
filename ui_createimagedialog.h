/********************************************************************************
** Form generated from reading UI file 'createimagedialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CREATEIMAGEDIALOG_H
#define UI_CREATEIMAGEDIALOG_H

#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_CreateImageDialog
{
public:
    QVBoxLayout *verticalLayout;
    QWidget *widget;
    QVBoxLayout *verticalLayout_5;
    QRadioButton *stdSingleButton;
    QRadioButton *stdEnhancedButton;
    QRadioButton *stdDoubleButton;
    QRadioButton *doubleDoubleButton;
    QRadioButton *harddiskButton;
    QRadioButton *customButton;
    QWidget *geometryWidget;
    QHBoxLayout *horizontalLayout_2;
    QVBoxLayout *verticalLayout_6;
    QLabel *label_4;
    QSpinBox *sectorsSpin;
    QVBoxLayout *verticalLayout_7;
    QLabel *label_5;
    QComboBox *densityCombo;
    QLabel *capacityLabel;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *CreateImageDialog)
    {
        if (CreateImageDialog->objectName().isEmpty())
            CreateImageDialog->setObjectName(QString::fromUtf8("CreateImageDialog"));
        CreateImageDialog->setWindowModality(Qt::ApplicationModal);
        CreateImageDialog->resize(310, 259);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(CreateImageDialog->sizePolicy().hasHeightForWidth());
        CreateImageDialog->setSizePolicy(sizePolicy);
        CreateImageDialog->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
        verticalLayout = new QVBoxLayout(CreateImageDialog);
        verticalLayout->setSpacing(10);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setSizeConstraint(QLayout::SetFixedSize);
        widget = new QWidget(CreateImageDialog);
        widget->setObjectName(QString::fromUtf8("widget"));
        verticalLayout_5 = new QVBoxLayout(widget);
        verticalLayout_5->setContentsMargins(0, 0, 0, 0);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        stdSingleButton = new QRadioButton(widget);
        stdSingleButton->setObjectName(QString::fromUtf8("stdSingleButton"));
        stdSingleButton->setChecked(true);

        verticalLayout_5->addWidget(stdSingleButton);

        stdEnhancedButton = new QRadioButton(widget);
        stdEnhancedButton->setObjectName(QString::fromUtf8("stdEnhancedButton"));

        verticalLayout_5->addWidget(stdEnhancedButton);

        stdDoubleButton = new QRadioButton(widget);
        stdDoubleButton->setObjectName(QString::fromUtf8("stdDoubleButton"));

        verticalLayout_5->addWidget(stdDoubleButton);

        doubleDoubleButton = new QRadioButton(widget);
        doubleDoubleButton->setObjectName(QString::fromUtf8("doubleDoubleButton"));

        verticalLayout_5->addWidget(doubleDoubleButton);

        harddiskButton = new QRadioButton(widget);
        harddiskButton->setObjectName(QString::fromUtf8("harddiskButton"));

        verticalLayout_5->addWidget(harddiskButton);

        customButton = new QRadioButton(widget);
        customButton->setObjectName(QString::fromUtf8("customButton"));

        verticalLayout_5->addWidget(customButton);

        geometryWidget = new QWidget(widget);
        geometryWidget->setObjectName(QString::fromUtf8("geometryWidget"));
        geometryWidget->setEnabled(false);
        horizontalLayout_2 = new QHBoxLayout(geometryWidget);
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        verticalLayout_6 = new QVBoxLayout();
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        label_4 = new QLabel(geometryWidget);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        verticalLayout_6->addWidget(label_4);

        sectorsSpin = new QSpinBox(geometryWidget);
        sectorsSpin->setObjectName(QString::fromUtf8("sectorsSpin"));
        sectorsSpin->setMinimum(1);
        sectorsSpin->setMaximum(65535);
        sectorsSpin->setValue(720);

        verticalLayout_6->addWidget(sectorsSpin);


        horizontalLayout_2->addLayout(verticalLayout_6);

        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        label_5 = new QLabel(geometryWidget);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        verticalLayout_7->addWidget(label_5);

        densityCombo = new QComboBox(geometryWidget);
        densityCombo->addItem(QString());
        densityCombo->addItem(QString());
        densityCombo->addItem(QString());
        densityCombo->addItem(QString());
        densityCombo->setObjectName(QString::fromUtf8("densityCombo"));

        verticalLayout_7->addWidget(densityCombo);


        horizontalLayout_2->addLayout(verticalLayout_7);


        verticalLayout_5->addWidget(geometryWidget);

        capacityLabel = new QLabel(widget);
        capacityLabel->setObjectName(QString::fromUtf8("capacityLabel"));
        capacityLabel->setEnabled(true);

        verticalLayout_5->addWidget(capacityLabel);


        verticalLayout->addWidget(widget);

        buttonBox = new QDialogButtonBox(CreateImageDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(CreateImageDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), CreateImageDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), CreateImageDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(CreateImageDialog);
    } // setupUi

    void retranslateUi(QDialog *CreateImageDialog)
    {
        CreateImageDialog->setWindowTitle(QCoreApplication::translate("CreateImageDialog", "Create a disk image", nullptr));
        stdSingleButton->setText(QCoreApplication::translate("CreateImageDialog", "Standard single density", nullptr));
        stdEnhancedButton->setText(QCoreApplication::translate("CreateImageDialog", "Standard enhanced (also called medium or dual) density", nullptr));
        stdDoubleButton->setText(QCoreApplication::translate("CreateImageDialog", "Standard double density", nullptr));
        doubleDoubleButton->setText(QCoreApplication::translate("CreateImageDialog", "Double sided double density", nullptr));
        harddiskButton->setText(QCoreApplication::translate("CreateImageDialog", "Double density harddisk", nullptr));
        customButton->setText(QCoreApplication::translate("CreateImageDialog", "Custom", nullptr));
        label_4->setText(QCoreApplication::translate("CreateImageDialog", "Number of sectors:", nullptr));
        label_5->setText(QCoreApplication::translate("CreateImageDialog", "Sector density:", nullptr));
        densityCombo->setItemText(0, QCoreApplication::translate("CreateImageDialog", "Single (128 bytes per sector)", nullptr));
        densityCombo->setItemText(1, QCoreApplication::translate("CreateImageDialog", "Double (256 bytes per sector)", nullptr));
        densityCombo->setItemText(2, QCoreApplication::translate("CreateImageDialog", "512 bytes per sector", nullptr));
        densityCombo->setItemText(3, QCoreApplication::translate("CreateImageDialog", "8192 bytes per sector", nullptr));

        capacityLabel->setText(QCoreApplication::translate("CreateImageDialog", "Total image capacity: 92160 bytes (90 K)", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CreateImageDialog: public Ui_CreateImageDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CREATEIMAGEDIALOG_H
