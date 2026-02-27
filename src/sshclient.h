#ifndef SSHCLIENT_H
#define SSHCLIENT_H

#include <QObject>
#include <QThread>
#include <QByteArray>
#include <QTimer>
#include <libssh/libssh.h>

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
    void processConnection(const QString &host, int port, const QString &user, const QString &password);
    void processWrite(const QByteArray &data);
    void processDisconnect();
    void setPollingInterval(int ms);

signals:
    // Signals sent back to the main thread
    void connected();
    void disconnected();
    void errorOccurred(const QString &msg);
    void dataReceived(const QByteArray &data);

private slots:
    void pollLoop(); // Non-blocking read loop

private:
    ssh_session m_session;
    ssh_channel m_channel;
    bool m_isConnected;
    int m_pollIntervalMs;

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
    void connectToHost(const QString &host, int port = 22, const QString &user = "", const QString &password = "");
    void disconnectFromHost();
    void write(const QByteArray &data);
    bool isConnected() const;

signals:
    // Signals for your GUI / ModemBridge
    void connected();
    void disconnected();
    void error(const QString &message);
    void rxData(const QByteArray &data);

private:
    // Internal Thread Management
    QThread m_thread;
    SshBackend *m_backend;
    bool m_connectedStatus;

signals:
    // Internal signals to bridge commands to the worker thread
    void _sigConnect(const QString &host, int port, const QString &user, const QString &password);
    void _sigWrite(const QByteArray &data);
    void _sigDisconnect();
};

#endif // SSHCLIENT_H
