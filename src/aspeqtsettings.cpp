/*
 * AspeQtSettings.cpp
 */

#include "aspeqtsettings.h"
#include "serialport.h"
#include "mainwindow.h"
#include <QDir>

AspeQtSettings::AspeQtSettings()
{
    mSettings = new QSettings(); //uses QApplication's info to determine setting to use

    // --- 1. General & UI Settings ---
    mIsFirstTime = mSettings->value("FirstTime", true).toBool();
    mSettings->setValue("FirstTime", false);
    mI18nLanguage = mSettings->value("I18nLanguage", "auto").toString();
    mMinimizeToTray = mSettings->value("MinimizeToTray", false).toBool();
    mUseLargeFont = mSettings->value("UseLargeFont", false).toBool();
    mExplorerOnTop = mSettings->value("ExplorerOnTop", false).toBool();
    mEnableShade = mSettings->value("EnableShadeByDefault", false).toBool();
    msaveWindowsPos = mSettings->value("SaveWindowsPosSize", true).toBool();
    msaveDiskVis = mSettings->value("SaveDiskVisibility", true).toBool();
    mdVis = mSettings->value("D9DOVisible", true).toBool();
    mFilterUnderscore = mSettings->value("FilterUnderscore", true).toBool();
    mUseCapitalLettersInPCLINK = mSettings->value("CapitalLettersInPCLINK", false).toBool();

    mMainX = mSettings->value("MainX", 20).toInt();
    mMainY = mSettings->value("MainY", 40).toInt();
    mMainW = mSettings->value("MainW", 688).toInt();
    mMainH = mSettings->value("MainH", 426).toInt();
    if (mMainW < 688 && mdVis) mMainW = 688;
    if (mMainH < 426 && mdVis) mMainH = 426;
    mMiniX = mSettings->value("MiniX", 8).toInt();
    mMiniY = mSettings->value("MiniY", 30).toInt();
    mPrtX = mSettings->value("PrtX", 25).toInt();
    mPrtY = mSettings->value("PrtY", 45).toInt();
    mPrtW = mSettings->value("PrtW", 600).toInt();
    mPrtH = mSettings->value("PrtH", 486).toInt();

    // --- 2. Standard Serial Port Backend ---
    mSerialPortName = mSettings->value("SerialPortName", StandardSerialPortBackend::defaultPortName()).toString();
    if(mSerialPortName.startsWith(SERIAL_PORT_LOCATION, Qt::CaseInsensitive)) {
        mSerialPortName.remove(0, strlen(SERIAL_PORT_LOCATION));
    }
    mSerialPortHandshakingMethod = mSettings->value("HandshakingMethod", 0).toInt();
    mSerialPortTriggerOnFallingEdge = mSettings->value("FallingEdge", false).toBool();
    mSerialPortHardwareUart = mSettings->value("SerialPortHardwareUart", false).toBool();
    mSerialPortWriteDelay = mSettings->value("WriteDelay", 1).toInt();
#ifdef Q_OS_WIN
    mSerialPortCompErrDelay = mSettings->value("CompErrDelay", 300).toInt();
#else
    mSerialPortCompErrDelay = mSettings->value("CompErrDelay", 800).toInt();
#endif
    mSerialPortMaximumSpeed = mSettings->value("MaximumSerialPortSpeed", 2).toInt();
    mSerialPortUsePokeyDivisors = mSettings->value("SerialPortUsePokeyDivisors", false).toBool();
    mSerialPortPokeyDivisor = mSettings->value("SerialPortPokeyDivisor", 6).toInt();

    // --- 3. Emulation & Virtual Devices ---
    mBackend = mSettings->value("Backend", 0).toInt();
    mAtariSioDriverName = mSettings->value("AtariSioDriverName", AtariSioBackend::defaultPortName()).toString();
    if(mAtariSioDriverName.startsWith(SERIAL_PORT_LOCATION, Qt::CaseInsensitive)) {
        mAtariSioDriverName.remove(0, strlen(SERIAL_PORT_LOCATION));
    }
    mAtariSioHandshakingMethod = mSettings->value("AtariSioHandshakingMethod", 0).toInt();
    mUseHighSpeedExeLoader = mSettings->value("UseHighSpeedExeLoader", false).toBool();
    mDisablePicoHiSpeed = mSettings->value("DisablePicoHiSpeed", false).toBool();
    mPrinterEmulation = mSettings->value("PrinterEmulation", true).toBool();
    mUseCustomCasBaud = mSettings->value("UseCustomCasBaud", false).toBool();
    mCustomCasBaud = mSettings->value("CustomCasBaud", 875).toInt();
    mLastBootDos = mSettings->value("LastBootDos", ":/boot_templates/$bootmyd").toString();


    // --- 4. Modem Bridge, RDevice & BBS Listener ---
    mModemTransportMode = mSettings->value("ModemBridge/TransportMode", 0).toInt();
    mModemBridgePhonebookPath = mSettings->value("ModemBridge/PhonebookPath", "").toString();
    mShowRDeviceWarning = mSettings->value("ShowRDeviceWarning", true).toBool();

#if defined(Q_OS_WIN)
    int defaultGuard = 50;
#elif defined(Q_OS_MAC)
    int defaultGuard = 20;
#else
    int defaultGuard = 10;
#endif

    for (int i = 0; i < 4; i++) {
        QString pfx = QString("ModemBridge/R%1/").arg(i + 1);
        mModemBridgeLocalEcho[i] = mSettings->value(pfx + "LocalEcho", false).toBool();
        mBbsListenerEnabled[i] = mSettings->value(pfx + "BbsListenerEnabled", false).toBool();
        mModemListenPort[i] = mSettings->value(pfx + "ListenPort", 2301 + i).toInt(); // 2301, 2302...
        mModemBridgePortName[i] = mSettings->value(pfx + "PortName", "").toString();
        mModemBridgeBaudRate[i] = mSettings->value(pfx + "BaudRate", 9600).toInt();
        mModemBridgeFlowControl[i] = mSettings->value(pfx + "FlowControl", true).toBool();
        mInvertCtsLogic[i] = mSettings->value(pfx + "InvertCts", true).toBool();
        mStreamGuardDelay[i] = mSettings->value(pfx + "StreamGuardDelay", defaultGuard).toInt();
    }

    // --- 5. TNFS & Web UI ---
    mWebUiEnabled = mSettings->value("WebUI/Enabled", false).toBool();
    mWebUiPort = mSettings->value("WebUI/HttpPort", 8080).toInt();
    mWebUiWsPort = mSettings->value("WebUI/WsPort", 8090).toInt();
    mRestoreTnfsLocation = mSettings->value("RestoreTnfsLocation", true).toBool();
    mTranslateEolOnPost = mSettings->value("PipeNetwork/TranslateEolOnPost", true).toBool();
    mTranslateEolOnGet = mSettings->value("PipeNetwork/TranslateEolOnGet", false).toBool();
    mUseURLSubmit = mSettings->value("URLSubmit", false).toBool();

    // --- Image / File Management Arrays ---
    mSettings->beginReadArray("MountedImageSettings");
    for (int i = 0; i < 15; i++) {
        mSettings->setArrayIndex(i);
        mMountedImageSettings[i].fileName = mSettings->value("FileName", QString()).toString();
        mMountedImageSettings[i].isWriteProtected = mSettings->value("IsWriteProtected", false).toBool();
        mMountedImageSettings[i].isHappyMode = mSettings->value("IsHappyMode", false).toBool();
    }
    mSettings->endArray();

    mSettings->beginReadArray("RecentImageSettings");
    for (int i = 0; i < NUM_RECENT_FILES; i++) {
        mSettings->setArrayIndex(i);
        mRecentImageSettings[i].fileName = mSettings->value("FileName", QString()).toString();
        mRecentImageSettings[i].isWriteProtected = mSettings->value("IsWriteProtected", false).toBool();
    }
    mSettings->endArray();

    mLastDiskImageDir = mSettings->value("LastDiskImageDir", "").toString();
    mLastFolderImageDir = mSettings->value("LastFolderImageDir", "").toString();
    mLastSessionDir = mSettings->value("LastSessionDir", "").toString();
    mLastExeDir = mSettings->value("LastExeDir", "").toString();
    mLastExtractDir = mSettings->value("LastExtractDir", "").toString();
    mLastPrinterTextDir = mSettings->value("LastPrinterTextDir", "").toString();
    mLastCasDir = mSettings->value("LastCasDir", "").toString();

    // printer
    mPrinterAutoPop = mSettings->value("Printer/AutoPop", false).toBool();
    mPrinterFeedMode = mSettings->value("Printer/FeedMode", 0).toInt();
    mPrinterStyle = mSettings->value("Printer/Style", 0).toInt();
    mPrinterMarginTop = mSettings->value("Printer/MarginTop", 108).toInt();
    mPrinterMarginLeft = mSettings->value("Printer/MarginLeft", 60).toInt();
    mPrinterMarginLength = mSettings->value("Printer/MarginLength", 2376).toInt();

    // Voice
    mVoiceVolume = mSettings->value("Voice/Volume", 10).toInt();
    mVoiceRate   = mSettings->value("Voice/Rate", 5).toInt();
    mVoicePitch  = mSettings->value("Voice/Pitch", 5).toInt();

}

