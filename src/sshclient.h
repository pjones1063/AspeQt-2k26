#ifndef SSHCLIENT_H
#define SSHCLIENT_H

#include <QObject>
#include <QThread>
#include <QByteArray>
#include <QTimer>
#include <libssh/libssh.h>
#include <libssh/sftp.h>

enum SshMode {
    ModeTerminal,
    ModeSftp
};

enum SftpAction {
    ActionMkdir,
    ActionRmdir,
    ActionDelete
};

// ============================================================================
// Internal Worker Class (Runs in background thread)
// ============================================================================
class SshBackend : public QObject {
    Q_OBJECT

public:
    explicit SshBackend(QObject *parent = nullptr);
    ~SshBackend();

public slots:
    // Actions triggered by the main thread
    void processConnection(const QString &host, int port, const QString &user, const QString &password, const QString &privateKeyPath, SshMode mode);
    void processWrite(const QByteArray &data);
    void processDisconnect();
    void setPollingInterval(int ms);
    void processSftpRequest(const QString &path, bool isDirectory);
    void processSftpAction(const QString &path, SftpAction action);
    void processSftpWrite(const QString &path, const QByteArray &data);
    void processSftpRename(const QString &oldPath, const QString &newPath);

signals:
    // Signals sent back to the main thread
    void connected();
    void disconnected();
    void errorOccurred(const QString &msg);
    void dataReceived(const QByteArray &data);
    void sftpTransferFinished();
    void sftpActionFinished(bool success, const QString &errorMsg);

private slots:
    void pollLoop(); // Non-blocking read loop

private:
    ssh_session m_session;
    ssh_channel m_channel;
    bool m_isConnected;
    int m_pollIntervalMs;
    sftp_session m_sftp;
    SshMode m_currentMode;

    // Helper to clean up libssh structs
    void cleanup();
};

// ============================================================================
// Public Interface Class (The "Wrapper" you use in ModemBridge)
// ============================================================================
class SshClient : public QObject {
    Q_OBJECT

public:
    explicit SshClient(QObject *parent = nullptr);
    ~SshClient();

    // -- Public API --
    void connectToHost(const QString &host, int port = 22, const QString &user = "", const QString &password = "", const QString &privateKeyPath = "", SshMode mode = ModeTerminal);
    void requestSftp(const QString &path, bool isDirectory);
    void requestSftpAction(const QString &path, SftpAction action);
    void requestSftpWrite(const QString &path, const QByteArray &data);
    void requestSftpRename(const QString &oldPath, const QString &newPath); // <-- MOVED HERE (Correct Spot)
    void disconnectFromHost();
    void write(const QByteArray &data);
    bool isConnected() const;

signals:
    // Signals for your GUI / ModemBridge
    void connected();
    void disconnected();
    void error(const QString &message);
    void rxData(const QByteArray &data);
    void sftpFinished();
    void sftpActionFinished(bool success, const QString &errorMsg);

private:
    // Internal Thread Management
    QThread m_thread;
    SshBackend *m_backend;
    bool m_connectedStatus;

signals:
    void _sigConnect(const QString &host, int port, const QString &user, const QString &password, const QString &privateKeyPath, SshMode mode);
    void _sigSftpRequest(const QString &path, bool isDirectory);
    void _sigSftpAction(const QString &path, SftpAction action);
    void _sigSftpWrite(const QString &path, const QByteArray &data);
    void _sigSftpRename(const QString &oldPath, const QString &newPath);
    void _sigWrite(const QByteArray &data);
    void _sigDisconnect();
};

#endif // SSHCLIENT_H