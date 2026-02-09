/*
 * mainwindow.cpp
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QToolButton>
#include <QLayout>
#include "tnfsbrowser.h"
#include "tnfsimage.h"

#include "diskimage.h"
#include "diskimagepro.h"
#include "diskimageatx.h"
#include "drivewidget.h"
#include "folderimage.h"
#include "pclink.h"
#include "miscdevices.h"
#include "pipenetwork.h"
#include "aspeqtsettings.h"
#include "autobootdialog.h"
#include "autoboot.h"
#include "cassettedialog.h"
#include "bootoptionsdialog.h"
#include "logdisplaydialog.h"
#include "infowidget.h"

#include <QEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QUrl>
#include <QScrollBar>
#include <QTranslator>
#include <QMessageBox>
#include <QWidget>
#include <QDrag>
#include <QtDebug>
#include <QScreen>
#include <QWindow>
#include <QFont>
#include <QClipboard>

#include "atarifilesystem.h"
// #include "miscutils.h"



AspeQtSettings *aspeqtSettings;
static MainWindow *mainWindow;

static QFile *logFile;
static QMutex *logMutex;

QString g_exefileName;
QString g_rclFileName;
QString g_aspeQtAppPath;
QRect g_savedGeometry;

char g_aspeclSlotNo;
bool g_disablePicoHiSpeed;
bool g_D9DOVisible = true;
bool g_miniMode = false;
bool g_shadeMode = false;
int g_savedWidth;
static QString g_lastTnfsUrl = "";


// ****************************** END OF GLOBALS ************************************//

// Displayed only in debug mode    "!d"
// Unimportant     (gray)          "!u"
// Normal          (black)         "!n"
// Important       (blue)          "!i"
// Warning         (brown)         "!w"
// Error           (red)           "!e"

void logMessageOutput(QtMsgType type, const QMessageLogContext &/*context*/, const QString &msg)
{
    logMutex->lock();
    logFile->write(QString::number((quint64)QThread::currentThreadId(), 16).toLatin1());
    switch (type) {
#if QT_VERSION >= 0x050500
        case QtInfoMsg:
            logFile->write(": [Info]    ");
            break;
#endif
        case QtDebugMsg:
            logFile->write(": [Debug]    ");
            break;
        case QtWarningMsg:
            logFile->write(": [Warning]  ");
            break;
        case QtCriticalMsg:
            logFile->write(": [Critical] ");
            break;
        case QtFatalMsg:
            logFile->write(": [Fatal]    ");
            break;
    }
    QByteArray localMsg = msg.toLocal8Bit();
    QByteArray displayMsg = localMsg.mid(3);
    logFile->write(displayMsg);
    logFile->write("\n");
    if (type == QtFatalMsg) {
        logFile->close();
        abort();
    }
    logMutex->unlock();

    if (msg[0] == '!') {
#ifdef QT_NO_DEBUG
        if (msg[1] == 'd') {
            return;
        }
#endif
        mainWindow->doLogMessage(localMsg.at(1), displayMsg);
    }
}

