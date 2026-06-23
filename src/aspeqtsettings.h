/*
 * aspeqtsettings.h
 */

#ifndef ASPEQTSETTINGS_H
#define ASPEQTSETTINGS_H

#include <QSettings>

#define NUM_RECENT_FILES 10

class AspeQtSettings
{
public:
    class ImageSettings {
    public:
        QString fileName;
        bool isWriteProtected;
        bool isHappyMode;
    };

    AspeQtSettings();
    ~AspeQtSettings();

    // --- 1. General & UI Settings ---
    bool isFirstTime();
    QString i18nLanguage();
    void setI18nLanguage(const QString &lang);
    bool minimizeToTray();
    void setMinimizeToTray(bool tray);
    bool useLargeFont();
    void setUseLargeFont(bool largeFont);
    bool explorerOnTop();
    void setExplorerOnTop(bool expOnTop);
    bool enableShade();
    void setEnableShade(bool shade);
    int shadeOpacity();
    void setShadeOpacity(int val);
    bool saveWindowsPos();
    void setsaveWindowsPos(bool saveMwp);
    bool saveDiskVis();
    void setsaveDiskVis(bool saveDvis);
    bool D9DOVisible();
    void setD9DOVisible(bool dVis);
    bool filterUnderscore();
    void setfilterUnderscore(bool filter);
    bool capitalLettersInPCLINK();
    void setCapitalLettersInPCLINK(bool caps);

    // Window Positions
    int lastVerticalPos();
    void setLastVerticalPos(int lastVpos);
    int lastHorizontalPos();
    void setLastHorizontalPos(int lastHpos);
    int lastWidth();
    void setLastWidth(int lastW);
    int lastHeight();
    void setLastHeight(int lastH);
    int lastMiniVerticalPos();
    void setLastMiniVerticalPos(int lastMVpos);
    int lastMiniHorizontalPos();
    void setLastMiniHorizontalPos(int lastMHpos);
    int lastPrtVerticalPos();
    void setLastPrtVerticalPos(int lastVpos);
    int lastPrtHorizontalPos();
    void setLastPrtHorizontalPos(int lastHpos);
    int lastPrtWidth();
    void setLastPrtWidth(int lastW);
    int lastPrtHeight();
    void setLastPrtHeight(int lastH);

    // --- 2. Standard Serial Port Backend ---
    QString serialPortName();
    void setSerialPortName(const QString &name);
    int serialPortHandshakingMethod();
    void setSerialPortHandshakingMethod(int method);
    bool serialPortTriggerOnFallingEdge();
    void setSerialPortTriggerOnFallingEdge(bool use);
    bool serialPortHardwareUart();
    void setSerialPortHardwareUart(bool enable);
    int serialPortMaximumSpeed();
    void setSerialPortMaximumSpeed(int speed);
    bool serialPortUsePokeyDivisors();
    void setSerialPortUsePokeyDivisors(bool use);
    int serialPortPokeyDivisor();
    void setSerialPortPokeyDivisor(int divisor);
    int serialPortWriteDelay();
    void setSerialPortWriteDelay(int delay);
    int serialPortCompErrDelay();
    void setSerialPortCompErrDelay(int delay);

    // --- 3. Emulation & Virtual Devices ---
    int backend();
    void setBackend(int backend);
    QString atariSioDriverName();
    void setAtariSioDriverName(const QString &name);
    int atariSioHandshakingMethod();
    void setAtariSioHandshakingMethod(int method);
    bool useHighSpeedExeLoader();
    void setUseHighSpeedExeLoader(bool use);
    bool disablePicoHiSpeed();
    void setDisablePicoHiSpeed(bool disable);
    bool printerEmulation();
    void setPrinterEmulation(bool status);
    bool useCustomCasBaud();
    void setUseCustomCasBaud(bool use);
    int customCasBaud();
    void setCustomCasBaud(int baud);
    QString lastBootDos();
    void setLastBootDos(const QString &dos);


    // --- 4. Modem Bridge, RDevice & BBS Listener ---
    int modemTransportMode(); // 0 = Emulation, 1 = Bridge
    void setModemTransportMode(int mode);
    QString modemBridgePhonebookPath();
    void setModemBridgePhonebookPath(const QString &path);
    void setEnableRDevice(int port, bool enabled);
    bool modemBridgeLocalEcho(int port);
    void setModemBridgeLocalEcho(int port, bool enabled);
    bool bbsListenerEnabled(int port);
    void setBbsListenerEnabled(int port, bool enable);
    int modemListenPort(int port);
    void setModemListenPort(int port, int listenPort);
    QString modemBridgePortName(int port);
    void setModemBridgePortName(int port, const QString &name);
    int modemBridgeBaudRate(int port);
    void setModemBridgeBaudRate(int port, int baud);
    bool modemBridgeFlowControl(int port);
    void setModemBridgeFlowControl(int port, bool enabled);
    bool invertCtsLogic(int port);
    void setInvertCtsLogic(int port, bool invert);
    int streamGuardDelay(int port);
    void setStreamGuardDelay(int port, int delay);
    bool showRDeviceWarning();
    void setShowRDeviceWarning(bool show);


    // --- 5. TNFS & Web UI ---
    bool isWebUiEnabled();
    void setWebUiEnabled(bool enabled);
    int webUiPort();
    void setWebUiPort(int port);
    int webUiWsPort();
    void setWebUiWsPort(int port);
    bool restoreTnfsLocation();
    void setRestoreTnfsLocation(bool enabled);
    bool translateEolOnPost();
    void setTranslateEolOnPost(bool enabled);
    bool translateEolOnGet();
    void setTranslateEolOnGet(bool enabled);
    bool isURLSubmitEnabled();
    void setURLSubmit(bool enabled);