AspeQtSettings::~AspeQtSettings()
{
    delete mSettings;
}

// ==========================================
// 1. General & UI Settings
// ==========================================
bool AspeQtSettings::isFirstTime() { return mIsFirstTime; }

QString AspeQtSettings::i18nLanguage() { return mI18nLanguage; }
void AspeQtSettings::setI18nLanguage(const QString &lang) {
    mI18nLanguage = lang;
    if(mSessionFileName == "") mSettings->setValue("I18nLanguage", mI18nLanguage);
}

bool AspeQtSettings::minimizeToTray() { return mMinimizeToTray; }
void AspeQtSettings::setMinimizeToTray(bool tray) {
    mMinimizeToTray = tray;
    mSettings->setValue("MinimizeToTray", mMinimizeToTray);
}

bool AspeQtSettings::useLargeFont() { return mUseLargeFont; }
void AspeQtSettings::setUseLargeFont(bool largeFont) {
    mUseLargeFont = largeFont;
    if(mSessionFileName == "") mSettings->setValue("UseLargeFont", mUseLargeFont);
}

bool AspeQtSettings::explorerOnTop() { return mExplorerOnTop; }
void AspeQtSettings::setExplorerOnTop(bool expOnTop) {
    mExplorerOnTop = expOnTop;
    if(mSessionFileName == "") mSettings->setValue("ExplorerOnTop", mExplorerOnTop);
}

bool AspeQtSettings::enableShade() { return mEnableShade; }
void AspeQtSettings::setEnableShade(bool shade) {
    mEnableShade = shade;
    if(mSessionFileName == "") mSettings->setValue("EnableShadeByDefault", mEnableShade);
}

int AspeQtSettings::shadeOpacity() { return mSettings->value("MainWindow/ShadeOpacity", 60).toInt(); }
void AspeQtSettings::setShadeOpacity(int val) { mSettings->setValue("MainWindow/ShadeOpacity", val); }

bool AspeQtSettings::saveWindowsPos() { return msaveWindowsPos; }
void AspeQtSettings::setsaveWindowsPos(bool saveMwp) {
    msaveWindowsPos = saveMwp;
    if(mSessionFileName == "") mSettings->setValue("SaveWindowsPosSize", msaveWindowsPos);
}

bool AspeQtSettings::saveDiskVis() { return msaveDiskVis; }
void AspeQtSettings::setsaveDiskVis(bool saveDvis) {
    msaveDiskVis = saveDvis;
    if(mSessionFileName == "") mSettings->setValue("SaveDiskVisibility", msaveDiskVis);
}

bool AspeQtSettings::D9DOVisible() { return mdVis; }
void AspeQtSettings::setD9DOVisible(bool dVis) {
    mdVis = dVis;
    if(mSessionFileName == "") mSettings->setValue("D9DOVisible", mdVis);
}

bool AspeQtSettings::filterUnderscore() { return mFilterUnderscore; }
void AspeQtSettings::setfilterUnderscore(bool filter) {
    mFilterUnderscore = filter;
    mSettings->setValue("FilterUnderscore", mFilterUnderscore);
}