void MainWindow::doLogMessage(int type, const QString &msg)
{
    emit logMessage(type, msg);
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), isClosing(false)
{
    // =========================================================================
    // STEP 1: LOGGING SETUP (MUST BE FIRST)
    // =========================================================================
    mainWindow = this;
    g_aspeQtAppPath = QCoreApplication::applicationDirPath();
    g_disablePicoHiSpeed = false;

    // Create/Open the log file immediately
    logFile = new QFile(QDir::temp().absoluteFilePath("aspeqt.log"));
    logFile->open(QFile::WriteOnly | QFile::Truncate | QFile::Unbuffered | QFile::Text);
    logMutex = new QMutex();

    // Install the Message Handler so qDebug() goes to the file
    connect(this, SIGNAL(logMessage(int,QString)), this, SLOT(uiMessage(int,QString)), Qt::QueuedConnection);
    qInstallMessageHandler(logMessageOutput);

    qDebug() << "!d" << tr("AspeQt started at %1.").arg(QDateTime::currentDateTime().toString());



    // =========================================================================
    // STEP 3: UI & SETTINGS SETUP (EXISTING LOGIC)
    // =========================================================================
    logWindow_ = NULL;
    tnfsClient = new TnfsClient(this);
    setAcceptDrops(true);

    /* Remove old temporaries */
    QDir tempDir = QDir::temp();
    QStringList filters;
    filters << "aspeqt-*";
    QFileInfoList list = tempDir.entryInfoList(filters, QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
    foreach(QFileInfo file, list) {
        deltree(file.absoluteFilePath());
    }

    QCoreApplication::setOrganizationName("13Leader.net");
    QCoreApplication::setOrganizationDomain("https://github.com/pjones1063/AspeQt-2k26/");
    QCoreApplication::setApplicationName("AspeQt");
    aspeqtSettings = new AspeQtSettings();

    loadTranslators();

    /* Setup UI */
    ui->setupUi(this);

    /* Add QActions for most recent */
    for( int i = 0; i < NUM_RECENT_FILES; ++i ) {
        QAction* recentAction = new QAction(this);
        connect(recentAction,SIGNAL(triggered()), this, SLOT(openRecent()));
        recentFilesActions_.append(recentAction);
    }

    QWidget* diskMenu = (QWidget*)menuBar()->children().at(1);
    diskMenu->addActions( recentFilesActions_ );

    createDeviceWidgets();

    this->infoWidget = new InfoWidget();
    ui->rightColumn->addWidget( infoWidget );

    // Parse command line arguments...
    QStringList AspeQtArgs = QCoreApplication::arguments();
    g_sessionFile = g_sessionFilePath = "";
    if (AspeQtArgs.size() > 1) {
        QFile sess;
        QString s = QDir::separator();
        int i = AspeQtArgs.at(1).lastIndexOf(s);
        if (i != -1) {
            i++;
            g_sessionFile = AspeQtArgs.at(1).right(AspeQtArgs.at(1).size() - i);
            g_sessionFilePath = AspeQtArgs.at(1).left(i);
            g_sessionFilePath = QDir::fromNativeSeparators(g_sessionFilePath);
            sess.setFileName(g_sessionFilePath+g_sessionFile);
            if (!sess.exists()) {
                QMessageBox::question(this, tr("Session file error"),
                                      tr("Requested session file not found..."), QMessageBox::Ok);
                g_sessionFile = g_sessionFilePath = "";
            }
        } else {
            if (AspeQtArgs.at(1) != "") {
                g_sessionFile = AspeQtArgs.at(1);
                g_sessionFilePath = QDir::currentPath();
                sess.setFileName(g_sessionFile);
                if (!sess.exists()) {
                    QMessageBox::question(this, tr("Session file error"),
                                          tr("Requested session file not found..."), QMessageBox::Ok);
                    g_sessionFile = g_sessionFilePath = "";
                }
            }
        }
    }

    aspeqtSettings->setSessionFile(g_sessionFile, g_sessionFilePath);
    aspeqtSettings->setMainWindowTitle(g_mainWindowTitle);

    g_mainWindowTitle = tr("AspeQt - Atari Serial Peripheral Emulator for Qt");
    if (g_sessionFile != "") {
        setWindowTitle(g_mainWindowTitle + tr(" -- Session: ") + g_sessionFile);
        aspeqtSettings->loadSessionFromFile(g_sessionFilePath+g_sessionFile);
    } else {
        setWindowTitle(g_mainWindowTitle);
    }
    setGeometry(aspeqtSettings->lastHorizontalPos(),aspeqtSettings->lastVerticalPos(),aspeqtSettings->lastWidth(),aspeqtSettings->lastHeight());

    /* Setup status bar */
    speedLabel = new QLabel(this);
    onOffLabel = new QLabel(this);
    prtOnOffLabel = new QLabel(this);

    clearMessagesLabel = new QLabel(this);
    speedLabel->setText(tr("19200 bits/sec"));
    onOffLabel->setMinimumWidth(21);
    prtOnOffLabel->setMinimumWidth(18);


    clearMessagesLabel->setMinimumWidth(21);
    clearMessagesLabel->setPixmap(QIcon(":/icons/silk-icons/icons/page_white_c.png").pixmap(16, 16, QIcon::Normal));
    clearMessagesLabel->setToolTip(tr("Clear messages"));
    clearMessagesLabel->setStatusTip(clearMessagesLabel->toolTip());

    speedLabel->setMinimumWidth(80);

    dlProgressBar = new QProgressBar(this);
    dlProgressBar->setRange(0, 100);
    dlProgressBar->setValue(0);
    dlProgressBar->setTextVisible(true);
    dlProgressBar->setFixedWidth(150);
    dlProgressBar->hide();

    ui->statusBar->addPermanentWidget(speedLabel);
    ui->statusBar->addPermanentWidget(onOffLabel);
    ui->statusBar->addPermanentWidget(prtOnOffLabel);
    ui->statusBar->addPermanentWidget(clearMessagesLabel);
    ui->statusBar->addPermanentWidget(dlProgressBar);

    // Opacity Slider
    opacitySlider = new QSlider(Qt::Horizontal, this);
    opacitySlider->setRange(20, 90);
    int savedOpacity = aspeqtSettings->shadeOpacity();
    if (savedOpacity < 20) savedOpacity = 60;
    opacitySlider->setValue(savedOpacity);
    opacitySlider->setFixedWidth(100);
    opacitySlider->setToolTip(tr("Adjust Shade Opacity"));
    opacitySlider->hide();


    sio = new SioWorker();

    // -------------------------------------------------------
    // DEVICE $45: PCLINK
    // -------------------------------------------------------
    PCLINK *pcLink = new PCLINK(sio);
    pcLink->setParent(nullptr);
    pcLink->moveToThread(sio);
    sio->installDevice(PCLINK_CDEVIC, pcLink);

    // -------------------------------------------------------
    // DEVICE $46: AspeQt Client (AspeCl)
    // -------------------------------------------------------
    AspeCl *client = new AspeCl(sio);
    client->setParent(nullptr);
    client->moveToThread(sio);
    sio->installDevice(0x46, client);

    // -------------------------------------------------------
    // DEVICE $57: Pipe Network (W:)
    // -------------------------------------------------------
    PipeNetwork *pipeNet = new PipeNetwork(sio);
    pipeNet->setParent(nullptr);
    pipeNet->moveToThread(sio);
    sio->installDevice(0x57, pipeNet);

    // -------------------------------------------------------
    // DEVICE $59: Clipboard (Y:)
    // -------------------------------------------------------
    ClipboardDevice *clip = new ClipboardDevice(sio);
    clip->setParent(nullptr);
    clip->moveToThread(sio);

    // Connect Signal (Background) -> Slot (Main Thread)
    connect(clip, &ClipboardDevice::requestClipSet, this, [](QString text) {
        QClipboard *cb = QGuiApplication::clipboard();
        if (cb) cb->setText(text);
    });
    sio->installDevice(0x59, clip);


    connect(opacitySlider, &QSlider::valueChanged, this, [this](int value){
        if (g_shadeMode) {
            setWindowOpacity(value / 100.0);
        }
    });

    connect(opacitySlider, &QSlider::sliderReleased, this, [this](){
        if (g_shadeMode && this->underMouse()) {
            setWindowOpacity(1.0);
        }
        aspeqtSettings->setShadeOpacity(opacitySlider->value());
    });

    ui->statusBar->addPermanentWidget(opacitySlider);

    ui->textEdit->installEventFilter(mainWindow);
    changeFonts();
    g_D9DOVisible =  aspeqtSettings->D9DOVisible();
    showHideDrives();

    connect(sio, SIGNAL(started()), this, SLOT(sioStarted()));
    connect(sio, SIGNAL(finished()), this, SLOT(sioFinished()));
    connect(sio, SIGNAL(statusChanged(QString)), this, SLOT(sioStatusChanged(QString)));



    shownFirstTime = true;

    // Restore State
    for (int i = 0; i < DISK_COUNT; i++) {
        AspeQtSettings::ImageSettings is;
        is = aspeqtSettings->mountedImageSetting(i);
        mountFile(i, is.fileName, is.isWriteProtected);
    }

    updateRecentFileActions();

    // -------------------------------------------------------
    // DEVICE: Smart Device
    // -------------------------------------------------------
    SmartDevice *smart = new SmartDevice(sio);
    smart->setParent(nullptr);
    smart->moveToThread(sio);
    sio->installDevice(SMART_CDEVIC, smart);

    textPrinterWindow = new TextPrinterWindow();
    docDisplayWindow = new DocDisplayWindow();

    connect(textPrinterWindow, SIGNAL(closed()), this, SLOT(textPrinterWindowClosed()));
    connect(docDisplayWindow, SIGNAL(closed()), this, SLOT(docDisplayWindowClosed()));


    // -------------------------------------------------------
    // DEVICE: Printer (P:)
    // -------------------------------------------------------
    Printer *printer = new Printer(sio);
    printer->setParent(nullptr);
    printer->moveToThread(sio);

    connect(printer, SIGNAL(print(QString)), textPrinterWindow, SLOT(print(QString)));
    sio->installDevice(PRINTER_BASE_CDEVIC, printer);
    setUpPrinterEmulationWidgets(aspeqtSettings->printerEmulation());

    ui->menuBar->installEventFilter(this);
    untitledName = 0;

    connect(&trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon.setIcon(windowIcon());
}

MainWindow::~MainWindow()
{
    if (ui->actionStartEmulation->isChecked()) {
        ui->actionStartEmulation->trigger();
    }

    delete aspeqtSettings;
    delete sio;

    delete ui;

    qDebug() << "!d" << tr("AspeQt stopped at %1.").arg(QDateTime::currentDateTime().toString());
    qInstallMessageHandler(0);
    delete logMutex;
    delete logFile;
}

void MainWindow::createDeviceWidgets()
{
    // 1. Calculate DPI-aware sizes based on the current platform's style
    int iconMetric = style()->pixelMetric(QStyle::PM_SmallIconSize);
    QSize dpiIconSize(iconMetric, iconMetric);
    int buttonDimension = iconMetric + 8; // Adds padding around the icon for better clickability

    for (int i = 0; i < DISK_COUNT; i++) {
        DriveWidget* deviceWidget = new DriveWidget(i);
        deviceWidget->setMinimumHeight(buttonDimension + 12);

        if (i < 8) {
            ui->leftColumn->addWidget(deviceWidget);
        } else {
            ui->rightColumn->addWidget(deviceWidget);
        }

        const AspeQtSettings::ImageSettings& is = aspeqtSettings->mountedImageSetting(i);
        deviceWidget->setHappyMode(is.isHappyMode);

        deviceWidget->setup();
        diskWidgets[i] = deviceWidget;

        // 2. Standardize existing internal buttons in the DriveWidget
        // This ensures the Mount, Save, and Eject buttons also scale on High DPI screens
        deviceWidget->setIconSize(dpiIconSize);

        // --- NEW: Mount TNFS Action ---
        QAction *mountTnfsAction = new QAction(QIcon(":icons/silk-icons/icons/world.png"), tr("Mount from TNFS Network..."), this);

        if (mountTnfsAction->icon().isNull()) {
            mountTnfsAction->setIcon(QIcon(":icons/silk-icons/icons/connect.png"));
        }

        connect(mountTnfsAction, &QAction::triggered, this, [this, i]() {
            on_actionMountTnfs_triggered(i);
        });

        // 3. Add to Context Menu (Right Click)
        diskWidgets[i]->addAction(mountTnfsAction);

        // 4. Create the TNFS Button with DPI-aware dimensions
        QToolButton *btnTnfs = new QToolButton(diskWidgets[i]);
        btnTnfs->setDefaultAction(mountTnfsAction);
        btnTnfs->setAutoRaise(false);

        // Use the calculated DPI-aware dimensions instead of hardcoded 22x22
        btnTnfs->setFixedSize(buttonDimension, buttonDimension);
        btnTnfs->setIconSize(dpiIconSize);

        btnTnfs->setToolTip(tr("Mount TNFS"));

        if (diskWidgets[i]->layout()) {
            QBoxLayout *layout = qobject_cast<QBoxLayout*>(diskWidgets[i]->layout());
            if (layout) {
                // Inserts at index 1 to sit between standard Mount and Save buttons
                layout->insertWidget(1, btnTnfs);
            } else {
                diskWidgets[i]->layout()->addWidget(btnTnfs);
            }
        }


        // Connect existing signals to slots
        connect(deviceWidget, SIGNAL(actionMountDisk(int)), this, SLOT(on_actionMountDisk_triggered(int)));
        connect(deviceWidget, SIGNAL(actionMountFolder(int)), this, SLOT(on_actionMountFolder_triggered(int)));
        connect(deviceWidget, SIGNAL(actionAutoSave(int)), this, SLOT(on_actionAutoSave_triggered(int)));
        connect(deviceWidget, SIGNAL(actionEject(int)), this, SLOT(on_actionEject_triggered(int)));
        connect(deviceWidget, SIGNAL(actionWriteProtect(int,bool)), this, SLOT(on_actionWriteProtect_triggered(int,bool)));
        connect(deviceWidget, SIGNAL(actionEditDisk(int)), this, SLOT(on_actionEditDisk_triggered(int)));
        connect(deviceWidget, SIGNAL(actionSave(int)), this, SLOT(on_actionSave(int)));
        connect(deviceWidget, SIGNAL(actionRevert(int)), this, SLOT(on_actionRevert(int)));
        connect(deviceWidget, SIGNAL(actionSaveAs(int)), this, SLOT(on_actionSaveAs_triggered(int)));
        connect(deviceWidget, SIGNAL(actionBootOptions(int)), this, SLOT(on_actionBootOption_triggered()));

        connect(this, SIGNAL(setFont(const QFont&)), deviceWidget, SLOT(setFont(const QFont&)));
        connect(deviceWidget, SIGNAL(actionHappyMode(int,bool)), this, SLOT(on_actionHappyMode_triggered(int,bool)));
    }

    ui->leftColumn->setAlignment(Qt::AlignTop);
    ui->rightColumn->setAlignment(Qt::AlignTop);

    changeFonts();
}


 void MainWindow::mousePressEvent(QMouseEvent *event)
 {
     int slot = containingDiskSlot(event->pos());

     if (event->button() == Qt::LeftButton
         && slot >= 0) {

         QDrag *drag = new QDrag((QWidget*)this);
         QMimeData *mimeData = new QMimeData;

         mimeData->setData("application/x-aspeqt-disk-image", QByteArray(1, slot));
         drag->setMimeData(mimeData);

         drag->exec();
     }

     if (event->button() == Qt::LeftButton && onOffLabel->geometry().translated(ui->statusBar->geometry().topLeft()).contains(event->pos())) {
         ui->actionStartEmulation->trigger();
     }

     if (event->button() == Qt::LeftButton && prtOnOffLabel->geometry().translated(ui->statusBar->geometry().topLeft()).contains(event->pos())) {
         ui->actionPrinterEmulation->trigger();     //
     }
     if (event->button() == Qt::LeftButton && clearMessagesLabel->geometry().translated(ui->statusBar->geometry().topLeft()).contains(event->pos())) {
         ui->textEdit->clear();
         emit sendLogText("");
     }
     if (event->button() == Qt::LeftButton && !speedLabel->isHidden() && speedLabel->geometry().translated(ui->statusBar->geometry().topLeft()).contains(event->pos())) {
        ui->actionOptions->trigger();
     }
}

 void MainWindow::dragEnterEvent(QDragEnterEvent *event)
 {
     if (event->mimeData()->hasUrls()) {
         event->acceptProposedAction();
     } else {
         event->ignore();
     }
 }

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    int i = containingDiskSlot(event->pos());
    if (i >= 0 && (event->mimeData()->hasUrls() ||
                   event->mimeData()->hasFormat("application/x-aspeqt-disk-image"))) {
        event->acceptProposedAction();
    } else {
        i = -1;
    }
    for (int j = 0; j < DISK_COUNT; j++) { //
        if (i == j) {
            diskWidgets[j]->setFrameShadow(QFrame::Sunken);
        } else {
            diskWidgets[j]->setFrameShadow(QFrame::Raised);
        }
    }
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *)
{
    for (int j = 0; j < DISK_COUNT; j++) { //
        diskWidgets[j]->setFrameShadow(QFrame::Raised);
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    for (int j = 0; j < DISK_COUNT; j++) { //
        diskWidgets[j]->setFrameShadow(QFrame::Raised);
    }
    int slot = containingDiskSlot(event->pos());
    if (!(event->mimeData()->hasUrls() ||
          event->mimeData()->hasFormat("application/x-aspeqt-disk-image")) ||
          slot < 0) {
        return;
    }

    if (event->mimeData()->hasFormat("application/x-aspeqt-disk-image")) {
        int source = event->mimeData()->data("application/x-aspeqt-disk-image").at(0);
        if (slot != source) {
            sio->swapDevices(slot + DISK_BASE_CDEVIC, source + DISK_BASE_CDEVIC);
            aspeqtSettings->swapImages(slot, source);

            PCLINK* pclink = reinterpret_cast<PCLINK*>(sio->getDevice(PCLINK_CDEVIC));
            if(pclink->hasLink(slot+1) || pclink->hasLink(source+1))
            {
                sio->uninstallDevice(PCLINK_CDEVIC);
                pclink->swapLinks(slot+1,source+1);
                sio->installDevice(PCLINK_CDEVIC,pclink);
            }
            qDebug() << "!n" << tr("Swapped disk %1 with disk %2.").arg(slot + 1).arg(source + 1);
        }
        return;
    }

    QStringList files;
    foreach (QUrl url, event->mimeData()->urls()) {
        if (!url.toLocalFile().isEmpty()) {
            files.append(url.toLocalFile());
        }
    }
    if (files.isEmpty()) {
        return;
    }

    FileTypes::FileType type = FileTypes::getFileType(files.at(0));

    if (type == FileTypes::Xex) {
        g_exefileName = files.at(0);  //
        bootExe(files.at(0));
        return;
    }

    if (type == FileTypes::Cas) {
        bool restart;
        restart = ui->actionStartEmulation->isChecked();
        if (restart) {
            ui->actionStartEmulation->trigger();
            sio->wait();
            qApp->processEvents();
        }

        CassetteDialog *dlg = new CassetteDialog(this, files.at(0));
        dlg->exec();
        delete dlg;

        if (restart) {
            ui->actionStartEmulation->trigger();
        }
        return;
    }

    mountFileWithDefaultProtection(slot, files[0]);
    files.removeAt(0);
    while (!files.isEmpty() && (slot = firstEmptyDiskSlot(slot, false)) >= 0) {
        mountFileWithDefaultProtection(slot, files[0]);
        files.removeAt(0);
    }
    slot = 0;
    while (!files.isEmpty() && (slot = firstEmptyDiskSlot(slot, false)) >= 0) {
        mountFileWithDefaultProtection(slot, files[0]);
        files.removeAt(0);
    }
    foreach(QString file, files) {
        qCritical() << "!e" << tr("Cannot mount '%1': No empty disk slots.").arg(file);
    }
    event->acceptProposedAction();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if(isClosing)
        return;
    isClosing = true;

    // Save various session settings  //
    if (aspeqtSettings->saveWindowsPos()) {
        if (g_miniMode) {
            saveMiniWindowGeometry();
        } else {
            saveWindowGeometry();
        }
    }
    if (g_sessionFile != "") aspeqtSettings->saveSessionToFile(g_sessionFilePath + "/" + g_sessionFile);
    aspeqtSettings->setD9DOVisible(g_D9DOVisible);
    bool wasRunning = ui->actionStartEmulation->isChecked();
    QMessageBox::StandardButton answer = QMessageBox::No;

    if (wasRunning) {
        ui->actionStartEmulation->trigger();
    }

    int toBeSaved = 0;

    for (int i = 0; i < DISK_COUNT; i++) {      //
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
        if (img && img->isModified()) {
            toBeSaved++;
        }
    }

    for (int i = 0; i < DISK_COUNT; i++) {      //
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
        if (img && img->isModified()) {
            toBeSaved--;
            answer = saveImageWhenClosing(i, answer, toBeSaved);
            if (answer == QMessageBox::NoToAll) {
                break;
            }
            if (answer == QMessageBox::Cancel) {
                if (wasRunning) {
                    ui->actionStartEmulation->trigger();
                }
                event->ignore();
                return;
            }
        }
    }

    //close any disk edit dialogs we have open
    for (int i = 0; i < DISK_COUNT; i++) {
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
        if (img && img->editDialog()) img->editDialog()->close();
    }

    delete textPrinterWindow;
    textPrinterWindow = NULL;
    //
    delete docDisplayWindow;
    docDisplayWindow = NULL;

    for (int i = DISK_BASE_CDEVIC; i < (DISK_BASE_CDEVIC+DISK_COUNT); i++) {
        SimpleDiskImage *s = qobject_cast <SimpleDiskImage*> (sio->getDevice(i));
        if (s) {
            s->close();
        }
    }

    event->accept();

}

void MainWindow::hideEvent(QHideEvent *event)
{
    if (aspeqtSettings->minimizeToTray()) {
        trayIcon.show();
        oldWindowFlags = windowFlags();
        oldWindowStates = windowState();
        setWindowFlags(Qt::Widget);
        hide();
        event->ignore();
        return;
    }
    QMainWindow::hideEvent(event);
}

void MainWindow::show()
{
    QMainWindow::show();
    if (shownFirstTime) {
        /* Open options dialog if it's the first time */
        if (aspeqtSettings->isFirstTime()) {
            if (QMessageBox::Yes == QMessageBox::question(this, tr("First run"),
                                       tr("You are running AspeQt for the first time.\n\nDo you want to open the options dialog?"),
                                       QMessageBox::Yes, QMessageBox::No)) {
                ui->actionOptions->trigger();
            }
        }
        qDebug() << "!d" << "Starting emulation";

        ui->actionStartEmulation->trigger();
    }
}

void MainWindow::enterEvent(QEnterEvent *event)
{
    if (g_miniMode && g_shadeMode) {
        setWindowOpacity(1.0);
    }

    QMainWindow::enterEvent(event);
}

void MainWindow::leaveEvent(QEvent *)
{
    if (g_miniMode && g_shadeMode) {
        // Use slider value (e.g., 60 becomes 0.6)
        setWindowOpacity(opacitySlider->value() / 100.0);
    }
}

void MainWindow::resizeEvent(QResizeEvent *)
{
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick) {
        on_actionLogWindow_triggered();
    }

    // 2. NEW LOGIC: Dragging by Menu Bar in Shade Mode
    if (obj == ui->menuBar && g_shadeMode) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        // A. On Click: Remember the offset, but IGNORE if clicking a menu item (File, Tools, etc.)
        if (event->type() == QEvent::MouseButtonPress && mouseEvent->button() == Qt::LeftButton) {
            if (ui->menuBar->actionAt(mouseEvent->pos())) {
                return false; // User clicked a menu item, let Qt handle it
            }
            m_dragPosition = mouseEvent->globalPosition().toPoint() - frameGeometry().topLeft();
            return true;
        }

        // B. On Move: Move the window relative to the mouse
        if (event->type() == QEvent::MouseMove && (mouseEvent->buttons() & Qt::LeftButton)) {
            move(mouseEvent->globalPosition().toPoint() - m_dragPosition);
            return true;
        }
    }


    return false;
}

