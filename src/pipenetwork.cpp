/*
 * pipenetwork.cpp
 * Network Streaming Device (W:) for AspeQt
 */

#include "pipenetwork.h"
#include <QtDebug>
#include <QUrl>
#include <QNetworkRequest> // Required for setAttribute
#include <QThread>

PipeNetwork::PipeNetwork(SioWorker *worker) :
    SioDevice(worker),
    m_manager(new QNetworkAccessManager(this)),
    m_reply(nullptr)
{
    reset();
}

PipeNetwork::~PipeNetwork()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
    }
}

void PipeNetwork::reset()
{
    m_rxBuffer.clear();
    m_txAccumulator.clear();
    m_netFinished = false;
    m_isWriteMode = false;

    // FIX: Don't clear m_lastUrl here, we might need it for CLOSE (POST)

    if (m_reply) {
        // FIX: Disconnect signals to prevent "Operation canceled" errors
        // when we intentionally abort a slow/stalled request.
        m_reply->disconnect();

        if (m_reply->isRunning()) m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
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

        // 1. Read the URL string (Filename) from Atari (sent as 256-byte frame)
        QByteArray urlFrame = sio->port()->readDataFrame(256);

        // SIO Protocol: ACK the data, then send Complete
        sio->port()->writeDataAck();
        sio->port()->writeComplete();

        // 2. Parse URL
        QString urlStr = QString::fromLatin1(urlFrame);

        // Cleanup: Stop at Atari EOL ($9B) or Null
        int eol = urlStr.indexOf(QChar(0x9B));
        if (eol != -1) urlStr.truncate(eol);

        // Remove "W:" or "N:" prefix
        if (urlStr.startsWith("W:", Qt::CaseInsensitive)) urlStr.remove(0, 2);
        if (urlStr.startsWith("N:", Qt::CaseInsensitive)) urlStr.remove(0, 2);

        // 3. Setup State
        reset();
        m_isWriteMode = (aux & 0x08); // Check ICAX1 for Write bit
        m_lastUrl = urlStr;           // FIX: Always save URL for later use

        qDebug() << "!n" << tr("[W:] Open %1: %2").arg(m_isWriteMode ? "Write" : "Read").arg(urlStr);

        // 4. Start Request (If Read Mode)
        if (!m_isWriteMode) {
            // FIX: Separate QUrl construction to avoid "Most Vexing Parse"
            QUrl url(urlStr);
            QNetworkRequest req(url);

            // Allow redirects (important for some web servers)
            req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);

            m_reply = m_manager->get(req);

            // Hook up signals to buffer data as it arrives
            connect(m_reply, &QNetworkReply::readyRead, this, [this](){
                m_rxBuffer.append(m_reply->readAll());
            });

            connect(m_reply, &QNetworkReply::finished, this, [this](){
                m_netFinished = true;
                // Ensure text files end with Atari EOL if missing
                if (!m_rxBuffer.isEmpty() && !m_rxBuffer.endsWith((char)0x9B)) {
                    m_rxBuffer.append((char)0x9B);
                }
            });

            connect(m_reply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError){
                if (m_reply) // Check if valid to avoid race condition
                    qWarning() << "!e" << "[W:] Network Error:" << m_reply->errorString();
                m_netFinished = true;
            });
        }
        break;
    }

        // ========================================================================
        // READ (0x52) - Stream Data to Atari
        // ========================================================================
    case 0x52:
    {
        if (!sio->port()->writeCommandAck()) return;

        // 1. Wait for data if buffer is empty (Busy Wait / Event Loop)
        // This prevents the Atari from getting garbage if network is laggy.
        if (m_rxBuffer.isEmpty() && !m_netFinished) {
            QEventLoop loop;
            QTimer timeout;
            timeout.setSingleShot(true);

            // Wait up to 5000ms (5s) for packets to arrive
            // This gives Python scripts time to wake up and reply.
            if (m_reply) {
                connect(m_reply, &QNetworkReply::readyRead, &loop, &QEventLoop::quit);
                connect(m_reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            }
            connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

            timeout.start(5000);
            loop.exec();
        }

        // 2. Prepare Frame (256 Bytes)
        QByteArray chunk;

        if (m_rxBuffer.isEmpty() && m_netFinished) {
            // EOF: Send 256 bytes of zeros to satisfy SIO protocol
            chunk.fill(0, 256);
            // Optional: Log EOF only once per session if needed
            // qDebug() << "!d" << "[W:] Sent EOF Frame";
        } else {
            int len = qMin(256, m_rxBuffer.size());
            chunk = m_rxBuffer.left(len);
            m_rxBuffer.remove(0, len);

            // Pad to 256 if this is the last partial chunk
            if (chunk.size() < 256) {
                chunk.append(QByteArray(256 - chunk.size(), (char)0x00));
            }
        }

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(chunk);
        break;
    }

        // ========================================================================
        // WRITE (0x57) - Buffer Data from Atari
        // ========================================================================
    case 0x57:
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray data = sio->port()->readDataFrame(256);
        if (data.isEmpty()) {
            sio->port()->writeDataNak();
            return;
        }
        // Convert Atari EOL to PC LF

        sio->port()->writeDataAck();
        QString chunkStr = QString::fromLatin1(data);
        chunkStr.replace(QChar(0x9B), QString("\n"));
        chunkStr.remove(QChar(0x00)); // Remove padding nulls
        m_txAccumulator.append(chunkStr.toLatin1());
        sio->port()->writeComplete();

        break;
    }

        // ========================================================================
        // CLOSE (0x43) - Cleanup or Execute POST
        // ========================================================================
    case 0x43:
    {
        if (!sio->port()->writeCommandAck()) return;

        // --- EXECUTE POST (If we were writing) ---
        if (m_isWriteMode && !m_txAccumulator.isEmpty()) {

            QUrl url(m_lastUrl);
            QNetworkRequest req(url);
            req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");

            // --- FIX: BLOCKING POST ---
            // Force the Atari to wait until Python actually receives the data.
            QNetworkReply *reply = m_manager->post(req, m_txAccumulator);

            QEventLoop loop;
            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec(); // Freezes this thread until Upload is Done

            reply->deleteLater();

            qDebug() << "!n" << tr("[W:] POST Synced to %1 (%2 bytes)").arg(m_lastUrl).arg(m_txAccumulator.size());
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
