/*
 * pipenetwork.cpp
 * Network Streaming Device (W:) for AspeQt
 * Updated: FTP (curl) with EOL Translation & Error Handling
 */

#include "pipenetwork.h"
#include <QtDebug>
#include <QUrl>
#include <QNetworkRequest>
#include <QThread>

PipeNetwork::PipeNetwork(SioWorker *worker) :
    SioDevice(worker),
    m_manager(new QNetworkAccessManager(this)),
    m_reply(nullptr),
    m_process(nullptr)
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

    // Cleanup Network Reply (HTTP)
    if (m_reply) {
        m_reply->disconnect();
        if (m_reply->isRunning()) m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    // Cleanup Process (FTP/Curl)
    if (m_process) {
        m_process->disconnect();
        if (m_process->state() != QProcess::NotRunning) {
            m_process->kill();
            m_process->waitForFinished(500);
        }
        m_process->deleteLater();
        m_process = nullptr;
    }
}

void PipeNetwork::handleCommand(quint8 command, quint16 aux)
{
    switch (command) {
    // ========================================================================
    // OPEN (0x4F) - Receives URL from Atari
    // ========================================================================
    case 0x4F:
    {
        if (!sio->port()->writeCommandAck()) return;

        // 1. Read the URL string (Filename) from Atari
        QByteArray urlFrame = sio->port()->readDataFrame(256);

        sio->port()->writeDataAck();
        sio->port()->writeComplete();

        // 2. Parse URL
        QString urlStr = QString::fromLatin1(urlFrame);
        int eol = urlStr.indexOf(QChar(0x9B));
        if (eol != -1) urlStr.truncate(eol);

        // Remove "W:" prefix
        if (urlStr.startsWith("W:", Qt::CaseInsensitive)) urlStr.remove(0, 2);

        // 3. Setup State
        reset();
        m_isWriteMode = (aux & 0x08);
        m_lastUrl = urlStr;

        qDebug() << "!n" << tr("[W:] Open %1: %2").arg(m_isWriteMode ? "Write" : "Read").arg(urlStr);

        // 4. Start Request (If Read Mode)
        if (!m_isWriteMode) {
            QUrl url(urlStr);

            if (url.scheme().toLower() == "ftp") {
                // --- USE CURL (FTP GET) ---
                m_process = new QProcess(this);

                // Handle Startup Errors
                connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error){
                    qWarning() << "!e" << "[W:] Curl Process Failed to Start:" << error;
                    m_netFinished = true;
                });

                // Connect Output (Data Received)
                connect(m_process, &QProcess::readyReadStandardOutput, this, [this](){
                    QByteArray raw = m_process->readAllStandardOutput();

                    // --- CRITICAL FIX: Translate EOLs ---
                    // Unix/Web uses \n (0x0A). Windows uses \r\n (0x0D 0x0A).
                    // Atari needs 0x9B.
                    raw.replace("\r", "");        // Remove CR
                    raw.replace('\n', (char)0x9B); // Convert LF to Atari EOL

                    m_rxBuffer.append(raw);
                });

                // Connect Finished
                connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                        this, [this](int exitCode, QProcess::ExitStatus exitStatus){

                            if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                                m_netFinished = true;
                                // Ensure file ends with EOL so INPUT doesn't hang
                                if (!m_rxBuffer.isEmpty() && !m_rxBuffer.endsWith((char)0x9B)) {
                                    m_rxBuffer.append((char)0x9B);
                                }
                            } else {
                                // -sS allows us to see the error message here
                                qWarning() << "!e" << "[W:] FTP Error (curl):" << m_process->readAllStandardError();
                                m_netFinished = true;
                            }
                        });

                QStringList args;
                // -sS = Silent mode but Show Errors
                args << "-sS" << urlStr;
                m_process->start("curl", args);

            } else {
                // --- USE QT NATIVE (HTTP) ---
                QNetworkRequest req(url);
                req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
                m_reply = m_manager->get(req);

                connect(m_reply, &QNetworkReply::readyRead, this, [this](){
                    m_rxBuffer.append(m_reply->readAll());
                });

                connect(m_reply, &QNetworkReply::finished, this, [this](){
                    m_netFinished = true;
                    if (!m_rxBuffer.isEmpty() && !m_rxBuffer.endsWith((char)0x9B)) {
                        m_rxBuffer.append((char)0x9B);
                    }
                });

                connect(m_reply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError){
                    if (m_reply)
                        qWarning() << "!e" << "[W:] Network Error:" << m_reply->errorString();
                    m_netFinished = true;
                });
            }
        }
        break;
    }

        // ========================================================================
        // READ (0x52) - Stream Data to Atari
        // ========================================================================
    case 0x52:
    {
        if (!sio->port()->writeCommandAck()) return;

        // 1. Wait for data
        if (m_rxBuffer.isEmpty() && !m_netFinished) {
            QEventLoop loop;
            QTimer timeout;
            timeout.setSingleShot(true);

            if (m_reply) {
                connect(m_reply, &QNetworkReply::readyRead, &loop, &QEventLoop::quit);
                connect(m_reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            }
            if (m_process) {
                connect(m_process, &QProcess::readyReadStandardOutput, &loop, &QEventLoop::quit);
                connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                        &loop, &QEventLoop::quit);
                connect(m_process, &QProcess::errorOccurred, &loop, &QEventLoop::quit);
            }

            connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
            timeout.start(5000);
            loop.exec();
        }

        // 2. Send Frame
        QByteArray chunk;
        if (m_rxBuffer.isEmpty() && m_netFinished) {
            chunk.fill(0, 256); // EOF
        } else {
            int len = qMin(256, m_rxBuffer.size());
            chunk = m_rxBuffer.left(len);
            m_rxBuffer.remove(0, len);
            if (chunk.size() < 256) {
                chunk.append(QByteArray(256 - chunk.size(), (char)0x00));
            }
        }

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(chunk);
        break;
    }

        // ========================================================================
        // WRITE (0x57) - Buffer Data
        // ========================================================================
    case 0x57:
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray data = sio->port()->readDataFrame(256);
        sio->port()->writeDataAck();

        QString chunkStr = QString::fromLatin1(data);
        chunkStr.replace(QChar(0x9B), QString("\n"));
        chunkStr.remove(QChar(0x00));
        m_txAccumulator.append(chunkStr.toLatin1());

        sio->port()->writeComplete();
        break;
    }

        // ========================================================================
        // CLOSE (0x43) - Execute POST/PUT
        // ========================================================================
    case 0x43:
    {
        if (!sio->port()->writeCommandAck()) return;

        if (m_isWriteMode && !m_txAccumulator.isEmpty()) {
            QUrl url(m_lastUrl);

            if (url.scheme().toLower() == "ftp") {
                // --- USE CURL (FTP PUT) ---
                qDebug() << "!n" << tr("[W:] FTP Uploading %1 bytes via Curl...").arg(m_txAccumulator.size());

                m_process = new QProcess(this);

                connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error){
                    qWarning() << "!e" << "[W:] Curl Upload Process Error:" << error;
                });

                QStringList args;
                args << "-sS" << "-T" << "-" << m_lastUrl;

                m_process->start("curl", args);

                if (m_process->waitForStarted()) {
                    m_process->write(m_txAccumulator);
                    m_process->closeWriteChannel();

                    m_process->waitForFinished(10000);

                    if (m_process->exitCode() == 0) {
                        qDebug() << "!n" << tr("[W:] FTP Upload Complete.");
                    } else {
                        qWarning() << "!e" << "[W:] FTP Upload Failed:" << m_process->readAllStandardError();
                    }
                } else {
                    qWarning() << "!e" << "[W:] Failed to start curl for upload.";
                }

            } else {
                // --- USE QT NATIVE (HTTP POST) ---
                QNetworkRequest req(url);
                req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");

                QNetworkReply *reply = m_manager->post(req, m_txAccumulator);

                QEventLoop loop;
                connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
                loop.exec();

                qDebug() << "!n" << tr("[W:] POST Synced to %1 (%2 bytes)").arg(m_lastUrl).arg(m_txAccumulator.size());
                reply->deleteLater();
            }
        }

        reset();
        sio->port()->writeComplete();
        break;
    }

    default:
        sio->port()->writeCommandNak();
        break;
    }
}