void MainWindow::on_actionLogWindow_triggered()
{
    if (logWindow_ == NULL )
    {
        logWindow_ = new LogDisplayDialog(this);
        int x, y, w, h;
        x = geometry().x();
        y = geometry().y();
        w = geometry().width();
        h = geometry().height();
        if (!g_miniMode) {
            logWindow_->setGeometry(x+w/1.9, y+30, logWindow_->geometry().width(), geometry().height());
        } else {
            logWindow_->setGeometry(x+20, y+60, w, h*2);
        }
        connect(this, SIGNAL(sendLogText(QString)), logWindow_, SLOT(getLogText(QString)));
        connect(this, SIGNAL(sendLogTextChange(QString)), logWindow_, SLOT(getLogTextChange(QString)));
        emit sendLogText(ui->textEdit->toHtml());
    }

    logWindow_->show();
}

void MainWindow::logChanged(QString text)
{
    emit sendLogTextChange(text);
}

void MainWindow::saveWindowGeometry()
{
    aspeqtSettings->setLastHorizontalPos(geometry().x());
    aspeqtSettings->setLastVerticalPos(geometry().y());
    aspeqtSettings->setLastWidth(geometry().width());
    aspeqtSettings->setLastHeight(geometry().height());
}

void MainWindow::saveMiniWindowGeometry()
{
    aspeqtSettings->setLastMiniHorizontalPos(geometry().x());
    aspeqtSettings->setLastMiniVerticalPos(geometry().y());
    aspeqtSettings->setShadeOpacity(opacitySlider->value());
}

