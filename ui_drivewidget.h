/********************************************************************************
** Form generated from reading UI file 'drivewidget.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DRIVEWIDGET_H
#define UI_DRIVEWIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_DriveWidget
{
public:
    QAction *actionMountDisk;
    QAction *actionMountFolder;
    QAction *actionEject;
    QAction *actionWriteProtect;
    QAction *actionEditDisk;
    QAction *actionSave;
    QAction *actionRevert;
    QAction *actionSaveAs;
    QAction *actionAutoSave;
    QAction *actionBootOption;
    QHBoxLayout *horizontalLayout;
    QLabel *driveLabel;
    QToolButton *buttonMountDisk;
    QToolButton *buttonMountFolder;
    QToolButton *buttonSave;
    QToolButton *autoSave;
    QToolButton *buttonEditDisk;
    QVBoxLayout *verticalLayout;
    QLabel *labelFileName;
    QLabel *labelImageProperties;
    QToolButton *buttonEject;

    void setupUi(QFrame *DriveWidget)
    {
        if (DriveWidget->objectName().isEmpty())
            DriveWidget->setObjectName(QString::fromUtf8("DriveWidget"));
        DriveWidget->resize(354, 37);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(DriveWidget->sizePolicy().hasHeightForWidth());
        DriveWidget->setSizePolicy(sizePolicy);
        DriveWidget->setMinimumSize(QSize(330, 0));
        DriveWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
        DriveWidget->setAutoFillBackground(false);
        DriveWidget->setFrameShape(QFrame::Panel);
        DriveWidget->setFrameShadow(QFrame::Raised);
        actionMountDisk = new QAction(DriveWidget);
        actionMountDisk->setObjectName(QString::fromUtf8("actionMountDisk"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/silk-icons/icons/disk.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionMountDisk->setIcon(icon);
        actionMountFolder = new QAction(DriveWidget);
        actionMountFolder->setObjectName(QString::fromUtf8("actionMountFolder"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/silk-icons/icons/folder.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionMountFolder->setIcon(icon1);
        actionEject = new QAction(DriveWidget);
        actionEject->setObjectName(QString::fromUtf8("actionEject"));
        actionEject->setEnabled(false);
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/silk-icons/icons/control_eject.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionEject->setIcon(icon2);
        actionWriteProtect = new QAction(DriveWidget);
        actionWriteProtect->setObjectName(QString::fromUtf8("actionWriteProtect"));
        actionWriteProtect->setCheckable(true);
        actionWriteProtect->setEnabled(false);
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/silk-icons/icons/lock_open.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon3.addFile(QString::fromUtf8(":/icons/silk-icons/icons/lock.png"), QSize(), QIcon::Normal, QIcon::On);
        actionWriteProtect->setIcon(icon3);
        actionEditDisk = new QAction(DriveWidget);
        actionEditDisk->setObjectName(QString::fromUtf8("actionEditDisk"));
        actionEditDisk->setCheckable(true);
        actionEditDisk->setEnabled(false);
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/icons/silk-icons/icons/folder_edit.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionEditDisk->setIcon(icon4);
        actionSave = new QAction(DriveWidget);
        actionSave->setObjectName(QString::fromUtf8("actionSave"));
        actionSave->setEnabled(false);
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/icons/silk-icons/icons/drive_disk.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionSave->setIcon(icon5);
        actionRevert = new QAction(DriveWidget);
        actionRevert->setObjectName(QString::fromUtf8("actionRevert"));
        actionRevert->setEnabled(false);
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/icons/silk-icons/icons/arrow_undo.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionRevert->setIcon(icon6);
        actionSaveAs = new QAction(DriveWidget);
        actionSaveAs->setObjectName(QString::fromUtf8("actionSaveAs"));
        actionSaveAs->setEnabled(false);
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/icons/silk-icons/icons/drive_rename.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionSaveAs->setIcon(icon7);
        actionAutoSave = new QAction(DriveWidget);
        actionAutoSave->setObjectName(QString::fromUtf8("actionAutoSave"));
        actionAutoSave->setCheckable(true);
        actionAutoSave->setChecked(false);
        actionAutoSave->setEnabled(false);
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/icons/silk-icons/icons/drive_go.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAutoSave->setIcon(icon8);
        actionBootOption = new QAction(DriveWidget);
        actionBootOption->setObjectName(QString::fromUtf8("actionBootOption"));
        actionBootOption->setEnabled(false);
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/flag_green.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionBootOption->setIcon(icon9);
        horizontalLayout = new QHBoxLayout(DriveWidget);
        horizontalLayout->setSpacing(3);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(3, 3, 3, 3);
        driveLabel = new QLabel(DriveWidget);
        driveLabel->setObjectName(QString::fromUtf8("driveLabel"));
        driveLabel->setEnabled(true);
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(driveLabel->sizePolicy().hasHeightForWidth());
        driveLabel->setSizePolicy(sizePolicy1);
        QPalette palette;
        QBrush brush(QColor(104, 104, 104, 255));
        brush.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::WindowText, brush);
        QBrush brush1(QColor(0, 0, 0, 255));
        brush1.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Text, brush1);
        palette.setBrush(QPalette::Inactive, QPalette::WindowText, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Text, brush1);
        QBrush brush2(QColor(120, 120, 120, 255));
        brush2.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Disabled, QPalette::WindowText, brush2);
        palette.setBrush(QPalette::Disabled, QPalette::Text, brush2);
        driveLabel->setPalette(palette);
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        driveLabel->setFont(font);
        driveLabel->setMargin(5);

        horizontalLayout->addWidget(driveLabel);

        buttonMountDisk = new QToolButton(DriveWidget);
        buttonMountDisk->setObjectName(QString::fromUtf8("buttonMountDisk"));
        buttonMountDisk->setEnabled(true);
        buttonMountDisk->setMinimumSize(QSize(28, 28));
        buttonMountDisk->setMaximumSize(QSize(28, 28));

        horizontalLayout->addWidget(buttonMountDisk);

        buttonMountFolder = new QToolButton(DriveWidget);
        buttonMountFolder->setObjectName(QString::fromUtf8("buttonMountFolder"));
        buttonMountFolder->setEnabled(true);
        buttonMountFolder->setMinimumSize(QSize(28, 28));
        buttonMountFolder->setMaximumSize(QSize(28, 28));

        horizontalLayout->addWidget(buttonMountFolder);

        buttonSave = new QToolButton(DriveWidget);
        buttonSave->setObjectName(QString::fromUtf8("buttonSave"));
        buttonSave->setEnabled(true);
        buttonSave->setMinimumSize(QSize(28, 28));
        buttonSave->setMaximumSize(QSize(28, 28));
        buttonSave->setCheckable(true);

        horizontalLayout->addWidget(buttonSave);

        autoSave = new QToolButton(DriveWidget);
        autoSave->setObjectName(QString::fromUtf8("autoSave"));
        autoSave->setEnabled(true);
        autoSave->setMinimumSize(QSize(28, 28));
        autoSave->setMaximumSize(QSize(28, 28));
        autoSave->setCheckable(false);
        autoSave->setAutoRaise(false);

        horizontalLayout->addWidget(autoSave);

        buttonEditDisk = new QToolButton(DriveWidget);
        buttonEditDisk->setObjectName(QString::fromUtf8("buttonEditDisk"));
        buttonEditDisk->setEnabled(true);
        buttonEditDisk->setMinimumSize(QSize(28, 28));
        buttonEditDisk->setMaximumSize(QSize(28, 28));
        buttonEditDisk->setCheckable(false);

        horizontalLayout->addWidget(buttonEditDisk);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        verticalLayout->setContentsMargins(5, -1, 5, -1);
        labelFileName = new QLabel(DriveWidget);
        labelFileName->setObjectName(QString::fromUtf8("labelFileName"));
        labelFileName->setEnabled(true);
        QSizePolicy sizePolicy2(QSizePolicy::Ignored, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(labelFileName->sizePolicy().hasHeightForWidth());
        labelFileName->setSizePolicy(sizePolicy2);
        QFont font1;
        font1.setFamily(QString::fromUtf8("MS Shell Dlg 2"));
        font1.setPointSize(8);
        font1.setBold(false);
        font1.setItalic(false);
        font1.setWeight(50);
        labelFileName->setFont(font1);
        labelFileName->setAutoFillBackground(false);
        labelFileName->setTextFormat(Qt::PlainText);
        labelFileName->setWordWrap(false);

        verticalLayout->addWidget(labelFileName);

        labelImageProperties = new QLabel(DriveWidget);
        labelImageProperties->setObjectName(QString::fromUtf8("labelImageProperties"));
        labelImageProperties->setEnabled(true);
        QSizePolicy sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(labelImageProperties->sizePolicy().hasHeightForWidth());
        labelImageProperties->setSizePolicy(sizePolicy3);
        QPalette palette1;
        QBrush brush3(QColor(128, 128, 128, 255));
        brush3.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::WindowText, brush3);
        QBrush brush4(QColor(157, 155, 144, 255));
        brush4.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::Mid, brush4);
        palette1.setBrush(QPalette::Inactive, QPalette::WindowText, brush3);
        palette1.setBrush(QPalette::Inactive, QPalette::Mid, brush4);
        QBrush brush5(QColor(118, 116, 108, 255));
        brush5.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Disabled, QPalette::WindowText, brush5);
        palette1.setBrush(QPalette::Disabled, QPalette::Mid, brush4);
        labelImageProperties->setPalette(palette1);
        labelImageProperties->setTextFormat(Qt::PlainText);
        labelImageProperties->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        labelImageProperties->setWordWrap(false);

        verticalLayout->addWidget(labelImageProperties);


        horizontalLayout->addLayout(verticalLayout);

        buttonEject = new QToolButton(DriveWidget);
        buttonEject->setObjectName(QString::fromUtf8("buttonEject"));
        buttonEject->setEnabled(true);
        buttonEject->setMinimumSize(QSize(21, 25));
        buttonEject->setMaximumSize(QSize(21, 25));
        buttonEject->setPopupMode(QToolButton::DelayedPopup);
        buttonEject->setAutoRaise(false);

        horizontalLayout->addWidget(buttonEject);


        retranslateUi(DriveWidget);

        QMetaObject::connectSlotsByName(DriveWidget);
    } // setupUi

    void retranslateUi(QFrame *DriveWidget)
    {
        DriveWidget->setWindowTitle(QCoreApplication::translate("DriveWidget", "Frame", nullptr));
        actionMountDisk->setText(QCoreApplication::translate("DriveWidget", "Mount disk image...", nullptr));
#if QT_CONFIG(tooltip)
        actionMountDisk->setToolTip(QCoreApplication::translate("DriveWidget", "Mount a disk image", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionMountDisk->setStatusTip(QCoreApplication::translate("DriveWidget", "Mount a disk image to D%1", nullptr));
#endif // QT_CONFIG(statustip)
        actionMountFolder->setText(QCoreApplication::translate("DriveWidget", "Mount folder image...", nullptr));
#if QT_CONFIG(tooltip)
        actionMountFolder->setToolTip(QCoreApplication::translate("DriveWidget", "Mount a folder image", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionMountFolder->setStatusTip(QCoreApplication::translate("DriveWidget", "Mount a folder image to D%1", nullptr));
#endif // QT_CONFIG(statustip)
        actionEject->setText(QCoreApplication::translate("DriveWidget", "Unmount", nullptr));
#if QT_CONFIG(tooltip)
        actionEject->setToolTip(QCoreApplication::translate("DriveWidget", "Unmount", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionEject->setStatusTip(QCoreApplication::translate("DriveWidget", "Unmount D%1", nullptr));
#endif // QT_CONFIG(statustip)
        actionWriteProtect->setText(QCoreApplication::translate("DriveWidget", "Write protected", nullptr));
#if QT_CONFIG(tooltip)
        actionWriteProtect->setToolTip(QCoreApplication::translate("DriveWidget", "Toggle write protection", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionWriteProtect->setStatusTip(QCoreApplication::translate("DriveWidget", "Toggle write protection for D%1", nullptr));
#endif // QT_CONFIG(statustip)
        actionEditDisk->setText(QCoreApplication::translate("DriveWidget", "Explore..", nullptr));
#if QT_CONFIG(tooltip)
        actionEditDisk->setToolTip(QCoreApplication::translate("DriveWidget", "Show properties", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionEditDisk->setStatusTip(QCoreApplication::translate("DriveWidget", "Show D%1's properties", nullptr));
#endif // QT_CONFIG(statustip)
        actionSave->setText(QCoreApplication::translate("DriveWidget", "Save", nullptr));
#if QT_CONFIG(tooltip)
        actionSave->setToolTip(QCoreApplication::translate("DriveWidget", "Save image", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionSave->setStatusTip(QCoreApplication::translate("DriveWidget", "Save D%1", nullptr));
#endif // QT_CONFIG(statustip)
        actionRevert->setText(QCoreApplication::translate("DriveWidget", "Revert to original", nullptr));
#if QT_CONFIG(tooltip)
        actionRevert->setToolTip(QCoreApplication::translate("DriveWidget", "Revert image to its last saved state", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionRevert->setStatusTip(QCoreApplication::translate("DriveWidget", "Revert D%1 to its last saved state", nullptr));
#endif // QT_CONFIG(statustip)
        actionSaveAs->setText(QCoreApplication::translate("DriveWidget", "Save as...", nullptr));
#if QT_CONFIG(tooltip)
        actionSaveAs->setToolTip(QCoreApplication::translate("DriveWidget", "Save to a file", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionSaveAs->setStatusTip(QCoreApplication::translate("DriveWidget", "Save D%1 to a file", nullptr));
#endif // QT_CONFIG(statustip)
        actionAutoSave->setText(QCoreApplication::translate("DriveWidget", "Auto save", nullptr));
#if QT_CONFIG(tooltip)
        actionAutoSave->setToolTip(QCoreApplication::translate("DriveWidget", "Commit changes to this disk automatically", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionAutoSave->setStatusTip(QCoreApplication::translate("DriveWidget", "Toggle Auto Commit ON/OFF", nullptr));
#endif // QT_CONFIG(statustip)
        actionBootOption->setText(QCoreApplication::translate("DriveWidget", "Folder Boot Options", nullptr));
#if QT_CONFIG(tooltip)
        actionBootOption->setToolTip(QCoreApplication::translate("DriveWidget", "Change Boot Options", nullptr));
#endif // QT_CONFIG(tooltip)
        driveLabel->setText(QCoreApplication::translate("DriveWidget", "1:", nullptr));
#if QT_CONFIG(tooltip)
        autoSave->setToolTip(QString());
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        autoSave->setStatusTip(QString());
#endif // QT_CONFIG(statustip)
        autoSave->setText(QString());
        labelFileName->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class DriveWidget: public Ui_DriveWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DRIVEWIDGET_H
