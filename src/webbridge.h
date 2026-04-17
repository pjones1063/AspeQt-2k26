#ifndef WEBBRIDGE_H
#define WEBBRIDGE_H

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>
#include "tnfsclient.h"

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

    // --- TNFS SLOTS ---
    void requestTnfsDirectoryList(int slot, const QString &host, const QString &path);
    void mountTnfsSilentUi(int slot, const QString &url);

    // --- CAS SLOTS ---
    void mountCasSilentUi(const QString &filePath);
    void playCasUi();
    void rewindCasUi();
    void ejectCasUi();

    void requestCurrentSavePathUi(); // NEW

private slots:
    void fetchNextTnfsBatch(); // Fired by the QTimer

signals:
    // ADDED FULLPATH TO THIS SIGNAL
    void diskStatusChanged(int slot, const QString &filename, const QString &properties, const QString &fullPath, bool autoSave, bool happyMode, bool writeProtected);

    void driveEmpty(int slot);
    void directoryListReceived(int slot, const QString &currentPath, const QJsonArray &files);
    void phonebookListReceived(const QJsonArray &entries);
    void globalStatusChanged(bool emulationRunning, bool printerRunning);
    void logTextReceived(const QString &logText);
    void tnfsDirectoryListReceived(int slot, const QString &host, const QString &path, const QJsonArray &files, bool isFinished);
    void tnfsHostHistoryReceived(const QStringList &history);
    void casStatusChanged(QString filename, bool isPlaying);
    void notificationReceived(QString message, bool isError);
    void printerImageReceived(const QString &base64Data);
    void currentSavePathReceived(const QString &path); // NEW

private:
    MainWindow *mainWindow;

    // TNFS Streaming State
    TnfsClient *m_tnfsClient;
    QTimer *m_tnfsTimer;
    int m_tnfsSlot;
    QString m_tnfsHost;
    QString m_tnfsPath;
};

#endif // WEBBRIDGE_H
