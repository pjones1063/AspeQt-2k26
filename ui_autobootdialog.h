/********************************************************************************
** Form generated from reading UI file 'autobootdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_AUTOBOOTDIALOG_H
#define UI_AUTOBOOTDIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_AutoBootDialog
{
public:
    QProgressBar *progressBar;
    QLabel *label;
    QPushButton *reloadButton;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *AutoBootDialog)
    {
        if (AutoBootDialog->objectName().isEmpty())
            AutoBootDialog->setObjectName(QString::fromUtf8("AutoBootDialog"));
        AutoBootDialog->resize(400, 140);
        AutoBootDialog->setMinimumSize(QSize(400, 140));
        AutoBootDialog->setMaximumSize(QSize(400, 140));
        progressBar = new QProgressBar(AutoBootDialog);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setGeometry(QRect(9, 9, 380, 21));
        progressBar->setMinimumSize(QSize(380, 21));
        progressBar->setMaximumSize(QSize(380, 21));
        progressBar->setValue(0);
        progressBar->setAlignment(Qt::AlignCenter);
        progressBar->setInvertedAppearance(false);
        label = new QLabel(AutoBootDialog);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(9, 36, 381, 66));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy);
        label->setMinimumSize(QSize(381, 66));
        label->setMaximumSize(QSize(381, 66));
        label->setScaledContents(false);
        label->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        label->setWordWrap(true);
        reloadButton = new QPushButton(AutoBootDialog);
        reloadButton->setObjectName(QString::fromUtf8("reloadButton"));
        reloadButton->setGeometry(QRect(9, 108, 75, 23));
        reloadButton->setMaximumSize(QSize(75, 23));
        buttonBox = new QDialogButtonBox(AutoBootDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(310, 110, 75, 23));
        buttonBox->setMaximumSize(QSize(75, 23));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel);

        retranslateUi(AutoBootDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), AutoBootDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), AutoBootDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(AutoBootDialog);
    } // setupUi

    void retranslateUi(QDialog *AutoBootDialog)
    {
        AutoBootDialog->setWindowTitle(QCoreApplication::translate("AutoBootDialog", "Boot executable", nullptr));
        label->setText(QCoreApplication::translate("AutoBootDialog", "Please reboot your Atari.\n"
"\n"
"Don't forget to remove any cartridges and disable BASIC by holding down the Atari Option button", nullptr));
#if QT_CONFIG(tooltip)
        reloadButton->setToolTip(QCoreApplication::translate("AutoBootDialog", "Use this button to re-load the executable \n"
"if it has been changed since the last re-boot \n"
"of your Atari computer - Useful for developers ", nullptr));
#endif // QT_CONFIG(tooltip)
        reloadButton->setText(QCoreApplication::translate("AutoBootDialog", "Reload", nullptr));
    } // retranslateUi

};

namespace Ui {
    class AutoBootDialog: public Ui_AutoBootDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_AUTOBOOTDIALOG_H
