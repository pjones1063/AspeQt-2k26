/********************************************************************************
** Form generated from reading UI file 'cassettedialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CASSETTEDIALOG_H
#define UI_CASSETTEDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_CassetteDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QProgressBar *progressBar;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *CassetteDialog)
    {
        if (CassetteDialog->objectName().isEmpty())
            CassetteDialog->setObjectName(QString::fromUtf8("CassetteDialog"));
        CassetteDialog->setWindowModality(Qt::ApplicationModal);
        CassetteDialog->resize(556, 252);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(CassetteDialog->sizePolicy().hasHeightForWidth());
        CassetteDialog->setSizePolicy(sizePolicy);
        verticalLayout = new QVBoxLayout(CassetteDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        label = new QLabel(CassetteDialog);
        label->setObjectName(QString::fromUtf8("label"));
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);
        label->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        label->setWordWrap(true);

        verticalLayout->addWidget(label);

        progressBar = new QProgressBar(CassetteDialog);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setValue(0);
        progressBar->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(progressBar);

        buttonBox = new QDialogButtonBox(CassetteDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(CassetteDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), CassetteDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), CassetteDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(CassetteDialog);
    } // setupUi

    void retranslateUi(QDialog *CassetteDialog)
    {
        CassetteDialog->setWindowTitle(QCoreApplication::translate("CassetteDialog", "Cassette image playback", nullptr));
        label->setText(QCoreApplication::translate("CassetteDialog", "Do whatever is necessary in your Atari to load this cassette image like rebooting while holding Option and Start buttons or entering \"CLOAD\" in the BASIC prompt.\n"
"\n"
"When you hear the beep sound, push the OK button below and press a key on your Atari at about the same time.", nullptr));
    } // retranslateUi

};

namespace Ui {
    class CassetteDialog: public Ui_CassetteDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CASSETTEDIALOG_H