bool AspeQtSettings::capitalLettersInPCLINK() { return mUseCapitalLettersInPCLINK; }
void AspeQtSettings::setCapitalLettersInPCLINK(bool caps) {
    mUseCapitalLettersInPCLINK = caps;
    mSettings->setValue("CapitalLettersInPCLINK", mUseCapitalLettersInPCLINK);
}

// Window Positions
int AspeQtSettings::lastVerticalPos() { return mMainY; }
void AspeQtSettings::setLastVerticalPos(int lastVpos) {
    mMainY = lastVpos;
    if(mSessionFileName == "") mSettings->setValue("MainY", mMainY);
}
int AspeQtSettings::lastHorizontalPos() { return mMainX; }
void AspeQtSettings::setLastHorizontalPos(int lastHpos) {
    mMainX = lastHpos;
    if(mSessionFileName == "") mSettings->setValue("MainX", mMainX);
}
int AspeQtSettings::lastWidth() { return mMainW; }
void AspeQtSettings::setLastWidth(int lastW) {
    mMainW = lastW;
    if(mSessionFileName == "") mSettings->setValue("MainW", mMainW);
}
int AspeQtSettings::lastHeight() { return mMainH; }
void AspeQtSettings::setLastHeight(int lastH) {
    mMainH = lastH;
    if(mSessionFileName == "") mSettings->setValue("MainH", mMainH);
}
int AspeQtSettings::lastMiniVerticalPos() { return mMiniY; }
void AspeQtSettings::setLastMiniVerticalPos(int lastMVpos) {
    mMiniY = lastMVpos;
    if(mSessionFileName == "") mSettings->setValue("MiniY", mMiniY);
}
int AspeQtSettings::lastMiniHorizontalPos() { return mMiniX; }
void AspeQtSettings::setLastMiniHorizontalPos(int lastMHpos) {
    mMiniX = lastMHpos;
    if(mSessionFileName == "") mSettings->setValue("MiniX", mMiniX);
}
int AspeQtSettings::lastPrtVerticalPos() { return mPrtY; }
void AspeQtSettings::setLastPrtVerticalPos(int lastPrtVpos) {
    mPrtY = lastPrtVpos;
    if(mSessionFileName == "") mSettings->setValue("PrtY", mPrtY);
}
int AspeQtSettings::lastPrtHorizontalPos() { return mPrtX; }
void AspeQtSettings::setLastPrtHorizontalPos(int lastPrtHpos) {
    mPrtX = lastPrtHpos;
    if(mSessionFileName == "") mSettings->setValue("PrtX", mPrtX);
}
int AspeQtSettings::lastPrtWidth() { return mPrtW; }
void AspeQtSettings::setLastPrtWidth(int lastPrtW) {
    mPrtW = lastPrtW;
    if(mSessionFileName == "") mSettings->setValue("PrtW", mPrtW);
}
int AspeQtSettings::lastPrtHeight() { return mPrtH; }
void AspeQtSettings::setLastPrtHeight(int lastPrtH) {
    mPrtH = lastPrtH;
    if(mSessionFileName == "") mSettings->setValue("PrtH", mPrtH);
}


// ==========================================
// 2. Standard Serial Port Backend
// ==========================================
QString AspeQtSettings::serialPortName() { return mSerialPortName; }
void AspeQtSettings::setSerialPortName(const QString &name) {
    mSerialPortName = name;
    if(mSessionFileName == "") mSettings->setValue("SerialPortName", mSerialPortName);
}

int AspeQtSettings::serialPortHandshakingMethod() { return mSerialPortHandshakingMethod; }
void AspeQtSettings::setSerialPortHandshakingMethod(int method) {
    mSerialPortHandshakingMethod = method;
    if(mSessionFileName == "") mSettings->setValue("HandshakingMethod", mSerialPortHandshakingMethod);
}

bool AspeQtSettings::serialPortTriggerOnFallingEdge() { return mSerialPortTriggerOnFallingEdge; }
void AspeQtSettings::setSerialPortTriggerOnFallingEdge(bool use) {
    mSerialPortTriggerOnFallingEdge = use;
    if(mSessionFileName == "") mSettings->setValue("FallingEdge", mSerialPortTriggerOnFallingEdge);
}

bool AspeQtSettings::serialPortHardwareUart() { return mSerialPortHardwareUart; }
void AspeQtSettings::setSerialPortHardwareUart(bool enable) {
    mSerialPortHardwareUart = enable;
    if(mSessionFileName == "") mSettings->setValue("SerialPortHardwareUart", mSerialPortHardwareUart);
}

int AspeQtSettings::serialPortMaximumSpeed() { return mSerialPortMaximumSpeed; }
void AspeQtSettings::setSerialPortMaximumSpeed(int speed) {
    mSerialPortMaximumSpeed = speed;
    if(mSessionFileName == "") mSettings->setValue("MaximumSerialPortSpeed", mSerialPortMaximumSpeed);
}

bool AspeQtSettings::serialPortUsePokeyDivisors() { return mSerialPortUsePokeyDivisors; }
void AspeQtSettings::setSerialPortUsePokeyDivisors(bool use) {
    mSerialPortUsePokeyDivisors = use;
    if(mSessionFileName == "") mSettings->setValue("SerialPortUsePokeyDivisors", mSerialPortUsePokeyDivisors);
}

int AspeQtSettings::serialPortPokeyDivisor() { return mSerialPortPokeyDivisor; }
void AspeQtSettings::setSerialPortPokeyDivisor(int divisor) {
    mSerialPortPokeyDivisor = divisor;
    if(mSessionFileName == "") mSettings->setValue("SerialPortPokeyDivisor", mSerialPortPokeyDivisor);
}

int AspeQtSettings::serialPortWriteDelay() { return mSerialPortWriteDelay; }
void AspeQtSettings::setSerialPortWriteDelay(int delay) {
    mSerialPortWriteDelay = delay;
    if(mSessionFileName == "") mSettings->setValue("WriteDelay", mSerialPortWriteDelay);
}

