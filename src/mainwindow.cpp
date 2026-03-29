/*
 * mainwindow.cpp
 */

#include "mainwindow.h"
#include "src/ui_mainwindow.h"
#include "ui_mainwindow.h"

#include <QToolButton>
#include <QLayout>
#include <QElapsedTimer>
#include <QWebSocketServer>
#include <QWebChannel>
#include <QHttpServer>
#include <QHttpServerResponse>
#include <QHttpServerResponder>
#include <QFile>
#include <QTcpServer>

#include "tnfsbrowser.h"
#include "tnfsimage.h"
#include "rdevice.h"

#include "diskimage.h"
#include "diskimagepro.h"
#include "diskimageatx.h"
#include "drivewidget.h"
#include "folderimage.h"
#include "pclink.h"
#include "miscdevices.h"
#include "pipenetwork.h"
#include "aspeqtsettings.h"
#include "cassettedialog.h"
#include "bootoptionsdialog.h"
#include "logdisplaydialog.h"
#include "infowidget.h"
#include "opcodes6502.h"
#include "xeximage.h"

#include "websocketclientwrapper.h"
#include "webbridge.h"


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
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QDesktopServices>
#include <qtoolbar.h>
#include "sectorinspectordialog.h"

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

    m_downloadCounter = 0;
    for (int i = 0; i < DISK_COUNT; i++) {
        m_slotDownloadId[i] = 0;
    }

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
                QMessageBox::warning(this, tr("Session file error"),
                                     tr("Requested session file not found..."));
                g_sessionFile = g_sessionFilePath = "";
            }
        } else {
            if (AspeQtArgs.at(1) != "") {
                g_sessionFile = AspeQtArgs.at(1);
                g_sessionFilePath = QDir::currentPath();
                sess.setFileName(g_sessionFile);
                if (!sess.exists()) {
                    QMessageBox::warning(this, tr("Session file error"),
                                         tr("Requested session file not found..."));
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


    // Initialize Headless Cassette Deck
    m_casWorker = nullptr;
    m_casTimer = new QTimer(this);
    m_casIsPlaying = false;
    connect(m_casTimer, &QTimer::timeout, this, &MainWindow::updateCasProgress);

    /* Setup status bar */
    speedLabel = new QLabel(this);
    speedLabel->setText(tr("19200 bits/sec"));
    speedLabel->setMinimumWidth(80);
    dlProgressBar = new QProgressBar(this);
    dlProgressBar->setRange(0, 100);
    dlProgressBar->setValue(0);
    dlProgressBar->setTextVisible(true);
    dlProgressBar->setFixedWidth(150);
    dlProgressBar->hide();

    ui->statusBar->addPermanentWidget(speedLabel);
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


    // A. Create LED Widgets (Simulated using CSS)
    // ------------------------------------------------
    ledRx = new QLabel(this);
    ledRx->setFixedSize(14, 14);
    ledRx->setToolTip(tr("RX: Data Receiving from Internet"));
    // Default Style: Dark Green Circle
    ledRx->setStyleSheet("min-width: 12px; min-height: 12px; border-radius: 7px; background-color: #004400; border: 1px solid #555;");

    ledTx = new QLabel(this);
    ledTx->setFixedSize(14, 14);
    ledTx->setToolTip(tr("TX: Data Sending from Atari"));
    // Default Style: Dark Red Circle
    ledTx->setStyleSheet("min-width: 12px; min-height: 12px; border-radius: 7px; background-color: #440000; border: 1px solid #555;");

    ledResetTimer = new QTimer(this);
    ledResetTimer->setInterval(75); // Blink speed (ms)
    connect(ledResetTimer, &QTimer::timeout, this, &MainWindow::resetLeds);


    // B. Create Control Buttons (Using QToolButton for ease)
    // ------------------------------------------------
    auto setupBtn = [](QToolButton* btn, QString iconName, QString text, QString tip) {
        // Try to load icon, fallback to text if missing
        QIcon icon(iconName);
        if (icon.isNull()) btn->setText(text);
        else btn->setIcon(icon);

        btn->setToolTip(tip);
        btn->setAutoRaise(true); // Makes it look flat like the label icons
        btn->setFixedSize(22, 22);
        btn->setIconSize(QSize(16, 16));
    };

    // Ensure mutual exclusivity: If both were somehow checked in settings, R: Device wins.
    if (aspeqtSettings->isRDeviceEnabled() && aspeqtSettings->isModemBridgeEnabled()) {
        aspeqtSettings->setModemBridgeEnabled(false); // Force legacy bridge off
    }


    // 2. Hangup
    btnHangup = new QToolButton(this);
    setupBtn(btnHangup, ":/icons/silk-icons/icons/disconnect.png", "H", tr("Hangup (NO CARRIER)"));
    connect(btnHangup, &QToolButton::clicked, [this]() {
        if (aspeqtSettings->isRDeviceEnabled()) {
            RDevice *rDev = qobject_cast<RDevice*>(sio->getDevice(0x50));
            if (rDev) rDev->hangup();
        } else if (modemBridge) {
            modemBridge->hangup();
        }
    });

    // 3. Macros
    btnMacroUser = new QToolButton(this);
    setupBtn(btnMacroUser, ":/icons/silk-icons/icons/user.png", "U", tr("Send Auto-User (ESC-U)"));
    connect(btnMacroUser, &QToolButton::clicked, [this]() {
        if (aspeqtSettings->isRDeviceEnabled()) {
            RDevice *rDev = qobject_cast<RDevice*>(sio->getDevice(0x50));
            if (rDev) rDev->injectMacro('U');
        } else if (modemBridge) {
            modemBridge->injectMacro('U');
        }
    });

    btnMacroPass = new QToolButton(this);
    setupBtn(btnMacroPass, ":/icons/silk-icons/icons/lock.png", "P", tr("Send Auto-Pass (ESC-P)"));
    connect(btnMacroPass, &QToolButton::clicked, [this]() {
        if (aspeqtSettings->isRDeviceEnabled()) {
            RDevice *rDev = qobject_cast<RDevice*>(sio->getDevice(0x50));
            if (rDev) rDev->injectMacro('P');
        } else if (modemBridge) {
            modemBridge->injectMacro('P');
        }
    });


    // SIO Trace Toggle
    btnSioTrace = new QToolButton(this);
    setupBtn(btnSioTrace, ":/icons/silk-icons/icons/monitor.png", "HEX", tr("Toggle SIO Hex Dump Trace"));
    btnSioTrace->setCheckable(true);
    connect(btnSioTrace, &QToolButton::clicked, this, &MainWindow::onSioTraceToggleClicked);

    // Disassembler Toggle
    btnDisasmToggle = new QToolButton(this);
    setupBtn(btnDisasmToggle, ":/icons/silk-icons/icons/page_white_text.png", "ASM", tr("Toggle 6502 Disassembler"));
    btnDisasmToggle->setCheckable(true);
    connect(btnDisasmToggle, &QToolButton::clicked, this, &MainWindow::onDisasmToggleClicked);


    // C. Add to Status Bar
    ui->statusBar->addPermanentWidget(ledRx);
    ui->statusBar->addPermanentWidget(ledTx);


    // C. Create Main Toolbar (Modern Qt Layout)
    // ------------------------------------------------
    QToolBar *mainToolBar = addToolBar(tr("Main Tools"));
    mainToolBar->setMovable(false);          // Lock it under the menu bar
    mainToolBar->setIconSize(QSize(16, 16)); // Keep icons uniform
    ui->actionShowPrinterTextOutput->setIcon(QIcon(":/icons/silk-icons/icons/page_white_text.png"));

    mainToolBar->addAction(ui->actionStartEmulation);
    mainToolBar->addAction(ui->actionPrinterEmulation);
    mainToolBar->addAction(ui->actionShowPrinterTextOutput);
    mainToolBar->addAction(ui->actionOptions);
    mainToolBar->addSeparator();

    // 1. Clear Log Button (Converted to a proper ToolButton)
    QToolButton *btnClearLog = new QToolButton(this);
    setupBtn(btnClearLog, ":/icons/silk-icons/icons/page_white_c.png", "C", tr("Clear log messages"));
    connect(btnClearLog, &QToolButton::clicked, this, [this]() {
        ui->textEdit->clear();
        emit sendLogText("");
    });

    QToolButton *btnShowLog = new QToolButton(this);
    setupBtn(btnShowLog, ":/icons/silk-icons/icons/page_white.png", "L", tr("Show Log Window"));
    connect(btnShowLog, &QToolButton::clicked, this, &MainWindow::on_actionLogWindow_triggered);

    ui->actionPhonebook->setIcon(QIcon(":/icons/silk-icons/icons/phone.png"));

    // 2. Add Diagnostic Tools to Toolbar
    mainToolBar->addWidget(btnShowLog);
    mainToolBar->addWidget(btnClearLog);
    mainToolBar->addWidget(btnSioTrace);
    mainToolBar->addWidget(btnDisasmToggle);

    // Built-in Qt Toolbar Separator
    mainToolBar->addSeparator();

    // 3. Add Modem Tools to Toolbar
    mainToolBar->addWidget(btnHangup);
    mainToolBar->addAction(ui->actionPhonebook);
    mainToolBar->addWidget(btnMacroUser);
    mainToolBar->addWidget(btnMacroPass);


    // D. Finalize Status Bar (Passive Indicators Only)
    // ------------------------------------------------
    // (speedLabel, onOffLabel, etc. were already added earlier in the constructor)
    ui->statusBar->addPermanentWidget(ledRx);
    ui->statusBar->addPermanentWidget(ledTx);

    sio = new SioWorker();

    connect(sio, &SioWorker::sioTrace, this, &MainWindow::onSioTraceData);
    connect(sio, &SioWorker::rxActivity, this, &MainWindow::blinkRx);
    connect(sio, &SioWorker::txActivity, this, &MainWindow::blinkTx);


    // -------------------------------------------------------
    // MODEM BRIDGE SETUP
    // -------------------------------------------------------
    modemBridge = new ModemBridge(this);

    // Connect Status Messages (Info)
    connect(modemBridge, &ModemBridge::statusMessage, this, [](const QString &msg) {
        // Use qDebug so the message handler catches it and colors it Blue (!i)
        qDebug() << "!i [ModemBridge]" << msg;
    });

    // Connect Error Messages (Error)
    connect(modemBridge, &ModemBridge::errorOccurred, this, [](const QString &err) {
        // Use qCritical so the message handler catches it and colors it Red (!e)
        qCritical() << "!e [ModemBridge]" << err;
    });

    // Commect Hex Dump
    connect(modemBridge, &ModemBridge::traceData, this, &MainWindow::onSioTraceData);
    connect(modemBridge, &ModemBridge::rxActivity, this, &MainWindow::blinkRx);
    connect(modemBridge, &ModemBridge::txActivity, this, &MainWindow::blinkTx);

    // Configure from Settings
    if (aspeqtSettings->isModemBridgeEnabled()) {
        modemBridge->setSerialPort(aspeqtSettings->modemBridgePortName(),
                                   aspeqtSettings->modemBridgeBaudRate());
        modemBridge->setFlowControl(aspeqtSettings->modemBridgeFlowControl());
        modemBridge->setLocalEcho(aspeqtSettings->modemBridgeLocalEcho());
        modemBridge->setTcpMode(aspeqtSettings->modemBridgeSshEnabled());

        // Load Phonebook path
        QString pbPath = aspeqtSettings->modemBridgePhonebookPath();
        if (pbPath.isEmpty()) pbPath = g_aspeQtAppPath + "/phonebook.xml";
        modemBridge->setPhonebookPath(pbPath);

        modemBridge->start();
    }
    updatePhonebookMenuState();

    // -------------------------------------------------------
    // R: Device
    // -------------------------------------------------------
    // When you create RDevice:
    RDevice *rDev = new RDevice(sio);
    rDev->setParent(nullptr);
    sio->installDevice(0x50, rDev);


    // -------------------------------------------------------
    // DEVICE $45: PCLINK
    // -------------------------------------------------------
    PCLINK *pcLink = new PCLINK(sio);
    pcLink->setParent(nullptr);
    pcLink->moveToThread(sio);
    sio->installDevice(PCLINK_CDEVIC, pcLink);


    // -------------------------------------------------------
    // DEVICE $46: AspeQt Client Device & legacy support
    // -------------------------------------------------------
    AspeqtClientDevice *client = new AspeqtClientDevice(sio);
    client->setParent(nullptr);
    client->moveToThread(sio);
    sio->installDevice(0x46, client);

    // Connections for remote disk management, booting, and printer control
    connect(client, SIGNAL(findNewSlot(int,bool)), this, SLOT(firstEmptyDiskSlot(int,bool)));
    connect(this, SIGNAL(newSlot(int)), client, SLOT(gotNewSlot(int)));
    connect(client, SIGNAL(mountFile(int,QString)), this, SLOT(mountFileWithDefaultProtection(int,QString)));
    connect(this, SIGNAL(fileMounted(bool)), client, SLOT(fileMounted(bool)));
    connect(client, SIGNAL(toggleAutoCommit(int,bool)), this, SLOT(autoCommit(int,bool)));
    connect(client, SIGNAL(bootExe(QString)), this, SLOT(bootExeTriggered(QString)));
    connect(client, SIGNAL(bootCas(QString)), this, SLOT(bootCasTriggered(QString)));
    connect(client, SIGNAL(togglePrinterServer(bool)), this, SLOT(printServer(bool)));


    // -------------------------------------------------------
    // DEVICE $57: Pipe Network (W:)
    // -------------------------------------------------------
    PipeNetwork *pipeNet1 = new PipeNetwork(sio);
    pipeNet1->setParent(nullptr);
    pipeNet1->moveToThread(sio);

    // Connect W1 to the shared handler
    connect(pipeNet1, &PipeNetwork::sendFireAndForget, this, &MainWindow::onFireAndForget);
    sio->installDevice(0x57, pipeNet1);


    // -------------------------------------------------------
    // DEVICE $58: Pipe Network (W2:)
    // -------------------------------------------------------
    PipeNetwork *pipeNet2 = new PipeNetwork(sio);
    pipeNet2->setParent(nullptr);
    pipeNet2->moveToThread(sio);

    // Connect W2 to the SAME shared handler
    connect(pipeNet2, &PipeNetwork::sendFireAndForget, this, &MainWindow::onFireAndForget);
    sio->installDevice(0x56, pipeNet2);

    // -------------------------------------------------------
    // DEVICE $58: Pipe Network (W3:)
    // -------------------------------------------------------
    PipeNetwork *pipeNet3 = new PipeNetwork(sio);
    pipeNet3->setParent(nullptr);
    pipeNet3->moveToThread(sio);

    // Connect W3 to the SAME shared handler
    connect(pipeNet3, &PipeNetwork::sendFireAndForget, this, &MainWindow::onFireAndForget);
    sio->installDevice(0x55, pipeNet3);


    // -------------------------------------------------------
    // DEVICE $58: Pipe Network (W4:)
    // -------------------------------------------------------
    PipeNetwork *pipeNet4 = new PipeNetwork(sio);
    pipeNet4->setParent(nullptr);
    pipeNet4->moveToThread(sio);

    // Connect W3 to the SAME shared handler
    connect(pipeNet4, &PipeNetwork::sendFireAndForget, this, &MainWindow::onFireAndForget);
    sio->installDevice(0x54, pipeNet4);


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
    sio->installDevice(0x70, smart);

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


    // -------------------------------------------------------
    // SETUP WEB UI SERVERS (Do not start them yet)
    // -------------------------------------------------------

    webSocketServer = nullptr;
    clientWrapper = nullptr;
    httpServer = nullptr;
    httpTcpServer = nullptr;

    webChannel = new QWebChannel(this);
    webBridge = new WebBridge(this, this);
    webChannel->registerObject("aspeqtBridge", webBridge);

    // Start servers immediately if enabled in settings
    if (aspeqtSettings->isWebUiEnabled()) {
        startWebUi();
    }

}

MainWindow::~MainWindow()
{
    if (ui->actionStartEmulation->isChecked()) {
        ui->actionStartEmulation->trigger();
    }

    delete aspeqtSettings;
    delete sio;
    delete ui;
    delete modemBridge;

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
        QAction *mountTnfsAction = new QAction(QIcon(":/icons/silk-icons/icons/world.png"), tr("Mount from TNFS Network..."), this);

        if (mountTnfsAction->icon().isNull()) {
            mountTnfsAction->setIcon(QIcon(":/icons/silk-icons/icons/connect.png"));
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
        // [FIX 1] Corrected Slot Name: on_actionSave -> on_actionSave_triggered
        connect(deviceWidget, SIGNAL(actionSave(int)), this, SLOT(on_actionSave_triggered(int)));
        connect(deviceWidget, SIGNAL(actionAutoSave(int,bool)), this, SLOT(on_actionAutoSave_triggered(int)));
        connect(deviceWidget, SIGNAL(actionRevert(int)), this, SLOT(on_actionRevert_triggered(int)));

        connect(deviceWidget, SIGNAL(actionMountDisk(int)), this, SLOT(on_actionMountDisk_triggered(int)));
        connect(deviceWidget, SIGNAL(actionMountFolder(int)), this, SLOT(on_actionMountFolder_triggered(int)));
        connect(deviceWidget, SIGNAL(actionEject(int)), this, SLOT(on_actionEject_triggered(int)));
        connect(deviceWidget, SIGNAL(actionWriteProtect(int,bool)), this, SLOT(on_actionWriteProtect_triggered(int,bool)));
        connect(deviceWidget, SIGNAL(actionEditDisk(int)), this, SLOT(on_actionEditDisk_triggered(int)));
        connect(deviceWidget, SIGNAL(actionSaveAs(int)), this, SLOT(on_actionSaveAs_triggered(int)));
        connect(deviceWidget, SIGNAL(actionBootOptions(int)), this, SLOT(on_actionBootOption_triggered()));
        connect(this, SIGNAL(setFont(const QFont&)), deviceWidget, SLOT(setFont(const QFont&)));
        connect(deviceWidget, SIGNAL(actionHappyMode(int,bool)), this, SLOT(on_actionHappyMode_triggered(int,bool)));
        connect(deviceWidget, SIGNAL(actionInspectSectors(int)), this, SLOT(on_actionInspectSectors_triggered(int)));
        connect(diskWidgets[i], SIGNAL(actionInfo(int)), this, SLOT(on_actionInfo_triggered(int)));

    }

    ui->leftColumn->setAlignment(Qt::AlignTop);
    ui->rightColumn->setAlignment(Qt::AlignTop);

    changeFonts();
}


 void MainWindow::mousePressEvent(QMouseEvent *event)
 {
     int slot = containingDiskSlot(event->position().toPoint());

     if (event->button() == Qt::LeftButton
         && slot >= 0) {

         QDrag *drag = new QDrag((QWidget*)this);
         QMimeData *mimeData = new QMimeData;

         mimeData->setData("application/x-aspeqt-disk-image", QByteArray(1, slot));
         drag->setMimeData(mimeData);

         drag->exec();
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
    int i = containingDiskSlot(event->position().toPoint());
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
    int slot = containingDiskSlot(event->position().toPoint());
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
    QMessageBox::StandardButton answer = QMessageBox::StandardButton::No;

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
            if (answer == QMessageBox::StandardButton::NoToAll) {
                break;
            }
            if (answer == QMessageBox::StandardButton::Cancel) {
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
            if (QMessageBox::StandardButton::Yes == QMessageBox::question(this, tr("First run"),
                                       tr("You are running AspeQt for the first time.\n\nDo you want to open the options dialog?"),
                                       QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No)) {
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
            logWindow_->setGeometry(x+w/1.9, y+30, 800, geometry().height());
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

    if (webBridge) emit webBridge->globalStatusChanged(ui->actionStartEmulation->isChecked(), enable);
}


void MainWindow::setUpPrinterEmulationWidgets(bool enable)
{
    if (enable) {
        ui->actionPrinterEmulation->setText(QApplication::translate("MainWindow", "Stop printer emulation", 0));
        ui->actionPrinterEmulation->setStatusTip(QApplication::translate("MainWindow", "Stop printer emulation", 0));
        ui->actionPrinterEmulation->setIcon(QIcon(":/icons/silk-icons/icons/printer.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
    } else {
        ui->actionPrinterEmulation->setText(QApplication::translate("MainWindow", "Start printer emulation", 0));
        ui->actionPrinterEmulation->setStatusTip(QApplication::translate("MainWindow", "Start printer emulation", 0));
        ui->actionPrinterEmulation->setIcon(QIcon(":/icons/silk-icons/icons/printer_delete.png").pixmap(16, 16, QIcon::Normal, QIcon::On));
    }
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
    if (webBridge) emit webBridge->globalStatusChanged(true, aspeqtSettings->printerEmulation());

}

void MainWindow::sioFinished()
{
    ui->actionStartEmulation->setText(tr("&Start emulation"));
    ui->actionStartEmulation->setToolTip(tr("Start SIO peripheral emulation"));
    ui->actionStartEmulation->setStatusTip(tr("Start SIO peripheral emulation"));
    ui->actionStartEmulation->setChecked(false);

    speedLabel->hide();
    speedLabel->clear();
    qWarning() << "!i" << tr("Emulation stopped.");

    RDevice *rDev = qobject_cast<RDevice*>(sio->getDevice(0x50));
    if (rDev) {   rDev->forceCommandMode();  }
    if (webBridge) emit webBridge->globalStatusChanged(false, aspeqtSettings->printerEmulation());
}

void MainWindow::sioStatusChanged(QString status)
{
    speedLabel->setText(status);
    speedLabel->show();

    blinkRx();
    blinkTx();
}


void MainWindow::deviceStatusChanged(int deviceNo)
{
    if (deviceNo >= DISK_BASE_CDEVIC && deviceNo < (DISK_BASE_CDEVIC+DISK_COUNT)) {

        DriveWidget *diskWidget = diskWidgets[deviceNo - DISK_BASE_CDEVIC];

        // --- 1. HANDLING TNFS DOWNLOADS ---
        if (m_slotDownloadId[deviceNo - DISK_BASE_CDEVIC] != 0) {
            diskWidget->showAsTNFSMounted(tr("Loading..."), tr("Downloading from TNFS..."));
            diskWidget->setFullPath(""); // Clear path while loading

            if (webBridge) {
                emit webBridge->diskStatusChanged(deviceNo - DISK_BASE_CDEVIC, "Loading...", "Downloading from TNFS...", "", false, false, true);
            }
            return;
        }

        SioDevice *device = sio->getDevice(deviceNo);

        // --- 2. HANDLING LOCAL EXECUTABLES (XEX) ---
        XexImage *xex = qobject_cast<XexImage *>(device);
        if (xex) {
            QString fullPath = xex->originalFileName();
            QFileInfo fi(fullPath);
            QString name = fi.fileName();

            diskWidget->setLabelToolTips(fi.absolutePath(), fullPath, tr("Executable (Local)"));
            diskWidget->setHappyMode(false);

            diskWidget->showAsImageMounted(name, tr("Executable (Local)"), false, false);
            diskWidget->setFullPath(fullPath); // Secretly set the path for the Info Modal

            if (webBridge) {
                emit webBridge->diskStatusChanged(deviceNo - DISK_BASE_CDEVIC, name, "Executable (Local)", fullPath, false, false, true);
            }
            return;
        }

        // --- 3. HANDLING TNFS NETWORK STREAMS ---
        TnfsImage *tnfsImg = qobject_cast<TnfsImage*>(device);
        if (tnfsImg) {
            QString fullUrl = tnfsImg->originalFileName();
            QString fileNameOnly = fullUrl;

            int lastSlash = fullUrl.lastIndexOf('/');
            if (lastSlash != -1) {
                fileNameOnly = fullUrl.mid(lastSlash + 1);
            }
            if (fileNameOnly.isEmpty()) fileNameOnly = fullUrl;

            diskWidget->setLabelToolTips(fullUrl, fullUrl, tr("TNFS Network Stream To Ram"));
            diskWidget->setHappyMode(false);

            diskWidget->showAsTNFSMounted(fileNameOnly, tr("TNFS Stream to RAM"));
            diskWidget->setFullPath(fullUrl); // Secretly set the path for the Info Modal

            if (webBridge) {
                emit webBridge->diskStatusChanged(deviceNo - DISK_BASE_CDEVIC, fileNameOnly, "TNFS Stream", fullUrl, false, false, true);
            }
            return;
        }

        // --- 4. HANDLING STANDARD DISK IMAGES & FOLDERS ---
        SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (device);
        if (img) {

            QString filenamelabel;
            int i = -1;

            if (img->description() == tr("Folder image")) {
                i = img->originalFileName().lastIndexOf("\\");
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
            QString fullPath = img->originalFileName(); // Grab the absolute path!

            if (img->description() == tr("Folder image")) {
                diskWidget->showAsFolderMounted(filenamelabel, img->description(), enableEdit);
            } else {
                bool enableSave = false;

                if (img->isModified()) {
                    if (!diskWidget->isAutoSaveEnabled()) {
                        enableSave = true;
                    } else {
                        bool saved;
                        saved = img->save();
                        if (!saved) {
                            int response = QMessageBox::question(this, tr("Save failed"),
                                                                 tr("'%1' cannot be saved, do you want to save the image with another name?").arg(img->originalFileName()),
                                                                 QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No);
                            if (response == QMessageBox::StandardButton::Yes) {
                                saveDiskAs(deviceNo);
                            }
                        }
                    }
                }
                diskWidget->showAsImageMounted(filenamelabel, img->description(), enableEdit, enableSave);
            }

            diskWidget->setFullPath(fullPath); // Secretly set the path for the Info Modal

            if (webBridge) {
                bool autoSave = diskWidget->isAutoSaveEnabled();
                bool happy = aspeqtSettings->mountedImageSetting(deviceNo - DISK_BASE_CDEVIC).isHappyMode;
                bool writeProtected = aspeqtSettings->mountedImageSetting(deviceNo - DISK_BASE_CDEVIC).isWriteProtected;

                emit webBridge->diskStatusChanged(deviceNo - DISK_BASE_CDEVIC, filenamelabel, img->description(), fullPath, autoSave, happy, writeProtected);
            }

        } else {
            // --- 5. HANDLING EMPTY DRIVES ---
            diskWidget->showAsEmpty();

            if (webBridge) {
                emit webBridge->diskStatusChanged(deviceNo - DISK_BASE_CDEVIC, "Empty", "--", "", false, false, false);
            }
        }
    }
}


void MainWindow::uiMessage(int t, QString message)
{
    if (message.at(0) == '"') {
        message.remove(0, 1);
    }
    if (message.at(message.size() - 1) == ' ' && message.at(message.size() - 2) == '"') {
        message.resize(message.size() - 2);
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

    // ui->statusBar->showMessage(message, 3000);

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

// update phonebook state
    updatePhonebookMenuState();

// Cycle the Web UI to apply any port or toggle changes
    stopWebUi();
    if (aspeqtSettings->isWebUiEnabled()) {
        startWebUi();
    }

    for (int i = DISK_BASE_CDEVIC; i < (DISK_BASE_CDEVIC+DISK_COUNT); i++) {    // 0x31 - 0x3E
        deviceStatusChanged(i);
    }



    RDevice *rDev = qobject_cast<RDevice*>(sio->getDevice(0x50));
    if (rDev) {
        bool isRDeviceActive = aspeqtSettings->isRDeviceEnabled();
        rDev->setEnabled(isRDeviceActive); // Updates internal flag and closes active sockets

        // If newly enabled, ensure the phonebook is loaded from the current path
        if (isRDeviceActive) {
            rDev->loadPhonebook(aspeqtSettings->modemBridgePhonebookPath());
        }
    }


    if (aspeqtSettings->isModemBridgeEnabled()) {
        // 1. Create if missing
        if (!modemBridge) {
            modemBridge = new ModemBridge(this);

            // Connect Status Messages (Info)
            // Connect Status Messages (Info)
            connect(modemBridge, &ModemBridge::statusMessage, this, [](const QString &msg) {
                // Use qDebug so the message handler catches it and colors it Blue (!i)
                qDebug() << "!i [ModemBridge]" << msg;
            });

            // Connect Error Messages (Error)
            connect(modemBridge, &ModemBridge::errorOccurred, this, [](const QString &err) {
                // Use qCritical so the message handler catches it and colors it Red (!e)
                qCritical() << "!e [ModemBridge]" << err;
            });
        }

        // 2. Update Configuration (in case Port/Baud changed)
        // Note: setSerialPort calls stop() internally if it's already running, so this is safe.


        modemBridge->setSerialPort(aspeqtSettings->modemBridgePortName(),
                                   aspeqtSettings->modemBridgeBaudRate());
        modemBridge->setFlowControl(aspeqtSettings->modemBridgeFlowControl());
        modemBridge->setLocalEcho(aspeqtSettings->modemBridgeLocalEcho());
        modemBridge->setTcpMode(aspeqtSettings->modemBridgeSshEnabled());

        // Load Phonebook path
        QString pbPath = aspeqtSettings->modemBridgePhonebookPath();
        if (pbPath.isEmpty()) pbPath = g_aspeQtAppPath + "/phonebook.xml";
        modemBridge->setPhonebookPath(pbPath);
        modemBridge->start();

    }
    else {
        // User Unchecked the box -> STOP THE BRIDGE (Drops DTR/RTS)
        if (modemBridge) {
            modemBridge->stop();
        }
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
    // Uses the helper function you added at the bottom of the file
    // to extract the HTML from resources and open it in Chrome/Edge.
    openResourceHtml(":/documentation/AspeQt User Manual-English.html");

    // Uncheck the menu item immediately since it's not a toggle anymore
    if (ui->actionDocumentation->isChecked()) {
        ui->actionDocumentation->setChecked(false);
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

    if (no >= 0 && no < DISK_COUNT && m_slotDownloadId[no] != 0) {
        qDebug() << "!w" << tr("Slot %1 download aborted by user.").arg(no+1);
        m_slotDownloadId[no] = 0; // Signals openUrl to terminate instantly
        diskWidgets[no]->showAsEmpty();
        if (webBridge) emit webBridge->diskStatusChanged(no, "Empty", "--","", false, false, false);
        return true; // Slot is successfully freed!
    }

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
        answer = saveImageWhenClosing(no, QMessageBox::StandardButton::No, 0);
        if (answer == QMessageBox::StandardButton::Cancel) {
            return false;
        }
    }

    // If ANY device exists in this slot (TNFS or Disk), remove it
    if (device) {
        sio->uninstallDevice(no + DISK_BASE_CDEVIC);

        sio->setHighSpeed(false);

        // This virtual destructor cleans up TnfsImage OR SimpleDiskImage
        device->deleteLater();

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

void MainWindow::bootExe(const QString& fileName)
{
    if (fileName.isEmpty()) return;

    // Use our new modern, non-blocking XEX class via the headless mounter!
    // We force it to slot 0 (Drive 1) because Atari requires executables to boot from D1:
    mountFileHeadless(0, fileName);

    // Optionally update the "Last Executable Directory" setting
    QFileInfo fi(fileName);
    aspeqtSettings->setLastExeDir(fi.absolutePath());
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
        diskWidgets[no]->setHappyMode(happy);

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
    // Always mount from "last image dir"
    QString dir = aspeqtSettings->lastDiskImageDir();

    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open a disk or executable image"),
                                                    dir,
                                                    tr("All Supported Images (*.atr *.xfd *.atx *.pro *.xex *.com);;"
                                                       "Atari Executables (*.xex *.com);;"
                                                       "SIO2PC ATR images (*.atr);;"
                                                       "XFormer XFD images (*.xfd);;"
                                                       "ATX images (*.atx);;"
                                                       "Pro images (*.pro);;"
                                                       "All files (*)"));

    // User canceled the dialog
    if (fileName.isEmpty()) {
        return;
    }

    // Save the directory for next time
    aspeqtSettings->setLastDiskImageDir(QFileInfo(fileName).absolutePath());

    // --- NEW: Route Executables to the modern headless loader ---
    if (fileName.endsWith(".xex", Qt::CaseInsensitive) || fileName.endsWith(".com", Qt::CaseInsensitive)) {
        mountFileHeadless(no, fileName);
    } else {
        // Standard floppy disk image mounting
        mountFileWithDefaultProtection(no, fileName);
    }
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
    if (!img) return; 
    img->setReadOnly(protectionEnabled);
    aspeqtSettings->setMountedImageProtection(no, protectionEnabled);
    diskWidgets[no]->setWriteProtect(protectionEnabled);
    deviceStatusChanged(no + DISK_BASE_CDEVIC);
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

    if (previousAnswer != QMessageBox::StandardButton::YesToAll) {
        QMessageBox::StandardButtons buttons;
        if (number) {
            buttons = QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::YesToAll | QMessageBox::StandardButton::NoToAll | QMessageBox::StandardButton::Cancel;
        } else {
            buttons = QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel;
        }
        previousAnswer = QMessageBox::question(this, tr("Image file unsaved"), tr("'%1' has unsaved changes, do you want to save it?")
                                       .arg(img->originalFileName()), buttons);
    }
    if (previousAnswer == QMessageBox::StandardButton::Yes || previousAnswer == QMessageBox::StandardButton::YesToAll) {
        saveDisk(no);
    }
    if (previousAnswer == QMessageBox::Close) {
        previousAnswer = QMessageBox::StandardButton::Cancel;
    }
    return previousAnswer;
}

void MainWindow::loadTranslators()
{
    qApp->removeTranslator(&aspeqt_qt_translator);
    qApp->removeTranslator(&aspeqt_translator);
    if (aspeqtSettings->i18nLanguage().compare("auto") == 0) {
        QString locale = QLocale::system().name();
        (void)aspeqt_translator.load(":/translations/i18n/aspeqt_" + locale);
        (void)aspeqt_qt_translator.load(":/translations/i18n/qt_" + locale);
        qApp->installTranslator(&aspeqt_qt_translator);
        qApp->installTranslator(&aspeqt_translator);
    } else if (aspeqtSettings->i18nLanguage().compare("en") != 0) {
        (void)aspeqt_translator.load(":/translations/i18n/aspeqt_" + aspeqtSettings->i18nLanguage());
        (void)aspeqt_qt_translator.load(":/translations/i18n/qt_" + aspeqtSettings->i18nLanguage());
        qApp->installTranslator(&aspeqt_qt_translator);
        qApp->installTranslator(&aspeqt_translator);
    }
}


/* mainwindow.cpp - saveDisk */
void MainWindow::saveDisk(int no)
{
    // --- FIX: USE CORRECT OFFSET FOR SioDevice ---
    SioDevice *device = sio->getDevice(no + DISK_BASE_CDEVIC);
    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (device);

    if (!img) return;

    // Check if it is TNFS
    if (qobject_cast<TnfsImage*>(device))  return;
    // ---------------------------------------------

    if (img->isUnnamed()) {
        saveDiskAs(no);
    } else {
        img->lock();
        bool saved = img->save();
        img->unlock();
        if (!saved) {
            if (QMessageBox::question(this, tr("Save failed"), tr("'%1' cannot be saved, do you want to save the image with another name?")
                                                                   .arg(img->originalFileName()), QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes) {
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
            .arg(img->originalFileName()), QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes) {
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
                .arg(fileName), QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::No) {
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
            .arg(img->originalFileName()), QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes) {
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
    QMessageBox::StandardButton answer = QMessageBox::StandardButton::No;

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
            if (answer == QMessageBox::StandardButton::NoToAll) {
                break;
            }
            if (answer == QMessageBox::StandardButton::Cancel) {
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

    // --- [FIX 1] STOP EMULATION & PREVENT AUTO-RESTART ---
    // We must ensure the SIO thread is dead before we start ripping out drives.
    // By stopping it here, the subsequent call to EjectAll will see it's
    // already stopped and WON'T try to restart it automatically.
    bool wasRunning = ui->actionStartEmulation->isChecked();
    if (wasRunning) {
        ui->actionStartEmulation->trigger(); // Stop
        sio->wait(); // Block until thread is truly finished
        qApp->processEvents();
    }

    // Eject existing images (Safe now because emulation is off)
    MainWindow::on_actionEjectAll_triggered();

    aspeqtSettings->setLastSessionDir(QFileInfo(fileName).absolutePath());
    g_sessionFile = QFileInfo(fileName).fileName();
    g_sessionFilePath = QFileInfo(fileName).absolutePath();

    // Pass Session file name, path and MainWindow title to AspeQtSettings
    aspeqtSettings->setSessionFile(g_sessionFile, g_sessionFilePath);
    aspeqtSettings->setMainWindowTitle(g_mainWindowTitle);

    aspeqtSettings->loadSessionFromFile(fileName);

    setWindowTitle(g_mainWindowTitle + tr(" -- Session: ") + g_sessionFile);
    setGeometry(aspeqtSettings->lastHorizontalPos(), aspeqtSettings->lastVerticalPos(), aspeqtSettings->lastWidth() , aspeqtSettings->lastHeight());

    // Mount the new images
    for (int i = 0; i < DISK_COUNT; i++) {
        AspeQtSettings::ImageSettings is;
        is = aspeqtSettings->mountedImageSetting(i);
        // Only attempt mount if filename is valid
        if (!is.fileName.isEmpty()) {
            mountFile(i, is.fileName, is.isWriteProtected);
        }
    }

    // --- [FIX 2] CORRECT VISIBILITY RESTORATION ---
    // OLD BUG: on_actionHideShowDrives_triggered() is a TOGGLE.
    // If the setting was TRUE, calling it set it to FALSE.
    // We must set the state directly instead.
    g_D9DOVisible = aspeqtSettings->D9DOVisible();
    showHideDrives();

    // Now that everything is safe and loaded, we can restart emulation
    // (setSession will trigger the Start action)
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

    // --- [FIX] FORCE EXTENSION ---
    // QFileDialog often returns the exact string typed by the user.
    // If they forgot ".aspeqt", we must append it so the Open dialog can find it later.
    if (!fileName.endsWith(".aspeqt", Qt::CaseInsensitive)) {
        fileName += ".aspeqt";
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

    // 3. FIX: Ensure the native Qt UI widget reflects this change (crucial for Web UI clicks)
    if (deviceId >= 0 && deviceId < DISK_COUNT) {
        diskWidgets[deviceId]->setHappyMode(enabled);
    }

    // 4. FIX: Broadcast the new state to the Web UI so the button turns yellow!
    deviceStatusChanged(deviceId + DISK_BASE_CDEVIC);
}



void MainWindow::on_actionMountTnfs_triggered(int deviceId)
{
    // 1. Set Busy Cursor immediately
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // 2. Launch the thread-safe browser with the LAST SAVED URL
    TnfsBrowser browser(this, aspeqtSettings->restoreTnfsLocation() ? g_lastTnfsUrl : "");

    // 3. Restore Cursor
    QApplication::restoreOverrideCursor();

    if (browser.exec() == QDialog::Accepted) {
        QString url = browser.getSelectedUrl();
        g_lastTnfsUrl = url;

        // Eject whatever is currently in that slot (This will ALSO abort an active download)
        if (!ejectImage(deviceId)) return;

        // --- NEW: Claim the slot with a unique Session ID ---
        m_downloadCounter++;
        int myId = m_downloadCounter;
        m_slotDownloadId[deviceId] = myId;

        // Instant "Loading..." Feedback
        diskWidgets[deviceId]->showAsTNFSMounted(tr("Loading..."), tr("Downloading from TNFS..."));
        if (webBridge) {
            emit webBridge->diskStatusChanged(deviceId, "Loading...", "Downloading from TNFS...","", false, false, false);
        }
        QCoreApplication::processEvents(); // Force UI update before blocking network call

        TnfsImage *tnfs = new TnfsImage(sio);
        tnfs->setParent(nullptr);   // Detach
        connect(tnfs, &TnfsImage::downloadProgress, this, &MainWindow::updateDownloadProgress);

        // --- NEW: Pass the Session ID pointers to the network loop! ---
        if (tnfs->openUrl(url, &m_slotDownloadId[deviceId], myId)) {

            // Did the user click Eject at the very last millisecond?
            if (m_slotDownloadId[deviceId] != myId) { delete tnfs; return; }

            // NOW safely push to the background!
            tnfs->moveToThread(sio);
            // Install into the SIO chain
            sio->installDevice(DISK_BASE_CDEVIC + deviceId, tnfs);

            // Clear the lock
            m_slotDownloadId[deviceId] = 0;

            // Trigger UI Update via the central handler
            deviceStatusChanged(DISK_BASE_CDEVIC + deviceId);
            qDebug() << "!i" << tr("Mounted TNFS Stream: %1").arg(url);
            dlProgressBar->hide();

        } else {
            delete tnfs;
            dlProgressBar->hide();

            // --- NEW: Only show the error & reset UI if WE are still the active process ---
            // If the ID changed, it means the user clicked Eject to abort it,
            // so we stay silent and let Eject handle the UI cleanup!
            if (m_slotDownloadId[deviceId] == myId) {
                m_slotDownloadId[deviceId] = 0; // Clear the lock

                diskWidgets[deviceId]->showAsEmpty();
                if (webBridge) {
                    emit webBridge->diskStatusChanged(deviceId, "Empty", "--","", false, false,false);
                }

                QMessageBox::critical(this, tr("Mount Error"), tr("Could not open TNFS stream from %1").arg(url));
            }
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


void MainWindow::on_actionPhonebook_triggered()
{
    // 1. Determine path (with fallback)
    QString pbPath = aspeqtSettings->modemBridgePhonebookPath();

    if (pbPath.isEmpty()) {
        return;
    }

    PhoneDirectory pd(this);
    pd.loadFromFile(pbPath);

    if (pd.exec() == QDialog::Accepted) {
        BbsEntry entry = pd.getSelectedEntry();

        if (!entry.name.isEmpty()) {
            // [FIX] Route to the correct active modem!
            if (aspeqtSettings->isRDeviceEnabled()) {
                RDevice *rDev = qobject_cast<RDevice*>(sio->getDevice(0x50));
                if (rDev) rDev->dial(entry);
            }
            else if (aspeqtSettings->isModemBridgeEnabled() && modemBridge) {
                modemBridge->dial(entry);
            }
        }
    }
}


void MainWindow::onFireAndForget(QString urlStr, QByteArray data)
{
    // 1. Create a Manager (parented to qApp so it cleans up automatically)
    QNetworkAccessManager *manager = new QNetworkAccessManager(QCoreApplication::instance());

    QUrl url(urlStr);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");

    // 2. Choose POST or GET based on data presence
    QNetworkReply *reply;
    if (!data.isEmpty()) {
        reply = manager->post(req, data);
    } else {
        reply = manager->get(req);
    }

    // 3. Handle Completion & Cleanup
    connect(reply, &QNetworkReply::finished, [reply, manager, urlStr](){
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "!e" << "[W:] Background Network Error:" << urlStr << reply->errorString();
        } else {
            qDebug() << "!n" << "[W:] Background Success:" << urlStr;
        }
        reply->deleteLater();
        manager->deleteLater(); // Destroy manager when the reply is done
    });
}

void MainWindow::updatePhonebookMenuState()
{
    QString pbPath = aspeqtSettings->modemBridgePhonebookPath();
    bool hasPhonebook = !pbPath.isEmpty();

    // Enable or disable the actions based on whether the path exists
    ui->actionPhonebook->setEnabled(hasPhonebook);
    btnMacroUser->setEnabled(hasPhonebook);
    btnMacroPass->setEnabled(hasPhonebook);

    // Update tooltips to reflect the current state
    if (!hasPhonebook) {
        ui->actionPhonebook->setToolTip(tr("Phonebook disabled. Set XML path in Options -> Modem Bridge."));
        btnMacroUser->setToolTip(tr("Macro User disabled. Set Phonebook XML path in Options."));
        btnMacroPass->setToolTip(tr("Macro Pass disabled. Set Phonebook XML path in Options."));
    } else {
        ui->actionPhonebook->setToolTip(tr("Open BBS Phonebook"));
        btnMacroUser->setToolTip(tr("Send Auto-User (ESC-U)"));
        btnMacroPass->setToolTip(tr("Send Auto-Pass (ESC-P)"));
    }
}



/* mainwindow.cpp */

void MainWindow::openResourceHtml(const QString &resourcePath)
{
    // 1. Define where to extract (User's Temp Folder)
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString fileName = QFileInfo(resourcePath).fileName();

    // Use toNativeSeparators for better Windows compatibility
    QString targetPath = QDir::toNativeSeparators(tempPath + "/" + fileName);

    QFile targetFile(targetPath);

    // 2. Handle Existing File
    if (targetFile.exists()) {
        // [FIX 1] Force Write permissions.
        // Resources often copy as Read-Only, causing "Access Denied" on removal.
        targetFile.setPermissions(QFile::WriteOwner | QFile::ReadOwner | QFile::WriteUser | QFile::ReadUser);

        // [FIX 2] Handle Browser Lock.
        // If the file is open in Edge/Chrome, remove() will fail on Windows.
        if (!targetFile.remove()) {
            qWarning() << "Could not remove temp file (likely locked by browser):" << targetPath;

            // If we can't delete it, just open the one that's already there.
            QDesktopServices::openUrl(QUrl::fromLocalFile(targetPath));
            return;
        }
    }

    // 3. Copy from Resource to Disk
    if (QFile::copy(resourcePath, targetPath)) {

        // [FIX 3] Ensure the NEW file is writable for next time
        QFile::setPermissions(targetPath, QFile::WriteOwner | QFile::ReadOwner | QFile::WriteUser | QFile::ReadUser);

        // 4. Launch in Browser
        QDesktopServices::openUrl(QUrl::fromLocalFile(targetPath));

    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not extract manual to: ") + targetPath);
    }
}


void MainWindow::blinkRx() {
    // Bright Green
    ledRx->setStyleSheet("min-width: 12px; min-height: 12px; border-radius: 7px; background-color: #00FF00; border: 1px solid #005500;");
    ledResetTimer->start();
}

void MainWindow::blinkTx() {
    // Bright Red
    ledTx->setStyleSheet("min-width: 12px; min-height: 12px; border-radius: 7px; background-color: #FF0000; border: 1px solid #550000;");
    ledResetTimer->start();
}

void MainWindow::resetLeds() {
    // Return to Dark (Off) state
    ledRx->setStyleSheet("min-width: 12px; min-height: 12px; border-radius: 7px; background-color: #004400; border: 1px solid #555;");
    ledTx->setStyleSheet("min-width: 12px; min-height: 12px; border-radius: 7px; background-color: #440000; border: 1px solid #555;");
}


void MainWindow::onSioTraceToggleClicked()
{
    bool showHex = btnSioTrace->isChecked();

    // If turning ON Hex Dump, force Disassembler OFF
    if (showHex) {
        btnDisasmToggle->setChecked(false);
        btnDisasmToggle->setIcon(QIcon(":/icons/silk-icons/icons/page_white_text.png"));
        btnDisasmToggle->setStyleSheet("");
    }

    bool showAsm = btnDisasmToggle->isChecked();

    // Enable backend tracing if EITHER tool is active
    if (sio) sio->setTraceEnabled(showHex || showAsm);

    if (showHex) {
        btnSioTrace->setIcon(QIcon(":/icons/silk-icons/icons/monitor_go.png"));
        btnSioTrace->setStyleSheet("background-color: #227722; color: white; border-radius: 4px;");
        qDebug() << "!i" << "[SIO Trace] Hex Dump Enabled";
    } else {
        btnSioTrace->setIcon(QIcon(":/icons/silk-icons/icons/monitor.png"));
        btnSioTrace->setStyleSheet("");
        qDebug() << "!i" << "[SIO Trace] Hex Dump Disabled";
    }
}

void MainWindow::onDisasmToggleClicked()
{
    bool showAsm = btnDisasmToggle->isChecked();

    // If turning ON Disassembler, force Hex Dump OFF
    if (showAsm) {
        btnSioTrace->setChecked(false);
        btnSioTrace->setIcon(QIcon(":/icons/silk-icons/icons/monitor.png"));
        btnSioTrace->setStyleSheet("");
    }

    bool showHex = btnSioTrace->isChecked();

    // Enable backend tracing if EITHER tool is active
    if (sio) sio->setTraceEnabled(showHex || showAsm);

    if (showAsm) {
        btnDisasmToggle->setIcon(QIcon(":/icons/silk-icons/icons/page_white_go.png"));
        btnDisasmToggle->setStyleSheet("background-color: #227722; color: white; border-radius: 4px;");
        qDebug() << "!i" << "[Disassembler] 6502 Disassembly Enabled";
    } else {
        btnDisasmToggle->setIcon(QIcon(":/icons/silk-icons/icons/page_white_text.png"));
        btnDisasmToggle->setStyleSheet("");
        qDebug() << "!i" << "[Disassembler] 6502 Disassembly Disabled";
    }
}


void MainWindow::onSioTraceData(const QString &dir, const QByteArray &data)
{
    if (data.isEmpty()) return;

    bool showHex = btnSioTrace->isChecked();
    bool showAsm = btnDisasmToggle->isChecked();

    // Safeguard: If neither is enabled, do nothing
    if (!showHex && !showAsm) return;

    // --- 1. RAW TIMING ANALYSIS ---
    static QElapsedTimer latencyTimer;
    QString latencyHeader;

    // Reset timer on every command to measure device response time
    if (dir.contains("CMD")) {
        latencyTimer.start();
    }
    // Show elapsed time for data responses
    else if (latencyTimer.isValid()) {
        latencyHeader = QString(" (Lat: %1ms)").arg(latencyTimer.elapsed());
    }

    // --- 2. SMART COMMAND DECODER ---
    if (dir.contains("CMD") && (data.size() == 4 || data.size() == 5)) {
        // We only print the Command Block decoder if Hex Trace is enabled
        if (showHex) {
            quint8 dev = static_cast<quint8>(data[0]);
            quint8 cmd = static_cast<quint8>(data[1]);
            quint16 aux = static_cast<quint8>(data[2]) | (static_cast<quint8>(data[3]) << 8);

            QString devName = "Unknown";
            if (dev >= 0x31 && dev <= 0x3F) devName = QString("D%1:").arg(dev - 0x30);
            else if (dev == 0x50) devName = "R1:";
            else if (dev == 0x40) devName = "P1:";
            else if (dev == 0x70) devName = "T1:";

            QString cmdName = QString("$%1").arg(cmd, 2, 16, QChar('0')).toUpper();
            if (cmd == 'R' || cmd == 0x52) cmdName = "READ";
            if (cmd == 'W' || cmd == 0x57) cmdName = "WRITE";
            if (cmd == 'S' || cmd == 0x53) cmdName = "STATUS";
            if (cmd == 'P' || cmd == 0x50) cmdName = "PUT/WRITE";
            if (cmd == 0x21) cmdName = "FORMAT";

            QString cmdLog = QString("<pre style='margin: 0; font-family: \"Courier New\", Courier, monospace;'><font color='#882288'><b>[SIO CMD] %1 %2 (Aux: %3)</b></font></pre>")
                                 .arg(devName).arg(cmdName).arg(aux);
            qDebug().noquote() << "!n" << cmdLog;
        }

        // Bail out. SIO commands are never executable ASM code anyway.
        return;
    }

    // --- 3. HEX DUMP WITH ZERO-DIMMING ---
    if (showHex) {
        QString headerColor = dir.startsWith("TX") ? "#C45911" : "#1177C4";
        QString dump = QString("<pre style='margin: 0; line-height: 1.2; font-family: \"Courier New\", Courier, monospace;'><font color='%1'><b>[SIO %2]</b></font> %3 bytes%4:\n")
                           .arg(headerColor).arg(dir).arg(data.size()).arg(latencyHeader);

        for (int i = 0; i < data.size(); i += 16) {
            QByteArray chunk = data.mid(i, 16);
            QString offset = QString("%1").arg(i, 4, 16, QChar('0')).toUpper();

            QString hex;
            for (int j = 0; j < 16; ++j) {
                if (j == 8) hex += " "; // Middle Gutter

                if (j < chunk.size()) {
                    quint8 byte = static_cast<quint8>(chunk[j]);
                    QString bHex = QString("%1").arg(byte, 2, 16, QChar('0')).toUpper();

                    if (byte == 0x00 || byte == 0x20) {
                        hex += QString("<font color='#444444'>%1</font> ").arg(bHex);
                    } else {
                        hex += bHex + " ";
                    }
                } else {
                    hex += "   "; // Alignment padding
                }
            }

            QString ascii;
            for (char c : chunk) {
                quint8 byte = static_cast<quint8>(c);
                if (byte >= 32 && byte <= 126) ascii += static_cast<char>(byte);
                else if (byte >= 160 && byte <= 254) ascii += static_cast<char>(byte - 128);
                else ascii += '.';
            }

            dump += QString("<font color='#888888'>%1:</font>  %2  <font color='#888888'>|</font>  %3\n")
                        .arg(offset).arg(hex).arg(ascii.toHtmlEscaped());
        }

        dump += "</pre>";
        qDebug().noquote() << "!n" << dump;
    }

    // --- 4. EXECUTABLE HEURISTIC DISASSEMBLER ---
    if (showAsm) {
        bool isExecutable = (data.size() > 5 && static_cast<quint8>(data[0]) == 0xFF && static_cast<quint8>(data[1]) == 0xFF);

        QString asmCode = "<pre style='color: #228822; margin: 0; font-family: \"Courier New\", Courier, monospace;'><b>[6502 Disassembly Suggestion]</b>\n";

        // Skip the FF FF header for disassembly
        int pc = isExecutable ? 2 : 0;

        while (pc < data.size()) {
            quint8 opByte = static_cast<quint8>(data[pc]);
            Opcode6502 op = opTable[opByte];

            QString line = QString("%1:  ").arg(pc, 4, 16, QChar('0')).toUpper();

            if (op.len == 1) {
                line += op.mnemonic;
            } else if (op.len == 2 && pc + 1 < data.size()) {
                quint8 val = static_cast<quint8>(data[pc+1]);
                line += QString(op.mnemonic).replace("xx", QString("%1").arg(val, 2, 16, QChar('0')).toUpper());
            } else if (op.len == 3 && pc + 2 < data.size()) {
                quint16 addr = static_cast<quint8>(data[pc+1]) | (static_cast<quint8>(data[pc+2]) << 8);
                line += QString(op.mnemonic).replace("xxxx", QString("%1").arg(addr, 4, 16, QChar('0')).toUpper());
            } else {
                line += QString(".BYTE $%1").arg(opByte, 2, 16, QChar('0')).toUpper();
            }

            asmCode += line + "\n";
            pc += op.len;

            if (pc > 128) {
                asmCode += "...(Truncated for brevity)...\n";
                break;
            }
        }
        asmCode += "</pre>";
        qDebug().noquote() << "!n" << asmCode;
    }
}


void MainWindow::on_actionInspectSectors_triggered(int deviceId)
{
    SimpleDiskImage *img = qobject_cast<SimpleDiskImage*>(sio->getDevice(deviceId + DISK_BASE_CDEVIC));

    if (img) {
        SectorInspectorDialog *dlg = new SectorInspectorDialog(img, this);
        dlg->setAttribute(Qt::WA_DeleteOnClose);
        dlg->show();
    }
}


// -----------------------------------------------------------------
// WEB UI slots
// -----------------------------------------------------------------


void MainWindow::refreshWebUi()
{
    // Loop through all 15 slots and force an update to the web browser
    for (int i = DISK_BASE_CDEVIC; i < (DISK_BASE_CDEVIC + DISK_COUNT); i++) {
        deviceStatusChanged(i);
    }

    if (webBridge) {
        emit webBridge->globalStatusChanged(ui->actionStartEmulation->isChecked(), aspeqtSettings->printerEmulation());
    }
}

void MainWindow::mountFileHeadless(int no, const QString &fileName)
{
    // 1. Check if there are unsaved changes just so we can log a warning
    SimpleDiskImage *img = qobject_cast<SimpleDiskImage*>(sio->getDevice(no + DISK_BASE_CDEVIC));

    if (img && img->isModified()) {
        qDebug() << "!w" << tr("[Web UI] Warning: Unsaved changes on disk in slot %1 were discarded by forced mount.").arg(no+1);
    }

    // 2. Forcibly eject the current disk WITHOUT triggering the GUI pop-up (ask = false)
    ejectImage(no, false);

    if (fileName.endsWith(".xex", Qt::CaseInsensitive) || fileName.endsWith(".com", Qt::CaseInsensitive)) {
        XexImage *xex = new XexImage(sio);
        xex->setParent(nullptr);
        // (Deleted moveToThread from here)

        if (xex->openLocalFile(fileName)) {

            // THEN move to the background!
            xex->moveToThread(sio); // <--- ADD IT HERE INSTEAD!
            sio->installDevice(DISK_BASE_CDEVIC + no, xex);
            deviceStatusChanged(DISK_BASE_CDEVIC + no);

            // Log it and notify the user
            qDebug() << "!i" << tr("[Web UI] Mounted Executable to slot %1: %2").arg(no+1).arg(fileName);
            if (no == 0) {
                uiMessage(0, tr("[Web UI] Executable ready. Please cold start the Atari."));
            }
        } else {
            delete xex;
            qDebug() << "!e" << tr("[Web UI] Failed to parse Executable: %1").arg(fileName);
            if (webBridge) {
                emit webBridge->diskStatusChanged(no, "Empty", "--","", false, false, false);
            }
        }
        return; // EXIT EARLY! Do not proceed to standard disk mounting.
    }

    // 3. Now that the slot is safely empty, mount the new file normally!
    const AspeQtSettings::ImageSettings* imgSetting = aspeqtSettings->getImageSettingsFromName(fileName);
    bool prot = (imgSetting != NULL) && imgSetting->isWriteProtected;
    mountFile(no, fileName, prot);
}


void MainWindow::ejectHeadless(int no)
{
    // 1. Check if modified to log a warning
    SimpleDiskImage *img = qobject_cast<SimpleDiskImage*>(sio->getDevice(no + DISK_BASE_CDEVIC));
    if (img && img->isModified()) {
        qDebug() << "!w" << tr("[Web UI] Warning: Unsaved changes on disk in slot %1 were discarded by forced eject.").arg(no+1);
    }

    // 2. Forcibly eject the current disk WITHOUT triggering the GUI pop-up (ask = false)
    ejectImage(no, false);
}

void MainWindow::toggleAutoSaveHeadless(int no)
{
    // Safely trigger the native checkbox click inside the DriveWidget
    if (no >= 0 && no < DISK_COUNT) {
        diskWidgets[no]->triggerAutoSaveClickIfEnabled();

        // Broadcast the new state to the Web UI
        deviceStatusChanged(DISK_BASE_CDEVIC + no);
    }
}


void MainWindow::hangupModem() {
    if (aspeqtSettings->isRDeviceEnabled()) {
        RDevice *rDev = qobject_cast<RDevice*>(sio->getDevice(0x50));
        if (rDev) rDev->hangup();
    } else if (modemBridge) {
        modemBridge->hangup();
    }
}

void MainWindow::sendMacroUser() {
    if (aspeqtSettings->isRDeviceEnabled()) {
        RDevice *rDev = qobject_cast<RDevice*>(sio->getDevice(0x50));
        if (rDev) rDev->injectMacro('U');
    } else if (modemBridge) {
        modemBridge->injectMacro('U');
    }
}

void MainWindow::sendMacroPass() {
    if (aspeqtSettings->isRDeviceEnabled()) {
        RDevice *rDev = qobject_cast<RDevice*>(sio->getDevice(0x50));
        if (rDev) rDev->injectMacro('P');
    } else if (modemBridge) {
        modemBridge->injectMacro('P');
    }
}

void MainWindow::dialBbsSilent(const QString &name, const QString &ip, int port, const QString &protocol, const QString &login, const QString &password) {
    // 1. Reconstruct the BbsEntry object
    BbsEntry entry;
    entry.name = name;
    entry.ip = ip;
    entry.port = port;
    entry.protocol = protocol;
    entry.login = login;
    entry.password = password;

    // 2. Send it to whichever modem is active!
    if (aspeqtSettings->isRDeviceEnabled()) {
        RDevice *rDev = qobject_cast<RDevice*>(sio->getDevice(0x50));
        if (rDev) rDev->dial(entry);
    } else if (aspeqtSettings->isModemBridgeEnabled() && modemBridge) {
        modemBridge->dial(entry);
    }

    qDebug() << "!i" << tr("[Web UI] Dialing BBS: %1 (%2)").arg(name, ip);
}

void MainWindow::toggleEmulationHeadless()
{
    // Safely simulate a real mouse click on the UI action
    ui->actionStartEmulation->trigger();
}

void MainWindow::togglePrinterHeadless()
{
    // Safely simulate a real mouse click on the UI action
    ui->actionPrinterEmulation->trigger();
}

QString MainWindow::getLogText() {
    // Grabs the plain text out of the native Qt Log window
    return ui->textEdit->toPlainText();
}


void MainWindow::mountTnfsHeadless(int no, const QString &url)
{
    // Eject disk OR abort existing download
    if (!ejectImage(no, false)) return;

    // Claim the slot with a unique Session ID
    m_downloadCounter++;
    int myId = m_downloadCounter;
    m_slotDownloadId[no] = myId;

    diskWidgets[no]->showAsTNFSMounted(tr("Loading..."), tr("Downloading from TNFS..."));
    if (webBridge) {
        emit webBridge->diskStatusChanged(no, "Loading...", "Downloading from TNFS...","", false, false, false);
    }
    QCoreApplication::processEvents();

    TnfsImage *tnfs = new TnfsImage(sio);
    tnfs->setParent(nullptr);
    connect(tnfs, &TnfsImage::downloadProgress, this, &MainWindow::updateDownloadProgress);

    // Pass the Session ID pointers!
    if (tnfs->openUrl(url, &m_slotDownloadId[no], myId)) {
        // Did an abort happen right at the very end?
        if (m_slotDownloadId[no] != myId) { delete tnfs; return; }

        tnfs->moveToThread(sio);
        sio->installDevice(DISK_BASE_CDEVIC + no, tnfs);

        m_slotDownloadId[no] = 0; // Clear lock
        deviceStatusChanged(DISK_BASE_CDEVIC + no);
        qDebug() << "!i" << tr("[Web UI] Mounted TNFS Stream: %1").arg(url);
        dlProgressBar->hide();
    } else {
        delete tnfs;
        dlProgressBar->hide();
        // Only reset the UI to empty if WE are still the active process
        if (m_slotDownloadId[no] == myId) {
            m_slotDownloadId[no] = 0;
            diskWidgets[no]->showAsEmpty();
            if (webBridge) emit webBridge->diskStatusChanged(no, "Empty", "--","", false, false, false);
            qDebug() << "!e" << tr("[Web UI] Failed to mount TNFS Stream: %1").arg(url);
            emit webBridge->notificationReceived(tr("Download failed or aborted: %1").arg(url), true);
        }
    }
}

void MainWindow::toggleWriteProtectHeadless(int no, bool enabled)
{
    if (no >= 0 && no < DISK_COUNT) {
        // Log it so you can see the Web UI command arriving
        qDebug() << "!i" << tr("[Web UI] Write Protect for slot %1 set to %2").arg(no+1).arg(enabled ? "ON" : "OFF");

        // Trigger the core logic directly (which now also updates the Qt UI automatically!)
        toggleWriteProtection(no, enabled);
    }
}

QString MainWindow::getPrinterText() {
    if (textPrinterWindow) {
        return textPrinterWindow->getAsciiText();
    }
    return "";
}

void MainWindow::startWebUi() {
    // 1. Create and start WebSocket Server
    if (!webSocketServer) {
        webSocketServer = new QWebSocketServer("AspeQtWeb", QWebSocketServer::NonSecureMode, this);
        clientWrapper = new WebSocketClientWrapper(webSocketServer, this);
        connect(clientWrapper, &WebSocketClientWrapper::clientConnected, webChannel, &QWebChannel::connectTo);

        if (webSocketServer->listen(QHostAddress::Any, aspeqtSettings->webUiWsPort())) {
            qDebug() << "!i" << tr("Web UI WebSocket Server started on port %1").arg(aspeqtSettings->webUiWsPort());
        } else {
            qDebug() << "!e" << tr("Failed to start Web UI WebSocket Server.");
        }
    }

    // 2. Create and start HTTP Server
    if (!httpServer) {
        httpServer = new QHttpServer(this);
        httpTcpServer = new QTcpServer(this);

        httpServer->route("/", [this]() {
            QFile file(":/webui/index.html");
            if (file.open(QIODevice::ReadOnly)) {
                QString html = QString::fromUtf8(file.readAll());
                html.replace("12345", QString::number(aspeqtSettings->webUiWsPort()));
                return QHttpServerResponse("text/html", html.toUtf8());
            }
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        });

        httpServer->route("/qwebchannel.js", QHttpServerRequest::Method::Get, []() {
            QFile file(":/webui/qwebchannel.js");
            if (file.open(QIODevice::ReadOnly)) {
                return QHttpServerResponse("application/javascript", file.readAll());
            }
            return QHttpServerResponse(QHttpServerResponder::StatusCode::NotFound);
        });

        if (httpTcpServer->listen(QHostAddress::Any, aspeqtSettings->webUiPort())) {
            httpServer->bind(httpTcpServer);
            qDebug() << "!i" << tr("HTTP Dashboard available at http://localhost:%1").arg(aspeqtSettings->webUiPort());
        } else {
            qDebug() << "!e" << tr("Failed to start HTTP Server.");
        }
    }
}

void MainWindow::stopWebUi() {
    // 1. Completely destroy the HTTP Server
    // Note: QHttpServer takes ownership of the QTcpServer when bound,
    // so deleting httpServer automatically deletes httpTcpServer safely.
    if (httpServer) {
        delete httpServer;
        httpServer = nullptr;
        httpTcpServer = nullptr;
    } else if (httpTcpServer) {
        delete httpTcpServer;
        httpTcpServer = nullptr;
    }

    // 2. Completely destroy the WebSocket Server & Clients
    if (clientWrapper) {
        delete clientWrapper;
        clientWrapper = nullptr;
    }
    if (webSocketServer) {
        delete webSocketServer;
        webSocketServer = nullptr;
    }

    qDebug() << "!w" << tr("Web Dashboard and WebSocket servers completely shut down.");
}


void MainWindow::mountCasHeadless(const QString &fileName)
{
    ejectCasHeadless(); // Clean up any currently running tape

    m_casWorker = new CassetteWorker();
    if (!m_casWorker->loadCasImage(fileName)) {
        qWarning() << "!e" << tr("[Web UI] Failed to load cassette image: %1").arg(fileName);
        delete m_casWorker;
        m_casWorker = nullptr;
        return;
    }

    m_casFileName = fileName;
    m_casIsPlaying = false;

    QFileInfo fi(fileName);
    qDebug() << "!i" << tr("[Web UI] Cassette Mounted: %1").arg(fi.fileName());

    // Broadcast state to the Web UI
    if (webBridge) {
        emit webBridge->casStatusChanged(fi.fileName(), false);
    }
}

void MainWindow::playCasHeadless()
{
    if (!m_casWorker) return;

    if (m_casWorker->isRunning()) {
        qDebug() << "!w" << tr("[Web UI] Cassette is already playing.");
        return;
    }

    qDebug() << "!n" << tr("[Web UI] Starting Cassette Playback.");

    // Start the FSK Audio generation thread!
    m_casWorker->start(QThread::TimeCriticalPriority);
    m_casIsPlaying = true;
    m_casTimer->start(1000); // Check status every second

    QFileInfo fi(m_casFileName);
    if (webBridge) emit webBridge->casStatusChanged(fi.fileName(), true);
}

void MainWindow::rewindCasHeadless()
{
    if (m_casFileName.isEmpty()) return;

    qDebug() << "!n" << tr("[Web UI] Rewinding Cassette...");

    // To rewind, we simply stop the worker and reload the file from the beginning
    QString currentFile = m_casFileName;
    ejectCasHeadless();
    mountCasHeadless(currentFile);
}

void MainWindow::ejectCasHeadless()
{
    if (m_casWorker) {
        if (m_casWorker->isRunning()) {
            // Force the background thread to stop streaming audio
            m_casWorker->terminate();
            m_casWorker->wait();
        }
        delete m_casWorker;
        m_casWorker = nullptr;
    }

    m_casTimer->stop();
    m_casIsPlaying = false;
    m_casFileName.clear();

    qDebug() << "!i" << tr("[Web UI] Cassette Ejected.");

    if (webBridge) {
        emit webBridge->casStatusChanged("", false);
    }
}

void MainWindow::updateCasProgress()
{
    // The timer checks if the tape has naturally reached the end of the file
    if (m_casWorker && !m_casWorker->isRunning()) {

        m_casTimer->stop();
        m_casIsPlaying = false;

        QFileInfo fi(m_casFileName);
        qDebug() << "!i" << tr("[Web UI] Cassette Playback Finished.");

        // Turn the green "Play" button back to gray on the phone
        if (webBridge) {
            emit webBridge->casStatusChanged(fi.fileName(), false);
        }
    }
}


void MainWindow::createBlankDiskHeadless(int slot, const QString &fileName, int type)
{
    if (slot < 0 || slot >= DISK_COUNT) return;

    // 1. Eject whatever is currently in the slot safely
    if (!ejectImage(slot, false)) return;

    // 2. Create the raw disk image in memory
    SimpleDiskImage *disk = new SimpleDiskImage(sio);
    connect(disk, SIGNAL(statusChanged(int)), this, SLOT(deviceStatusChanged(int)), Qt::QueuedConnection);

    if (!disk->create(++untitledName)) {
        delete disk;
        if (webBridge) emit webBridge->notificationReceived(tr("Failed to initialize blank disk in memory."), true);
        return;
    }

    // 3. Configure the Sector Geometry based on the user's selection
    DiskGeometry g;
    int secCount = 720;
    int secSize = 128;

    if (type == 1) { secCount = 1040; secSize = 128; }      // Enhanced Density (130K)
    else if (type == 2) { secCount = 720; secSize = 256; }  // Double Density (180K)
    // type == 0 is standard Single Density (90K)

    uint size = secCount * secSize;
    if (secSize == 256) {
        if (secCount >= 3) size -= 384;
        else size -= secCount * 128;
    }

    g.initialize(size, secSize);

    // 4. Format the disk with Atari DOS parameters
    if (!disk->format(g)) {
        delete disk;
        if (webBridge) emit webBridge->notificationReceived(tr("Failed to format blank disk."), true);
        return;
    }

    // 5. Save the physical .atr file to the host PC immediately
    QString safeFileName = fileName;
    if (!safeFileName.toLower().endsWith(".atr")) safeFileName += ".atr";

    // Save it to the last directory the user accessed
    QString fullPath = aspeqtSettings->lastDiskImageDir() + "/" + safeFileName;

    disk->lock();
    bool saved = disk->saveAs(fullPath);
    disk->unlock();

    if (!saved) {
        delete disk;
        if (webBridge) emit webBridge->notificationReceived(tr("Failed to save blank disk to host PC."), true);
        return;
    }

    // 6. Mount it to the active SIO chain!
    sio->installDevice(DISK_BASE_CDEVIC + slot, disk);
    aspeqtSettings->mountImage(slot, fullPath, false);
    deviceStatusChanged(DISK_BASE_CDEVIC + slot);

    // 7. Tell the webUI we succeeded
    if (webBridge) emit webBridge->notificationReceived(tr("Blank disk created: %1").arg(safeFileName), false);
    qDebug() << "!i" << tr("[Web UI] Created and mounted blank disk: %1 in slot %2").arg(fullPath).arg(slot+1);
}



void MainWindow::on_actionInfo_triggered(int deviceId)
{
    DriveWidget *dw = diskWidgets[deviceId];
    if (!dw) return;

    QString driveLetter = (deviceId < 9) ? QString::number(deviceId + 1) : QString((char)('J' + deviceId - 9));

    QString fileName = dw->getFileName();
    QString props = dw->getFileProps();
    QString path = dw->getFullPath();

    if (fileName.isEmpty()) fileName = tr("Empty");
    if (props.isEmpty()) props = "--";
    if (path.isEmpty()) path = tr("No file mounted.");

    // Format a sleek, rich-text message box to match the Web UI
    // Format a sleek, rich-text message box to match the Web UI
    // Wrapping it in an HTML table forces Qt to render it with a minimum width!
    QString msg = tr("<table width='350'><tr><td>"
                     "<b>Slot %1:</b><br><br>"
                     "<b>Filename:</b><br>%2<br><br>"
                     "<b>Format / Type:</b><br>%3<br><br>"
                     "<b>Absolute Path:</b><br>%4"
                     "</td></tr></table>")
                      .arg(driveLetter)
                      .arg(fileName)
                      .arg(props)
                      .arg(path);

    QMessageBox::information(this, tr("Drive Details"), msg);

}
