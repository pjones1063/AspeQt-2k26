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

void SshBackend::processConnection(const QString &host, int port, const QString &user, const QString &password, const QString &privateKeyPath) {
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


    // ---------------------------------------------------------
    // CRITICAL FIX: The Ultimate Retro-SSH Compatibility Block
    // ---------------------------------------------------------

    // 1. KEX: Bypass buggy mlkem, add legacy DH Exchange
    const char* safe_kex = "curve25519-sha256,curve25519-sha256@libssh.org,"
                           "ecdh-sha2-nistp256,ecdh-sha2-nistp384,ecdh-sha2-nistp521,"
                           "diffie-hellman-group18-sha512,diffie-hellman-group16-sha512,"
                           "diffie-hellman-group14-sha256,diffie-hellman-group14-sha1,"
                           "diffie-hellman-group-exchange-sha1,"
                           "diffie-hellman-group1-sha1";
    ssh_options_set(m_session, SSH_OPTIONS_KEY_EXCHANGE, safe_kex);

    // 2. HOST KEYS: Add DSA (ssh-dss)
    const char* safe_hostkeys = "ssh-ed25519,ecdsa-sha2-nistp256,ecdsa-sha2-nistp384,"
                                "ecdsa-sha2-nistp521,rsa-sha2-512,rsa-sha2-256,ssh-rsa,"
                                "ssh-dss";
    ssh_options_set(m_session, SSH_OPTIONS_HOSTKEYS, safe_hostkeys);

    // 3. CIPHERS: Add Blowfish, Cast128, and Arcfour (RC4)
    const char* legacy_ciphers = "aes256-gcm@openssh.com,aes128-gcm@openssh.com,"
                                 "aes256-ctr,aes192-ctr,aes128-ctr,"
                                 "aes256-cbc,aes192-cbc,aes128-cbc,3des-cbc,"
                                 "blowfish-cbc,cast128-cbc,arcfour256,arcfour128,arcfour";
    ssh_options_set(m_session, SSH_OPTIONS_CIPHERS_C_S, legacy_ciphers);
    ssh_options_set(m_session, SSH_OPTIONS_CIPHERS_S_C, legacy_ciphers);

    // 4. MACs: Add Truncated SHA1/MD5
    const char* legacy_macs = "hmac-sha2-512,hmac-sha2-256,hmac-sha1,hmac-md5,"
                              "hmac-sha1-96,hmac-md5-96";
    ssh_options_set(m_session, SSH_OPTIONS_HMAC_C_S, legacy_macs);
    ssh_options_set(m_session, SSH_OPTIONS_HMAC_S_C, legacy_macs);

    // ---------------------------------------------------------


    // Connect to Server
    int rc = ssh_connect(m_session);
    if (rc != SSH_OK) {
        emit errorOccurred(QString("SSH Connection Failed: %1").arg(ssh_get_error(m_session)));
        cleanup();
        return;
    }

    // --- AUTHENTICATION ROUTING ---
    if (!privateKeyPath.isEmpty()) {
        // 1. PUBLIC KEY AUTHENTICATION
        ssh_key privkey = nullptr;
        const char* passphrase = password.isEmpty() ? nullptr : password.toUtf8().constData();

        // Attempt to load the private key file
        int import_rc = ssh_pki_import_privkey_file(privateKeyPath.toUtf8().constData(), passphrase, nullptr, nullptr, &privkey);

        if (import_rc != SSH_OK) {
            emit errorOccurred(QString("SSH Key Error: Could not load private key from %1. Check path or passphrase.").arg(privateKeyPath));
            cleanup();
            return;
        }

        // Authenticate using the loaded key
        rc = ssh_userauth_publickey(m_session, nullptr, privkey);
        ssh_key_free(privkey); // Free the key memory immediately after attempt

        if (rc != SSH_AUTH_SUCCESS) {
            emit errorOccurred(QString("SSH Key Auth Failed: %1").arg(ssh_get_error(m_session)));
            cleanup();
            return;
        }

    } else {
        // 2. STANDARD PASSWORD AUTHENTICATION
        rc = ssh_userauth_password(m_session, nullptr, password.toUtf8().constData());
        if (rc != SSH_AUTH_SUCCESS) {
            emit errorOccurred(QString("SSH Password Auth Failed: %1").arg(ssh_get_error(m_session)));
            cleanup();
            return;
        }
    }

    // Open a Channel
    m_channel = ssh_channel_new(m_session);
    if (!m_channel) {
        emit errorOccurred("SSH Channel Error: Failed to create channel.");
        cleanup();
        return;
    }

    rc = ssh_channel_open_session(m_channel);
    if (rc != SSH_OK) {
        emit errorOccurred(QString("SSH Session Error: %1").arg(ssh_get_error(m_session)));
        cleanup();
        return;
    }

    // Request a PTY (Terminal)
    rc = ssh_channel_request_pty(m_channel);
    if (rc != SSH_OK) {
        emit errorOccurred(QString("SSH PTY Error: %1").arg(ssh_get_error(m_session)));
        cleanup();
        return;
    }

    // Request a Shell
    rc = ssh_channel_request_shell(m_channel);
    if (rc != SSH_OK) {
        emit errorOccurred(QString("SSH Shell Error: %1").arg(ssh_get_error(m_session)));
        cleanup();
        return;
    }

    // Success!
    m_isConnected = true;
    emit connected();

    // Start polling loop
    QTimer::singleShot(0, this, &SshBackend::pollLoop);
}

