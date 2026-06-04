#ifndef WEBBRIDGE_H
#define WEBBRIDGE_H

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QFutureWatcher>
#include "inetworkclient.h" // <--- NEW: Universal Interface

class MainWindow; // Forward declaration

class WebBridge : public QObject
{
    Q_OBJECT

public:
    explicit WebBridge(MainWindow *mainWin, QObject *parent = nullptr);

public slots:
    void mountDiskUi(int slot);
    void mountFolderUi(int slot);
    void ejectDiskUi(int slot);
    void saveDiskUi(int slot);
    void setHappyModeUi(int slot, bool enabled);
    void toggleWriteProtectUi(int slot, bool enabled);
    void requestFullStatus();
    void toggleAutoSaveUi(int slot);
    void requestPrinterImageUi();

    void requestDirectoryList(int slot, const QString &path);
    void mountFileSilentUi(int slot, const QString &filePath);
    void createBlankDiskUi(int slot, const QString &folder, const QString &filename, int type);
    void uploadAndMountUi(int slot, const QString &filename, const QString &base64Data);

    // --- TOOLBAR & MODEM SLOTS ---
    void toggleEmulationUi();
    void togglePrinterUi();
    void requestLogTextUi();
    void hangupUi();
    void macroUserUi();
    void macroPassUi();

    // --- PHONEBOOK SLOTS ---
    void requestPhonebookList();
    void dialBbsUi(const QString &name, const QString &ip, int port, const QString &protocol, const QString &login, const QString &password);

    // --- UNIVERSAL NETWORK SLOTS ---
    void requestNetworkDirectoryList(int slot, const QString &urlString); // <--- NEW: Accepts full URL
    void mountTnfsSilentUi(int slot, const QString &url);

    // --- CAS SLOTS ---
    void mountCasSilentUi(const QString &filePath);
    void playCasUi();
    void rewindCasUi();
    void ejectCasUi();

    void requestCurrentSavePathUi();
    void swapDiskUi(int slot);

private slots:
    void onNetBatchFetched(); // --- [NEW] Universal Async Handler ---

signals:
    void diskStatusChanged(int slot, const QString &filename, const QString &properties, const QString &fullPath, bool autoSave, bool happyMode, bool writeProtected);
    void driveEmpty(int slot);
    void directoryListReceived(int slot, const QString &currentPath, const QJsonArray &files);
    void phonebookListReceived(const QJsonArray &entries);
    void globalStatusChanged(bool emulationRunning, bool printerRunning);
    void logTextReceived(const QString &logText);

    // Kept original signal names so the JS frontend doesn't break
    void tnfsDirectoryListReceived(int slot, const QString &host, const QString &path, const QJsonArray &files, bool isFinished);
    void tnfsHostHistoryReceived(const QStringList &history);

    void casStatusChanged(QString filename, bool isPlaying);
    void notificationReceived(QString message, bool isError);
    void printerImageReceived(const QString &base64Data);
    void currentSavePathReceived(const QString &path);

private:
    MainWindow *mainWindow;

    // --- Universal Network Streaming State ---
    INetworkClient *m_netClient;
    int m_netSlot;
    QString m_netHost;
    QString m_netPath;

    // --- Async Background Threading ---
    void triggerNextNetBatch();
    QFutureWatcher<QList<INetworkClient::DirectoryEntry>> *m_netWatcher;
};

#endif // WEBBRIDGE_H
