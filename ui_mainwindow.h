/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionEjectAll;
    QAction *actionOptions;
    QAction *actionStartEmulation;
    QAction *actionMountRecent_0;
    QAction *actionMountDisk;
    QAction *actionMountFolder;
    QAction *actionNewImage;
    QAction *actionSaveSession;
    QAction *actionOpenSession;
    QAction *actionBootExe;
    QAction *actionSave_1;
    QAction *actionShowPrinterTextOutput;
    QAction *actionPlaybackCassette;
    QAction *actionQuit;
    QAction *actionAbout;
    QAction *actionHelp;
    QAction *actionContents;
    QAction *actionIndex;
    QAction *actionDocumentation;
    QAction *actionDefaultImageGroup;
    QAction *actionPrinterEmulation;
    QAction *actionHideShowDrives;
    QAction *actionToggleMiniMode;
    QAction *actionToggleShade;
    QAction *actionLogWindow;
    QWidget *centralWidget;
    QGridLayout *gridLayout;
    QTextEdit *textEdit;
    QHBoxLayout *horizontalLayout;
    QGridLayout *leftColumn;
    QGridLayout *rightColumn;
    QMenuBar *menuBar;
    QMenu *menu_Disk;
    QMenu *menu_File;
    QMenu *menu_Tools;
    QMenu *menu_Help;
    QMenu *menu_Window;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->setEnabled(true);
        MainWindow->resize(694, 499);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/main-icon/AspeQt.ico"), QSize(), QIcon::Normal, QIcon::Off);
        MainWindow->setWindowIcon(icon);
        MainWindow->setStyleSheet(QString::fromUtf8(""));
        MainWindow->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
        MainWindow->setIconSize(QSize(24, 24));
        actionEjectAll = new QAction(MainWindow);
        actionEjectAll->setObjectName(QString::fromUtf8("actionEjectAll"));
        actionEjectAll->setCheckable(false);
        actionEjectAll->setEnabled(true);
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/silk-icons/icons/control_eject.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionEjectAll->setIcon(icon1);
        actionEjectAll->setIconVisibleInMenu(true);
        actionOptions = new QAction(MainWindow);
        actionOptions->setObjectName(QString::fromUtf8("actionOptions"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/other-icons/run.ico"), QSize(), QIcon::Normal, QIcon::Off);
        actionOptions->setIcon(icon2);
        actionStartEmulation = new QAction(MainWindow);
        actionStartEmulation->setObjectName(QString::fromUtf8("actionStartEmulation"));
        actionStartEmulation->setCheckable(true);
        actionStartEmulation->setChecked(false);
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/silk-icons/icons/disconnect.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon3.addFile(QString::fromUtf8(":/icons/silk-icons/icons/connect.png"), QSize(), QIcon::Normal, QIcon::On);
        actionStartEmulation->setIcon(icon3);
        actionStartEmulation->setIconVisibleInMenu(true);
        actionMountRecent_0 = new QAction(MainWindow);
        actionMountRecent_0->setObjectName(QString::fromUtf8("actionMountRecent_0"));
        actionMountRecent_0->setText(QString::fromUtf8(""));
        actionMountDisk = new QAction(MainWindow);
        actionMountDisk->setObjectName(QString::fromUtf8("actionMountDisk"));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/devices/3floppy_unmount.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionMountDisk->setIcon(icon4);
        actionMountFolder = new QAction(MainWindow);
        actionMountFolder->setObjectName(QString::fromUtf8("actionMountFolder"));
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/icons/silk-icons/icons/folder.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionMountFolder->setIcon(icon5);
        actionNewImage = new QAction(MainWindow);
        actionNewImage->setObjectName(QString::fromUtf8("actionNewImage"));
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/devices/media_floppy.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionNewImage->setIcon(icon6);
        actionNewImage->setIconVisibleInMenu(true);
        actionSaveSession = new QAction(MainWindow);
        actionSaveSession->setObjectName(QString::fromUtf8("actionSaveSession"));
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/icons/silk-icons/icons/monitor_go.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionSaveSession->setIcon(icon7);
        actionOpenSession = new QAction(MainWindow);
        actionOpenSession->setObjectName(QString::fromUtf8("actionOpenSession"));
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/icons/silk-icons/icons/monitor.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionOpenSession->setIcon(icon8);
        actionBootExe = new QAction(MainWindow);
        actionBootExe->setObjectName(QString::fromUtf8("actionBootExe"));
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/icons/Atari.ico"), QSize(), QIcon::Normal, QIcon::Off);
        actionBootExe->setIcon(icon9);
        actionBootExe->setVisible(true);
        actionBootExe->setIconVisibleInMenu(true);
        actionSave_1 = new QAction(MainWindow);
        actionSave_1->setObjectName(QString::fromUtf8("actionSave_1"));
        actionSave_1->setEnabled(false);
        QIcon icon10;
        icon10.addFile(QString::fromUtf8(":/icons/silk-icons/icons/drive_disk.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionSave_1->setIcon(icon10);
        actionShowPrinterTextOutput = new QAction(MainWindow);
        actionShowPrinterTextOutput->setObjectName(QString::fromUtf8("actionShowPrinterTextOutput"));
        actionShowPrinterTextOutput->setCheckable(true);
        QIcon icon11;
        icon11.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/devices/printer1.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionShowPrinterTextOutput->setIcon(icon11);
        actionPlaybackCassette = new QAction(MainWindow);
        actionPlaybackCassette->setObjectName(QString::fromUtf8("actionPlaybackCassette"));
        QIcon icon12;
        icon12.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/devices/media_tape.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionPlaybackCassette->setIcon(icon12);
        actionQuit = new QAction(MainWindow);
        actionQuit->setObjectName(QString::fromUtf8("actionQuit"));
        QIcon icon13;
        icon13.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/exit.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionQuit->setIcon(icon13);
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName(QString::fromUtf8("actionAbout"));
        QIcon icon14;
        icon14.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/help_about.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAbout->setIcon(icon14);
        actionHelp = new QAction(MainWindow);
        actionHelp->setObjectName(QString::fromUtf8("actionHelp"));
        actionContents = new QAction(MainWindow);
        actionContents->setObjectName(QString::fromUtf8("actionContents"));
        actionContents->setEnabled(false);
        actionIndex = new QAction(MainWindow);
        actionIndex->setObjectName(QString::fromUtf8("actionIndex"));
        actionDocumentation = new QAction(MainWindow);
        actionDocumentation->setObjectName(QString::fromUtf8("actionDocumentation"));
        actionDocumentation->setCheckable(true);
        QIcon icon15;
        icon15.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/help_contents.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionDocumentation->setIcon(icon15);
        actionDefaultImageGroup = new QAction(MainWindow);
        actionDefaultImageGroup->setObjectName(QString::fromUtf8("actionDefaultImageGroup"));
        QIcon icon16;
        icon16.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/document_save_all.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionDefaultImageGroup->setIcon(icon16);
        actionPrinterEmulation = new QAction(MainWindow);
        actionPrinterEmulation->setObjectName(QString::fromUtf8("actionPrinterEmulation"));
        actionPrinterEmulation->setCheckable(false);
        actionPrinterEmulation->setChecked(false);
        QIcon icon17;
        icon17.addFile(QString::fromUtf8(":/icons/silk-icons/icons/printer.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionPrinterEmulation->setIcon(icon17);
        actionHideShowDrives = new QAction(MainWindow);
        actionHideShowDrives->setObjectName(QString::fromUtf8("actionHideShowDrives"));
        QIcon icon18;
        icon18.addFile(QString::fromUtf8(":/icons/silk-icons/icons/drive_add.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionHideShowDrives->setIcon(icon18);
        actionToggleMiniMode = new QAction(MainWindow);
        actionToggleMiniMode->setObjectName(QString::fromUtf8("actionToggleMiniMode"));
        QIcon icon19;
        icon19.addFile(QString::fromUtf8(":/icons/other-icons/MiniCooper 22.ico"), QSize(), QIcon::Normal, QIcon::Off);
        actionToggleMiniMode->setIcon(icon19);
        actionToggleShade = new QAction(MainWindow);
        actionToggleShade->setObjectName(QString::fromUtf8("actionToggleShade"));
        actionToggleShade->setEnabled(false);
        QIcon icon20;
        icon20.addFile(QString::fromUtf8(":/icons/other-icons/fade.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionToggleShade->setIcon(icon20);
        actionLogWindow = new QAction(MainWindow);
        actionLogWindow->setObjectName(QString::fromUtf8("actionLogWindow"));
        QIcon icon21;
        icon21.addFile(QString::fromUtf8(":/icons/silk-icons/icons/page_white_text.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionLogWindow->setIcon(icon21);
        centralWidget = new QWidget(MainWindow);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        centralWidget->setEnabled(true);
        gridLayout = new QGridLayout(centralWidget);
        gridLayout->setSpacing(6);
        gridLayout->setContentsMargins(11, 11, 11, 11);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        textEdit = new QTextEdit(centralWidget);
        textEdit->setObjectName(QString::fromUtf8("textEdit"));
        QSizePolicy sizePolicy1(QSizePolicy::Ignored, QSizePolicy::Ignored);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(textEdit->sizePolicy().hasHeightForWidth());
        textEdit->setSizePolicy(sizePolicy1);
        textEdit->setMinimumSize(QSize(0, 16));
        textEdit->setContextMenuPolicy(Qt::DefaultContextMenu);
        textEdit->setTextInteractionFlags(Qt::NoTextInteraction);

        gridLayout->addWidget(textEdit, 1, 0, 1, 2);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetDefaultConstraint);
        leftColumn = new QGridLayout();
        leftColumn->setSpacing(6);
        leftColumn->setObjectName(QString::fromUtf8("leftColumn"));

        horizontalLayout->addLayout(leftColumn);

        rightColumn = new QGridLayout();
        rightColumn->setSpacing(6);
        rightColumn->setObjectName(QString::fromUtf8("rightColumn"));

        horizontalLayout->addLayout(rightColumn);


        gridLayout->addLayout(horizontalLayout, 0, 0, 1, 2);

        MainWindow->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 694, 22));
        menuBar->setNativeMenuBar(false);
        menu_Disk = new QMenu(menuBar);
        menu_Disk->setObjectName(QString::fromUtf8("menu_Disk"));
        menu_File = new QMenu(menuBar);
        menu_File->setObjectName(QString::fromUtf8("menu_File"));
        menu_Tools = new QMenu(menuBar);
        menu_Tools->setObjectName(QString::fromUtf8("menu_Tools"));
        menu_Help = new QMenu(menuBar);
        menu_Help->setObjectName(QString::fromUtf8("menu_Help"));
        menu_Window = new QMenu(menuBar);
        menu_Window->setObjectName(QString::fromUtf8("menu_Window"));
        menu_Window->setEnabled(true);
        MainWindow->setMenuBar(menuBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        MainWindow->setStatusBar(statusBar);

        menuBar->addAction(menu_File->menuAction());
        menuBar->addAction(menu_Disk->menuAction());
        menuBar->addAction(menu_Tools->menuAction());
        menuBar->addAction(menu_Window->menuAction());
        menuBar->addAction(menu_Help->menuAction());
        menu_Disk->addAction(actionNewImage);
        menu_Disk->addAction(actionMountDisk);
        menu_Disk->addAction(actionMountFolder);
        menu_Disk->addAction(actionHideShowDrives);
        menu_Disk->addAction(actionEjectAll);
        menu_Disk->addSeparator();
        menu_File->addAction(actionStartEmulation);
        menu_File->addAction(actionBootExe);
        menu_File->addAction(actionPlaybackCassette);
        menu_File->addAction(actionPrinterEmulation);
        menu_File->addAction(actionShowPrinterTextOutput);
        menu_File->addSeparator();
        menu_File->addAction(actionOpenSession);
        menu_File->addAction(actionSaveSession);
        menu_File->addSeparator();
        menu_File->addAction(actionQuit);
        menu_File->addSeparator();
        menu_Tools->addAction(actionOptions);
        menu_Help->addAction(actionAbout);
        menu_Help->addAction(actionDocumentation);
        menu_Window->addAction(actionToggleMiniMode);
        menu_Window->addAction(actionToggleShade);
        menu_Window->addAction(actionLogWindow);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        actionEjectAll->setText(QCoreApplication::translate("MainWindow", "Unmount &all", nullptr));
#if QT_CONFIG(statustip)
        actionEjectAll->setStatusTip(QCoreApplication::translate("MainWindow", "Unmount all", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionEjectAll->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+U", nullptr));
#endif // QT_CONFIG(shortcut)
        actionOptions->setText(QCoreApplication::translate("MainWindow", "&Options...", nullptr));
#if QT_CONFIG(tooltip)
        actionOptions->setToolTip(QCoreApplication::translate("MainWindow", "Open options dialog", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionOptions->setStatusTip(QCoreApplication::translate("MainWindow", "Open options dialog", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionOptions->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+O", nullptr));
#endif // QT_CONFIG(shortcut)
        actionStartEmulation->setText(QCoreApplication::translate("MainWindow", "&Start emulation", nullptr));
#if QT_CONFIG(tooltip)
        actionStartEmulation->setToolTip(QCoreApplication::translate("MainWindow", "Start SIO peripheral emulation", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionStartEmulation->setStatusTip(QCoreApplication::translate("MainWindow", "Start SIO peripheral emulation", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionStartEmulation->setShortcut(QCoreApplication::translate("MainWindow", "Alt+E", nullptr));
#endif // QT_CONFIG(shortcut)
#if QT_CONFIG(tooltip)
        actionMountRecent_0->setToolTip(QCoreApplication::translate("MainWindow", "Mount to the first available slot", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionMountRecent_0->setStatusTip(QCoreApplication::translate("MainWindow", "Mount to the first available slot", nullptr));
#endif // QT_CONFIG(statustip)
        actionMountDisk->setText(QCoreApplication::translate("MainWindow", "Mount &disk image...", nullptr));
#if QT_CONFIG(tooltip)
        actionMountDisk->setToolTip(QCoreApplication::translate("MainWindow", "Mount a disk image to the first available slot", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionMountDisk->setStatusTip(QCoreApplication::translate("MainWindow", "Mount a disk image to the first available slot", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionMountDisk->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+D", nullptr));
#endif // QT_CONFIG(shortcut)
        actionMountFolder->setText(QCoreApplication::translate("MainWindow", "Mount &folder image...", nullptr));
#if QT_CONFIG(tooltip)
        actionMountFolder->setToolTip(QCoreApplication::translate("MainWindow", "Mount a folder image to the first available slot", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionMountFolder->setStatusTip(QCoreApplication::translate("MainWindow", "Mount a folder image to the first available slot", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionMountFolder->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+F", nullptr));
#endif // QT_CONFIG(shortcut)
        actionNewImage->setText(QCoreApplication::translate("MainWindow", "New disk image...", nullptr));
#if QT_CONFIG(tooltip)
        actionNewImage->setToolTip(QCoreApplication::translate("MainWindow", "Create a new disk image file and mount it to the first available slot", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionNewImage->setStatusTip(QCoreApplication::translate("MainWindow", "Create a new disk image file and mount it to the first available slot", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionNewImage->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+N", nullptr));
#endif // QT_CONFIG(shortcut)
        actionSaveSession->setText(QCoreApplication::translate("MainWindow", "&Save session", nullptr));
#if QT_CONFIG(tooltip)
        actionSaveSession->setToolTip(QCoreApplication::translate("MainWindow", "Save current session to a file", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionSaveSession->setStatusTip(QCoreApplication::translate("MainWindow", "Save current session", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionSaveSession->setShortcut(QCoreApplication::translate("MainWindow", "Alt+S", nullptr));
#endif // QT_CONFIG(shortcut)
        actionOpenSession->setText(QCoreApplication::translate("MainWindow", "&Open session...", nullptr));
#if QT_CONFIG(tooltip)
        actionOpenSession->setToolTip(QCoreApplication::translate("MainWindow", "Open a previously saved session", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionOpenSession->setStatusTip(QCoreApplication::translate("MainWindow", "Open a previously saved session", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionOpenSession->setShortcut(QCoreApplication::translate("MainWindow", "Alt+O", nullptr));
#endif // QT_CONFIG(shortcut)
        actionBootExe->setText(QCoreApplication::translate("MainWindow", "&Boot Atari executable...", nullptr));
#if QT_CONFIG(tooltip)
        actionBootExe->setToolTip(QCoreApplication::translate("MainWindow", "Boot an Atari executable", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionBootExe->setStatusTip(QCoreApplication::translate("MainWindow", "Boot an Atari executable", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionBootExe->setShortcut(QCoreApplication::translate("MainWindow", "Alt+B", nullptr));
#endif // QT_CONFIG(shortcut)
        actionSave_1->setText(QCoreApplication::translate("MainWindow", "Save", nullptr));
#if QT_CONFIG(tooltip)
        actionSave_1->setToolTip(QCoreApplication::translate("MainWindow", "Save D1", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionSave_1->setStatusTip(QCoreApplication::translate("MainWindow", "Save D1", nullptr));
#endif // QT_CONFIG(statustip)
        actionShowPrinterTextOutput->setText(QCoreApplication::translate("MainWindow", "Show printer text output", nullptr));
#if QT_CONFIG(tooltip)
        actionShowPrinterTextOutput->setToolTip(QCoreApplication::translate("MainWindow", "Show printer text output", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionShowPrinterTextOutput->setStatusTip(QCoreApplication::translate("MainWindow", "Show printer text output", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionShowPrinterTextOutput->setShortcut(QCoreApplication::translate("MainWindow", "Alt+Shift+T", nullptr));
#endif // QT_CONFIG(shortcut)
        actionPlaybackCassette->setText(QCoreApplication::translate("MainWindow", "Playback cassette image...", nullptr));
#if QT_CONFIG(tooltip)
        actionPlaybackCassette->setToolTip(QCoreApplication::translate("MainWindow", "Playback a cassette image", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionPlaybackCassette->setStatusTip(QCoreApplication::translate("MainWindow", "Playback a cassette image", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionPlaybackCassette->setShortcut(QCoreApplication::translate("MainWindow", "Alt+C", nullptr));
#endif // QT_CONFIG(shortcut)
        actionQuit->setText(QCoreApplication::translate("MainWindow", "&Quit", nullptr));
#if QT_CONFIG(statustip)
        actionQuit->setStatusTip(QCoreApplication::translate("MainWindow", "Quit AspeQt", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionQuit->setShortcut(QCoreApplication::translate("MainWindow", "Alt+Q", nullptr));
#endif // QT_CONFIG(shortcut)
        actionAbout->setText(QCoreApplication::translate("MainWindow", "&About AspeQt", nullptr));
#if QT_CONFIG(shortcut)
        actionAbout->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+A", nullptr));
#endif // QT_CONFIG(shortcut)
        actionHelp->setText(QCoreApplication::translate("MainWindow", "Help", nullptr));
        actionContents->setText(QCoreApplication::translate("MainWindow", "Contents", nullptr));
        actionIndex->setText(QCoreApplication::translate("MainWindow", "Index", nullptr));
        actionDocumentation->setText(QCoreApplication::translate("MainWindow", "User Manual", nullptr));
#if QT_CONFIG(tooltip)
        actionDocumentation->setToolTip(QCoreApplication::translate("MainWindow", "User Manual", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionDocumentation->setStatusTip(QCoreApplication::translate("MainWindow", "User Manual", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionDocumentation->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Shift+U", nullptr));
#endif // QT_CONFIG(shortcut)
        actionDefaultImageGroup->setText(QCoreApplication::translate("MainWindow", "Save mounted image group as default", nullptr));
        actionPrinterEmulation->setText(QCoreApplication::translate("MainWindow", "Stop printer emulation", nullptr));
#if QT_CONFIG(statustip)
        actionPrinterEmulation->setStatusTip(QCoreApplication::translate("MainWindow", "Stop printer emulation", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionPrinterEmulation->setShortcut(QCoreApplication::translate("MainWindow", "Alt+P", nullptr));
#endif // QT_CONFIG(shortcut)
        actionHideShowDrives->setText(QCoreApplication::translate("MainWindow", "Hide drives D9-DO", nullptr));
#if QT_CONFIG(shortcut)
        actionHideShowDrives->setShortcut(QCoreApplication::translate("MainWindow", "Alt+H", nullptr));
#endif // QT_CONFIG(shortcut)
        actionToggleMiniMode->setText(QCoreApplication::translate("MainWindow", "Toggle mini mode", nullptr));
#if QT_CONFIG(shortcut)
        actionToggleMiniMode->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+M", nullptr));
#endif // QT_CONFIG(shortcut)
        actionToggleShade->setText(QCoreApplication::translate("MainWindow", "Toggle shade mode", nullptr));
#if QT_CONFIG(shortcut)
        actionToggleShade->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+S", nullptr));
#endif // QT_CONFIG(shortcut)
        actionLogWindow->setText(QCoreApplication::translate("MainWindow", "Open log window", nullptr));
#if QT_CONFIG(tooltip)
        actionLogWindow->setToolTip(QCoreApplication::translate("MainWindow", "Open log window", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionLogWindow->setStatusTip(QCoreApplication::translate("MainWindow", "Open log window", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionLogWindow->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+L", nullptr));
#endif // QT_CONFIG(shortcut)
        menu_Disk->setTitle(QCoreApplication::translate("MainWindow", "&Disk", nullptr));
        menu_File->setTitle(QCoreApplication::translate("MainWindow", "&File", nullptr));
        menu_Tools->setTitle(QCoreApplication::translate("MainWindow", "&Tools", nullptr));
        menu_Help->setTitle(QCoreApplication::translate("MainWindow", "&Help", nullptr));
        menu_Window->setTitle(QCoreApplication::translate("MainWindow", "Window", nullptr));
        (void)MainWindow;
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