int AspeQtSettings::serialPortCompErrDelay() { return mSerialPortCompErrDelay; }
void AspeQtSettings::setSerialPortCompErrDelay(int delay) {
    mSerialPortCompErrDelay = delay;
    if(mSessionFileName == "") mSettings->setValue("CompErrDelay", mSerialPortCompErrDelay);
}


// ==========================================
// 3. Emulation & Virtual Devices
// ==========================================
int AspeQtSettings::backend() { return mBackend; }
void AspeQtSettings::setBackend(int backend) {
    mBackend = backend;
    if(mSessionFileName == "") mSettings->setValue("Backend", mBackend);
}

QString AspeQtSettings::atariSioDriverName() { return mAtariSioDriverName; }
void AspeQtSettings::setAtariSioDriverName(const QString &name) {
    mAtariSioDriverName = name;
    if(mSessionFileName == "") mSettings->setValue("AtariSioDriverName", mAtariSioDriverName);
}

int AspeQtSettings::atariSioHandshakingMethod() { return mAtariSioHandshakingMethod; }
void AspeQtSettings::setAtariSioHandshakingMethod(int method) {
    mAtariSioHandshakingMethod = method;
    if(mSessionFileName == "") mSettings->setValue("AtariSioHandshakingMethod", mAtariSioHandshakingMethod);
}

bool AspeQtSettings::useHighSpeedExeLoader() { return mUseHighSpeedExeLoader; }
void AspeQtSettings::setUseHighSpeedExeLoader(bool use) {
    mUseHighSpeedExeLoader = use;
    if(mSessionFileName == "") mSettings->setValue("UseHighSpeedExeLoader", mUseHighSpeedExeLoader);
}

bool AspeQtSettings::disablePicoHiSpeed() { return mDisablePicoHiSpeed; }
void AspeQtSettings::setDisablePicoHiSpeed(bool disable) {
    mDisablePicoHiSpeed = disable;
    if(mSessionFileName == "") mSettings->setValue("DisablePicoHiSpeed", mDisablePicoHiSpeed);
}

bool AspeQtSettings::printerEmulation() { return mPrinterEmulation; }
void AspeQtSettings::setPrinterEmulation(bool status) {
    mPrinterEmulation = status;
    if(mSessionFileName == "") mSettings->setValue("PrinterEmulation", mPrinterEmulation);
}

bool AspeQtSettings::useCustomCasBaud() { return mUseCustomCasBaud; }
void AspeQtSettings::setUseCustomCasBaud(bool use) {
    mUseCustomCasBaud = use;
    if(mSessionFileName == "") mSettings->setValue("UseCustomCasBaud", mUseCustomCasBaud);
}

int AspeQtSettings::customCasBaud() { return mCustomCasBaud; }
void AspeQtSettings::setCustomCasBaud(int baud) {
    mCustomCasBaud = baud;
    if(mSessionFileName == "") mSettings->setValue("CustomCasBaud", mCustomCasBaud);
}

QString AspeQtSettings::lastBootDos() { return mLastBootDos; }
void AspeQtSettings::setLastBootDos(const QString &dos) {
    mLastBootDos = dos;
    if(mSessionFileName == "") mSettings->setValue("LastBootDos", mLastBootDos);
}


// ==========================================
// 4. Modem Bridge, RDevice & BBS Listener
// ==========================================

// ==========================================
// 4. Modem Bridge, RDevice & BBS Listener
// ==========================================
int AspeQtSettings::modemTransportMode() { return mModemTransportMode; }
void AspeQtSettings::setModemTransportMode(int mode) {
    mModemTransportMode = mode;
    if(mSessionFileName == "") mSettings->setValue("ModemBridge/TransportMode", mModemTransportMode);
}

QString AspeQtSettings::modemBridgePhonebookPath() { return mModemBridgePhonebookPath; }
void AspeQtSettings::setModemBridgePhonebookPath(const QString &path) {
    mModemBridgePhonebookPath = path;
    if(mSessionFileName == "") mSettings->setValue("ModemBridge/PhonebookPath", mModemBridgePhonebookPath);
}

bool AspeQtSettings::showRDeviceWarning() { return mShowRDeviceWarning; }
void AspeQtSettings::setShowRDeviceWarning(bool show) {
    mShowRDeviceWarning = show;
    if(mSessionFileName == "") mSettings->setValue("ShowRDeviceWarning", mShowRDeviceWarning);
}

bool AspeQtSettings::modemBridgeLocalEcho(int port) { return mModemBridgeLocalEcho[port]; }
void AspeQtSettings::setModemBridgeLocalEcho(int port, bool enabled) {
    mModemBridgeLocalEcho[port] = enabled;
    if(mSessionFileName == "") mSettings->setValue(QString("ModemBridge/R%1/LocalEcho").arg(port+1), enabled);
}

bool AspeQtSettings::bbsListenerEnabled(int port) { return mBbsListenerEnabled[port]; }
void AspeQtSettings::setBbsListenerEnabled(int port, bool enable) {
    mBbsListenerEnabled[port] = enable;
    if(mSessionFileName == "") mSettings->setValue(QString("ModemBridge/R%1/BbsListenerEnabled").arg(port+1), enable);
}

int AspeQtSettings::modemListenPort(int port) { return mModemListenPort[port]; }
void AspeQtSettings::setModemListenPort(int port, int listenPort) {
    mModemListenPort[port] = listenPort;
    if(mSessionFileName == "") mSettings->setValue(QString("ModemBridge/R%1/ListenPort").arg(port+1), listenPort);
}

QString AspeQtSettings::modemBridgePortName(int port) { return mModemBridgePortName[port]; }
void AspeQtSettings::setModemBridgePortName(int port, const QString &name) {
    mModemBridgePortName[port] = name;
    if(mSessionFileName == "") mSettings->setValue(QString("ModemBridge/R%1/PortName").arg(port+1), name);
}

int AspeQtSettings::modemBridgeBaudRate(int port) { return mModemBridgeBaudRate[port]; }
void AspeQtSettings::setModemBridgeBaudRate(int port, int baud) {
    mModemBridgeBaudRate[port] = baud;
    if(mSessionFileName == "") mSettings->setValue(QString("ModemBridge/R%1/BaudRate").arg(port+1), baud);
}

