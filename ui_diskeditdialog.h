/********************************************************************************
** Form generated from reading UI file 'diskeditdialog.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DISKEDITDIALOG_H
#define UI_DISKEDITDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableView>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DiskEditDialog
{
public:
    QAction *actionToParent;
    QAction *actionAddFiles;
    QAction *actionExtractFiles;
    QAction *actionTextConversion;
    QAction *actionDeleteSelectedFiles;
    QAction *actionPrint;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QTableView *aView;
    QCheckBox *onTopBox;
    QMenuBar *menubar;
    QStatusBar *statusbar;
    QToolBar *toolBar;

    void setupUi(QMainWindow *DiskEditDialog)
    {
        if (DiskEditDialog->objectName().isEmpty())
            DiskEditDialog->setObjectName(QString::fromUtf8("DiskEditDialog"));
        DiskEditDialog->setWindowModality(Qt::NonModal);
        DiskEditDialog->resize(504, 424);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(DiskEditDialog->sizePolicy().hasHeightForWidth());
        DiskEditDialog->setSizePolicy(sizePolicy);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/silk-icons/icons/folder_edit.png"), QSize(), QIcon::Normal, QIcon::Off);
        DiskEditDialog->setWindowIcon(icon);
        actionToParent = new QAction(DiskEditDialog);
        actionToParent->setObjectName(QString::fromUtf8("actionToParent"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/silk-icons/icons/arrow_up.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionToParent->setIcon(icon1);
        actionAddFiles = new QAction(DiskEditDialog);
        actionAddFiles->setObjectName(QString::fromUtf8("actionAddFiles"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/silk-icons/icons/add.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAddFiles->setIcon(icon2);
        actionExtractFiles = new QAction(DiskEditDialog);
        actionExtractFiles->setObjectName(QString::fromUtf8("actionExtractFiles"));
        actionExtractFiles->setEnabled(false);
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/silk-icons/icons/page_white_go.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionExtractFiles->setIcon(icon3);
        actionTextConversion = new QAction(DiskEditDialog);
        actionTextConversion->setObjectName(QString::fromUtf8("actionTextConversion"));
        actionTextConversion->setCheckable(true);
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/icons/silk-icons/icons/page_white_text.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionTextConversion->setIcon(icon4);
        actionDeleteSelectedFiles = new QAction(DiskEditDialog);
        actionDeleteSelectedFiles->setObjectName(QString::fromUtf8("actionDeleteSelectedFiles"));
        actionDeleteSelectedFiles->setEnabled(false);
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/icons/silk-icons/icons/delete.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionDeleteSelectedFiles->setIcon(icon5);
        actionPrint = new QAction(DiskEditDialog);
        actionPrint->setObjectName(QString::fromUtf8("actionPrint"));
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/icons/silk-icons/icons/printer.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionPrint->setIcon(icon6);
        centralwidget = new QWidget(DiskEditDialog);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        aView = new QTableView(centralwidget);
        aView->setObjectName(QString::fromUtf8("aView"));
        aView->setContextMenuPolicy(Qt::ActionsContextMenu);
        aView->setAcceptDrops(true);
        aView->setEditTriggers(QAbstractItemView::EditKeyPressed);
        aView->setProperty("showDropIndicator", QVariant(false));
        aView->setDragEnabled(true);
        aView->setDragDropOverwriteMode(false);
        aView->setDragDropMode(QAbstractItemView::DragDrop);
        aView->setAlternatingRowColors(true);
        aView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        aView->setSelectionBehavior(QAbstractItemView::SelectRows);
        aView->setShowGrid(false);
        aView->setSortingEnabled(true);
        aView->horizontalHeader()->setDefaultSectionSize(150);
        aView->horizontalHeader()->setHighlightSections(false);
        aView->horizontalHeader()->setProperty("showSortIndicator", QVariant(true));
        aView->horizontalHeader()->setStretchLastSection(true);
        aView->verticalHeader()->setVisible(false);
        aView->verticalHeader()->setDefaultSectionSize(17);

        gridLayout->addWidget(aView, 0, 0, 1, 1);

        onTopBox = new QCheckBox(centralwidget);
        onTopBox->setObjectName(QString::fromUtf8("onTopBox"));

        gridLayout->addWidget(onTopBox, 1, 0, 1, 1);

        DiskEditDialog->setCentralWidget(centralwidget);
        menubar = new QMenuBar(DiskEditDialog);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 504, 21));
        DiskEditDialog->setMenuBar(menubar);
        statusbar = new QStatusBar(DiskEditDialog);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        DiskEditDialog->setStatusBar(statusbar);
        toolBar = new QToolBar(DiskEditDialog);
        toolBar->setObjectName(QString::fromUtf8("toolBar"));
        toolBar->setMovable(false);
        toolBar->setAllowedAreas(Qt::NoToolBarArea);
        toolBar->setIconSize(QSize(16, 16));
        toolBar->setFloatable(true);
        DiskEditDialog->addToolBar(Qt::TopToolBarArea, toolBar);

        toolBar->addAction(actionToParent);
        toolBar->addSeparator();
        toolBar->addAction(actionAddFiles);
        toolBar->addAction(actionExtractFiles);
        toolBar->addSeparator();
        toolBar->addAction(actionDeleteSelectedFiles);
        toolBar->addSeparator();
        toolBar->addAction(actionTextConversion);
        toolBar->addAction(actionPrint);

        retranslateUi(DiskEditDialog);

        QMetaObject::connectSlotsByName(DiskEditDialog);
    } // setupUi

    void retranslateUi(QMainWindow *DiskEditDialog)
    {
        DiskEditDialog->setWindowTitle(QCoreApplication::translate("DiskEditDialog", "MainWindow", nullptr));
        actionToParent->setText(QCoreApplication::translate("DiskEditDialog", "Go to the parent directory", nullptr));
#if QT_CONFIG(tooltip)
        actionToParent->setToolTip(QCoreApplication::translate("DiskEditDialog", "Go to the parent directory", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionToParent->setStatusTip(QCoreApplication::translate("DiskEditDialog", "Go to the parent directory", nullptr));
#endif // QT_CONFIG(statustip)
        actionAddFiles->setText(QCoreApplication::translate("DiskEditDialog", "Add files...", nullptr));
#if QT_CONFIG(tooltip)
        actionAddFiles->setToolTip(QCoreApplication::translate("DiskEditDialog", "Add files to this directory", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionAddFiles->setStatusTip(QCoreApplication::translate("DiskEditDialog", "Add files to this directory", nullptr));
#endif // QT_CONFIG(statustip)
        actionExtractFiles->setText(QCoreApplication::translate("DiskEditDialog", "Extract files...", nullptr));
#if QT_CONFIG(tooltip)
        actionExtractFiles->setToolTip(QCoreApplication::translate("DiskEditDialog", "Extract selected files", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionExtractFiles->setStatusTip(QCoreApplication::translate("DiskEditDialog", "Extract selected files", nullptr));
#endif // QT_CONFIG(statustip)
        actionTextConversion->setText(QCoreApplication::translate("DiskEditDialog", "Text conversion", nullptr));
#if QT_CONFIG(tooltip)
        actionTextConversion->setToolTip(QCoreApplication::translate("DiskEditDialog", "Text conversion is off", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionTextConversion->setStatusTip(QCoreApplication::translate("DiskEditDialog", "Text conversion is off", nullptr));
#endif // QT_CONFIG(statustip)
        actionDeleteSelectedFiles->setText(QCoreApplication::translate("DiskEditDialog", "Delete", nullptr));
#if QT_CONFIG(tooltip)
        actionDeleteSelectedFiles->setToolTip(QCoreApplication::translate("DiskEditDialog", "Delete selected files", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionDeleteSelectedFiles->setStatusTip(QCoreApplication::translate("DiskEditDialog", "Delete selected files", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionDeleteSelectedFiles->setShortcut(QCoreApplication::translate("DiskEditDialog", "Del", nullptr));
#endif // QT_CONFIG(shortcut)
        actionPrint->setText(QCoreApplication::translate("DiskEditDialog", "Print", nullptr));
#if QT_CONFIG(tooltip)
        actionPrint->setToolTip(QCoreApplication::translate("DiskEditDialog", "Print Directory Listing", nullptr));
#endif // QT_CONFIG(tooltip)
        onTopBox->setText(QCoreApplication::translate("DiskEditDialog", "Stay on Top", nullptr));
        toolBar->setWindowTitle(QCoreApplication::translate("DiskEditDialog", "toolBar", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DiskEditDialog: public Ui_DiskEditDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DISKEDITDIALOG_H
