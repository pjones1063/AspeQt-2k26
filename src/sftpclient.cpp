#include "sftpclient.h"
#include <QEventLoop>
#include <QTimer>
#include <QDebug>

SftpClient::SftpClient(QObject *parent) : INetworkClient(parent)
{
    m_sshClient = new SshClient(this);
    m_listingFinished = true;

    // Pipe incoming payload data from the background thread into our accumulator
    connect(m_sshClient, &SshClient::rxData, this, &SftpClient::onRxData);
}

SftpClient::~SftpClient()
{
    if (m_sshClient->isConnected()) {
        m_sshClient->disconnectFromHost();
    }
    // m_sshClient is parented to 'this', so it will automatically be deleted.
}

void SftpClient::setCredentials(const QString &user, const QString &pass)
{
    m_user = user;
    m_pass = pass;
}

void SftpClient::onRxData(const QByteArray &data)
{
    m_networkBuffer.append(data);
}

// ============================================================================
// CONNECTION ENGINE (Bridging Async to Sync)
// ============================================================================

bool SftpClient::connectToHost(const QString &host, quint16 port)
{
    if (port == 0) port = 22; // Default SSH/SFTP Port

    bool success = false;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    // Create temporary connections to control the event loop
    auto connConn = connect(m_sshClient, &SshClient::connected, [&]() {
        success = true;
        loop.quit();
    });

    auto connErr = connect(m_sshClient, &SshClient::error, [&](const QString &msg) {
        qCritical() << "!e" << "SFTP Connection Error:" << msg;
        success = false;
        loop.quit();
    });

    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_sshClient->connectToHost(host, port, m_user, m_pass, "", ModeSftp);

    timeout.start(10000); // 10 second connection timeout
    loop.exec();

    // Clean up connections so they don't fire during subsequent file transfers
    disconnect(connConn);
    disconnect(connErr);

    return success;
}

// ============================================================================
// DIRECTORY ENGINE
// ============================================================================

bool SftpClient::beginListing(const QString &path)
{
    if (!m_sshClient->isConnected()) return false;

    m_directoryCache.clear();
    m_networkBuffer.clear();

    bool success = false;
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);

    auto connFinish = connect(m_sshClient, &SshClient::sftpFinished, [&]() {
        success = true;
        loop.quit();
    });

    auto connErr = connect(m_sshClient, &SshClient::error, [&](const QString &msg) {
        qCritical() << "!e" << "SFTP Directory Error:" << msg;
        success = false;
        loop.quit();
    });

    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    m_sshClient->requestSftp(path, true, ""); // true = directory request

    timeout.start(10000);
    loop.exec();

    disconnect(connFinish);
    disconnect(connErr);

    if (success) {
        parseDirectoryData();
        m_listingFinished = false;
    }

    return success;
}

void SftpClient::parseDirectoryData()
{
    // The SshBackend formats directory lists with a trailing slash for folders, separated by newlines.
    QString rawList = QString::fromUtf8(m_networkBuffer);
    QStringList lines = rawList.split('\n', Qt::SkipEmptyParts);

    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty()) continue;

        DirectoryEntry entry;
        if (line.endsWith('/')) {
            entry.isDirectory = true;
            line.chop(1); // Remove the slash for clean UI display
        } else {
            entry.isDirectory = false;
        }

        entry.name = line;
        m_directoryCache.append(entry);
    }
}

QList<INetworkClient::DirectoryEntry> SftpClient::fetchNextBatch(int count)
{
    QList<DirectoryEntry> entries;

    int processed = 0;
    while (!m_directoryCache.isEmpty() && processed < count) {
        entries.append(m_directoryCache.takeFirst());
        processed++;
    }

    if (m_directoryCache.isEmpty()) {
        m_listingFinished = true;
    }

    return entries;
}

void SftpClient::endListing()
{
    m_directoryCache.clear();
    m_listingFinished = true;
}

// ============================================================================
// FILE TRANSFER ENGINE
// ============================================================================

quint32 SftpClient::getFileSize(const QString &/*path*/)
{
    // File sizes are determined dynamically after the buffer is populated
    return 0;
}

quint32 SftpClient::getFileSize(quint8 /*handle*/)
{
    // Once openFile finishes, m_downloadBuffer contains the entire file
    return m_downloadBuffer.size();
}

quint8 SftpClient::openFile(const QString &path)
{
    if (!m_sshClient->isConnected()) return 0xFF;

    m_downloadBuffer.clear();
    m_networkBuffer.clear();

    bool success = false;
    QEventLoop loop;

    auto connFinish = connect(m_sshClient, &SshClient::sftpFinished, [&]() {
        success = true;
        loop.quit();
    });

    auto connErr = connect(m_sshClient, &SshClient::error, [&](const QString &msg) {
        qCritical() << "!e" << "SFTP Download Error:" << msg;
        success = false;
        loop.quit();
    });

    // Start the asynchronous file read request
    m_sshClient->requestSftp(path, false, "");

    // Wait until the entire file is dumped into m_networkBuffer
    loop.exec();

    disconnect(connFinish);
    disconnect(connErr);

    if (success) {
        // Move the raw payload into our stable read buffer
        m_downloadBuffer = m_networkBuffer;
        return 0x01; // Dummy handle denoting success
    }

    return 0xFF; // Failure
}

QByteArray SftpClient::readFile(quint8 handle, quint32 offset, quint32 size)
{
    Q_UNUSED(handle);

    // Safely dole out 32KB chunks from the completed RAM buffer
    return m_downloadBuffer.mid(offset, size);
}

void SftpClient::closeFile(quint8 handle)
{
    Q_UNUSED(handle);

    // Free the RAM buffer once TnfsImage finishes processing
    m_downloadBuffer.clear();
    m_networkBuffer.clear();
}