bool AspeQtSettings::modemBridgeFlowControl(int port) { return mModemBridgeFlowControl[port]; }
void AspeQtSettings::setModemBridgeFlowControl(int port, bool enabled) {
    mModemBridgeFlowControl[port] = enabled;
    if(mSessionFileName == "") mSettings->setValue(QString("ModemBridge/R%1/FlowControl").arg(port+1), enabled);
}

bool AspeQtSettings::invertCtsLogic(int port) { return mInvertCtsLogic[port]; }
void AspeQtSettings::setInvertCtsLogic(int port, bool invert) {
    mInvertCtsLogic[port] = invert;
    if(mSessionFileName == "") mSettings->setValue(QString("ModemBridge/R%1/InvertCts").arg(port+1), invert);
}

int AspeQtSettings::streamGuardDelay(int port) { return mStreamGuardDelay[port]; }
void AspeQtSettings::setStreamGuardDelay(int port, int delay) {
    mStreamGuardDelay[port] = delay;
    if(mSessionFileName == "") mSettings->setValue(QString("ModemBridge/R%1/StreamGuardDelay").arg(port+1), delay);
}



// ==========================================
// 5. TNFS & Web UI
// ==========================================
bool AspeQtSettings::isWebUiEnabled() { return mWebUiEnabled; }
void AspeQtSettings::setWebUiEnabled(bool enabled) {
    mWebUiEnabled = enabled;
    if(mSessionFileName == "") mSettings->setValue("WebUI/Enabled", mWebUiEnabled);
}

int AspeQtSettings::webUiPort() { return mWebUiPort; }
void AspeQtSettings::setWebUiPort(int port) {
    mWebUiPort = port;
    if(mSessionFileName == "") mSettings->setValue("WebUI/HttpPort", mWebUiPort);
}

int AspeQtSettings::webUiWsPort() { return mWebUiWsPort; }
void AspeQtSettings::setWebUiWsPort(int port) {
    mWebUiWsPort = port;
    if(mSessionFileName == "") mSettings->setValue("WebUI/WsPort", mWebUiWsPort);
}

bool AspeQtSettings::restoreTnfsLocation() { return mRestoreTnfsLocation; }
void AspeQtSettings::setRestoreTnfsLocation(bool enabled) {
    mRestoreTnfsLocation = enabled;
    mSettings->setValue("RestoreTnfsLocation", mRestoreTnfsLocation);
}

bool AspeQtSettings::translateEolOnPost() { return mTranslateEolOnPost; }
void AspeQtSettings::setTranslateEolOnPost(bool enabled) {
    mTranslateEolOnPost = enabled;
    if(mSessionFileName == "") mSettings->setValue("PipeNetwork/TranslateEolOnPost", enabled);
}

bool AspeQtSettings::translateEolOnGet() { return mTranslateEolOnGet; }
void AspeQtSettings::setTranslateEolOnGet(bool enabled) {
    mTranslateEolOnGet = enabled;
    if(mSessionFileName == "") mSettings->setValue("PipeNetwork/TranslateEolOnGet", enabled);
}

bool AspeQtSettings::isURLSubmitEnabled() { return mUseURLSubmit; }
void AspeQtSettings::setURLSubmit(bool enabled) {
    mUseURLSubmit = enabled;
    mSettings->setValue("URLSubmit", mUseURLSubmit);
}


// ==========================================
// 6. Image & File Management
// ==========================================
const AspeQtSettings::ImageSettings* AspeQtSettings::getImageSettingsFromName(const QString &fileName)
{
    ImageSettings *is = NULL;
    int i;
    bool found = false;

    for (i = 0; i < 15; i++) {          //
        if (mMountedImageSettings[i].fileName == fileName) {
            is = &mMountedImageSettings[i];
            found = true;
            break;
        }
    }
    if (!found) {
        for (i = 0; i < NUM_RECENT_FILES; i++) {
            if (mRecentImageSettings[i].fileName == fileName) {
                is = &mRecentImageSettings[i];
                found = true;
                break;
            }
        }
    }
    return is;
}

const AspeQtSettings::ImageSettings& AspeQtSettings::mountedImageSetting(int no) { return mMountedImageSettings[no]; }
const AspeQtSettings::ImageSettings& AspeQtSettings::recentImageSetting(int no) { return mRecentImageSettings[no]; }

void AspeQtSettings::setMountedImageProtection(int no, bool prot)
{
    mMountedImageSettings[no].isWriteProtected = prot;
    if(mSessionFileName == "") mSettings->setValue(QString("MountedImageSettings/%1/IsWriteProtected").arg(no+1), prot);
}

void AspeQtSettings::setMountedImageSetting(int no, const QString &fileName, bool prot, bool happy)
{
    mMountedImageSettings[no].fileName = fileName;
    mMountedImageSettings[no].isWriteProtected = prot;
    mMountedImageSettings[no].isHappyMode = happy;

    if(mSessionFileName == "") {
        mSettings->setValue(QString("MountedImageSettings/%1/FileName").arg(no+1), fileName);
        mSettings->setValue(QString("MountedImageSettings/%1/IsWriteProtected").arg(no+1), prot);
        mSettings->setValue(QString("MountedImageSettings/%1/IsHappyMode").arg(no+1), happy);
    }
}

void AspeQtSettings::mountImage(int no, const QString &fileName, bool prot)
{
    if (fileName.isEmpty()) return;

    int i;
    bool found = false;
    for (i = 0; i < NUM_RECENT_FILES; i++) {
        if (mRecentImageSettings[i].fileName == fileName) {
            found = true;
            break;
        }
    }
    if (found) {
        for (int j = i; j < (NUM_RECENT_FILES-1); j++) {
            mRecentImageSettings[j] = mRecentImageSettings[j + 1];
        }
        mRecentImageSettings[(NUM_RECENT_FILES-1)].fileName = "";
        writeRecentImageSettings();
    }
    setMountedImageSetting(no, fileName, prot);
}

void AspeQtSettings::unmountImage(int no)
{
    ImageSettings is = mMountedImageSettings[no];

    if (!is.fileName.isEmpty()) {
        for (int i = (NUM_RECENT_FILES-1); i > 0; i--) {
            mRecentImageSettings[i] = mRecentImageSettings[i - 1];
        }
        mRecentImageSettings[0] = is;
        writeRecentImageSettings();
    }
    setMountedImageSetting(no, "", false);
}

