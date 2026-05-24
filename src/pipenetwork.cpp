/*
 * pipenetwork.cpp
 * Network Streaming Device (W:) for AspeQt
 * Refactored Architecture
 */

#include "pipenetwork.h"
#include "aspeqtsettings.h"
#include <QtDebug>
#include <QUrl>
#include <QNetworkRequest>
#include <QRegularExpression>

PipeNetwork::PipeNetwork(SioWorker *worker) :
    SioDevice(worker),
    m_manager(new QNetworkAccessManager(this)),
    m_reply(nullptr),
    m_process(nullptr),
    m_tcpSocket(nullptr),
    m_sshClient(new SshClient(this))
{
    // Auto-pipe incoming SFTP data into our standard buffer routine
    connect(m_sshClient, &SshClient::rxData, this, &PipeNetwork::appendRxData);
    reset();
}

PipeNetwork::~PipeNetwork()
{
    reset();
}

void PipeNetwork::reset()
{
    m_rxBuffer.clear();
    m_txAccumulator.clear();
    m_dirFilter.clear();
    m_netFinished = false;
    m_isWriteMode = false;
    m_protocol = ProtoNone;

    if (m_reply) {
        m_reply->disconnect();
        if (m_reply->isRunning()) m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    if (m_process) {
        m_process->disconnect();
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(500);
        }
        m_process->deleteLater();
        m_process = nullptr;
    }

    if (m_tcpSocket) {
        m_tcpSocket->disconnect();
        m_tcpSocket->abort();
        m_tcpSocket->deleteLater();
        m_tcpSocket = nullptr;
    }

    if (m_sshClient) {
        // Disconnect gracefully if a session was left open
        if (m_sshClient->isConnected()) {
            m_sshClient->disconnectFromHost();
        }

        // --- BULLETPROOF FIX: Explicitly destroy the tracked Connection Handles ---
        for (const auto &conn : m_sftpConnections) {
            disconnect(conn);
        }
        m_sftpConnections.clear();
    }
}

bool PipeNetwork::shouldTranslate(quint16 aux, bool globalSetting)
{
    int aux2 = (aux >> 8) & 0xFF;
    if (aux2 == 1) return true;   // Force TEXT
    if (aux2 == 2) return false;  // Force BINARY
    return globalSetting;         // Default
}

QString PipeNetwork::cleanUrl(QString raw)
{
    while (raw.endsWith(QChar(0x00))) raw.chop(1);

    int eol = raw.indexOf(QChar(0x9B));
    if (eol != -1) raw.truncate(eol);

    int colon = raw.indexOf(':');
    if (colon == 1 || colon == 2) {
        return raw.mid(colon + 1);
    }
    return raw;
}

void PipeNetwork::appendRxData(QByteArray rawData)
{
    if (m_sessionTranslate) {
        rawData.replace("\r", "");
        rawData.replace('\n', (char)0x9B);
    }
    m_rxBuffer.append(rawData);
}

// ========================================================================
// PROTOCOL CONNECTION HELPERS
// ========================================================================

void PipeNetwork::openTcpConnection(const QUrl &url)
{
    m_protocol = ProtoTcp;
    m_tcpSocket = new QTcpSocket(this);

    connect(m_tcpSocket, &QTcpSocket::readyRead, this, [this](){
        appendRxData(m_tcpSocket->readAll());
    });

    connect(m_tcpSocket, &QTcpSocket::disconnected, this, [this](){
        m_netFinished = true;
        if (m_sessionTranslate && !m_rxBuffer.isEmpty() && !m_rxBuffer.endsWith((char)0x9B)) {
            m_rxBuffer.append((char)0x9B);
        }
    });

    connect(m_tcpSocket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError){
        qWarning() << "!e" << "[W:] TCP Error:" << m_tcpSocket->errorString();
        m_netFinished = true;
    });

    m_tcpSocket->connectToHost(url.host(), url.port());

    if (!m_tcpSocket->waitForConnected(3000)) {
        qWarning() << "!e" << "[W:] TCP Connection failed to" << url.host() << ":" << url.port();
        m_netFinished = true;
    }
}

