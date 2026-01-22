/********************************************************************************
** Form generated from reading UI file 'bootoptionsdialog.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BOOTOPTIONSDIALOG_H
#define UI_BOOTOPTIONSDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QRadioButton>

QT_BEGIN_NAMESPACE

class Ui_BootOptionsDialog
{
public:
    QDialogButtonBox *buttonBox;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QLabel *label;
    QRadioButton *smartDOS;
    QRadioButton *spartaDOS;
    QRadioButton *myPicoDOS;
    QRadioButton *myDOS;
    QRadioButton *atariDOS;
    QRadioButton *dosXL;
    QLabel *label_2;
    QCheckBox *disablePicoHiSpeed;

    void setupUi(QDialog *BootOptionsDialog)
    {
        if (BootOptionsDialog->objectName().isEmpty())
            BootOptionsDialog->setObjectName("BootOptionsDialog");
        BootOptionsDialog->resize(319, 294);
        BootOptionsDialog->setMinimumSize(QSize(319, 294));
        BootOptionsDialog->setMaximumSize(QSize(319, 294));
        buttonBox = new QDialogButtonBox(BootOptionsDialog);
        buttonBox->setObjectName("buttonBox");
        buttonBox->setGeometry(QRect(20, 250, 281, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        groupBox = new QGroupBox(BootOptionsDialog);
        groupBox->setObjectName("groupBox");
        groupBox->setGeometry(QRect(10, 10, 291, 232));
        QFont font;
        font.setBold(true);
        groupBox->setFont(font);
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName("gridLayout");
        label = new QLabel(groupBox);
        label->setObjectName("label");
        label->setFont(font);
        label->setStyleSheet(QString::fromUtf8("color: rgb(111, 111, 111);"));

        gridLayout->addWidget(label, 0, 0, 1, 3);

        smartDOS = new QRadioButton(groupBox);
        smartDOS->setObjectName("smartDOS");
        smartDOS->setStyleSheet(QString::fromUtf8("color: rgb(140, 96, 115);"));

        gridLayout->addWidget(smartDOS, 4, 0, 1, 3);

        spartaDOS = new QRadioButton(groupBox);
        spartaDOS->setObjectName("spartaDOS");
        spartaDOS->setFont(font);
        spartaDOS->setStyleSheet(QString::fromUtf8("color: rgb(140, 96, 115);"));

        gridLayout->addWidget(spartaDOS, 5, 0, 1, 3);

        myPicoDOS = new QRadioButton(groupBox);
        myPicoDOS->setObjectName("myPicoDOS");
        myPicoDOS->setFont(font);
        myPicoDOS->setStyleSheet(QString::fromUtf8("color: rgb(140, 96, 115);"));

        gridLayout->addWidget(myPicoDOS, 6, 0, 1, 3);

        myDOS = new QRadioButton(groupBox);
        myDOS->setObjectName("myDOS");
        myDOS->setFont(font);
        myDOS->setStyleSheet(QString::fromUtf8("color: rgb(140, 96, 115);"));

        gridLayout->addWidget(myDOS, 2, 0, 1, 3);

        atariDOS = new QRadioButton(groupBox);
        atariDOS->setObjectName("atariDOS");
        atariDOS->setFont(font);
        atariDOS->setStyleSheet(QString::fromUtf8("color: rgb(140, 96, 115);"));
        atariDOS->setChecked(true);

        gridLayout->addWidget(atariDOS, 1, 0, 1, 3);

        dosXL = new QRadioButton(groupBox);
        dosXL->setObjectName("dosXL");
        dosXL->setStyleSheet(QString::fromUtf8("color: rgb(140, 96, 115);"));

        gridLayout->addWidget(dosXL, 3, 0, 1, 3);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName("label_2");
        QFont font1;
        font1.setBold(false);
        font1.setItalic(true);
        label_2->setFont(font1);

        gridLayout->addWidget(label_2, 8, 0, 2, 3);

        disablePicoHiSpeed = new QCheckBox(groupBox);
        disablePicoHiSpeed->setObjectName("disablePicoHiSpeed");
        disablePicoHiSpeed->setEnabled(false);
        QFont font2;
        font2.setBold(false);
        disablePicoHiSpeed->setFont(font2);

        gridLayout->addWidget(disablePicoHiSpeed, 7, 0, 1, 2);


        retranslateUi(BootOptionsDialog);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, BootOptionsDialog, qOverload<>(&QDialog::accept));
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, BootOptionsDialog, qOverload<>(&QDialog::reject));

        QMetaObject::connectSlotsByName(BootOptionsDialog);
    } // setupUi

    void retranslateUi(QDialog *BootOptionsDialog)
    {
        BootOptionsDialog->setWindowTitle(QCoreApplication::translate("BootOptionsDialog", "Folder Boot Options", nullptr));
        groupBox->setTitle(QCoreApplication::translate("BootOptionsDialog", "Folder Image Boot Options", nullptr));
        label->setText(QCoreApplication::translate("BootOptionsDialog", "Select the DOS you want to boot your Atari with", nullptr));
        smartDOS->setText(QCoreApplication::translate("BootOptionsDialog", "SmartDOS 6.1D", nullptr));
        spartaDOS->setText(QCoreApplication::translate("BootOptionsDialog", "SpartaDOS 3.2G", nullptr));
        myPicoDOS->setText(QCoreApplication::translate("BootOptionsDialog", "MyPicoDOS 4.05 (Standard)", nullptr));
        myDOS->setText(QCoreApplication::translate("BootOptionsDialog", "MyDOS 4.x", nullptr));
        atariDOS->setText(QCoreApplication::translate("BootOptionsDialog", "AtariDOS 2.x", nullptr));
        dosXL->setText(QCoreApplication::translate("BootOptionsDialog", "DosXL 2.x", nullptr));
        label_2->setText(QCoreApplication::translate("BootOptionsDialog", "(Check if you're already using a high-speed OS)", nullptr));
        disablePicoHiSpeed->setText(QCoreApplication::translate("BootOptionsDialog", "Disable high speed SIO", nullptr));
    } // retranslateUi

};

namespace Ui {
    class BootOptionsDialog: public Ui_BootOptionsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BOOTOPTIONSDIALOG_H
