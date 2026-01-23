/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/mainwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN10MainWindowE = QtMocHelpers::stringData(
    "MainWindow",
    "logMessage",
    "",
    "type",
    "msg",
    "newSlot",
    "slot",
    "fileMounted",
    "mounted",
    "sendLogText",
    "logText",
    "sendLogTextChange",
    "logTextChange",
    "setFont",
    "font",
    "show",
    "firstEmptyDiskSlot",
    "startFrom",
    "createOne",
    "mountFileWithDefaultProtection",
    "no",
    "fileName",
    "autoCommit",
    "st",
    "openRecent",
    "bootExeTriggered",
    "bootCasTriggered",
    "printServer",
    "on",
    "on_actionPlaybackCassette_triggered",
    "on_actionShowPrinterTextOutput_triggered",
    "on_actionBootExe_triggered",
    "on_actionSaveSession_triggered",
    "on_actionOpenSession_triggered",
    "on_actionNewImage_triggered",
    "on_actionMountFolder_triggered",
    "on_actionMountDisk_triggered",
    "on_actionEjectAll_triggered",
    "on_actionOptions_triggered",
    "on_actionStartEmulation_triggered",
    "on_actionPrinterEmulation_triggered",
    "on_actionHideShowDrives_triggered",
    "on_actionQuit_triggered",
    "on_actionAbout_triggered",
    "on_actionDocumentation_triggered",
    "deviceId",
    "on_actionEject_triggered",
    "on_actionWriteProtect_triggered",
    "writeProtectEnabled",
    "on_actionMountRecent_triggered",
    "on_actionEditDisk_triggered",
    "on_actionSave_triggered",
    "on_actionAutoSave_triggered",
    "on_actionSaveAs_triggered",
    "on_actionRevert_triggered",
    "on_actionBootOption_triggered",
    "on_actionToggleMiniMode_triggered",
    "on_actionToggleShade_triggered",
    "on_actionLogWindow_triggered",
    "showHideDrives",
    "sioFinished",
    "sioStarted",
    "sioStatusChanged",
    "status",
    "textPrinterWindowClosed",
    "docDisplayWindowClosed",
    "deviceStatusChanged",
    "deviceNo",
    "uiMessage",
    "t",
    "message",
    "trayIconActivated",
    "QSystemTrayIcon::ActivationReason",
    "reason",
    "keepBootExeOpen",
    "saveWindowGeometry",
    "saveMiniWindowGeometry",
    "logChanged",
    "text",
    "changeFonts"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN10MainWindowE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      60,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,  374,    2, 0x06,    1 /* Public */,
       5,    1,  379,    2, 0x06,    4 /* Public */,
       7,    1,  382,    2, 0x06,    6 /* Public */,
       9,    1,  385,    2, 0x06,    8 /* Public */,
      11,    1,  388,    2, 0x06,   10 /* Public */,
      13,    1,  391,    2, 0x06,   12 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      15,    0,  394,    2, 0x0a,   14 /* Public */,
      16,    2,  395,    2, 0x0a,   15 /* Public */,
      16,    1,  400,    2, 0x2a,   18 /* Public | MethodCloned */,
      16,    0,  403,    2, 0x2a,   20 /* Public | MethodCloned */,
      19,    2,  404,    2, 0x0a,   21 /* Public */,
      22,    2,  409,    2, 0x0a,   24 /* Public */,
      24,    0,  414,    2, 0x0a,   27 /* Public */,
      25,    1,  415,    2, 0x0a,   28 /* Public */,
      26,    1,  418,    2, 0x0a,   30 /* Public */,
      27,    1,  421,    2, 0x0a,   32 /* Public */,
      29,    0,  424,    2, 0x08,   34 /* Private */,
      30,    0,  425,    2, 0x08,   35 /* Private */,
      31,    0,  426,    2, 0x08,   36 /* Private */,
      32,    0,  427,    2, 0x08,   37 /* Private */,
      33,    0,  428,    2, 0x08,   38 /* Private */,
      34,    0,  429,    2, 0x08,   39 /* Private */,
      35,    0,  430,    2, 0x08,   40 /* Private */,
      36,    0,  431,    2, 0x08,   41 /* Private */,
      37,    0,  432,    2, 0x08,   42 /* Private */,
      38,    0,  433,    2, 0x08,   43 /* Private */,
      39,    0,  434,    2, 0x08,   44 /* Private */,
      40,    0,  435,    2, 0x08,   45 /* Private */,
      41,    0,  436,    2, 0x08,   46 /* Private */,
      42,    0,  437,    2, 0x08,   47 /* Private */,
      43,    0,  438,    2, 0x08,   48 /* Private */,
      44,    0,  439,    2, 0x08,   49 /* Private */,
      36,    1,  440,    2, 0x08,   50 /* Private */,
      35,    1,  443,    2, 0x08,   52 /* Private */,
      46,    1,  446,    2, 0x08,   54 /* Private */,
      47,    2,  449,    2, 0x08,   56 /* Private */,
      49,    1,  454,    2, 0x08,   59 /* Private */,
      50,    1,  457,    2, 0x08,   61 /* Private */,
      51,    1,  460,    2, 0x08,   63 /* Private */,
      52,    1,  463,    2, 0x08,   65 /* Private */,
      53,    1,  466,    2, 0x08,   67 /* Private */,
      54,    1,  469,    2, 0x08,   69 /* Private */,
      55,    0,  472,    2, 0x08,   71 /* Private */,
      56,    0,  473,    2, 0x08,   72 /* Private */,
      57,    0,  474,    2, 0x08,   73 /* Private */,
      58,    0,  475,    2, 0x08,   74 /* Private */,
      59,    0,  476,    2, 0x08,   75 /* Private */,
      60,    0,  477,    2, 0x08,   76 /* Private */,
      61,    0,  478,    2, 0x08,   77 /* Private */,
      62,    1,  479,    2, 0x08,   78 /* Private */,
      64,    0,  482,    2, 0x08,   80 /* Private */,
      65,    0,  483,    2, 0x08,   81 /* Private */,
      66,    1,  484,    2, 0x08,   82 /* Private */,
      68,    2,  487,    2, 0x08,   84 /* Private */,
      71,    1,  492,    2, 0x08,   87 /* Private */,
      74,    0,  495,    2, 0x08,   89 /* Private */,
      75,    0,  496,    2, 0x08,   90 /* Private */,
      76,    0,  497,    2, 0x08,   91 /* Private */,
      77,    1,  498,    2, 0x08,   92 /* Private */,
      79,    0,  501,    2, 0x08,   94 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void, QMetaType::Bool,    8,
    QMetaType::Void, QMetaType::QString,   10,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::QFont,   14,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Int, QMetaType::Int, QMetaType::Bool,   17,   18,
    QMetaType::Int, QMetaType::Int,   17,
    QMetaType::Int,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,   20,   21,
    QMetaType::Void, QMetaType::Int, QMetaType::Bool,   20,   23,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   21,
    QMetaType::Void, QMetaType::QString,   21,
    QMetaType::Void, QMetaType::Bool,   28,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   45,
    QMetaType::Void, QMetaType::Int,   45,
    QMetaType::Void, QMetaType::Int,   45,
    QMetaType::Void, QMetaType::Int, QMetaType::Bool,   45,   48,
    QMetaType::Void, QMetaType::QString,   21,
    QMetaType::Void, QMetaType::Int,   45,
    QMetaType::Void, QMetaType::Int,   45,
    QMetaType::Void, QMetaType::Int,   45,
    QMetaType::Void, QMetaType::Int,   45,
    QMetaType::Void, QMetaType::Int,   45,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   63,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   67,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,   69,   70,
    QMetaType::Void, 0x80000000 | 72,   73,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   78,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_ZN10MainWindowE.offsetsAndSizes,
    qt_meta_data_ZN10MainWindowE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN10MainWindowE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'logMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'newSlot'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'fileMounted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'sendLogText'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'sendLogTextChange'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'setFont'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QFont &, std::false_type>,
        // method 'show'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'firstEmptyDiskSlot'
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'firstEmptyDiskSlot'
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'firstEmptyDiskSlot'
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'mountFileWithDefaultProtection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'autoCommit'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'openRecent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'bootExeTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'bootCasTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'printServer'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'on_actionPlaybackCassette_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionShowPrinterTextOutput_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionBootExe_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionSaveSession_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionOpenSession_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionNewImage_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionMountFolder_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionMountDisk_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionEjectAll_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionOptions_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionStartEmulation_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionPrinterEmulation_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionHideShowDrives_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionQuit_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionAbout_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionDocumentation_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionMountDisk_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'on_actionMountFolder_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'on_actionEject_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'on_actionWriteProtect_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'on_actionMountRecent_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'on_actionEditDisk_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'on_actionSave_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'on_actionAutoSave_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'on_actionSaveAs_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'on_actionRevert_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'on_actionBootOption_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionToggleMiniMode_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionToggleShade_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'on_actionLogWindow_triggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showHideDrives'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sioFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sioStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sioStatusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'textPrinterWindowClosed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'docDisplayWindowClosed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'deviceStatusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'uiMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString, std::false_type>,
        // method 'trayIconActivated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QSystemTrayIcon::ActivationReason, std::false_type>,
        // method 'keepBootExeOpen'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveWindowGeometry'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveMiniWindowGeometry'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'logChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'changeFonts'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->logMessage((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->newSlot((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 2: _t->fileMounted((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->sendLogText((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->sendLogTextChange((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 5: _t->setFont((*reinterpret_cast< std::add_pointer_t<QFont>>(_a[1]))); break;
        case 6: _t->show(); break;
        case 7: { int _r = _t->firstEmptyDiskSlot((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 8: { int _r = _t->firstEmptyDiskSlot((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 9: { int _r = _t->firstEmptyDiskSlot();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 10: _t->mountFileWithDefaultProtection((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 11: _t->autoCommit((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 12: _t->openRecent(); break;
        case 13: _t->bootExeTriggered((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 14: _t->bootCasTriggered((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 15: _t->printServer((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 16: _t->on_actionPlaybackCassette_triggered(); break;
        case 17: _t->on_actionShowPrinterTextOutput_triggered(); break;
        case 18: _t->on_actionBootExe_triggered(); break;
        case 19: _t->on_actionSaveSession_triggered(); break;
        case 20: _t->on_actionOpenSession_triggered(); break;
        case 21: _t->on_actionNewImage_triggered(); break;
        case 22: _t->on_actionMountFolder_triggered(); break;
        case 23: _t->on_actionMountDisk_triggered(); break;
        case 24: _t->on_actionEjectAll_triggered(); break;
        case 25: _t->on_actionOptions_triggered(); break;
        case 26: _t->on_actionStartEmulation_triggered(); break;
        case 27: _t->on_actionPrinterEmulation_triggered(); break;
        case 28: _t->on_actionHideShowDrives_triggered(); break;
        case 29: _t->on_actionQuit_triggered(); break;
        case 30: _t->on_actionAbout_triggered(); break;
        case 31: _t->on_actionDocumentation_triggered(); break;
        case 32: _t->on_actionMountDisk_triggered((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 33: _t->on_actionMountFolder_triggered((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 34: _t->on_actionEject_triggered((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 35: _t->on_actionWriteProtect_triggered((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 36: _t->on_actionMountRecent_triggered((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 37: _t->on_actionEditDisk_triggered((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 38: _t->on_actionSave_triggered((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 39: _t->on_actionAutoSave_triggered((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 40: _t->on_actionSaveAs_triggered((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 41: _t->on_actionRevert_triggered((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 42: _t->on_actionBootOption_triggered(); break;
        case 43: _t->on_actionToggleMiniMode_triggered(); break;
        case 44: _t->on_actionToggleShade_triggered(); break;
        case 45: _t->on_actionLogWindow_triggered(); break;
        case 46: _t->showHideDrives(); break;
        case 47: _t->sioFinished(); break;
        case 48: _t->sioStarted(); break;
        case 49: _t->sioStatusChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 50: _t->textPrinterWindowClosed(); break;
        case 51: _t->docDisplayWindowClosed(); break;
        case 52: _t->deviceStatusChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 53: _t->uiMessage((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 54: _t->trayIconActivated((*reinterpret_cast< std::add_pointer_t<QSystemTrayIcon::ActivationReason>>(_a[1]))); break;
        case 55: _t->keepBootExeOpen(); break;
        case 56: _t->saveWindowGeometry(); break;
        case 57: _t->saveMiniWindowGeometry(); break;
        case 58: _t->logChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 59: _t->changeFonts(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (MainWindow::*)(int , const QString & );
            if (_q_method_type _q_method = &MainWindow::logMessage; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (MainWindow::*)(int );
            if (_q_method_type _q_method = &MainWindow::newSlot; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (MainWindow::*)(bool );
            if (_q_method_type _q_method = &MainWindow::fileMounted; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (MainWindow::*)(QString );
            if (_q_method_type _q_method = &MainWindow::sendLogText; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _q_method_type = void (MainWindow::*)(QString );
            if (_q_method_type _q_method = &MainWindow::sendLogTextChange; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _q_method_type = void (MainWindow::*)(const QFont & );
            if (_q_method_type _q_method = &MainWindow::setFont; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN10MainWindowE.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 60)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 60;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 60)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 60;
    }
    return _id;
}

// SIGNAL 0
void MainWindow::logMessage(int _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void MainWindow::newSlot(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void MainWindow::fileMounted(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void MainWindow::sendLogText(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void MainWindow::sendLogTextChange(QString _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void MainWindow::setFont(const QFont & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
