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

PipeNetwork::PipeNetwork(SioWorker *worker) :
    SioDevice(worker),
    m_manager(new QNetworkAccessManager(this)),
    m_reply(nullptr),
    m_process(nullptr),
    m_tcpSocket(nullptr)
{
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
    int eol = raw.indexOf(QChar(0x9B));
    if (eol != -1) raw.truncate(eol);

    int colon = raw.indexOf(':');
    if (colon == 1 || colon == 2) {
        return raw.mid(colon + 1);
    }
    return raw;
}

// Unified helper to apply EOL translation to incoming data
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

    connect(m_process, &QProcess::readyReadStandardOutput, this, [this](){
        appendRxData(m_process->readAllStandardOutput());
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
        m_isWriteMode = (aux & 0x08);
        bool global = m_isWriteMode ? aspeqtSettings->translateEolOnPost() : aspeqtSettings->translateEolOnGet();
        m_sessionTranslate = shouldTranslate(aux, global);

        QUrl url(m_lastUrl);
        QString scheme = url.scheme().toLower();

        // TCP opens instantly regardless of Read/Write Mode
        if (scheme == "tcp") {
            openTcpConnection(url);
        }
        // HTTP/FTP only open instantly if in READ mode.
        // If in WRITE mode, they wait for CLOSE (0x43) to send accumulated data.
        else if (!m_isWriteMode) {
            if (scheme == "ftp") openFtpConnection(m_lastUrl);
            else openHttpConnection(url);
        }
        // Track protocol for WRITE mode execution later
        else {
            m_protocol = (scheme == "ftp") ? ProtoFtp : ProtoHttp;
        }
        break;
    }


    case 0x52: // READ
    {
        // FIX: HTTP and FTP are one-way per session based on OPEN mode.
        // TCP is bidirectional (Mode 12). Allow READ if protocol is TCP.
        if (m_isWriteMode && m_protocol != ProtoTcp) {
            sio->port()->writeCommandNak();
            return;
        }

        if (!sio->port()->writeCommandAck()) return;

        // Smart Event Loop: Only wait on the active protocol
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
            }

            connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
            timeout.start(5000); // 5 second timeout to prevent Emulator UI freezing
            loop.exec();
        }

        // Send Frame back to Atari
        if (m_rxBuffer.isEmpty() && m_netFinished) {
            sio->port()->writeError(); // Connection closed
        } else {
            // Grab up to 256 bytes from the network buffer
            int len = qMin(256, m_rxBuffer.size());
            QByteArray chunk = m_rxBuffer.left(len);
            m_rxBuffer.remove(0, len);

            // Pad the remainder with 0x00 to satisfy the Atari's 256-byte SIO requirement
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

        // Live Stream (TCP) vs Accumulate (HTTP/FTP)
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

        // Execute batch uploads only for HTTP/FTP
        if (m_isWriteMode && m_protocol != ProtoTcp && !m_txAccumulator.isEmpty()) {

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
                    if (m_process->exitCode() == 0) qDebug() << "!n" << tr("[W:] FTP Upload Complete.");
                    else qWarning() << "!e" << "[W:] FTP Upload Failed:" << m_process->readAllStandardError();
                }

            } else if (m_protocol == ProtoHttp) {
                QNetworkRequest req((QUrl(m_lastUrl)));
                req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
                QNetworkReply *reply = m_manager->post(req, m_txAccumulator);

                QEventLoop loop;
                connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
                loop.exec();
                reply->deleteLater();
            }
        }

        reset(); // Safely teardown sockets and clear memory
        sio->port()->writeComplete();
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
