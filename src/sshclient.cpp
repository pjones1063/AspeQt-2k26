#include "sshclient.h"
#include <QDebug>

// ============================================================================
// SshBackend Implementation (Background Thread Logic)
// ============================================================================

SshBackend::SshBackend(QObject *parent)
    : QObject(parent), m_session(nullptr), m_channel(nullptr), m_isConnected(false), m_pollIntervalMs(10)
{
}

SshBackend::~SshBackend() {
    cleanup();
}

void SshBackend::cleanup() {
    m_isConnected = false;

    if (m_channel) {
        if (ssh_channel_is_open(m_channel)) ssh_channel_close(m_channel);
        ssh_channel_free(m_channel);
        m_channel = nullptr;
    }
    if (m_session) {
        ssh_disconnect(m_session);
        ssh_free(m_session);
        m_session = nullptr;
    }
}

void SshBackend::processConnection(const QString &host, int port, const QString &user, const QString &password) {
    // Ensure clean state before starting
    cleanup();

    m_session = ssh_new();
    if (!m_session) {
        emit errorOccurred("Internal Error: Failed to create SSH session structure.");
        return;
    }

    // Set SSH Options
    ssh_options_set(m_session, SSH_OPTIONS_HOST, host.toUtf8().constData());
    int portInt = port;
    ssh_options_set(m_session, SSH_OPTIONS_PORT, &portInt);

    if (!user.isEmpty()) {
        ssh_options_set(m_session, SSH_OPTIONS_USER, user.toUtf8().constData());
    }

    // 1. Connect (Blocking call, but safe here in background thread)
    int rc = ssh_connect(m_session);
    if (rc != SSH_OK) {
        emit errorOccurred(QString("Connection Error: %1").arg(ssh_get_error(m_session)));
        cleanup();
        return;
    }

    // 2. Authenticate (Password)
    // NOTE: This is a blocking call.
    rc = ssh_userauth_password(m_session, nullptr, password.toUtf8().constData());
    if (rc != SSH_AUTH_SUCCESS) {
        emit errorOccurred(QString("Authentication Failed: %1").arg(ssh_get_error(m_session)));
        cleanup();
        return;
    }

    // 3. Create Channel
    m_channel = ssh_channel_new(m_session);
    if (!m_channel) {
        emit errorOccurred("Error: Failed to create SSH channel.");
        cleanup();
        return;
    }

    rc = ssh_channel_open_session(m_channel);
    if (rc != SSH_OK) {
        emit errorOccurred("Error: Failed to open SSH session channel.");
        cleanup();
        return;
    }

    // 4. Request PTY (Pseudo Teletype)
    // Critical for BBS/Interactive use so the server sends us raw data/prompts
    rc = ssh_channel_request_pty(m_channel);
    if (rc != SSH_OK) {
        emit errorOccurred("Error: Failed to request PTY.");
        cleanup();
        return;
    }

    // Resize PTY to standard terminal (80x24)
    ssh_channel_change_pty_size(m_channel, 80, 24);

    // 5. Request Shell
    rc = ssh_channel_request_shell(m_channel);
    if (rc != SSH_OK) {
        emit errorOccurred("Error: Failed to request Shell.");
        cleanup();
        return;
    }

    // Success!
    m_isConnected = true;
    emit connected();

    // Start the non-blocking read loop
    QTimer::singleShot(m_pollIntervalMs, this, &SshBackend::pollLoop);
}

void SshBackend::processWrite(const QByteArray &data) {
    if (!m_isConnected || !m_channel) return;

    int rc = ssh_channel_write(m_channel, data.constData(), data.size());
    if (rc == SSH_ERROR) {
        emit errorOccurred(QString("Write Error: %1").arg(ssh_get_error(m_session)));
        processDisconnect();
    }
}

void SshBackend::processDisconnect() {
    if (m_isConnected) {
        cleanup();
        emit disconnected();
    }
}

void SshBackend::setPollingInterval(int ms) {
    m_pollIntervalMs = ms;
}

void SshBackend::pollLoop() {
    if (!m_isConnected || !m_channel) return;

    // 1. Check if the server closed the connection (EOF)
    if (ssh_channel_is_eof(m_channel)) {
        processDisconnect();
        return;
    }

    // 2. Non-blocking read
    // We try to read pending data.
    // is_stderr = 0 (read stdout)
    char buffer[4096];
    int nbytes = ssh_channel_read_nonblocking(m_channel, buffer, sizeof(buffer), 0);

    if (nbytes > 0) {
        // Data found! Send it up.
        emit dataReceived(QByteArray(buffer, nbytes));
    } else if (nbytes < 0) {
        // Error state
        // (Note: nbytes == 0 just means no data right now, which is fine)
        processDisconnect();
        return;
    }

    // 3. Re-schedule this slot to run again (Infinite Loop via Event Loop)
    if (m_isConnected) {
        QTimer::singleShot(m_pollIntervalMs, this, &SshBackend::pollLoop);
    }
}


// ============================================================================
// SshClient Implementation (Public Interface)
// ============================================================================

SshClient::SshClient(QObject *parent)
    : QObject(parent), m_backend(new SshBackend(nullptr)), m_connectedStatus(false)
{
    // Move the backend object to the background thread
    m_backend->moveToThread(&m_thread);

    // ---------------------------------------------------------
    // Signal Wiring (Main Thread -> Worker Thread)
    // ---------------------------------------------------------
    connect(this, &SshClient::_sigConnect, m_backend, &SshBackend::processConnection);
    connect(this, &SshClient::_sigWrite, m_backend, &SshBackend::processWrite);
    connect(this, &SshClient::_sigDisconnect, m_backend, &SshBackend::processDisconnect);

    // ---------------------------------------------------------
    // Signal Wiring (Worker Thread -> Main Thread)
    // ---------------------------------------------------------
    connect(m_backend, &SshBackend::connected, this, [this]() {
        m_connectedStatus = true;
        emit connected();
    });

    connect(m_backend, &SshBackend::disconnected, this, [this]() {
        m_connectedStatus = false;
        emit disconnected();
    });

    connect(m_backend, &SshBackend::errorOccurred, this, &SshClient::error);
    connect(m_backend, &SshBackend::dataReceived, this, &SshClient::rxData);

    // ---------------------------------------------------------
    // Thread Lifecycle
    // ---------------------------------------------------------

    // When the thread finishes, delete the backend object automatically
    connect(&m_thread, &QThread::finished, m_backend, &QObject::deleteLater);

    // Start the event loop for the worker
    m_thread.start();
}

SshClient::~SshClient() {
    // Graceful shutdown
    disconnectFromHost();
    m_thread.quit();
    m_thread.wait();
}

void SshClient::connectToHost(const QString &host, int port, const QString &user, const QString &password) {
    // Emit signal to trigger the worker in the other thread
    emit _sigConnect(host, port, user, password);
}

void SshClient::disconnectFromHost() {
    emit _sigDisconnect();
}

void SshClient::write(const QByteArray &data) {
    emit _sigWrite(data);
}

bool SshClient::isConnected() const {
    return m_connectedStatus;
}