void AspeQtSettings::swapImages(int no1, int no2)
{
    ImageSettings is1 = mountedImageSetting(no1);
    ImageSettings is2 = mountedImageSetting(no2);
    setMountedImageSetting(no1, is2.fileName, is2.isWriteProtected);
    setMountedImageSetting(no2, is1.fileName, is1.isWriteProtected);
}

void AspeQtSettings::writeRecentImageSettings()
{
    mSettings->beginWriteArray("RecentImageSettings");
    for (int i = 0; i < NUM_RECENT_FILES; i++) {
        mSettings->setArrayIndex(i);
        mSettings->setValue("FileName", mRecentImageSettings[i].fileName);
        mSettings->setValue("IsWriteProtected", mRecentImageSettings[i].isWriteProtected);
    }
    mSettings->endArray();
}

QString AspeQtSettings::lastDiskImageDir() { return mLastDiskImageDir; }
void AspeQtSettings::setLastDiskImageDir(const QString &dir) {
    mLastDiskImageDir = dir;
    mSettings->setValue("LastDiskImageDir", mLastDiskImageDir);
}

QString AspeQtSettings::lastFolderImageDir() { return mLastFolderImageDir; }
void AspeQtSettings::setLastFolderImageDir(const QString &dir) {
    mLastFolderImageDir = dir;
    mSettings->setValue("LastFolderImageDir", mLastFolderImageDir);
}

QString AspeQtSettings::lastSessionDir() { return mLastSessionDir; }
void AspeQtSettings::setLastSessionDir(const QString &dir) {
    mLastSessionDir = dir;
    mSettings->setValue("LastSessionDir", mLastSessionDir);
}

QString AspeQtSettings::lastExeDir() { return mLastExeDir; }
void AspeQtSettings::setLastExeDir(const QString &dir) {
    mLastExeDir = dir;
    mSettings->setValue("LastExeDir", mLastExeDir);
}

QString AspeQtSettings::lastExtractDir() { return mLastExtractDir; }
void AspeQtSettings::setLastExtractDir(const QString &dir) {
    mLastExtractDir = dir;
    mSettings->setValue("LastExtractDir", mLastExtractDir);
}

QString AspeQtSettings::lastPrinterTextDir() { return mLastPrinterTextDir; }
void AspeQtSettings::setLastPrinterTextDir(const QString &dir) {
    mLastPrinterTextDir = dir;
    mSettings->setValue("LastPrinterTextDir", mLastPrinterTextDir);
}

QString AspeQtSettings::lastCasDir() { return mLastCasDir; }
void AspeQtSettings::setLastCasDir(const QString &dir) {
    mLastCasDir = dir;
    mSettings->setValue("LastCasDir", mLastCasDir);
}

QString AspeQtSettings::lastRclDir()
{
    QString dir = mSettings->value("LastRclDir", "").toString();
    if (dir.isEmpty()) dir = lastDiskImageDir();
    if (dir.isEmpty()) dir = QDir::homePath();
    return dir;
}
void AspeQtSettings::setLastRclDir(const QString &dir) {
    if(mSessionFileName == "") mSettings->setValue("LastRclDir", dir);
}


// ==========================================
// 7. Session Management
// ==========================================
void AspeQtSettings::setSessionFile(const QString &g_sessionFile, const QString &g_sessionFilePath)
{
    mSessionFileName = g_sessionFile;
    mSessionFilePath = g_sessionFilePath;
}

void AspeQtSettings::setMainWindowTitle(const QString &g_mainWindowTitle)
{
    mMainWindowTitle = g_mainWindowTitle;
}