    // -- 6. Printer
    bool printerAutoPop() const;
    void setPrinterAutoPop(bool autoPop);
    int printerFeedMode() const;
    void setPrinterFeedMode(int mode);
    int printerStyle() const;
    void setPrinterStyle(int style);
    bool isPrinterClearRequested() const;
    void setPrinterClearRequested(bool req);
    int printerMarginTop() const;
    void setPrinterMarginTop(int margin);
    int printerMarginLeft() const;
    void setPrinterMarginLeft(int margin);
    int printerMarginLength() const;
    void setPrinterMarginLength(int length);


    // --- 7. Image & File Management ---
    const ImageSettings* getImageSettingsFromName(const QString &fileName);
    const ImageSettings& mountedImageSetting(int no);
    void setMountedImageSetting(int no, const QString &fileName, bool prot, bool happy = false);
    void setMountedImageProtection(int no, bool prot);
    const ImageSettings& recentImageSetting(int no);
    void mountImage(int no, const QString &fileName, bool prot);
    void unmountImage(int no);
    void swapImages(int no1, int no2);

    QString lastDiskImageDir();
    void setLastDiskImageDir(const QString &dir);
    QString lastFolderImageDir();
    void setLastFolderImageDir(const QString &dir);
    QString lastSessionDir();
    void setLastSessionDir(const QString &dir);
    QString lastExeDir();
    void setLastExeDir(const QString &dir);
    QString lastExtractDir();
    void setLastExtractDir(const QString &dir);
    QString lastPrinterTextDir();
    void setLastPrinterTextDir(const QString &dir);
    QString lastCasDir();
    void setLastCasDir(const QString &dir);
    QString lastRclDir();
    void setLastRclDir(const QString &dir);

    // --- 8. Session Management ---
    void setSessionFile(const QString &g_sessionFile, const QString &g_sessionFilePath);
    void saveSessionToFile(const QString &fileName);
    void loadSessionFromFile(const QString &fileName);
    void setMainWindowTitle(const QString &g_mainWindowTitle);

    // --- 9. Voice Synthesizer (A:) ---
    int voiceVolume();
    void setVoiceVolume(int vol);
    int voiceRate();
    void setVoiceRate(int rate);
    int voicePitch();
    void setVoicePitch(int pitch);


private:
    QSettings *mSettings;
    void writeRecentImageSettings();

    // 1. General / UI Data
    bool mIsFirstTime;
    QString mI18nLanguage;
    bool mMinimizeToTray;
    bool mUseLargeFont;
    bool mExplorerOnTop;
    bool mEnableShade;
    bool msaveWindowsPos;
    bool msaveDiskVis;
    bool mdVis;
    bool mFilterUnderscore;
    bool mUseCapitalLettersInPCLINK;
    int mMainX, mMainY, mMainW, mMainH;
    int mMiniX, mMiniY;
    int mPrtX, mPrtY, mPrtW, mPrtH;

    // 2. Standard Serial Port Data
    QString mSerialPortName;
    int mSerialPortHandshakingMethod;
    bool mSerialPortTriggerOnFallingEdge;
    bool mSerialPortHardwareUart;
    int mSerialPortWriteDelay;
    int mSerialPortCompErrDelay;
    int mSerialPortMaximumSpeed;
    bool mSerialPortUsePokeyDivisors;
    int mSerialPortPokeyDivisor;

    // 3. Emulation / Virtual Devices Data
    int mBackend;
    QString mAtariSioDriverName;
    int mAtariSioHandshakingMethod;
    bool mUseHighSpeedExeLoader;
    bool mDisablePicoHiSpeed;
    bool mPrinterEmulation;
    bool mUseCustomCasBaud;
    int mCustomCasBaud;
    QString mLastBootDos;

    // 4. Modem Bridge, RDevice & BBS Listener Data
    int mModemTransportMode;
    QString mModemBridgePhonebookPath;
    bool mShowRDeviceWarning;

    bool mEnableRDevice[4];
    bool mModemBridgeLocalEcho[4];
    bool mBbsListenerEnabled[4];
    int mModemListenPort[4];
    QString mModemBridgePortName[4];
    int mModemBridgeBaudRate[4];
    bool mModemBridgeFlowControl[4];
    bool mInvertCtsLogic[4];
    int mStreamGuardDelay[4];


    // 5. TNFS & Web UI Data
    bool mWebUiEnabled;
    int mWebUiPort;
    int mWebUiWsPort;
    bool mRestoreTnfsLocation;
    bool mTranslateEolOnPost;
    bool mTranslateEolOnGet;
    bool mUseURLSubmit;

    // 6 - Printer options
    bool mPrinterAutoPop;
    int mPrinterFeedMode;
    int mPrinterStyle;
    bool mPrinterClearRequested;
    int mPrinterMarginTop;
    int mPrinterMarginLeft;
    int mPrinterMarginLength;

    // 7. Image / File Data
    ImageSettings mMountedImageSettings[16];
    ImageSettings mRecentImageSettings[NUM_RECENT_FILES];
    QString mLastDiskImageDir;
    QString mLastFolderImageDir;
    QString mLastSessionDir;
    QString mLastExeDir;
    QString mLastExtractDir;
    QString mLastPrinterTextDir;
    QString mLastCasDir;

    // 8. Session Data
    QString mSessionFileName;
    QString mSessionFilePath;
    QString mMainWindowTitle;

    // --- 9. Voice Synthesizer (A:) ---
    int mVoiceVolume;
    int mVoiceRate;
    int mVoicePitch;

};

extern AspeQtSettings *aspeqtSettings;

#endif // ASPEQTSETTINGS_H