void PipeNetwork::openFtpConnection(const QString &urlStr)
{
    m_protocol = ProtoFtp;
    m_process = new QProcess(this);

    connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error){
        qWarning() << "!e" << "[W:] Curl Process Failed:" << error;
        m_netFinished = true;
    });

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this, urlStr](){
        QByteArray output = m_process->readAllStandardOutput();
        // If the URL ends with a slash, it's a directory request.
        if (urlStr.endsWith('/')) {
            formatDirectoryListing(output);
        } else {
            appendRxData(output);
        }
    });

    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus){
                if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                    m_netFinished = true;
                    if (m_sessionTranslate && !m_rxBuffer.isEmpty() && !m_rxBuffer.endsWith((char)0x9B)) {
                        m_rxBuffer.append((char)0x9B);
                    }
                } else {
                    qWarning() << "!e" << "[W:] FTP Error:" << m_process->readAllStandardError();
                    m_netFinished = true;
                }
            });

    m_process->start("curl", QStringList() << "-sS" << urlStr);
}

void PipeNetwork::openHttpConnection(const QUrl &url)
{
    m_protocol = ProtoHttp;
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
    m_reply = m_manager->get(req);

    connect(m_reply, &QNetworkReply::readyRead, this, [this](){
        appendRxData(m_reply->readAll());
    });

    connect(m_reply, &QNetworkReply::finished, this, [this](){
        m_netFinished = true;
        if (m_sessionTranslate && !m_rxBuffer.isEmpty() && !m_rxBuffer.endsWith((char)0x9B)) {
            m_rxBuffer.append((char)0x9B);
        }
    });

    connect(m_reply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError){
        if (m_reply) qWarning() << "!e" << "[W:] Network Error:" << m_reply->errorString();
        m_netFinished = true;
    });
}

void PipeNetwork::openSftpConnection(const QUrl &url)
{
    m_protocol = ProtoSftp;

    m_sftpConnections.append(connect(m_sshClient, &SshClient::connected, this, [this, url]() {
        QString path = url.path();
        if (path.isEmpty()) path = "/";

        bool isDir = path.endsWith('/');
        m_sshClient->requestSftp(path, isDir, m_dirFilter);
    }));

    m_sftpConnections.append(connect(m_sshClient, &SshClient::sftpFinished, this, [this]() {
        m_netFinished = true;
        if (m_sessionTranslate && !m_rxBuffer.isEmpty() && !m_rxBuffer.endsWith((char)0x9B)) {
            m_rxBuffer.append((char)0x9B);
        }
    }));

    m_sftpConnections.append(connect(m_sshClient, &SshClient::error, this, [this](const QString &msg) {
        qWarning() << "!e" << "[W:] SFTP Error:" << msg;
        m_netFinished = true;
    }));

    m_sshClient->connectToHost(
        url.host(),
        url.port(22),
        url.userName(),
        url.password(),
        "",
        ModeSftp
        );
}

// ========================================================================
// SIO COMMAND HANDLER
// ========================================================================