void MainWindow::on_actionToggleShade_triggered()
{
    if (g_shadeMode) {
        // --- TURNING OFF SHADE MODE ---
        setWindowFlags(Qt::WindowSystemMenuHint);
        setWindowOpacity(1.0);
        opacitySlider->hide(); // Hide slider
        g_shadeMode = false;
        QMainWindow::show();
    } else {
        // --- TURNING ON SHADE MODE ---
        setWindowFlags(Qt::FramelessWindowHint);

        // Use the slider value immediately instead of hardcoded 0.25
        double opacity = opacitySlider->value() / 100.0;
        setWindowOpacity(opacity);

        // Only show slider if we are also in Mini Mode
        if (g_miniMode) {
            opacitySlider->show();
        }

        g_shadeMode = true;
        QMainWindow::show();
    }
}


// Toggle Mini Mode //
void MainWindow::on_actionToggleMiniMode_triggered()
{
    g_miniMode = !g_miniMode;

    int i;
    for( i = 1; i < 8; ++i ) {
        diskWidgets[i]->setVisible( !g_miniMode );
    }

    for( ; i < DISK_COUNT; ++i ) {
        diskWidgets[i]->setVisible( !g_miniMode && g_D9DOVisible );
    }

    if(!g_miniMode) {
        // --- RESTORE NORMAL MODE ---
        if (g_D9DOVisible) {
            setMinimumWidth(688);
        } else {
            setMinimumWidth(344);
        }

        setMinimumHeight(426);
        setMaximumHeight(QWIDGETSIZE_MAX);
        ui->textEdit->setVisible(true);
        ui->actionHideShowDrives->setEnabled(true);
        saveMiniWindowGeometry();
        setGeometry(g_savedGeometry);

        setWindowOpacity(1.0);
        setWindowFlags(Qt::WindowSystemMenuHint);

        ui->actionToggleShade->setDisabled(true);

        // Hide the slider when leaving mini mode
        opacitySlider->hide();

        g_shadeMode = false;
        QMainWindow::show();

    } else {
        // --- ENTER MINI MODE ---

        g_savedGeometry = geometry();
        ui->textEdit->setVisible(false);
        setMinimumWidth(400);

        // --- FIX: Dynamic Height for High DPI ---
        // Prevents the window from collapsing to 0 height or cutting off buttons
        int metric = style()->pixelMetric(QStyle::PM_SmallIconSize);
        int miniHeight = qMax(140, metric * 6);

        setMinimumHeight(miniHeight);
        setMaximumHeight(miniHeight);

        setGeometry(aspeqtSettings->lastMiniHorizontalPos(), aspeqtSettings->lastMiniVerticalPos(),
                    minimumWidth(), minimumHeight());

        ui->actionHideShowDrives->setDisabled(true);
        ui->actionToggleShade->setEnabled(true);

        if (aspeqtSettings->enableShade()) {
            setWindowFlags(Qt::FramelessWindowHint);
            g_shadeMode = true;

            // --- FIX: Smart Opacity Check ---
            // If the mouse is already over the window, stay opaque (1.0).
            // If the mouse is away, apply the slider transparency.
            if (this->underMouse()) {
                setWindowOpacity(1.0);
            } else {
                setWindowOpacity(opacitySlider->value() / 100.0);
            }

        } else {
            g_shadeMode = false;
        }

        // --- FIX: Check Slider Visibility AFTER Shade Mode is Finalized ---
        if (g_shadeMode) {
            opacitySlider->show();
        } else {
            opacitySlider->hide();
        }

        QMainWindow::show();
    }
}

