/********************************************************************************
** Form generated from reading UI file 'optionsdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OPTIONSDIALOG_H
#define UI_OPTIONSDIALOG_H

#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_OptionsDialog
{
public:
    QTreeWidget *treeWidget;
    QStackedWidget *stackedWidget;
    QWidget *page_serialPort;
    QVBoxLayout *verticalLayout_2;
    QGroupBox *serialPortGroup;
    QVBoxLayout *verticalLayout_3;
    QCheckBox *serialPortBox;
    QLabel *serialPortDeviceNameLabel;
    QComboBox *serialPortComboBox;
    QLabel *serialPortHandshakeLabel;
    QComboBox *serialPortHandshakeCombo;
    QCheckBox *serialPortFallingEdge;
    QLabel *serialPortWriteDelayLabel;
    QComboBox *serialPortWriteDelayCombo;
    QLabel *serialPortBaudLabel;
    QComboBox *serialPortBaudCombo;
    QCheckBox *serialPortUseDivisorsBox;
    QLabel *serialPortDivisorLabel;
    QSpinBox *serialPortDivisorEdit;
    QLabel *serialPortCompErrDelayLabel;
    QSpinBox *serialPortCompErrDelayBox;
    QSpacerItem *serialPortSpacer1;
    QWidget *page_atariSio;
    QVBoxLayout *verticalLayout_4;
    QGroupBox *atariSioGroup;
    QVBoxLayout *verticalLayout_5;
    QCheckBox *atariSioBox;
    QLabel *atariSioDriverNameLabel;
    QLineEdit *atariSioDriverNameEdit;
    QLabel *atariSioHandshakingMethodLabel;
    QComboBox *atariSioHandshakingMethodCombo;
    QSpacerItem *atariSioSpacer1;
    QWidget *page_emulation;
    QVBoxLayout *verticalLayout_10;
    QGroupBox *emulationGroup;
    QGridLayout *gridLayout_3;
    QSpacerItem *emulationSpacer1;
    QCheckBox *emulationUseCustomCasBaudBox;
    QSpacerItem *verticalSpacer_2;
    QLabel *atariSioDriverNameLabel_2;
    QCheckBox *URLSubmit;
    QLabel *label;
    QToolButton *buttonRclFolder;
    QCheckBox *emulationHighSpeedExeLoaderBox;
    QLabel *label_3;
    QCheckBox *capitalLettersPCLINK;
    QLabel *label_4;
    QLineEdit *RclNameEdit;
    QSpinBox *emulationCustomCasBaudSpin;
    QLabel *atariSioDriverNameLabel_3;
    QCheckBox *filterUscore;
    QLabel *label_2;
    QSpacerItem *emulationSpacer2;
    QLineEdit *RclCommand;
    QWidget *page_i18n;
    QGridLayout *gridLayout;
    QGroupBox *uiGroup;
    QGridLayout *gridLayout_2;
    QLabel *i18nLanguageLabel;
    QComboBox *i18nLanguageCombo;
    QCheckBox *minimizeToTrayBox;
    QCheckBox *saveWinPosBox;
    QSpacerItem *i18nSpacer2;
    QSpacerItem *verticalSpacer;
    QCheckBox *saveDiskVisBox;
    QCheckBox *enableShade;
    QCheckBox *useLargerFont;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *OptionsDialog)
    {
        if (OptionsDialog->objectName().isEmpty())
            OptionsDialog->setObjectName(QString::fromUtf8("OptionsDialog"));
        OptionsDialog->setWindowModality(Qt::ApplicationModal);
        OptionsDialog->resize(638, 465);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(OptionsDialog->sizePolicy().hasHeightForWidth());
        OptionsDialog->setSizePolicy(sizePolicy);
        OptionsDialog->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
        OptionsDialog->setModal(true);
        treeWidget = new QTreeWidget(OptionsDialog);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem(treeWidget);
        __qtreewidgetitem->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        QTreeWidgetItem *__qtreewidgetitem1 = new QTreeWidgetItem(__qtreewidgetitem);
        __qtreewidgetitem1->setCheckState(0, Qt::Checked);
        QTreeWidgetItem *__qtreewidgetitem2 = new QTreeWidgetItem(__qtreewidgetitem);
        __qtreewidgetitem2->setCheckState(0, Qt::Unchecked);
        new QTreeWidgetItem(treeWidget);
        new QTreeWidgetItem(treeWidget);
        treeWidget->setObjectName(QString::fromUtf8("treeWidget"));
        treeWidget->setGeometry(QRect(9, 9, 256, 361));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(treeWidget->sizePolicy().hasHeightForWidth());
        treeWidget->setSizePolicy(sizePolicy1);
        treeWidget->setHeaderHidden(true);
        stackedWidget = new QStackedWidget(OptionsDialog);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setGeometry(QRect(306, 9, 320, 400));
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(stackedWidget->sizePolicy().hasHeightForWidth());
        stackedWidget->setSizePolicy(sizePolicy2);
        page_serialPort = new QWidget();
        page_serialPort->setObjectName(QString::fromUtf8("page_serialPort"));
        verticalLayout_2 = new QVBoxLayout(page_serialPort);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        serialPortGroup = new QGroupBox(page_serialPort);
        serialPortGroup->setObjectName(QString::fromUtf8("serialPortGroup"));
        verticalLayout_3 = new QVBoxLayout(serialPortGroup);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        serialPortBox = new QCheckBox(serialPortGroup);
        serialPortBox->setObjectName(QString::fromUtf8("serialPortBox"));
        serialPortBox->setEnabled(false);
        serialPortBox->setCheckable(true);
        serialPortBox->setChecked(true);

        verticalLayout_3->addWidget(serialPortBox);

        serialPortDeviceNameLabel = new QLabel(serialPortGroup);
        serialPortDeviceNameLabel->setObjectName(QString::fromUtf8("serialPortDeviceNameLabel"));
        QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(serialPortDeviceNameLabel->sizePolicy().hasHeightForWidth());
        serialPortDeviceNameLabel->setSizePolicy(sizePolicy3);

        verticalLayout_3->addWidget(serialPortDeviceNameLabel);

        serialPortComboBox = new QComboBox(serialPortGroup);
        serialPortComboBox->setObjectName(QString::fromUtf8("serialPortComboBox"));

        verticalLayout_3->addWidget(serialPortComboBox);

        serialPortHandshakeLabel = new QLabel(serialPortGroup);
        serialPortHandshakeLabel->setObjectName(QString::fromUtf8("serialPortHandshakeLabel"));
        QSizePolicy sizePolicy4(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(serialPortHandshakeLabel->sizePolicy().hasHeightForWidth());
        serialPortHandshakeLabel->setSizePolicy(sizePolicy4);

        verticalLayout_3->addWidget(serialPortHandshakeLabel);

        serialPortHandshakeCombo = new QComboBox(serialPortGroup);
        serialPortHandshakeCombo->addItem(QString());
        serialPortHandshakeCombo->addItem(QString());
        serialPortHandshakeCombo->addItem(QString());
        serialPortHandshakeCombo->addItem(QString());
        serialPortHandshakeCombo->addItem(QString());
        serialPortHandshakeCombo->setObjectName(QString::fromUtf8("serialPortHandshakeCombo"));

        verticalLayout_3->addWidget(serialPortHandshakeCombo);

        serialPortFallingEdge = new QCheckBox(serialPortGroup);
        serialPortFallingEdge->setObjectName(QString::fromUtf8("serialPortFallingEdge"));

        verticalLayout_3->addWidget(serialPortFallingEdge);

        serialPortWriteDelayLabel = new QLabel(serialPortGroup);
        serialPortWriteDelayLabel->setObjectName(QString::fromUtf8("serialPortWriteDelayLabel"));
        serialPortWriteDelayLabel->setEnabled(true);
        sizePolicy4.setHeightForWidth(serialPortWriteDelayLabel->sizePolicy().hasHeightForWidth());
        serialPortWriteDelayLabel->setSizePolicy(sizePolicy4);

        verticalLayout_3->addWidget(serialPortWriteDelayLabel);

        serialPortWriteDelayCombo = new QComboBox(serialPortGroup);
        serialPortWriteDelayCombo->addItem(QString());
        serialPortWriteDelayCombo->addItem(QString());
        serialPortWriteDelayCombo->addItem(QString());
        serialPortWriteDelayCombo->addItem(QString());
        serialPortWriteDelayCombo->addItem(QString());
        serialPortWriteDelayCombo->addItem(QString());
        serialPortWriteDelayCombo->addItem(QString());
        serialPortWriteDelayCombo->setObjectName(QString::fromUtf8("serialPortWriteDelayCombo"));
        serialPortWriteDelayCombo->setEnabled(true);

        verticalLayout_3->addWidget(serialPortWriteDelayCombo);

        serialPortBaudLabel = new QLabel(serialPortGroup);
        serialPortBaudLabel->setObjectName(QString::fromUtf8("serialPortBaudLabel"));
        sizePolicy4.setHeightForWidth(serialPortBaudLabel->sizePolicy().hasHeightForWidth());
        serialPortBaudLabel->setSizePolicy(sizePolicy4);

        verticalLayout_3->addWidget(serialPortBaudLabel);

        serialPortBaudCombo = new QComboBox(serialPortGroup);
        serialPortBaudCombo->addItem(QString());
        serialPortBaudCombo->addItem(QString());
        serialPortBaudCombo->addItem(QString());
        serialPortBaudCombo->setObjectName(QString::fromUtf8("serialPortBaudCombo"));

        verticalLayout_3->addWidget(serialPortBaudCombo);

        serialPortUseDivisorsBox = new QCheckBox(serialPortGroup);
        serialPortUseDivisorsBox->setObjectName(QString::fromUtf8("serialPortUseDivisorsBox"));

        verticalLayout_3->addWidget(serialPortUseDivisorsBox);

        serialPortDivisorLabel = new QLabel(serialPortGroup);
        serialPortDivisorLabel->setObjectName(QString::fromUtf8("serialPortDivisorLabel"));
        serialPortDivisorLabel->setEnabled(false);
        sizePolicy4.setHeightForWidth(serialPortDivisorLabel->sizePolicy().hasHeightForWidth());
        serialPortDivisorLabel->setSizePolicy(sizePolicy4);

        verticalLayout_3->addWidget(serialPortDivisorLabel);

        serialPortDivisorEdit = new QSpinBox(serialPortGroup);
        serialPortDivisorEdit->setObjectName(QString::fromUtf8("serialPortDivisorEdit"));
        serialPortDivisorEdit->setEnabled(false);
        serialPortDivisorEdit->setMaximum(40);
        serialPortDivisorEdit->setValue(6);

        verticalLayout_3->addWidget(serialPortDivisorEdit);

        serialPortCompErrDelayLabel = new QLabel(serialPortGroup);
        serialPortCompErrDelayLabel->setObjectName(QString::fromUtf8("serialPortCompErrDelayLabel"));

        verticalLayout_3->addWidget(serialPortCompErrDelayLabel);

        serialPortCompErrDelayBox = new QSpinBox(serialPortGroup);
        serialPortCompErrDelayBox->setObjectName(QString::fromUtf8("serialPortCompErrDelayBox"));
        serialPortCompErrDelayBox->setMinimum(250);
        serialPortCompErrDelayBox->setMaximum(2000);
        serialPortCompErrDelayBox->setSingleStep(10);
        serialPortCompErrDelayBox->setValue(800);

        verticalLayout_3->addWidget(serialPortCompErrDelayBox);

        serialPortSpacer1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(serialPortSpacer1);


        verticalLayout_2->addWidget(serialPortGroup);

        stackedWidget->addWidget(page_serialPort);
        page_atariSio = new QWidget();
        page_atariSio->setObjectName(QString::fromUtf8("page_atariSio"));
        verticalLayout_4 = new QVBoxLayout(page_atariSio);
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        atariSioGroup = new QGroupBox(page_atariSio);
        atariSioGroup->setObjectName(QString::fromUtf8("atariSioGroup"));
        verticalLayout_5 = new QVBoxLayout(atariSioGroup);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        atariSioBox = new QCheckBox(atariSioGroup);
        atariSioBox->setObjectName(QString::fromUtf8("atariSioBox"));
        atariSioBox->setEnabled(false);

        verticalLayout_5->addWidget(atariSioBox);

        atariSioDriverNameLabel = new QLabel(atariSioGroup);
        atariSioDriverNameLabel->setObjectName(QString::fromUtf8("atariSioDriverNameLabel"));
        sizePolicy3.setHeightForWidth(atariSioDriverNameLabel->sizePolicy().hasHeightForWidth());
        atariSioDriverNameLabel->setSizePolicy(sizePolicy3);

        verticalLayout_5->addWidget(atariSioDriverNameLabel);

        atariSioDriverNameEdit = new QLineEdit(atariSioGroup);
        atariSioDriverNameEdit->setObjectName(QString::fromUtf8("atariSioDriverNameEdit"));

        verticalLayout_5->addWidget(atariSioDriverNameEdit);

        atariSioHandshakingMethodLabel = new QLabel(atariSioGroup);
        atariSioHandshakingMethodLabel->setObjectName(QString::fromUtf8("atariSioHandshakingMethodLabel"));
        sizePolicy4.setHeightForWidth(atariSioHandshakingMethodLabel->sizePolicy().hasHeightForWidth());
        atariSioHandshakingMethodLabel->setSizePolicy(sizePolicy4);

        verticalLayout_5->addWidget(atariSioHandshakingMethodLabel);

        atariSioHandshakingMethodCombo = new QComboBox(atariSioGroup);
        atariSioHandshakingMethodCombo->addItem(QString());
        atariSioHandshakingMethodCombo->addItem(QString());
        atariSioHandshakingMethodCombo->addItem(QString());
        atariSioHandshakingMethodCombo->setObjectName(QString::fromUtf8("atariSioHandshakingMethodCombo"));

        verticalLayout_5->addWidget(atariSioHandshakingMethodCombo);

        atariSioSpacer1 = new QSpacerItem(20, 148, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(atariSioSpacer1);


        verticalLayout_4->addWidget(atariSioGroup);

        stackedWidget->addWidget(page_atariSio);
        page_emulation = new QWidget();
        page_emulation->setObjectName(QString::fromUtf8("page_emulation"));
        verticalLayout_10 = new QVBoxLayout(page_emulation);
        verticalLayout_10->setSpacing(0);
        verticalLayout_10->setObjectName(QString::fromUtf8("verticalLayout_10"));
        verticalLayout_10->setContentsMargins(0, 0, 0, 0);
        emulationGroup = new QGroupBox(page_emulation);
        emulationGroup->setObjectName(QString::fromUtf8("emulationGroup"));
        sizePolicy1.setHeightForWidth(emulationGroup->sizePolicy().hasHeightForWidth());
        emulationGroup->setSizePolicy(sizePolicy1);
        emulationGroup->setMinimumSize(QSize(300, 0));
        gridLayout_3 = new QGridLayout(emulationGroup);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        emulationSpacer1 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout_3->addItem(emulationSpacer1, 1, 1, 1, 1);

        emulationUseCustomCasBaudBox = new QCheckBox(emulationGroup);
        emulationUseCustomCasBaudBox->setObjectName(QString::fromUtf8("emulationUseCustomCasBaudBox"));
        QSizePolicy sizePolicy5(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(emulationUseCustomCasBaudBox->sizePolicy().hasHeightForWidth());
        emulationUseCustomCasBaudBox->setSizePolicy(sizePolicy5);
        emulationUseCustomCasBaudBox->setMinimumSize(QSize(300, 0));

        gridLayout_3->addWidget(emulationUseCustomCasBaudBox, 2, 0, 1, 2);

        verticalSpacer_2 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout_3->addItem(verticalSpacer_2, 4, 1, 1, 1);

        atariSioDriverNameLabel_2 = new QLabel(emulationGroup);
        atariSioDriverNameLabel_2->setObjectName(QString::fromUtf8("atariSioDriverNameLabel_2"));
        sizePolicy3.setHeightForWidth(atariSioDriverNameLabel_2->sizePolicy().hasHeightForWidth());
        atariSioDriverNameLabel_2->setSizePolicy(sizePolicy3);

        gridLayout_3->addWidget(atariSioDriverNameLabel_2, 12, 0, 1, 1);

        URLSubmit = new QCheckBox(emulationGroup);
        URLSubmit->setObjectName(QString::fromUtf8("URLSubmit"));

        gridLayout_3->addWidget(URLSubmit, 11, 0, 1, 1);

        label = new QLabel(emulationGroup);
        label->setObjectName(QString::fromUtf8("label"));
        label->setMinimumSize(QSize(0, 0));
        QFont font;
        font.setItalic(false);
        label->setFont(font);

        gridLayout_3->addWidget(label, 5, 0, 1, 1);

        buttonRclFolder = new QToolButton(emulationGroup);
        buttonRclFolder->setObjectName(QString::fromUtf8("buttonRclFolder"));
        buttonRclFolder->setEnabled(true);
        buttonRclFolder->setMinimumSize(QSize(28, 28));
        buttonRclFolder->setMaximumSize(QSize(28, 28));

        gridLayout_3->addWidget(buttonRclFolder, 13, 1, 1, 1);

        emulationHighSpeedExeLoaderBox = new QCheckBox(emulationGroup);
        emulationHighSpeedExeLoaderBox->setObjectName(QString::fromUtf8("emulationHighSpeedExeLoaderBox"));
        emulationHighSpeedExeLoaderBox->setMinimumSize(QSize(300, 0));
        emulationHighSpeedExeLoaderBox->setChecked(true);

        gridLayout_3->addWidget(emulationHighSpeedExeLoaderBox, 0, 0, 1, 2);

        label_3 = new QLabel(emulationGroup);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout_3->addWidget(label_3, 8, 0, 1, 1);

        capitalLettersPCLINK = new QCheckBox(emulationGroup);
        capitalLettersPCLINK->setObjectName(QString::fromUtf8("capitalLettersPCLINK"));

        gridLayout_3->addWidget(capitalLettersPCLINK, 9, 0, 1, 1);

        label_4 = new QLabel(emulationGroup);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gridLayout_3->addWidget(label_4, 10, 0, 1, 1);

        RclNameEdit = new QLineEdit(emulationGroup);
        RclNameEdit->setObjectName(QString::fromUtf8("RclNameEdit"));
        RclNameEdit->setEnabled(true);
        RclNameEdit->setReadOnly(true);

        gridLayout_3->addWidget(RclNameEdit, 13, 0, 1, 1);

        emulationCustomCasBaudSpin = new QSpinBox(emulationGroup);
        emulationCustomCasBaudSpin->setObjectName(QString::fromUtf8("emulationCustomCasBaudSpin"));
        emulationCustomCasBaudSpin->setEnabled(false);
        emulationCustomCasBaudSpin->setMinimumSize(QSize(230, 0));
        emulationCustomCasBaudSpin->setMinimum(425);
        emulationCustomCasBaudSpin->setMaximum(875);
        emulationCustomCasBaudSpin->setValue(600);

        gridLayout_3->addWidget(emulationCustomCasBaudSpin, 3, 0, 1, 2);

        atariSioDriverNameLabel_3 = new QLabel(emulationGroup);
        atariSioDriverNameLabel_3->setObjectName(QString::fromUtf8("atariSioDriverNameLabel_3"));
        sizePolicy3.setHeightForWidth(atariSioDriverNameLabel_3->sizePolicy().hasHeightForWidth());
        atariSioDriverNameLabel_3->setSizePolicy(sizePolicy3);

        gridLayout_3->addWidget(atariSioDriverNameLabel_3, 15, 0, 1, 1);

        filterUscore = new QCheckBox(emulationGroup);
        filterUscore->setObjectName(QString::fromUtf8("filterUscore"));
        filterUscore->setMinimumSize(QSize(300, 0));
        filterUscore->setMaximumSize(QSize(16777215, 16777215));
        filterUscore->setChecked(true);

        gridLayout_3->addWidget(filterUscore, 6, 0, 1, 2);

        label_2 = new QLabel(emulationGroup);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setMinimumSize(QSize(300, 0));
        QFont font1;
        font1.setItalic(true);
        label_2->setFont(font1);

        gridLayout_3->addWidget(label_2, 7, 0, 1, 2);

        emulationSpacer2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_3->addItem(emulationSpacer2, 14, 1, 1, 1);

        RclCommand = new QLineEdit(emulationGroup);
        RclCommand->setObjectName(QString::fromUtf8("RclCommand"));
        RclCommand->setEnabled(true);

        gridLayout_3->addWidget(RclCommand, 16, 0, 1, 2);


        verticalLayout_10->addWidget(emulationGroup);

        stackedWidget->addWidget(page_emulation);
        page_i18n = new QWidget();
        page_i18n->setObjectName(QString::fromUtf8("page_i18n"));
        gridLayout = new QGridLayout(page_i18n);
        gridLayout->setSpacing(0);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        uiGroup = new QGroupBox(page_i18n);
        uiGroup->setObjectName(QString::fromUtf8("uiGroup"));
        uiGroup->setMinimumSize(QSize(258, 208));
        gridLayout_2 = new QGridLayout(uiGroup);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        i18nLanguageLabel = new QLabel(uiGroup);
        i18nLanguageLabel->setObjectName(QString::fromUtf8("i18nLanguageLabel"));

        gridLayout_2->addWidget(i18nLanguageLabel, 0, 0, 1, 1);

        i18nLanguageCombo = new QComboBox(uiGroup);
        i18nLanguageCombo->setObjectName(QString::fromUtf8("i18nLanguageCombo"));

        gridLayout_2->addWidget(i18nLanguageCombo, 1, 0, 1, 1);

        minimizeToTrayBox = new QCheckBox(uiGroup);
        minimizeToTrayBox->setObjectName(QString::fromUtf8("minimizeToTrayBox"));

        gridLayout_2->addWidget(minimizeToTrayBox, 4, 0, 1, 1);

        saveWinPosBox = new QCheckBox(uiGroup);
        saveWinPosBox->setObjectName(QString::fromUtf8("saveWinPosBox"));
        saveWinPosBox->setChecked(true);

        gridLayout_2->addWidget(saveWinPosBox, 5, 0, 1, 1);

        i18nSpacer2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_2->addItem(i18nSpacer2, 9, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout_2->addItem(verticalSpacer, 3, 0, 1, 1);

        saveDiskVisBox = new QCheckBox(uiGroup);
        saveDiskVisBox->setObjectName(QString::fromUtf8("saveDiskVisBox"));
        saveDiskVisBox->setChecked(true);

        gridLayout_2->addWidget(saveDiskVisBox, 6, 0, 1, 1);

        enableShade = new QCheckBox(uiGroup);
        enableShade->setObjectName(QString::fromUtf8("enableShade"));
        enableShade->setChecked(true);

        gridLayout_2->addWidget(enableShade, 7, 0, 1, 1);

        useLargerFont = new QCheckBox(uiGroup);
        useLargerFont->setObjectName(QString::fromUtf8("useLargerFont"));
        useLargerFont->setEnabled(true);
        useLargerFont->setChecked(false);

        gridLayout_2->addWidget(useLargerFont, 8, 0, 1, 1);


        gridLayout->addWidget(uiGroup, 0, 0, 1, 1);

        stackedWidget->addWidget(page_i18n);
        buttonBox = new QDialogButtonBox(OptionsDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(10, 430, 156, 23));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Save);

        retranslateUi(OptionsDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), OptionsDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), OptionsDialog, SLOT(reject()));

        stackedWidget->setCurrentIndex(2);
        serialPortWriteDelayCombo->setCurrentIndex(1);
        serialPortBaudCombo->setCurrentIndex(2);


        QMetaObject::connectSlotsByName(OptionsDialog);
    } // setupUi

    void retranslateUi(QDialog *OptionsDialog)
    {
        OptionsDialog->setWindowTitle(QCoreApplication::translate("OptionsDialog", "Options", nullptr));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(0, QCoreApplication::translate("OptionsDialog", "1", nullptr));

        const bool __sortingEnabled = treeWidget->isSortingEnabled();
        treeWidget->setSortingEnabled(false);
        QTreeWidgetItem *___qtreewidgetitem1 = treeWidget->topLevelItem(0);
        ___qtreewidgetitem1->setText(0, QCoreApplication::translate("OptionsDialog", "Serial I/O backends", nullptr));
        QTreeWidgetItem *___qtreewidgetitem2 = ___qtreewidgetitem1->child(0);
        ___qtreewidgetitem2->setText(0, QCoreApplication::translate("OptionsDialog", "Standard serial port", nullptr));
        QTreeWidgetItem *___qtreewidgetitem3 = ___qtreewidgetitem1->child(1);
        ___qtreewidgetitem3->setText(0, QCoreApplication::translate("OptionsDialog", "AtariSIO", nullptr));
        QTreeWidgetItem *___qtreewidgetitem4 = treeWidget->topLevelItem(1);
        ___qtreewidgetitem4->setText(0, QCoreApplication::translate("OptionsDialog", "Emulation", nullptr));
        QTreeWidgetItem *___qtreewidgetitem5 = treeWidget->topLevelItem(2);
        ___qtreewidgetitem5->setText(0, QCoreApplication::translate("OptionsDialog", "User interface", nullptr));
        treeWidget->setSortingEnabled(__sortingEnabled);

        serialPortGroup->setTitle(QCoreApplication::translate("OptionsDialog", "Standard serial port backend options", nullptr));
        serialPortBox->setText(QCoreApplication::translate("OptionsDialog", "Use this backend", nullptr));
        serialPortDeviceNameLabel->setText(QCoreApplication::translate("OptionsDialog", "Port name:", nullptr));
        serialPortHandshakeLabel->setText(QCoreApplication::translate("OptionsDialog", "Handshake method:", nullptr));
        serialPortHandshakeCombo->setItemText(0, QCoreApplication::translate("OptionsDialog", "RI", nullptr));
        serialPortHandshakeCombo->setItemText(1, QCoreApplication::translate("OptionsDialog", "DSR", nullptr));
        serialPortHandshakeCombo->setItemText(2, QCoreApplication::translate("OptionsDialog", "CTS", nullptr));
        serialPortHandshakeCombo->setItemText(3, QCoreApplication::translate("OptionsDialog", "NONE", nullptr));
        serialPortHandshakeCombo->setItemText(4, QCoreApplication::translate("OptionsDialog", "SOFTWARE (SIO2BT)", nullptr));

        serialPortFallingEdge->setText(QCoreApplication::translate("OptionsDialog", "Trigger on falling edge", nullptr));
        serialPortWriteDelayLabel->setText(QCoreApplication::translate("OptionsDialog", "Write delay [ms]:", nullptr));
        serialPortWriteDelayCombo->setItemText(0, QCoreApplication::translate("OptionsDialog", "0", nullptr));
        serialPortWriteDelayCombo->setItemText(1, QCoreApplication::translate("OptionsDialog", "10", nullptr));
        serialPortWriteDelayCombo->setItemText(2, QCoreApplication::translate("OptionsDialog", "20", nullptr));
        serialPortWriteDelayCombo->setItemText(3, QCoreApplication::translate("OptionsDialog", "30", nullptr));
        serialPortWriteDelayCombo->setItemText(4, QCoreApplication::translate("OptionsDialog", "40", nullptr));
        serialPortWriteDelayCombo->setItemText(5, QCoreApplication::translate("OptionsDialog", "50", nullptr));
        serialPortWriteDelayCombo->setItemText(6, QCoreApplication::translate("OptionsDialog", "60", nullptr));

        serialPortBaudLabel->setText(QCoreApplication::translate("OptionsDialog", "High speed mode baud rate:", nullptr));
        serialPortBaudCombo->setItemText(0, QCoreApplication::translate("OptionsDialog", "19200 (1x)", nullptr));
        serialPortBaudCombo->setItemText(1, QCoreApplication::translate("OptionsDialog", "38400 (2x)", nullptr));
        serialPortBaudCombo->setItemText(2, QCoreApplication::translate("OptionsDialog", "57600 (3x)", nullptr));

        serialPortUseDivisorsBox->setText(QCoreApplication::translate("OptionsDialog", "Use non-standard speeds", nullptr));
        serialPortDivisorLabel->setText(QCoreApplication::translate("OptionsDialog", "High speed mode POKEY divisor:", nullptr));
        serialPortCompErrDelayLabel->setText(QCoreApplication::translate("OptionsDialog", "<html><head/><body>Complete/Error response delay (\316\274s)<br>See manual for more information</body></html>", nullptr));
        serialPortCompErrDelayBox->setSuffix(QCoreApplication::translate("OptionsDialog", "\316\274s", nullptr));
        atariSioGroup->setTitle(QCoreApplication::translate("OptionsDialog", "AtariSIO backend options", nullptr));
        atariSioBox->setText(QCoreApplication::translate("OptionsDialog", "Use this backend", nullptr));
        atariSioDriverNameLabel->setText(QCoreApplication::translate("OptionsDialog", "Device name:", nullptr));
        atariSioDriverNameEdit->setText(QCoreApplication::translate("OptionsDialog", "/dev/atarisio0", nullptr));
        atariSioHandshakingMethodLabel->setText(QCoreApplication::translate("OptionsDialog", "Handshake method:", nullptr));
        atariSioHandshakingMethodCombo->setItemText(0, QCoreApplication::translate("OptionsDialog", "RI", nullptr));
        atariSioHandshakingMethodCombo->setItemText(1, QCoreApplication::translate("OptionsDialog", "DSR", nullptr));
        atariSioHandshakingMethodCombo->setItemText(2, QCoreApplication::translate("OptionsDialog", "CTS", nullptr));

        emulationGroup->setTitle(QCoreApplication::translate("OptionsDialog", "Emulation", nullptr));
        emulationUseCustomCasBaudBox->setText(QCoreApplication::translate("OptionsDialog", "Use custom baud rate for cassette emulation", nullptr));
        atariSioDriverNameLabel_2->setText(QCoreApplication::translate("OptionsDialog", "AspeQt Home Folder:", nullptr));
        URLSubmit->setText(QCoreApplication::translate("OptionsDialog", "URL Submit", nullptr));
        label->setText(QCoreApplication::translate("OptionsDialog", "Folder Images:", nullptr));
        emulationHighSpeedExeLoaderBox->setText(QCoreApplication::translate("OptionsDialog", "Use high speed executable loader", nullptr));
        label_3->setText(QCoreApplication::translate("OptionsDialog", "PCLINK:", nullptr));
        capitalLettersPCLINK->setText(QCoreApplication::translate("OptionsDialog", "CAPITAL letters in file names", nullptr));
        label_4->setText(QCoreApplication::translate("OptionsDialog", "Smart Device:", nullptr));
        RclNameEdit->setText(QCoreApplication::translate("OptionsDialog", "/dev/atarisio0", nullptr));
        atariSioDriverNameLabel_3->setText(QCoreApplication::translate("OptionsDialog", "Host Comannd", nullptr));
        filterUscore->setText(QCoreApplication::translate("OptionsDialog", "Filter out underscore character from file names", nullptr));
        label_2->setText(QCoreApplication::translate("OptionsDialog", "        (Required for AtariDOS compatibility)", nullptr));
        RclCommand->setText(QString());
        uiGroup->setTitle(QCoreApplication::translate("OptionsDialog", "User inteface", nullptr));
        i18nLanguageLabel->setText(QCoreApplication::translate("OptionsDialog", "Language:", nullptr));
        minimizeToTrayBox->setText(QCoreApplication::translate("OptionsDialog", "Minimize to system tray", nullptr));
        saveWinPosBox->setText(QCoreApplication::translate("OptionsDialog", "Save window positions and sizes", nullptr));
        saveDiskVisBox->setText(QCoreApplication::translate("OptionsDialog", "Save D9-DO drive visibility status", nullptr));
        enableShade->setText(QCoreApplication::translate("OptionsDialog", "Enable Shade in Mini Mode by default", nullptr));
        useLargerFont->setText(QCoreApplication::translate("OptionsDialog", "Use larger font in drive slot descriptions", nullptr));
#if QT_CONFIG(tooltip)
        buttonBox->setToolTip(QCoreApplication::translate("OptionsDialog", "Save/Commit or Cancel/Ignore changes made to the settings", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        buttonBox->setStatusTip(QString());
#endif // QT_CONFIG(statustip)
    } // retranslateUi

};

namespace Ui {
    class OptionsDialog: public Ui_OptionsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OPTIONSDIALOG_H