void SshBackend::processWrite(const QByteArray &data) {
    if (!m_isConnected || !m_channel) return;

    int written = ssh_channel_write(m_channel, data.constData(), data.size());
    if (written < 0) {
        emit errorOccurred(QString("SSH Write Error: %1").arg(ssh_get_error(m_session)));
        cleanup();
        emit disconnected();
    }
}

void SshBackend::processDisconnect() {
    cleanup();
    emit disconnected();
}

void SshBackend::setPollingInterval(int ms) {
    m_pollIntervalMs = ms;
}

void SshBackend::pollLoop() {
    if (!m_isConnected || !m_channel) return;

    // Check if the channel is still open
    if (ssh_channel_is_eof(m_channel) || !ssh_channel_is_open(m_channel)) {
        cleanup();
        emit disconnected();
        return;
    }

    // Non-blocking read
    char buffer[2048];
    int nbytes = ssh_channel_read_nonblocking(m_channel, buffer, sizeof(buffer), 0);

    if (nbytes > 0) {
        emit dataReceived(QByteArray(buffer, nbytes));
        // Keep reading immediately if there's data
        QTimer::singleShot(0, this, &SshBackend::pollLoop);
    } else if (nbytes < 0) {
        emit errorOccurred(QString("SSH Read Error: %1").arg(ssh_get_error(m_session)));
        cleanup();
        emit disconnected();
    } else {
        // No data available right now, poll again later
        QTimer::singleShot(m_pollIntervalMs, this, &SshBackend::pollLoop);
    }
}

// ============================================================================
// SshClient Implementation (Main Thread Wrapper)
// ============================================================================

SshClient::SshClient(QObject *parent) : QObject(parent), m_connectedStatus(false) {

    ssh_init();

    m_backend = new SshBackend();
    m_backend->moveToThread(&m_thread);

    // ---------------------------------------------------------
    // Signal Wiring (Main Thread -> Worker Thread)
    // ---------------------------------------------------------
    connect(this, &SshClient::_sigConnect, m_backend, &SshBackend::processConnection);
    connect(this, &SshClient::_sigDisconnect, m_backend, &SshBackend::processDisconnect);
    connect(this, &SshClient::_sigWrite, m_backend, &SshBackend::processWrite);

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
    connect(&m_thread, &QThread::finished, m_backend, &QObject::deleteLater);

    // Start the event loop for the worker
    m_thread.start();
}

SshClient::~SshClient() {
    disconnectFromHost();
    m_thread.quit();
    m_thread.wait();
    ssh_finalize();
}

void SshClient::connectToHost(const QString &host, int port, const QString &user, const QString &password, const QString &privateKeyPath) {
    emit _sigConnect(host, port, user, password, privateKeyPath);
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
