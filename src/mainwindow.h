/*
 * mainwindow.h
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QtWidgets/QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QtDebug>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QTranslator>
#include <QSystemTrayIcon>
#include <QTextEdit>
#include <QSlider>
#include <QEnterEvent>
#include <QProgressBar>
#include <QWebSocketServer>
#include <QWebChannel>
#include <QHttpServer>

#include "modembridge.h"
#include "optionsdialog.h"
#include "aboutdialog.h"
#include "createimagedialog.h"
#include "diskeditdialog.h"
#include "serialport.h"
#include "sioworker.h"
#include "textprinterwindow.h"
#include "docdisplaywindow.h"
#include "drivewidget.h"
#include "infowidget.h"
#include "tnfsclient.h"
#include "miscdevices.h"
#include "phonedirectory.h"
#include "bbsdata.h"
#include "aspeqtclientdevice.h"



class WebBridge;
class WebSocketClientWrapper;

namespace Ui
{
    class MainWindow;
}
class DiskWidget
{
public:
    QLabel *fileNameLabel;
    QLabel *imagePropertiesLabel;
    QAction *saveAction;
    QAction *autoSaveAction;        //
    QAction *bootOptionAction;      //
    QAction *saveAsAction;
    QAction *revertAction;
    QAction *mountDiskAction;
    QAction *mountFolderAction;
    QAction *ejectAction;
    QAction *writeProtectAction;
    QAction *editAction;
    QFrame *frame;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QString g_sessionFile;
    QString g_sessionFilePath;
    QString g_mainWindowTitle;
    QWebSocketServer *webSocketServer = nullptr;
    WebSocketClientWrapper *clientWrapper = nullptr;
    QWebChannel *webChannel = nullptr;
    WebBridge *webBridge = nullptr;
    QTcpServer *httpTcpServer = nullptr;
    QString getPrinterText();

public slots:
    void show();
    int  firstEmptyDiskSlot(int startFrom = 0, bool createOne = true);
    void mountFileWithDefaultProtection(int no, const QString &fileName);
    void autoCommit(int no, bool st);
    void openRecent();
    void bootExeTriggered(const QString &fileName);
    void bootCasTriggered(const QString &fileName);
    void toggleWriteProtectHeadless(int no, bool enabled);
    void printServer(bool on);
    void updateDownloadProgress(qint64 bytesRead, qint64 totalBytes);
    void onFireAndForget(QString url, QByteArray data);
    void on_actionPhonebook_triggered();
    void refreshWebUi();
    void mountFileHeadless(int no, const QString &fileName);
    void ejectHeadless(int no);
    void toggleAutoSaveHeadless(int no);
    void toggleEmulationHeadless();
    void togglePrinterHeadless();
    void mountTnfsHeadless(int no, const QString &url);
    void createBlankDiskHeadless(int slot, const QString &folder, const QString &fileName, int type);
    void startWebUi();
    void stopWebUi();

private:
    int untitledName;
    Ui::MainWindow *ui;
    SioWorker *sio;
    bool shownFirstTime;
    DriveWidget* diskWidgets[DISK_COUNT];
    volatile int m_slotDownloadId[DISK_COUNT];
    int m_downloadCounter;
    InfoWidget* infoWidget;
    QLabel *speedLabel;
    TextPrinterWindow *textPrinterWindow;
    DocDisplayWindow *docDisplayWindow;
    QTranslator aspeqt_translator, aspeqt_qt_translator;
    QSystemTrayIcon trayIcon;
    Qt::WindowFlags oldWindowFlags;
    Qt::WindowStates oldWindowStates;
    QString lastMessage;
    int lastMessageRepeat;
    TnfsClient *tnfsClient;
    bool isClosing;

    QDialog *logWindow_;
    QList<QAction*> recentFilesActions_;
    QSlider *opacitySlider;
    QPoint m_dragPosition;
    QProgressBar *dlProgressBar;

    ModemBridge *modemBridge;

    QToolButton *btnMacroUser;   // Auto-User (ESC-U)
    QToolButton *btnMacroPass;   // Auto-Pass (ESC-P)
    QToolButton *btnHangup;      // Hangup Command
    QLabel *ledRx;               // Green "Download" LED
    QLabel *ledTx;               // Red "Upload" LED
    QTimer *ledResetTimer;       // Turns LEDs off after 50m
    QToolButton *btnSioTrace;    // SIO Tracer
    QToolButton *btnDisasmToggle; // dis-asm

    QHttpServer *httpServer = nullptr;

    // --- Headless Cassette Deck ---
    CassetteWorker *m_casWorker;
    QTimer *m_casTimer;
    QString m_casFileName;
    bool m_casIsPlaying;


    void setSession();  //
    void updateRecentFileActions();
    int containingDiskSlot(const QPoint &point);
    void bootExe(const QString &fileName);
    void mountFile(int no, const QString &fileName, bool prot);
    void mountDiskImage(int no);
    void mountFolderImage(int no);
    bool ejectImage(int no, bool ask = true);
    void toggleWriteProtection(int no, bool protectionEnabled);
    void openEditor(int no);
    void saveDisk(int no);
    void saveDiskAs(int no);
    void revertDisk(int no);
    QMessageBox::StandardButton saveImageWhenClosing(int no, QMessageBox::StandardButton previousAnswer, int number);
    void loadTranslators();
    void autoSaveDisk(int no);                                              //
    void setUpPrinterEmulationWidgets(bool enabled);
    void updatePhonebookMenuState();
    void openResourceHtml(const QString &resourcePath);
    void createDeviceWidgets();

protected:
    void mousePressEvent(QMouseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *);
    void dropEvent(QDropEvent *event);
    void closeEvent(QCloseEvent *event);
    void hideEvent(QHideEvent *event);
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *);
    void resizeEvent(QResizeEvent *);
    bool eventFilter(QObject *obj, QEvent *event);
    //void leaveEvent(QEvent *event) override;

signals:
    void logMessage(int type, const QString &msg);
    void newSlot (int slot);
    void fileMounted(bool mounted);
    void sendLogText (QString logText);
    void sendLogTextChange (QString logTextChange);
    void setFont(const QFont &font);

public:
    void doLogMessage(int type, const QString &msg);
    QString getLogText();

private slots:
    void on_actionPlaybackCassette_triggered();
    void on_actionShowPrinterTextOutput_triggered();
    void on_actionBootExe_triggered();
    void on_actionSaveSession_triggered();
    void on_actionOpenSession_triggered();
    void on_actionNewImage_triggered();
    void on_actionMountFolder_triggered();
    void on_actionMountDisk_triggered();
    void on_actionEjectAll_triggered();
    void on_actionOptions_triggered();
    void on_actionStartEmulation_triggered();
    void on_actionPrinterEmulation_triggered();
    void on_actionHideShowDrives_triggered();
    void on_actionQuit_triggered();
    void on_actionAbout_triggered();
    void on_actionDocumentation_triggered();
    void on_actionMountTnfs_triggered(int deviceId);

    // Device widget events
    void on_actionMountDisk_triggered(int deviceId);
    void on_actionMountFolder_triggered(int deviceId);
    void on_actionEject_triggered(int deviceId);
    void on_actionWriteProtect_triggered(int deviceId, bool writeProtectEnabled);
    void on_actionMountRecent_triggered(const QString &fileName);
    void on_actionEditDisk_triggered(int deviceId);
    void on_actionSave_triggered(int deviceId);
    void on_actionAutoSave_triggered(int deviceId);
    void on_actionSaveAs_triggered(int deviceId);
    void on_actionRevert_triggered(int deviceId);
    void on_actionInspectSectors_triggered(int deviceId);
    void on_actionInfo_triggered(int deviceId);

    void on_actionBootOption_triggered();
    void on_actionToggleMiniMode_triggered();
    void on_actionToggleShade_triggered();
    void on_actionLogWindow_triggered();

    void showHideDrives();
    void sioFinished();
    void sioStarted();
    void sioStatusChanged(QString status);
    void textPrinterWindowClosed();
    void docDisplayWindowClosed();
    void deviceStatusChanged(int deviceNo);
    void uiMessage(int t, const QString message);
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void keepBootExeOpen();
    void saveWindowGeometry();
    void saveMiniWindowGeometry();
    void logChanged(QString text);
    void changeFonts();
    void on_actionHappyMode_triggered(int deviceId, bool enabled);

    void blinkRx();
    void blinkTx();
    void resetLeds();

    void mountCasHeadless(const QString &fileName);
    void playCasHeadless();
    void rewindCasHeadless();
    void ejectCasHeadless();
    void updateCasProgress();

    void onSioTraceToggleClicked();
    void onDisasmToggleClicked();
    void onSioTraceData(const QString &dir, const QByteArray &data);
    void hangupModem();
    void sendMacroUser();
    void sendMacroPass();
    void dialBbsSilent(const QString &name, const QString &ip, int port, const QString &protocol, const QString &login, const QString &password);

};

#endif // MAINWINDOW_H
