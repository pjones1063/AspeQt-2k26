/*
 * pipenetwork.cpp
 * Network Streaming Device (W:) for AspeQt
 * Updated: FTP (curl) with EOL Translation & Error Handling
 */

#include "pipenetwork.h"
#include "aspeqtsettings.h"
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

bool PipeNetwork::shouldTranslate(quint16 aux, bool globalSetting)
{
    int aux2 = (aux >> 8) & 0xFF; // Extract High Byte
    if (aux2 == 1) return true;   // Force TEXT (Translate)
    if (aux2 == 2) return false;  // Force BINARY (Raw)
    return globalSetting;         // Default
}


QString PipeNetwork::cleanUrl(QString raw)
{
    // 1. Strip Atari EOLs (0x9B)
    int eol = raw.indexOf(QChar(0x9B));
    if (eol != -1) raw.truncate(eol);

    // 2. Strip Device Prefix (W:, W1:, W2:)
    // Check for colon in position 1 ("W:") or 2 ("W1:")
    int colon = raw.indexOf(':');
    if (colon == 1 || colon == 2) {
        // Ensure strictly that we aren't stripping "http:" (colon at 4)
        return raw.mid(colon + 1);
    }

    return raw;
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
        QString raw = QString::fromLatin1(urlFrame);
        QString urlStr = cleanUrl(raw);

        // 3. Setup State
        reset();
        m_isWriteMode = (aux & 0x08);
        m_lastUrl = urlStr;
        bool global = m_isWriteMode ? aspeqtSettings->translateEolOnPost() : aspeqtSettings->translateEolOnGet();
        m_sessionTranslate = shouldTranslate(aux, global);

        if (!m_isWriteMode) {
            QUrl url(urlStr);

            if (url.scheme().toLower() == "ftp") {
                // --- USE CURL (FTP GET) ---
                m_process = new QProcess(this);
                connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error){
                    qWarning() << "!e" << "[W:] Curl Process Failed to Start:" << error;
                    m_netFinished = true;
                });

                // Connect Output (Data Received)
                connect(m_process, &QProcess::readyReadStandardOutput, this, [this](){
                    QByteArray rawData = m_process->readAllStandardOutput();

                    if (m_sessionTranslate) {
                        rawData.replace("\r", "");
                        rawData.replace('\n', (char)0x9B);
                    }

                    m_rxBuffer.append(rawData);
                });

                // Connect Finished
                connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                        this, [this](int exitCode, QProcess::ExitStatus exitStatus){

                            if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                                m_netFinished = true;
                                // Optional: Ensure file ends with EOL if translating
                                if (m_sessionTranslate &&
                                    !m_rxBuffer.isEmpty() && !m_rxBuffer.endsWith((char)0x9B)) {
                                    m_rxBuffer.append((char)0x9B);
                                }
                            } else {
                                qWarning() << "!e" << "[W:] FTP Error (curl):" << m_process->readAllStandardError();
                                m_netFinished = true;
                            }
                        });

                QStringList args;
                args << "-sS" << urlStr;
                m_process->start("curl", args);

            } else {
                // --- USE QT NATIVE (HTTP) ---
                QNetworkRequest req(url);
                req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
                m_reply = m_manager->get(req);

                connect(m_reply, &QNetworkReply::readyRead, this, [this](){
                    QByteArray rawData = m_reply->readAll();

                    if (m_sessionTranslate) {
                        rawData.replace("\r", "");
                        rawData.replace('\n', (char)0x9B);
                    }

                    m_rxBuffer.append(rawData);
                });

                connect(m_reply, &QNetworkReply::finished, this, [this](){
                    m_netFinished = true;
                    if (m_sessionTranslate &&
                        !m_rxBuffer.isEmpty() && !m_rxBuffer.endsWith((char)0x9B)) {
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

        if (m_isWriteMode) {
            sio->port()->writeCommandNak(); // Error: invalid command for this mode
            return;
        }

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
        // WRITE (0x57) - Buffer Data from Atari
        // ========================================================================
    case 0x57:
    {
        if (!sio->port()->writeCommandAck()) return;

        // 1. Read the 256-byte frame from Atari
        QByteArray data = sio->port()->readDataFrame(256);
        sio->port()->writeDataAck();

        // 2. Process Data based on Translation Mode
        if (m_sessionTranslate) {
            // --- TEXT MODE ---
            // Convert to String, Replace EOLs, and Strip Null Padding
            QString chunkStr = QString::fromLatin1(data);

            // Convert Atari EOL (0x9B) -> Unix Newline
            chunkStr.replace(QChar(0x9B), QString("\n"));

            // Remove Nulls (Padding from the Atari buffer)
            chunkStr.remove(QChar(0x00));

            m_txAccumulator.append(chunkStr.toLatin1());
        } else {
            // --- BINARY MODE ---
            m_txAccumulator.append(data);
        }

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
                reply->deleteLater();
            }
        }

        reset();
        sio->port()->writeComplete();
        break;
    }

        // ========================================================================
        // XIO 44 (0x50 'P') - ONE-SHOT POST
        // ========================================================================
    case 0x50:
    {
        if (!sio->port()->writeCommandAck()) return;

        // 1. Read the "URL,Data" string
        QByteArray packet = sio->port()->readDataFrame(256);
        sio->port()->writeDataAck();

        bool doTranslate  = shouldTranslate(aux, aspeqtSettings->translateEolOnPost());

        // 2. Convert to String
        QString raw = QString::fromLatin1(packet);

        // 3. Clean up the Frame
        while (raw.endsWith(QChar(0x00))) {
            raw.chop(1);
        }

        // 4. Handle End-Of-Line Logic
        if (doTranslate) {
            // 1. If the very last char is 0x9B, it's just the command terminator. Remove it.
            if (raw.endsWith(QChar(0x9B))) {
                raw.chop(1);
            }
            // 2. Convert any remaining internal 0x9B to Newline (\n)
            raw.replace(QChar(0x9B), QString("\n"));
        } else {
            // --- LEGACY MODE (Translation OFF) ---
            int eol = raw.indexOf(QChar(0x9B));
            if (eol != -1) raw.truncate(eol);
        }

        // 5. Split URL / Data
        QString urlStr;
        QByteArray postData;

        int commaPos = raw.indexOf(',');
        if (commaPos != -1) {
            // Found a separator: "http://site.com,DataPayload"
            urlStr = cleanUrl(raw.left(commaPos).trimmed());
            QString dataPart = raw.mid(commaPos + 1);
            postData = dataPart.toLatin1();
        } else {
            // No separator: Just a GET request (or empty POST)
            urlStr = cleanUrl(raw.trimmed());
            postData = "";
        }

        // 6. Handoff to Main Thread
        emit sendFireAndForget(urlStr, postData);

        sio->port()->writeComplete();
        break;
    }


    default:
        sio->port()->writeCommandNak();
        break;
    }

}