void AspeQtSettings::saveSessionToFile(const QString &fileName)
{
    extern bool g_miniMode;
    QSettings s(fileName, QSettings::IniFormat);

    s.beginGroup("AspeQt");
    s.setValue("Backend", mBackend);
    s.setValue("AtariSioDriverName", mAtariSioDriverName);
    s.setValue("AtariSioHandshakingMethod", mAtariSioHandshakingMethod);
    s.setValue("SerialPortName", mSerialPortName);
    s.setValue("HandshakingMethod", mSerialPortHandshakingMethod);
    s.setValue("FallingEdge", mSerialPortTriggerOnFallingEdge);
    s.setValue("SerialPortHardwareUart", mSerialPortHardwareUart);
    s.setValue("WriteDelay", mSerialPortWriteDelay);
    s.setValue("CompErrDelay", mSerialPortCompErrDelay);
    s.setValue("MaximumSerialPortSpeed", mSerialPortMaximumSpeed);
    s.setValue("SerialPortUsePokeyDivisors", mSerialPortUsePokeyDivisors);
    s.setValue("SerialPortPokeyDivisor", mSerialPortPokeyDivisor);
    s.setValue("UseHighSpeedExeLoader", mUseHighSpeedExeLoader);
    s.setValue("PrinterEmulation", mPrinterEmulation);
    s.setValue("CustomCasBaud", mCustomCasBaud);
    s.setValue("UseCustomCasBaud", mUseCustomCasBaud);
    s.setValue("I18nLanguage", mI18nLanguage);
    s.setValue("SaveWindowsPosSize", msaveWindowsPos);
    s.setValue("SaveDiskVisibility", msaveDiskVis);
    s.setValue("D9DOVisible", mdVis);
    if (g_miniMode) {
        s.setValue("MiniX", mMiniX);
        s.setValue("MiniY", mMiniY);
    } else {
        s.setValue("MainX", mMainX);
        s.setValue("MainY", mMainY);
        s.setValue("MainW", mMainW);
        s.setValue("MainH", mMainH);
    }
    s.setValue("PrtX", mPrtX);
    s.setValue("PrtY", mPrtY);
    s.setValue("PrtW", mPrtW);
    s.setValue("PrtH", mPrtH);
    s.setValue("FilterUnderscore", mFilterUnderscore);
    s.setValue("CapitalLettersInPCLINK", mUseCapitalLettersInPCLINK);
    s.setValue("URLSubmit", mUseURLSubmit);
    s.setValue("UseLargeFont", mUseLargeFont);
    s.setValue("ExplorerOnTop", mExplorerOnTop);
    s.setValue("EnableShadeByDefault", mEnableShade);
    s.setValue("RestoreTnfsLocation", mRestoreTnfsLocation);
    s.setValue("LastBootDos", mLastBootDos);
    s.setValue("DisablePicoHiSpeed", mDisablePicoHiSpeed);
    s.setValue("TranslateEolOnPost", mTranslateEolOnPost);
    s.setValue("TranslateEolOnGet", mTranslateEolOnGet);

    // --- Modem Bridge Matrix Saving ---
    s.setValue("ModemBridge/TransportMode", mModemTransportMode);
    s.setValue("ModemBridge/PhonebookPath", mModemBridgePhonebookPath);
    s.setValue("ShowRDeviceWarning", mShowRDeviceWarning);

    s.setValue("Voice/Volume", mVoiceVolume);
    s.setValue("Voice/Rate", mVoiceRate);
    s.setValue("Voice/Pitch", mVoicePitch);

    for (int i = 0; i < 4; i++) {
        QString pfx = QString("ModemBridge/R%1/").arg(i + 1);
        s.setValue(pfx + "LocalEcho", mModemBridgeLocalEcho[i]);
        s.setValue(pfx + "BbsListenerEnabled", mBbsListenerEnabled[i]);
        s.setValue(pfx + "ListenPort", mModemListenPort[i]);
        s.setValue(pfx + "PortName", mModemBridgePortName[i]);
        s.setValue(pfx + "BaudRate", mModemBridgeBaudRate[i]);
        s.setValue(pfx + "FlowControl", mModemBridgeFlowControl[i]);
        s.setValue(pfx + "InvertCts", mInvertCtsLogic[i]);
        s.setValue(pfx + "StreamGuardDelay", mStreamGuardDelay[i]);
    }



    s.setValue("WebUI/Enabled", mWebUiEnabled);
    s.setValue("WebUI/HttpPort", mWebUiPort);
    s.setValue("WebUI/WsPort", mWebUiWsPort); 

    s.setValue("PrinterEmulation", mPrinterEmulation);
    s.setValue("Printer/AutoPop", mPrinterAutoPop);
    s.setValue("Printer/FeedMode", mPrinterFeedMode);
    s.setValue("Printer/Style", mPrinterStyle);
    s.setValue("Printer/MarginTop", mPrinterMarginTop);
    s.setValue("Printer/MarginLeft", mPrinterMarginLeft);
    s.setValue("Printer/MarginLength", mPrinterMarginLength);

    s.endGroup();

    s.beginWriteArray("MountedImageSettings");
    for (int i = 0; i < 15; i++) {
        ImageSettings& is = mMountedImageSettings[i];
        s.setArrayIndex(i);
        s.setValue("FileName", is.fileName);
        s.setValue("IsWriteProtected", is.isWriteProtected);
        s.setValue("IsHappyMode", is.isHappyMode);
    }
    s.endArray();
}