void PipeNetwork::handleCommand(quint8 command, quint16 aux)
{
    switch (command) {

    case 0x4F: // OPEN
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray urlFrame = sio->port()->readDataFrame(256);
        sio->port()->writeDataAck();
        sio->port()->writeComplete();

        reset(); // Clear previous state

        m_lastUrl = cleanUrl(QString::fromLatin1(urlFrame));
        if (!m_lastUrl.contains("://") && !m_currentPath.isEmpty()) {
            m_lastUrl = m_currentPath + m_lastUrl;
        }

        // --- CORRECTED: Parse Directory Filters based on SIO aux1 ---
        m_dirFilter.clear();
        int aux1 = aux & 0xFF; // aux1 is the Atari OPEN mode

        if (aux1 == 6) { // Mode 6 is explicitly a Directory Read
            int lastSlash = m_lastUrl.lastIndexOf('/');
            if (lastSlash != -1) {
                QString filenamePart = m_lastUrl.mid(lastSlash + 1);
                if (!filenamePart.isEmpty()) {
                    m_dirFilter = filenamePart;
                    m_lastUrl.chop(filenamePart.length()); // Leave a clean directory URL
                }
            }
        }

        m_isWriteMode = (aux & 0x08);
        bool global = m_isWriteMode ? aspeqtSettings->translateEolOnPost() : aspeqtSettings->translateEolOnGet();
        m_sessionTranslate = shouldTranslate(aux, global);

        QUrl url(m_lastUrl);
        QString scheme = url.scheme().toLower();

        if (scheme == "tcp") {
            openTcpConnection(url);
        }
        else if (!m_isWriteMode) {
            if (scheme == "ftp") openFtpConnection(m_lastUrl);
            else if (scheme == "sftp") openSftpConnection(url);
            else openHttpConnection(url);
        }
        else {
            if (scheme == "ftp") m_protocol = ProtoFtp;
            else if (scheme == "sftp") m_protocol = ProtoSftp;
            else m_protocol = ProtoHttp;
        }
        break;
    }

    case 0x52: // READ
    {
        if (m_isWriteMode && m_protocol != ProtoTcp) {
            sio->port()->writeCommandNak();
            return;
        }

        if (!sio->port()->writeCommandAck()) return;

        if (m_rxBuffer.isEmpty() && !m_netFinished) {
            QEventLoop loop;
            QTimer timeout;
            timeout.setSingleShot(true);

            if (m_protocol == ProtoHttp && m_reply) {
                connect(m_reply, &QNetworkReply::readyRead, &loop, &QEventLoop::quit);
                connect(m_reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            } else if (m_protocol == ProtoFtp && m_process) {
                connect(m_process, &QProcess::readyReadStandardOutput, &loop, &QEventLoop::quit);
                connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::quit);
            } else if (m_protocol == ProtoTcp && m_tcpSocket) {
                connect(m_tcpSocket, &QTcpSocket::readyRead, &loop, &QEventLoop::quit);
                connect(m_tcpSocket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
            } else if (m_protocol == ProtoSftp && m_sshClient) {
                connect(m_sshClient, &SshClient::sftpFinished, &loop, &QEventLoop::quit);
                connect(m_sshClient, &SshClient::error, &loop, [&loop](const QString &msg){
                    Q_UNUSED(msg);
                    loop.quit();
                });
            }

            connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
            timeout.start(15000);
            loop.exec();
        }

        if (m_rxBuffer.isEmpty() && m_netFinished) {
            sio->port()->writeError();
        } else {
            int len = qMin(256, m_rxBuffer.size());
            QByteArray chunk = m_rxBuffer.left(len);
            m_rxBuffer.remove(0, len);

            if (chunk.size() < 256) {
                chunk.append(QByteArray(256 - chunk.size(), (char)0x00));
            }

            sio->port()->writeComplete();
            sio->port()->writeDataFrame(chunk);
        }
        break;
    }

    case 0x57: // WRITE
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray data = sio->port()->readDataFrame(256);
        sio->port()->writeDataAck();

        QByteArray chunkToSend;
        if (m_sessionTranslate) {
            QString chunkStr = QString::fromLatin1(data);
            chunkStr.replace(QChar(0x9B), QString("\n"));
            chunkStr.remove(QChar(0x00));
            chunkToSend = chunkStr.toLatin1();
        } else {
            chunkToSend = data;
        }

        if (m_protocol == ProtoTcp) {
            if (m_tcpSocket && m_tcpSocket->state() == QAbstractSocket::ConnectedState) {
                m_tcpSocket->write(chunkToSend);
            }
        } else {
            m_txAccumulator.append(chunkToSend);
        }

        sio->port()->writeComplete();
        break;
    }



    case 0x43: // CLOSE
    {
        if (!sio->port()->writeCommandAck()) return;
        bool closeSuccess = false; // <-- FLAG FAILURE DEFAULT

        if (m_isWriteMode && m_protocol != ProtoTcp) {
            while (!m_txAccumulator.isEmpty() && m_txAccumulator.endsWith('\0')) {
                m_txAccumulator.chop(1);
            }

            if (m_protocol == ProtoFtp) {
                m_process = new QProcess(this);
                connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error){
                    qWarning() << "!e" << "[W:] Curl Upload Error:" << error;
                });

                m_process->start("curl", QStringList() << "-sS" << "-T" << "-" << m_lastUrl);
                if (m_process->waitForStarted()) {
                    m_process->write(m_txAccumulator);
                    m_process->closeWriteChannel();
                    m_process->waitForFinished(10000);

                    if (m_process->exitCode() == 0) {
                        qDebug() << "!n" << tr("[W:] FTP Upload Complete.");
                        closeSuccess = true; // <-- EXPLICIT SUCCESS
                    } else {
                        qWarning() << "!e" << "[W:] FTP Upload Failed:" << m_process->readAllStandardError();
                    }
                }

            } else if (m_protocol == ProtoHttp) {
                QNetworkRequest req((QUrl(m_lastUrl)));
                req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
                QNetworkReply *reply = m_manager->post(req, m_txAccumulator);

                QEventLoop loop;
                connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
                loop.exec();

                if (reply->error() != QNetworkReply::NoError) {
                    qWarning() << "!e" << "[W:] HTTP Upload Failed:" << reply->errorString();
                } else {
                    closeSuccess = true; // <-- EXPLICIT SUCCESS
                }
                reply->deleteLater();

            } else if (m_protocol == ProtoSftp) {
                QUrl url(m_lastUrl);
                QEventLoop loop;
                QTimer timeout;
                timeout.setSingleShot(true);

                connect(m_sshClient, &SshClient::connected, &loop, [this, url]() {
                    m_sshClient->requestSftpWrite(url.path(), m_txAccumulator);
                });

                connect(m_sshClient, &SshClient::sftpFinished, &loop, [&loop, &closeSuccess]() {
                    closeSuccess = true; // <-- EXPLICIT SUCCESS
                    loop.quit();
                });

                connect(m_sshClient, &SshClient::error, &loop, [&loop](const QString &msg) {
                    qWarning() << "!e" << "[W:] SFTP Write Error:" << msg;
                    loop.quit();
                });

                connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

                m_sshClient->connectToHost(url.host(), url.port(22), url.userName(), url.password(), "", ModeSftp);
                timeout.start(15000);
                loop.exec();
                m_sshClient->disconnectFromHost();
            }
        } else {
            closeSuccess = true; // Read mode or TCP closes cleanly automatically
        }

        reset();

        // --- NEW: Throw Error 144 on the Atari if it failed! ---
        if (closeSuccess) {
            sio->port()->writeComplete();
        } else {
            sio->port()->writeError();
        }
        break;
    }


    case 0x21: // XIO 33 (Delete File)
    case 0x2A: // XIO 42 (Remove Directory)
    case 0x2C: // XIO 44 (Make Directory)
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray packet = sio->port()->readDataFrame(256);
        sio->port()->writeDataAck();

        QString fullUrl = cleanUrl(QString::fromLatin1(packet));
        if (!fullUrl.contains("://") && !m_currentPath.isEmpty()) {
            fullUrl = m_currentPath + fullUrl;
        }

        QUrl url(fullUrl);
        QString scheme = url.scheme().toLower();
        QString path = url.path();

        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

        bool actionSuccess = false; // <-- FLAG FAILURE DEFAULT

        if (scheme == "sftp") {
            SftpAction action = (command == 0x21) ? ActionDelete :
                                    (command == 0x2A) ? ActionRmdir : ActionMkdir;

            connect(m_sshClient, &SshClient::connected, &loop, [this, path, action]() {
                m_sshClient->requestSftpAction(path, action);
            });

            // Capture the boolean by reference [&actionSuccess]
            connect(m_sshClient, &SshClient::sftpActionFinished, &loop, [&loop, &actionSuccess](bool success, QString err) {
                actionSuccess = success; // Lambda properly assigns true on success
                if (!success) qWarning() << "!e" << "[W:] SFTP Action Failed:" << err;
                loop.quit();
            });

            m_sshClient->connectToHost(url.host(), url.port(22), url.userName(), url.password(), "", ModeSftp);
            timeout.start(10000);
            loop.exec();
            m_sshClient->disconnectFromHost();

        } else if (scheme == "ftp") {
            QString ftpCmd;
            QString baseUrl = "ftp://" + url.host() + url.adjusted(QUrl::RemovePath).path();

            if (command == 0x21) ftpCmd = "DELE " + path;
            else if (command == 0x2A) ftpCmd = "RMD " + path;
            else if (command == 0x2C) ftpCmd = "MKD " + path;

            m_process = new QProcess(this);
            connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::quit);

            m_process->start("curl", QStringList() << "-sS" << "-Q" << ftpCmd << baseUrl);
            timeout.start(5000);
            loop.exec();

            if (m_process->exitCode() != 0) {
                qWarning() << "!e" << "[W:] FTP Action Failed:" << m_process->readAllStandardError();
            } else {
                actionSuccess = true; // <-- EXPLICIT SUCCESS
            }
            m_process->deleteLater();
            m_process = nullptr;
        }

        // --- NEW: Conditionally report success or failure to the Atari ---
        if (actionSuccess) {
            sio->port()->writeComplete();
        } else {
            sio->port()->writeError();
        }
        break;
    }


    case 0x29: // XIO 41 (Change Directory)
    {
        if (!sio->port()->writeCommandAck()) return;
        QByteArray pathFrame = sio->port()->readDataFrame(256);
        sio->port()->writeDataAck();

        QString newPath = cleanUrl(QString::fromLatin1(pathFrame));
        QString targetPath = m_currentPath; // Calculate what path WOULD be

        if (newPath == "..") {
            QUrl currentUrl(targetPath);
            QString path = currentUrl.path();
            if (path != "/" && path != "") {
                if (path.endsWith('/')) path.chop(1);
                int lastSlash = path.lastIndexOf('/');
                if (lastSlash != -1) {
                    path.truncate(lastSlash + 1);
                    currentUrl.setPath(path);
                    targetPath = currentUrl.toString();
                }
            }
        } else if (newPath.contains("://")) {
            targetPath = newPath;
            if (!targetPath.endsWith('/')) targetPath.append('/');
        } else if (newPath.startsWith('/')) {
            QUrl currentUrl(targetPath);
            currentUrl.setPath(newPath);
            targetPath = currentUrl.toString();
            if (!targetPath.endsWith('/')) targetPath.append('/');
        } else {
            targetPath.append(newPath);
            if (!targetPath.endsWith('/')) targetPath.append('/');
        }

        QUrl url(targetPath);
        QString scheme = url.scheme().toLower();
        bool cdSuccess = false; // <-- FLAG FAILURE DEFAULT

        // --- NEW: Asynchronously Validate the Path before committing it ---
        if (scheme == "sftp") {
            QEventLoop loop;
            QTimer timeout;
            timeout.setSingleShot(true);
            connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

            connect(m_sshClient, &SshClient::connected, &loop, [this, url]() {
                m_sshClient->requestSftpAction(url.path(), ActionCheckDir);
            });

            connect(m_sshClient, &SshClient::sftpActionFinished, &loop, [&loop, &cdSuccess](bool success, QString err) {
                cdSuccess = success;
                if (!success) qWarning() << "!e" << "[W:] SFTP CD Failed: Directory does not exist.";
                loop.quit();
            });

            connect(m_sshClient, &SshClient::error, &loop, [&loop](const QString &msg) {
                qWarning() << "!e" << "[W:] SFTP Error:" << msg;
                loop.quit();
            });

            m_sshClient->connectToHost(url.host(), url.port(22), url.userName(), url.password(), "", ModeSftp);
            timeout.start(10000);
            loop.exec();
            m_sshClient->disconnectFromHost();
        } else {
            cdSuccess = true; // Fallback: FTP/HTTP assumes path is valid as they don't do pre-flight checks currently.
        }

        // --- NEW: Only update local path and ACK if the server verified it ---
        if (cdSuccess) {
            m_currentPath = targetPath;
            sio->port()->writeComplete();
        } else {
            sio->port()->writeError();
        }
        break;
    }


    case 0x20: // XIO 32 (Rename)
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray packet = sio->port()->readDataFrame(256);
        sio->port()->writeDataAck();

        QString raw = cleanUrl(QString::fromLatin1(packet));
        int commaPos = raw.indexOf(',');

        if (commaPos == -1) {
            sio->port()->writeError();
            break;
        }

        QString oldStr = raw.left(commaPos).trimmed();
        QString newStr = raw.mid(commaPos + 1).trimmed();

        if (!oldStr.contains("://") && !m_currentPath.isEmpty()) {
            oldStr = m_currentPath + oldStr;
        }

        QUrl oldUrl(oldStr);
        QString scheme = oldUrl.scheme().toLower();
        QString oldPath = oldUrl.path();

        QString basePath = oldPath.section('/', 0, -2);
        QString newPath = basePath + "/" + newStr;

        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

        bool renameSuccess = false; // <-- FLAG FAILURE DEFAULT

        if (scheme == "sftp") {
            connect(m_sshClient, &SshClient::connected, &loop, [this, oldPath, newPath]() {
                m_sshClient->requestSftpRename(oldPath, newPath);
            });

            connect(m_sshClient, &SshClient::sftpActionFinished, &loop, [&loop, &renameSuccess](bool success, QString err) {
                renameSuccess = success;
                if (!success) qWarning() << "!e" << "[W:] SFTP Rename Failed:" << err;
                loop.quit();
            });

            connect(m_sshClient, &SshClient::error, &loop, [&loop](const QString &msg) {
                qWarning() << "!e" << "[W:] SFTP Rename Error:" << msg;
                loop.quit();
            });

            m_sshClient->connectToHost(oldUrl.host(), oldUrl.port(22), oldUrl.userName(), oldUrl.password(), "", ModeSftp);
            timeout.start(10000);
            loop.exec();
            m_sshClient->disconnectFromHost();

        } else if (scheme == "ftp") {
            QString baseUrl = "ftp://" + oldUrl.host() + oldUrl.adjusted(QUrl::RemovePath).path();

            m_process = new QProcess(this);
            connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), &loop, &QEventLoop::quit);

            m_process->start("curl", QStringList() << "-sS"
                                                   << "-Q" << "RNFR " + oldPath
                                                   << "-Q" << "RNTO " + newPath
                                                   << baseUrl);
            timeout.start(5000);
            loop.exec();

            if (m_process->exitCode() != 0) {
                qWarning() << "!e" << "[W:] FTP Rename Failed:" << m_process->readAllStandardError();
            } else {
                renameSuccess = true; // <-- EXPLICIT SUCCESS
            }
            m_process->deleteLater();
            m_process = nullptr;
        }

        // --- NEW: Conditionally report success or failure to the Atari ---
        if (renameSuccess) {
            sio->port()->writeComplete();
        } else {
            sio->port()->writeError();
        }
        break;
    }



    case 0x50: // XIO 44 (ONE-SHOT POST)
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray packet = sio->port()->readDataFrame(256);
        sio->port()->writeDataAck();

        bool doTranslate = shouldTranslate(aux, aspeqtSettings->translateEolOnPost());
        QString raw = QString::fromLatin1(packet);

        while (raw.endsWith(QChar(0x00))) raw.chop(1);

        if (doTranslate) {
            if (raw.endsWith(QChar(0x9B))) raw.chop(1);
            raw.replace(QChar(0x9B), QString("\n"));
        } else {
            int eol = raw.indexOf(QChar(0x9B));
            if (eol != -1) raw.truncate(eol);
        }

        QString urlStr;
        QByteArray postData;
        int commaPos = raw.indexOf(',');

        if (commaPos != -1) {
            urlStr = cleanUrl(raw.left(commaPos).trimmed());
            postData = raw.mid(commaPos + 1).toLatin1();
        } else {
            urlStr = cleanUrl(raw.trimmed());
        }

        emit sendFireAndForget(urlStr, postData);
        sio->port()->writeComplete();
        break;
    }

    default:
        sio->port()->writeCommandNak();
        break;
    }
}

void PipeNetwork::formatDirectoryListing(QByteArray rawListing)
{
    m_rxBuffer.clear();
    QStringList lines = QString::fromLatin1(rawListing).split('\n', Qt::SkipEmptyParts);

    // --- UPDATED: Match the SFTP Regex safety logic ---
    QRegularExpression rx;
    if (!m_dirFilter.isEmpty()) {
        QString safeFilter = m_dirFilter;
        if (safeFilter == "*.*") safeFilter = "*";

        rx = QRegularExpression(QRegularExpression::wildcardToRegularExpression(safeFilter), QRegularExpression::CaseInsensitiveOption);
    }

    for (const QString &line : lines) {
        bool isDir = line.startsWith('d');
        QString name = line.section(' ', -1).trimmed();

        if (!name.isEmpty() && name != "." && name != "..") {

            if (!m_dirFilter.isEmpty() && !rx.match(name).hasMatch()) {
                continue;
            }

            if (isDir) name.append("/");

            m_rxBuffer.append(name.toLatin1());
            m_rxBuffer.append((char)0x9B);
        }
    }
}