void MainWindow::showHideDrives()
{
    for( int i = 8; i < DISK_COUNT; ++i ) {
        diskWidgets[i]->setVisible(g_D9DOVisible);
    }

    infoWidget->setVisible(g_D9DOVisible);

    if( g_D9DOVisible ) {
        ui->actionHideShowDrives->setText(QApplication::translate("MainWindow", "Hide drives D9-DO", 0));
        ui->actionHideShowDrives->setStatusTip(QApplication::translate("MainWindow", "Hide drives D9-DO", 0));
        ui->actionHideShowDrives->setIcon(QIcon(":/icons/silk-icons/icons/drive_add.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
        setMinimumWidth(688);
    } else {
        ui->actionHideShowDrives->setText(QApplication::translate("MainWindow", "Show drives D9-DO", 0));
        ui->actionHideShowDrives->setStatusTip(QApplication::translate("MainWindow", "Show drives D9-DO", 0));
        ui->actionHideShowDrives->setIcon(QIcon(":/icons/silk-icons/icons/drive_delete.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
        setMinimumWidth(344);
    }
}

// Toggle Hide/Show drives D9-DO  //
void MainWindow::on_actionHideShowDrives_triggered()
{
    g_D9DOVisible = !g_D9DOVisible;
    g_miniMode = false;

    showHideDrives();

    setGeometry(geometry().x(), geometry().y(), 0, geometry().height());
    saveWindowGeometry();
}

// Toggle printer Emulation ON/OFF //


void MainWindow::on_actionPrinterEmulation_triggered()
{
    if (aspeqtSettings->printerEmulation())
        printServer(false);
     else
        printServer(true);
}


void MainWindow::printServer(bool enable)
{
    setUpPrinterEmulationWidgets(enable);
    aspeqtSettings->setPrinterEmulation(enable);
    if(!enable)
        qWarning() << "!i" << tr("Printer emulation stopped.");
    else
        qWarning() << "!i" << tr("Printer emulation started.");

}

void MainWindow::setUpPrinterEmulationWidgets(bool enable)
{
    if (enable) {
        ui->actionPrinterEmulation->setText(QApplication::translate("MainWindow", "Stop printer emulation", 0));
        ui->actionPrinterEmulation->setStatusTip(QApplication::translate("MainWindow", "Stop printer emulation", 0));
        ui->actionPrinterEmulation->setIcon(QIcon(":/icons/silk-icons/icons/printer.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
        prtOnOffLabel->setPixmap(QIcon(":/icons/silk-icons/icons/printer.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
    } else {
        ui->actionPrinterEmulation->setText(QApplication::translate("MainWindow", "Start printer emulation", 0));
        ui->actionPrinterEmulation->setStatusTip(QApplication::translate("MainWindow", "Start printer emulation", 0));
        ui->actionPrinterEmulation->setIcon(QIcon(":/icons/silk-icons/icons/printer_delete.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
        prtOnOffLabel->setPixmap(QIcon(":/icons/silk-icons/icons/printer_delete.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
    }
    prtOnOffLabel->setToolTip(ui->actionPrinterEmulation->toolTip());
    prtOnOffLabel->setStatusTip(ui->actionPrinterEmulation->statusTip());
}

void MainWindow::on_actionStartEmulation_triggered()
{
    if (ui->actionStartEmulation->isChecked()) {
        sio->start(QThread::TimeCriticalPriority);
    } else {
        sio->setPriority(QThread::NormalPriority);
        sio->wait();
        qApp->processEvents();
    }
}

void MainWindow::sioStarted()
{
    ui->actionStartEmulation->setText(tr("&Stop emulation"));
    ui->actionStartEmulation->setToolTip(tr("Stop SIO peripheral emulation"));
    ui->actionStartEmulation->setStatusTip(tr("Stop SIO peripheral emulation"));
    onOffLabel->setPixmap(ui->actionStartEmulation->icon().pixmap(16, QIcon::Normal, QIcon::On));
    onOffLabel->setToolTip(ui->actionStartEmulation->toolTip());
    onOffLabel->setStatusTip(ui->actionStartEmulation->statusTip());
}

void MainWindow::sioFinished()
{
    ui->actionStartEmulation->setText(tr("&Start emulation"));
    ui->actionStartEmulation->setToolTip(tr("Start SIO peripheral emulation"));
    ui->actionStartEmulation->setStatusTip(tr("Start SIO peripheral emulation"));
    ui->actionStartEmulation->setChecked(false);
    onOffLabel->setPixmap(ui->actionStartEmulation->icon().pixmap(16, QIcon::Normal, QIcon::Off));
    onOffLabel->setToolTip(ui->actionStartEmulation->toolTip());
    onOffLabel->setStatusTip(ui->actionStartEmulation->statusTip());
    speedLabel->hide();
    speedLabel->clear();
    qWarning() << "!i" << tr("Emulation stopped.");
}

void MainWindow::sioStatusChanged(QString status)
{
    speedLabel->setText(status);
    speedLabel->show();
}

void MainWindow::deviceStatusChanged(int deviceNo)
{
    if (deviceNo >= DISK_BASE_CDEVIC && deviceNo < (DISK_BASE_CDEVIC+DISK_COUNT)) { // 0x31 - 0x3E

        // 1. Get Generic Device
        SioDevice *device = sio->getDevice(deviceNo);
        DriveWidget *diskWidget = diskWidgets[deviceNo - DISK_BASE_CDEVIC];

        // 2. Check for TNFS Image FIRST
        TnfsImage *tnfsImg = qobject_cast<TnfsImage*>(device);
        if (tnfsImg) {
            // Force Save and Edit to FALSE (Gray out buttons)
            QString fullUrl = tnfsImg->originalFileName();
            QString fileNameOnly = fullUrl;

            // --- FIX: Extract purely the filename from the URL ---
            int lastSlash = fullUrl.lastIndexOf('/');
            if (lastSlash != -1) {
                fileNameOnly = fullUrl.mid(lastSlash + 1);
            }
            if (fileNameOnly.isEmpty()) fileNameOnly = fullUrl;

            diskWidget->setLabelToolTips(fullUrl, fullUrl, tr("TNFS Network Stream To Ram"));

            // --- FIX: Force Happy Mode OFF for TNFS streams ---
            diskWidget->setHappyMode(false);

            // Arg 3 (Edit) = false, Arg 4 (Save) = false
            diskWidget->showAsTNFSMounted(fileNameOnly, tr("TNFS Stream to RAM"));
            return; // EXIT HERE so SimpleDiskImage logic doesn't override it
        }

        // 3. Fallback to Standard Disk Image Logic
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (device);
        if (img) {

            // Show file name without the path and set toolTip & statusTip to show the path separately //
            QString filenamelabel;
            int i = -1;

            if (img->description() == tr("Folder image")) {
                i = img->originalFileName().lastIndexOf("\\"); // NOTE: This expects folder separators to be '\\'
            } else {
                i = img->originalFileName().lastIndexOf("/");
            }
            if (i == -1) {
                i = img->originalFileName().lastIndexOf("/");
            }
            if ((i != -1) || (img->originalFileName().mid(0, 14) == "Untitled image")) {
                filenamelabel = img->originalFileName().right(img->originalFileName().size() - ++i);
            } else {
                filenamelabel = "!!!!!!!!.!!!";
            }

            diskWidget->setLabelToolTips(img->originalFileName().left(i - 1),
                                         img->originalFileName(),
                                         img->description());

            bool enableEdit = img->editDialog() != 0;

            if (img->description() == tr("Folder image")) {
                diskWidget->showAsFolderMounted(filenamelabel, img->description(), enableEdit);
            } else {
                bool enableSave = false;

                if (img->isModified()) {
                    if (!diskWidget->isAutoSaveEnabled()) {    //
                        enableSave = true;
                    } else {
                        // Image is modified and autosave is checked, so save the image (no need to lock it)  //
                        bool saved;
                        saved = img->save();
                        if (!saved) {
                            int response = QMessageBox::question(this, tr("Save failed"),
                                            tr("'%1' cannot be saved, do you want to save the image with another name?").arg(img->originalFileName()),
                                            QMessageBox::Yes, QMessageBox::No);
                            if (response == QMessageBox::Yes) {
                                saveDiskAs(deviceNo);
                            }
                        }
                    }
                }
                diskWidget->showAsImageMounted(filenamelabel, img->description(), enableEdit, enableSave);
            }
        } else {
            diskWidget->showAsEmpty();
        }
    }
}

void MainWindow::uiMessage(int t, QString message)
{
    if (message.at(0) == '"') {
        message.remove(0, 1);
    }
    if (message.at(message.count() - 1) == ' ' && message.at(message.count() - 2) == '"') {
        message.resize(message.count() - 2);
    }

    if (message == lastMessage) {
        lastMessageRepeat++;
        message = QString("%1 [x%2]").arg(message).arg(lastMessageRepeat);
        ui->textEdit->moveCursor(QTextCursor::End);
        QTextCursor cursor = ui->textEdit->textCursor();
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
    } else {
        lastMessage = message;
        lastMessageRepeat = 1;
    }

    ui->statusBar->showMessage(message, 3000);

    switch (t) {
        case 'd':
            message = QString("<span style='color:green'>%1</span>").arg(message);
            break;
        case 'u':
            message = QString("<span style='color:gray'>%1</span>").arg(message);
            break;
        case 'n':
            message = QString("<span style='color:black'>%1</span>").arg(message);
            break;
        case 'i':
            message = QString("<span style='color:blue'>%1</span>").arg(message);
            break;
        case 'w':
            message = QString("<span style='color:brown'>%1</span>").arg(message);
            break;
        case 'e':
            message = QString("<span style='color:red'>%1</span>").arg(message);
            break;
        default:
            message = QString("<span style='color:purple'>%1</span>").arg(message);
            break;
    }

    ui->textEdit->append(message);
    ui->textEdit->verticalScrollBar()->setSliderPosition(ui->textEdit->verticalScrollBar()->maximum());
    ui->textEdit->horizontalScrollBar()->setSliderPosition(ui->textEdit->horizontalScrollBar()->minimum());
    logChanged(message);
}

void MainWindow::on_actionOptions_triggered()
{
    bool restart;
    restart = ui->actionStartEmulation->isChecked();
    if (restart) {
        ui->actionStartEmulation->trigger();
        sio->wait();
        qApp->processEvents();
    }
    OptionsDialog optionsDialog(this);
    optionsDialog.exec() ;

// Change drive slot description fonts
    changeFonts();

// load translators and retranslate
    loadTranslators();

// retranslate Designer Form
    ui->retranslateUi(this);

    for (int i = DISK_BASE_CDEVIC; i < (DISK_BASE_CDEVIC+DISK_COUNT); i++) {    // 0x31 - 0x3E
        deviceStatusChanged(i);
    }
    
    ui->actionStartEmulation->trigger();
}

void MainWindow::changeFonts()
{
    if (aspeqtSettings->useLargeFont()) {
        QFont font("Arial Black", 9, QFont::Normal);
        emit setFont(font);
    } else {
        QFont font("MS Shell Dlg 2,8", 8, QFont::Normal);
        emit setFont(font);
    }
 }

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog aboutDialog(this, VERSION);
    aboutDialog.exec();
}
//
void MainWindow::on_actionDocumentation_triggered()
{
    QString dir = aspeqtSettings->lastSessionDir();

    if (ui->actionDocumentation->isChecked()) {
        docDisplayWindow->show();
    } else {
        docDisplayWindow->hide();
    }
}
//
void MainWindow::docDisplayWindowClosed()
{
     ui->actionDocumentation->setChecked(false);
}
// Restart emulation and re-translate following a session load //
void MainWindow::setSession()
{
    bool restart;
    restart = ui->actionStartEmulation->isChecked();
    if (restart) {
        ui->actionStartEmulation->trigger();
       sio->wait();
        qApp->processEvents();
    }

    // load translators and retranslate
    loadTranslators();
    ui->retranslateUi(this);
    for (int i = DISK_BASE_CDEVIC; i < (DISK_BASE_CDEVIC+DISK_COUNT); i++) {
        deviceStatusChanged(i);
    }

    ui->actionStartEmulation->trigger();
}



void MainWindow::openRecent()
{
    qDebug("open recent");
    QAction *action = qobject_cast<QAction*>(sender());
    if(action)
    {
        mountFileWithDefaultProtection(firstEmptyDiskSlot(), action->text());
    }
}

void MainWindow::updateRecentFileActions()
{
    for(int i = 0; i < NUM_RECENT_FILES; ++i)
    {
        QAction* action = this->recentFilesActions_[i];
        const AspeQtSettings::ImageSettings& image = aspeqtSettings->recentImageSetting(i);

        if(image.fileName != "" )
        {
            action->setVisible(true);
            action->setText(image.fileName);
        }
        else
        {
            action->setVisible(false);
        }
    }
}


bool MainWindow::ejectImage(int no, bool ask)
{
    PCLINK* pclink = reinterpret_cast<PCLINK*>(sio->getDevice(PCLINK_CDEVIC));
    if(pclink->hasLink(no+1))
    {
        sio->uninstallDevice(PCLINK_CDEVIC);
        pclink->resetLink(no+1);
        sio->installDevice(PCLINK_CDEVIC,pclink);
    }

    // --- FIX: Handle generic SioDevice (Supports TNFS + Disk Images) ---
    SioDevice *device = sio->getDevice(no + DISK_BASE_CDEVIC);
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (device);

    // Only ask to save if it is a Disk Image AND has modifications
    if (ask && img && img->isModified()) {
        QMessageBox::StandardButton answer;
        answer = saveImageWhenClosing(no, QMessageBox::No, 0);
        if (answer == QMessageBox::Cancel) {
            return false;
        }
    }

    // If ANY device exists in this slot (TNFS or Disk), remove it
    if (device) {
        sio->uninstallDevice(no + DISK_BASE_CDEVIC);

        sio->setHighSpeed(false);

        // This virtual destructor cleans up TnfsImage OR SimpleDiskImage
        delete device;

        // Force UI update
        diskWidgets[no]->showAsEmpty();
        aspeqtSettings->unmountImage(no);
        updateRecentFileActions();
        deviceStatusChanged(no + DISK_BASE_CDEVIC);
        qDebug() << "!n" << tr("Unmounted disk %1").arg(no + 1);
    }
    return true;
}


int MainWindow::containingDiskSlot(const QPoint &point)
{
    int i;
    QPoint distance = centralWidget()->geometry().topLeft();
    for (i=0; i < DISK_COUNT; i++) {    //
        QRect rect = diskWidgets[i]->geometry().translated(distance);
        if (rect.contains(point)) {
            break;
        }
    }
    if (i > DISK_COUNT-1) {   //
        i = -1;
    }
    return i;
}

int MainWindow::firstEmptyDiskSlot(int startFrom, bool createOne)
{
    int i;
    for (i = startFrom; i < DISK_COUNT; i++) {  //
        if (!sio->getDevice(DISK_BASE_CDEVIC + i)) {
            break;
        }
    }
    if (i > DISK_COUNT-1) {   //
        if (createOne) {
            i = DISK_COUNT-1;
        } else {
            i = -1;
        }
    }
    emit newSlot(i);        //
    return i;
}

void MainWindow::bootExe(const QString &fileName)
{
    SioDevice *old = sio->getDevice(DISK_BASE_CDEVIC);
    AutoBoot loader(sio, old);    
    AutoBootDialog dlg(this);

    bool highSpeed =    aspeqtSettings->useHighSpeedExeLoader() &&
                        (aspeqtSettings->serialPortHandshakingMethod() != HANDSHAKE_SOFTWARE);

    if (!loader.open(fileName, highSpeed)) {
        return;
    }

    sio->uninstallDevice(DISK_BASE_CDEVIC);
    sio->installDevice(DISK_BASE_CDEVIC, &loader);
    connect(&loader, SIGNAL(booterStarted()), &dlg, SLOT(booterStarted()));
    connect(&loader, SIGNAL(booterLoaded()), &dlg, SLOT(booterLoaded()));
    connect(&loader, SIGNAL(blockRead(int, int)), &dlg, SLOT(blockRead(int, int)));
    connect(&loader, SIGNAL(loaderDone()), &dlg, SLOT(loaderDone()));
    //connect(&dlg, SIGNAL(keepOpen()), this, SLOT(keepBootExeOpen()));

    dlg.exec();

    sio->uninstallDevice(DISK_BASE_CDEVIC);
    if (old) {
        sio->installDevice(DISK_BASE_CDEVIC, old);
        SimpleDiskImage *d = qobject_cast <SimpleDiskImage*> (old);
        d = qobject_cast <SimpleDiskImage*> (sio->getDevice(DISK_BASE_CDEVIC));
    }
    if(!g_exefileName.isEmpty()) bootExe(g_exefileName);
}

// Make boot executable dialog persistant until it's manually closed //
void MainWindow::keepBootExeOpen()
{
    bootExe(g_exefileName);
}


void MainWindow::bootExeTriggered(const QString &fileName)
{
    QString path = aspeqtSettings->lastExeDir();
    g_exefileName = path + "/" + fileName;
    if (!g_exefileName.isEmpty()) {
        aspeqtSettings->setLastExeDir(QFileInfo(g_exefileName).absolutePath());
        bootExe(g_exefileName);
    }
}



void MainWindow::mountFileWithDefaultProtection(int no, const QString &fileName)
{
    // If fileName was passed from RCL it is an 8.1 name, so we need to find
    // the full PC name in order to validate it.  //
    QString atariFileName, path;

    g_rclFileName = fileName;
    atariFileName = fileName;

    if(atariFileName.left(1) == "*") {
        atariFileName = atariFileName.mid(1);
        path = aspeqtSettings->lastDiskImageDir();
        if(atariFileName == "") {
            sio->port()->writeDataNak();
            return;
        } else {
            atariFileName =  path + "/" + atariFileName;
        }
    }

    const AspeQtSettings::ImageSettings* imgSetting = aspeqtSettings->getImageSettingsFromName(atariFileName);
    bool prot = (imgSetting!=NULL) && imgSetting->isWriteProtected;
    mountFile(no, atariFileName, prot);
}

void MainWindow::mountFile(int no, const QString &fileName, bool /*prot*/)
{
    SimpleDiskImage *disk;
    bool isDir = false;
    bool ask   = true;

    if (fileName.isEmpty()) {
        if(g_rclFileName.left(1) == "*") emit fileMounted(false);  //
        return;
    }

    FileTypes::FileType type = FileTypes::getFileType(fileName);

    if (type == FileTypes::Dir) {
        disk = new FolderImage(sio);
        isDir = true;

    } else if (type == FileTypes::Pro || type == FileTypes::ProGz) {
        disk = new DiskImagePro(sio);

    } else if (type == FileTypes::Atx || type == FileTypes::AtxGz) {
        disk = new DiskImageAtx(sio);

    } else {
        disk = new SimpleDiskImage(sio);
    }

    if (disk) {
        if(g_rclFileName.left(1) == "*") ask = false;
        if (!disk->open(fileName, type) || !ejectImage(no, ask) ) {
            aspeqtSettings->unmountImage(no);
            delete disk;
            if(g_rclFileName.left(1) == "*") emit fileMounted(false);  //
            return;
        }

        sio->installDevice(DISK_BASE_CDEVIC + no, disk);

        bool happy = aspeqtSettings->mountedImageSetting(no).isHappyMode;
        disk->setHappyMode(happy);

        PCLINK* pclink = reinterpret_cast<PCLINK*>(sio->getDevice(PCLINK_CDEVIC));
        if(isDir || pclink->hasLink(no+1))
        {
            sio->uninstallDevice(PCLINK_CDEVIC);
            if(isDir)
            {
                pclink->setLink(no+1,QDir::toNativeSeparators(fileName).toLatin1());
            }
            else
            {
                pclink->resetLink(no+1);
            }
            sio->installDevice(PCLINK_CDEVIC,pclink);
        }

        diskWidgets[no]->updateFromImage(disk);

        aspeqtSettings->mountImage(no, fileName, disk->isReadOnly());
        updateRecentFileActions();
        connect(disk, SIGNAL(statusChanged(int)), this, SLOT(deviceStatusChanged(int)), Qt::QueuedConnection);
        deviceStatusChanged(DISK_BASE_CDEVIC + no);

        // Extract the file name without the path //
        QString filenamelabel;
        int i = fileName.lastIndexOf("/");
        if (i != -1) {
            i++;
            filenamelabel = fileName.right(fileName.size() - i);
        }

        qDebug() << "!n" << tr("[%1] Mounted '%2' as '%3'.")
                .arg(disk->deviceName())
                .arg(filenamelabel)
                .arg(disk->description());

        if(g_rclFileName.left(1) == "*") emit fileMounted(true);  //
    }
}

void MainWindow::mountDiskImage(int no)
{
    QString dir;
// Always mount from "last image dir" //
//    if (diskWidgets[no].fileNameLabel->text().isEmpty()) {
        dir = aspeqtSettings->lastDiskImageDir();
//    } else {
//        dir = QFileInfo(diskWidgets[no].fileNameLabel->text()).absolutePath();
//    }
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open a disk image"),
                                                    dir,
                                                    tr(
                                                   "All Atari disk images (*.atr *.xfd *.atx *.pro);;"
//                                                    "All Atari disk images (*.atr *.xfd *.pro);;"
                                                    "SIO2PC ATR images (*.atr);;"
                                                    "XFormer XFD images (*.xfd);;"
                                                    "ATX images (*.atx);;"
                                                    "Pro images (*.pro);;"
                                                    "All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
    aspeqtSettings->setLastDiskImageDir(QFileInfo(fileName).absolutePath());
    mountFileWithDefaultProtection(no, fileName);
}

void MainWindow::mountFolderImage(int no)
{
    QString dir;
    // Always mount from "last folder dir" //
    dir = aspeqtSettings->lastFolderImageDir();
    QString fileName = QFileDialog::getExistingDirectory(this, tr("Open a folder image"), dir);
    fileName = QDir::fromNativeSeparators(fileName);    //
    if (fileName.isEmpty()) {
        return;
    }
    aspeqtSettings->setLastFolderImageDir(fileName);
    mountFileWithDefaultProtection(no, fileName);
}

void MainWindow::toggleWriteProtection(int no, bool protectionEnabled)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
    
    // --- SAFETY CHECK ---
    if (!img) return; 
    // --------------------

    img->setReadOnly(protectionEnabled);
    aspeqtSettings->setMountedImageProtection(no, protectionEnabled);
}

void MainWindow::openEditor(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
    
    // --- SAFETY CHECK ---
    if (!img) return; 
    // --------------------

    if (img->editDialog()) {
        img->editDialog()->close();
    } else {
        DiskEditDialog *dlg = new DiskEditDialog();
        dlg->go(img);
        dlg->show();
    }
}

QMessageBox::StandardButton MainWindow::saveImageWhenClosing(int no, QMessageBox::StandardButton previousAnswer, int number)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));

    if (previousAnswer != QMessageBox::YesToAll) {
        QMessageBox::StandardButtons buttons;
        if (number) {
            buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::YesToAll | QMessageBox::NoToAll | QMessageBox::Cancel;
        } else {
            buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
        }
        previousAnswer = QMessageBox::question(this, tr("Image file unsaved"), tr("'%1' has unsaved changes, do you want to save it?")
                                       .arg(img->originalFileName()), buttons);
    }
    if (previousAnswer == QMessageBox::Yes || previousAnswer == QMessageBox::YesToAll) {
        saveDisk(no);
    }
    if (previousAnswer == QMessageBox::Close) {
        previousAnswer = QMessageBox::Cancel;
    }
    return previousAnswer;
}

void MainWindow::loadTranslators()
{
    qApp->removeTranslator(&aspeqt_qt_translator);
    qApp->removeTranslator(&aspeqt_translator);
    if (aspeqtSettings->i18nLanguage().compare("auto") == 0) {
        QString locale = QLocale::system().name();
        aspeqt_translator.load(":/translations/i18n/aspeqt_" + locale);
        aspeqt_qt_translator.load(":/translations/i18n/qt_" + locale);
        qApp->installTranslator(&aspeqt_qt_translator);
        qApp->installTranslator(&aspeqt_translator);
    } else if (aspeqtSettings->i18nLanguage().compare("en") != 0) {
        aspeqt_translator.load(":/translations/i18n/aspeqt_" + aspeqtSettings->i18nLanguage());
        aspeqt_qt_translator.load(":/translations/i18n/qt_" + aspeqtSettings->i18nLanguage());
        qApp->installTranslator(&aspeqt_qt_translator);
        qApp->installTranslator(&aspeqt_translator);
    }
}

void MainWindow::saveDisk(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));

    // --- SAFETY CHECK ---
    if (!img) return;
    SioDevice *device = sio->getDevice(no);
    if (qobject_cast<TnfsImage*>(device))  return;

    if (img->isUnnamed()) {
        saveDiskAs(no);
    } else {
        img->lock();
        bool saved = img->save();
        img->unlock();
        if (!saved) {
            if (QMessageBox::question(this, tr("Save failed"), tr("'%1' cannot be saved, do you want to save the image with another name?")
                .arg(img->originalFileName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
                saveDiskAs(no);
            }
        }
    }
}
//
void MainWindow::autoCommit(int no, bool st)
{

    SioDevice *device = sio->getDevice(no + DISK_BASE_CDEVIC);
    if (qobject_cast<TnfsImage*>(device)) return;

    if( no < DISK_COUNT )
    {
        if ( (diskWidgets[no]->isAutoSaveEnabled() && st) || (!diskWidgets[no]->isAutoSaveEnabled() && !st) )
                    diskWidgets[no]->triggerAutoSaveClickIfEnabled();
    }
}

void MainWindow::autoSaveDisk(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
    
    // --- SAFETY CHECK ---
    if (!img) return;
    SioDevice *device = sio->getDevice(no + DISK_BASE_CDEVIC);
    if (qobject_cast<TnfsImage*>(device)) return;

    DriveWidget* widget = diskWidgets[no];

    if (img->isUnnamed()) {
        saveDiskAs(no);
        widget->updateFromImage(img);
        return;
    }

    bool autoSaveEnabled = widget->isAutoSaveEnabled();

    if (autoSaveEnabled) {
        qDebug() << "!n" << tr("[Disk %1] Auto-commit ON.").arg(no+1);
    } else {
        qDebug() << "!n" << tr("[Disk %1] Auto-commit OFF.").arg(no+1);
    }

    bool saved;

    img->lock();
    saved = img->save();
    img->unlock();
    if (!saved) {
        if (QMessageBox::question(this, tr("Save failed"), tr("'%1' cannot be saved, do you want to save the image with another name?")
            .arg(img->originalFileName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
            saveDiskAs(no);
        }
    }
    widget->updateFromImage(img);
}
//
void MainWindow::saveDiskAs(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
    
    // --- SAFETY CHECK ---
    if (!img) return; 
    // --------------------

    QString dir, fileName;
    bool saved = false;

    if (img->isUnnamed()) {
        dir = aspeqtSettings->lastDiskImageDir();
    } else {
        dir = QFileInfo(img->originalFileName()).absolutePath();
    }

    do {
        fileName = QFileDialog::getSaveFileName(this, tr("Save image as"),
                                 dir,
                                 tr(
//                                                    "All Atari disk images (*.atr *.xfd *.atx *.pro);;"
                                                    "All Atari disk images (*.atr *.xfd *.pro);;"
                                                    "SIO2PC ATR images (*.atr);;"
                                                    "XFormer XFD images (*.xfd);;"
//                                                    "ATX images (*.atx);;"
                                                    "Pro images (*.pro);;"
                                                    "All files (*)"));
        if (fileName.isEmpty()) {
            return;
        }

        img->lock();
        saved = img->saveAs(fileName);
        img->unlock();

        if (!saved) {
            if (QMessageBox::question(this, tr("Save failed"), tr("'%1' cannot be saved, do you want to save the image with another name?")
                .arg(fileName), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
                break;
            }
        }

    } while (!saved);

    if (saved) {
        aspeqtSettings->setLastDiskImageDir(QFileInfo(fileName).absolutePath());
    }
    aspeqtSettings->unmountImage(no);
    aspeqtSettings->mountImage(no, fileName, img->isReadOnly());
}

void MainWindow::revertDisk(int no)
{
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(no + DISK_BASE_CDEVIC));
    
    // --- SAFETY CHECK ---
    if (!img) return;
    SioDevice *device = sio->getDevice(no + DISK_BASE_CDEVIC);
    if (qobject_cast<TnfsImage*>(device)) return;

    if (QMessageBox::question(this, tr("Revert to last saved"),
            tr("Do you really want to revert '%1' to its last saved state? You will lose the changes that has been made.")
            .arg(img->originalFileName()), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
        img->lock();
        img->reopen();
        img->unlock();
        deviceStatusChanged(DISK_BASE_CDEVIC + no);
    }
}


// Slots for handling actions for devices.
void MainWindow::on_actionMountDisk_triggered(int deviceId) {mountDiskImage(deviceId);}
void MainWindow::on_actionMountFolder_triggered(int deviceId) {mountFolderImage(deviceId);}
void MainWindow::on_actionEject_triggered(int deviceId) {ejectImage(deviceId);}
void MainWindow::on_actionWriteProtect_triggered(int deviceId, bool writeProtectEnabled) {toggleWriteProtection(deviceId, writeProtectEnabled);}
void MainWindow::on_actionEditDisk_triggered(int deviceId) {openEditor(deviceId);}
void MainWindow::on_actionSave_triggered(int deviceId) {saveDisk(deviceId);}
//
void MainWindow::on_actionAutoSave_triggered(int deviceId) {autoSaveDisk(deviceId);}
void MainWindow::on_actionSaveAs_triggered(int deviceId) {saveDiskAs(deviceId);}
void MainWindow::on_actionRevert_triggered(int deviceId) {revertDisk(deviceId);}


void MainWindow::on_actionMountRecent_triggered(const QString &fileName) {mountFileWithDefaultProtection(firstEmptyDiskSlot(), fileName);}


void MainWindow::on_actionEjectAll_triggered()
{
    QMessageBox::StandardButton answer = QMessageBox::No;

    int toBeSaved = 0;

    for (int i = 0; i < DISK_COUNT; i++) {  //
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
        if (img && img->isModified()) {
            toBeSaved++;
        }
    }

    if (!toBeSaved) {
        for (int i = DISK_COUNT-1; i >= 0; i--) {
            ejectImage(i);
        }
        return;
    }

    sio->setHighSpeed(false);

    bool wasRunning = ui->actionStartEmulation->isChecked();
    if (wasRunning) {
        ui->actionStartEmulation->trigger();
    }

    for (int i = DISK_COUNT-1; i >= 0; i--) {
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
        if (img && img->isModified()) {
            toBeSaved--;
            answer = saveImageWhenClosing(i, answer, toBeSaved);
            if (answer == QMessageBox::NoToAll) {
                break;
            }
            if (answer == QMessageBox::Cancel) {
                if (wasRunning) {
                    ui->actionStartEmulation->trigger();
                }
                return;
            }
        }
    }
    for (int i = DISK_COUNT-1; i >= 0; i--) {
        ejectImage(i, false);
    }
    if (wasRunning) {
        ui->actionStartEmulation->trigger();
    }
}
void MainWindow::on_actionMountDisk_triggered()
{
    mountDiskImage(firstEmptyDiskSlot(0, true));
}

void MainWindow::on_actionMountFolder_triggered()
{
    mountFolderImage(firstEmptyDiskSlot(0, true));
}

void MainWindow::on_actionNewImage_triggered()
{
    CreateImageDialog dlg(this);
    if (!dlg.exec()) {
        return;
    };

    SimpleDiskImage *disk = new SimpleDiskImage(sio);
    connect(disk, SIGNAL(statusChanged(int)), this, SLOT(deviceStatusChanged(int)), Qt::QueuedConnection);

    if (!disk->create(++untitledName)) {
        delete disk;
        return;
    }

    DiskGeometry g;
    uint size = dlg.sectorCount() * dlg.sectorSize();
    if (dlg.sectorSize() == 256) {
        if (dlg.sectorCount() >= 3) {
            size -= 384;
        } else {
            size -= dlg.sectorCount() * 128;
        }
    }
    g.initialize(size, dlg.sectorSize());

    if (!disk->format(g)) {
        delete disk;
        return;
    }

    int no = firstEmptyDiskSlot(0, true);

    if (!ejectImage(no)) {
        delete disk;
        return;
    }

    sio->installDevice(DISK_BASE_CDEVIC + no, disk);
    deviceStatusChanged(DISK_BASE_CDEVIC + no);
    qDebug() << "!n" << tr("[%1] Mounted '%2' as '%3'.")
            .arg(disk->deviceName())
            .arg(disk->originalFileName())
            .arg(disk->description());
}

void MainWindow::on_actionOpenSession_triggered()
{
    QString dir = aspeqtSettings->lastSessionDir();
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open session"),
                                 dir,
                                 tr(
                                         "AspeQt sessions (*.aspeqt);;"
                                         "All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
// First eject existing images, then mount session images and restore mainwindow position and size //
    MainWindow::on_actionEjectAll_triggered();

    aspeqtSettings->setLastSessionDir(QFileInfo(fileName).absolutePath());
    g_sessionFile = QFileInfo(fileName).fileName();
    g_sessionFilePath = QFileInfo(fileName).absolutePath();

// Pass Session file name, path and MainWindow title to AspeQtSettings //
    aspeqtSettings->setSessionFile(g_sessionFile, g_sessionFilePath);
    aspeqtSettings->setMainWindowTitle(g_mainWindowTitle);

    aspeqtSettings->loadSessionFromFile(fileName);

    setWindowTitle(g_mainWindowTitle + tr(" -- Session: ") + g_sessionFile);
    setGeometry(aspeqtSettings->lastHorizontalPos(), aspeqtSettings->lastVerticalPos(), aspeqtSettings->lastWidth() , aspeqtSettings->lastHeight());

    for (int i = 0; i < DISK_COUNT; i++) {  //
        AspeQtSettings::ImageSettings is;
        is = aspeqtSettings->mountedImageSetting(i);
        mountFile(i, is.fileName, is.isWriteProtected);
    }
    g_D9DOVisible =  aspeqtSettings->D9DOVisible();
    on_actionHideShowDrives_triggered();
    setSession();
}
void MainWindow::on_actionSaveSession_triggered()
{
    QString dir = aspeqtSettings->lastSessionDir();
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save session as"),
                                 dir,
                                 tr(
                                         "AspeQt sessions (*.aspeqt);;"
                                         "All files (*)"));
    if (fileName.isEmpty()) {
        return;
    }
    aspeqtSettings->setLastSessionDir(QFileInfo(fileName).absolutePath());

// Save mainwindow position and size to session file //
    if (aspeqtSettings->saveWindowsPos()) {
        aspeqtSettings->setLastHorizontalPos(geometry().x());
        aspeqtSettings->setLastVerticalPos(geometry().y());
        aspeqtSettings->setLastWidth(geometry().width());
        aspeqtSettings->setLastHeight(geometry().height());
    }
    aspeqtSettings->saveSessionToFile(fileName);
}

void MainWindow::on_actionBootExe_triggered()
{
    QString dir = aspeqtSettings->lastExeDir();
    g_exefileName = QFileDialog::getOpenFileName(this, tr("Open executable"),
                                 dir,
                                 tr(
                                         "Atari executables (*.xex *.com *.exe);;"
                                         "All files (*)"));

    if (!g_exefileName.isEmpty()) {
        aspeqtSettings->setLastExeDir(QFileInfo(g_exefileName).absolutePath());
        bootExe(g_exefileName);
    }
}




void MainWindow::on_actionShowPrinterTextOutput_triggered()
{
    if (ui->actionShowPrinterTextOutput->isChecked()) {
        textPrinterWindow->setGeometry(aspeqtSettings->lastPrtHorizontalPos() ,aspeqtSettings->lastPrtVerticalPos(),aspeqtSettings->lastPrtWidth(),aspeqtSettings->lastPrtHeight());
        textPrinterWindow->show();
    } else {
        textPrinterWindow->hide();
    }
}

void MainWindow::textPrinterWindowClosed()
{
    ui->actionShowPrinterTextOutput->setChecked(false);
}


void MainWindow::on_actionPlaybackCassette_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open a cassette image"),
                                                    aspeqtSettings->lastCasDir(),
                                                    tr(
                                                    "CAS images (*.cas);;"
                                                    "All files (*)"));

    if (fileName.isEmpty())
    {
        return;
    }
    aspeqtSettings->setLastCasDir(QFileInfo(fileName).absolutePath());

    bool restart;
    restart = ui->actionStartEmulation->isChecked();
    if (restart) {
        ui->actionStartEmulation->trigger();
        sio->wait();
        qApp->processEvents();
    }
    bootCasTriggered(fileName);
 }


void MainWindow::bootCasTriggered(const QString &fileName)
{
    CassetteDialog *dlg = new CassetteDialog(this, fileName);
    dlg->exec();
    delete dlg;

//    if (restart) {
        ui->actionStartEmulation->trigger();
//    }
}


void MainWindow::on_actionQuit_triggered()
{
    close();
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        setWindowFlags(oldWindowFlags);
        setWindowState(oldWindowStates);
        show();
        activateWindow();
        raise();
        trayIcon.hide();
    }
}

void MainWindow::on_actionBootOption_triggered()
{
    QString folderPath = aspeqtSettings->mountedImageSetting(0).fileName;
    BootOptionsDialog bod(folderPath, this);
    bod.exec();
}



void MainWindow::on_actionHappyMode_triggered(int deviceId, bool enabled)
{
    // 1. Update the internal Disk Image object if it exists
    SimpleDiskImage *img = qobject_cast<SimpleDiskImage*>(sio->getDevice(deviceId + DISK_BASE_CDEVIC));
    if (img) {
        img->setHappyMode(enabled);
    }

    // 2. Save to settings so it's remembered next time (persistence)
    const AspeQtSettings::ImageSettings& is = aspeqtSettings->mountedImageSetting(deviceId);
    aspeqtSettings->setMountedImageSetting(deviceId, is.fileName, is.isWriteProtected, enabled);

    qDebug() << "!i" << tr("Drive %1 Happy Mode %2.")
                            .arg(deviceId + 1)
                            .arg(enabled ? tr("Enabled") : tr("Disabled"));
}


/*
 * mainwindow.cpp
 * (Snippet showing updated on_actionMountTnfs_triggered)
 */



void MainWindow::on_actionMountTnfs_triggered(int deviceId)
{
    // 1. Set Busy Cursor immediately (from Main Window context)
    // This covers the time taken by TnfsBrowser's constructor to Connect & Refresh.
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // 2. Launch the thread-safe browser with the LAST SAVED URL
    TnfsBrowser browser(this, aspeqtSettings->restoreTnfsLocation() ? g_lastTnfsUrl : "");

    // 3. Restore Cursor (Constructor is done, UI is ready to show)
    QApplication::restoreOverrideCursor();

    if (browser.exec() == QDialog::Accepted) {
        QString url = browser.getSelectedUrl();

        // --- Save the URL for next time ---
        g_lastTnfsUrl = url;

        // Eject whatever is currently in that slot
        if (!ejectImage(deviceId)) return;

        TnfsImage *tnfs = new TnfsImage(sio);
        tnfs->setParent(nullptr);   // Detach
        tnfs->moveToThread(sio);    // Move to SIO Thread
        connect(tnfs, &TnfsImage::downloadProgress, this, &MainWindow::updateDownloadProgress);

        // Connect/Open the stream
        if (tnfs->openUrl(url)) {
            // Install into the SIO chain
            sio->installDevice(DISK_BASE_CDEVIC + deviceId, tnfs);
            // Trigger UI Update via the central handler
            deviceStatusChanged(DISK_BASE_CDEVIC + deviceId);
            qDebug() << "!i" << tr("Mounted TNFS Stream: %1").arg(url);
            dlProgressBar->hide();
        } else {
            QMessageBox::critical(this, tr("Mount Error"), tr("Could not open TNFS stream from 13leader.net"));
            delete tnfs;
            dlProgressBar->hide();
        }
    }
}



void MainWindow::updateDownloadProgress(qint64 bytesRead, qint64 totalBytes)
{
    if (dlProgressBar->isHidden()) {
        dlProgressBar->show();
    }

    // "Marquee" mode: setRange(0, 0).
    // This creates an infinite animation block that cycles back to 0 automatically.
    dlProgressBar->setRange(0, 0);

    // Show bytes downloaded text
    if (bytesRead > 0) {
        // Convert to KB for readability
        QString sizeText = QString::number(bytesRead / 1024) + " KB";
        dlProgressBar->setFormat(tr("Downloading: %1").arg(sizeText));
    } else {
        dlProgressBar->setFormat(tr("Downloading..."));
    }
}