void AspeQtSettings::loadSessionFromFile(const QString &fileName)
{
    QSettings s(fileName, QSettings::IniFormat);
    s.beginGroup("AspeQt");
    mBackend = s.value("Backend", 0).toInt();
    mAtariSioDriverName = s.value("AtariSioDriverName", AtariSioBackend::defaultPortName()).toString();
    mAtariSioHandshakingMethod = s.value("AtariSioHandshakingMethod", 0).toInt();
    mSerialPortName = s.value("SerialPortName", StandardSerialPortBackend::defaultPortName()).toString();
    mSerialPortHandshakingMethod = s.value("HandshakingMethod", 0).toInt();
    mSerialPortTriggerOnFallingEdge = s.value("FallingEdge", false).toBool();
    mSerialPortHardwareUart = s.value("SerialPortHardwareUart", false).toBool();
    mSerialPortWriteDelay = s.value("WriteDelay", 1).toInt();
    mSerialPortCompErrDelay = s.value("CompErrDelay", 1).toInt();
    mSerialPortMaximumSpeed = s.value("MaximumSerialPortSpeed", 2).toInt();
    mSerialPortUsePokeyDivisors = s.value("SerialPortUsePokeyDivisors", false).toBool();
    mSerialPortPokeyDivisor = s.value("SerialPortPokeyDivisor", 6).toInt();
    mUseHighSpeedExeLoader = s.value("UseHighSpeedExeLoader", false).toBool();
    mPrinterEmulation = s.value("PrinterEmulation", true).toBool();
    mCustomCasBaud = s.value("CustomCasBaud", 875).toInt();
    mUseCustomCasBaud = s.value("UseCustomCasBaud", false).toBool();
    mI18nLanguage = s.value("I18nLanguage").toString();
    msaveWindowsPos = s.value("SaveWindowsPosSize", true).toBool();
    msaveDiskVis = s.value("SaveDiskVisibility", true).toBool();
    mdVis = s.value("D9DOVisible", true).toBool();
    mMainX = s.value("MainX", 20).toInt();
    mMainY = s.value("MainY", 40).toInt();
    mMainW = s.value("MainW", 688).toInt();
    mMainH = s.value("MainH", 426).toInt();
    if (mMainW < 688 && mdVis) mMainW = 688;
    if (mMainH < 426 && mdVis) mMainH = 426;
    mMiniX = s.value("MiniX", 8).toInt();
    mMiniY = s.value("MiniY", 30).toInt();
    mPrtX = s.value("PrtX", 20).toInt();
    mPrtY = s.value("PrtY", 40).toInt();
    mPrtW = s.value("PrtW", 600).toInt();
    mPrtH = s.value("PrtH", 486).toInt();
    mFilterUnderscore = s.value("FilterUnderscore", true).toBool();
    mUseCapitalLettersInPCLINK = s.value("CapitalLettersInPCLINK", false).toBool();
    mUseURLSubmit = s.value("URLSubmit", false).toBool();
    mUseLargeFont = s.value("UseLargeFont", false).toBool();
    mExplorerOnTop = s.value("ExplorerOnTop", false).toBool();
    mEnableShade = s.value("EnableShadeByDefault", true).toBool();
    mRestoreTnfsLocation = s.value("RestoreTnfsLocation", true).toBool();
    mLastBootDos = s.value("LastBootDos", ":/boot_templates/$bootmyd").toString();
    mDisablePicoHiSpeed = s.value("DisablePicoHiSpeed", false).toBool();
    mTranslateEolOnPost = s.value("TranslateEolOnPost", true).toBool();
    mTranslateEolOnGet = s.value("TranslateEolOnGet", false).toBool();

    mVoiceVolume = mSettings->value("Voice/Volume", 10).toInt();
    mVoiceRate   = mSettings->value("Voice/Rate", 5).toInt();
    mVoicePitch  = mSettings->value("Voice/Pitch", 5).toInt();

    // --- Modem Bridge Matrix Loading ---
    mModemTransportMode = s.value("ModemBridge/TransportMode", 0).toInt();
    mModemBridgePhonebookPath = s.value("ModemBridge/PhonebookPath", "").toString();
    mShowRDeviceWarning = s.value("ShowRDeviceWarning", true).toBool();

#if defined(Q_OS_WIN)
    int defaultGuard = 50;
#elif defined(Q_OS_MAC)
    int defaultGuard = 20;
#else
    int defaultGuard = 10;
#endif

    for (int i = 0; i < 4; i++) {
        QString pfx = QString("ModemBridge/R%1/").arg(i + 1);
        mEnableRDevice[i] = s.value(pfx + "Enabled", (i == 0)).toBool();
        mModemBridgeLocalEcho[i] = s.value(pfx + "LocalEcho", false).toBool();
        mBbsListenerEnabled[i] = s.value(pfx + "BbsListenerEnabled", false).toBool();
        mModemListenPort[i] = s.value(pfx + "ListenPort", 2301 + i).toInt();
        mModemBridgePortName[i] = s.value(pfx + "PortName", "").toString();
        mModemBridgeBaudRate[i] = s.value(pfx + "BaudRate", 9600).toInt();
        mModemBridgeFlowControl[i] = s.value(pfx + "FlowControl", true).toBool();
        mInvertCtsLogic[i] = s.value(pfx + "InvertCts", true).toBool();
        mStreamGuardDelay[i] = s.value(pfx + "StreamGuardDelay", defaultGuard).toInt();
    }

    mWebUiEnabled = s.value("WebUI/Enabled", false).toBool();
    mWebUiPort = s.value("WebUI/HttpPort", 8080).toInt();
    mWebUiWsPort = s.value("WebUI/WsPort", 12345).toInt();
    mPrinterEmulation = s.value("PrinterEmulation", true).toBool();
    mPrinterAutoPop = s.value("Printer/AutoPop", false).toBool();
    mPrinterFeedMode = s.value("Printer/FeedMode", 0).toInt();
    mPrinterStyle = s.value("Printer/Style", 0).toInt();
    mPrinterMarginTop = s.value("Printer/MarginTop", 108).toInt();
    mPrinterMarginLeft = s.value("Printer/MarginLeft", 60).toInt();
    mPrinterMarginLength = s.value("Printer/MarginLength", 2376).toInt();

    s.endGroup();

    s.beginReadArray("MountedImageSettings");
    for (int i = 0; i < 15; i++) {
        s.setArrayIndex(i);
        setMountedImageSetting(i,
                               s.value("FileName", "").toString(),
                               s.value("IsWriteProtected", false).toBool(),
                               s.value("IsHappyMode", false).toBool());
    }
    s.endArray();
}

//printer options

bool AspeQtSettings::printerAutoPop() const { return mPrinterAutoPop; }
void AspeQtSettings::setPrinterAutoPop(bool autoPop) {
    mPrinterAutoPop = autoPop;
    if(mSessionFileName == "") mSettings->setValue("Printer/AutoPop", autoPop);
}

int AspeQtSettings::printerFeedMode() const { return mPrinterFeedMode; }
void AspeQtSettings::setPrinterFeedMode(int mode) {
    mPrinterFeedMode = mode;
    if(mSessionFileName == "") mSettings->setValue("Printer/FeedMode", mode);
}

int AspeQtSettings::printerStyle() const { return mPrinterStyle; }
void AspeQtSettings::setPrinterStyle(int style) {
    mPrinterStyle = style;
    if(mSessionFileName == "") mSettings->setValue("Printer/Style", style);
}

bool AspeQtSettings::isPrinterClearRequested() const { return mPrinterClearRequested; }
void AspeQtSettings::setPrinterClearRequested(bool req) { mPrinterClearRequested = req; }

int AspeQtSettings::printerMarginTop() const { return mPrinterMarginTop; }
void AspeQtSettings::setPrinterMarginTop(int margin) {
    mPrinterMarginTop = margin;
    if(mSessionFileName == "") mSettings->setValue("Printer/MarginTop", margin);
}

int AspeQtSettings::printerMarginLeft() const { return mPrinterMarginLeft; }
void AspeQtSettings::setPrinterMarginLeft(int margin) {
    mPrinterMarginLeft = margin;
    if(mSessionFileName == "") mSettings->setValue("Printer/MarginLeft", margin);
}

int AspeQtSettings::printerMarginLength() const { return mPrinterMarginLength; }
void AspeQtSettings::setPrinterMarginLength(int length) {
    mPrinterMarginLength = length;
    if(mSessionFileName == "") mSettings->setValue("Printer/MarginLength", length);
}

int AspeQtSettings::voiceVolume() { return mVoiceVolume; }
void AspeQtSettings::setVoiceVolume(int vol) {
    mVoiceVolume = vol;
    if(mSessionFileName == "") mSettings->setValue("Voice/Volume", vol);
}

int AspeQtSettings::voiceRate() { return mVoiceRate; }
void AspeQtSettings::setVoiceRate(int rate) {
    mVoiceRate = rate;
    if(mSessionFileName == "") mSettings->setValue("Voice/Rate", rate);
}

int AspeQtSettings::voicePitch() { return mVoicePitch; }
void AspeQtSettings::setVoicePitch(int pitch) {
    mVoicePitch = pitch;
    if(mSessionFileName == "") mSettings->setValue("Voice/Pitch", pitch);
}
