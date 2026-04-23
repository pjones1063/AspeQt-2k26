#include "tnfsclient.h"
#include <QHostInfo>
#include <QDebug>
#include <algorithm>
#include <QElapsedTimer>
#include <QVariant>
#include <QVector>
#include <QNetworkDatagram>

TnfsClient::TnfsClient(QObject *parent) : QObject(parent) {
    socket = new QUdpSocket(this);

    // 1MB OS Buffer to prevent dropping pipelined responses
    socket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, QVariant(1024 * 1024));

    socket->bind(QHostAddress::AnyIPv4);
    m_sessionId = 0;
    m_sequence = 0;
}

TnfsClient::~TnfsClient() {
    socket->close();
}

bool TnfsClient::connectToHost(const QString &host, quint16 port) {
    serverPort = port;
    QHostInfo info = QHostInfo::fromName(host);
    if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) {
        qCritical() << "!e" << "TNFS: Host resolution failed for" << host;
        return false;
    }
    serverAddr = info.addresses().first();
    m_sessionId = 0;
    m_sequence = 0;
    return true;
}

// ====================================================================================
// SYNCHRONOUS COMMAND ENGINE (Mount, Open, Stat)
// ====================================================================================
QByteArray TnfsClient::sendCommand(quint8 cmd, const QByteArray &data) {
    QMutexLocker locker(&netMutex);

    while (socket->hasPendingDatagrams()) {
        socket->receiveDatagram();
    }

    const int MAX_RETRIES = 4;
    int timeouts[] = {50, 150, 500, 500};
    quint8 currentSeq = m_sequence;

    QByteArray packet;
    packet.reserve(4 + data.size());
    packet.append(static_cast<char>(m_sessionId & 0xFF));
    packet.append(static_cast<char>((m_sessionId >> 8) & 0xFF));
    packet.append(static_cast<char>(currentSeq));
    packet.append(static_cast<char>(cmd));
    packet.append(data);

    for (int retry = 0; retry < MAX_RETRIES; ++retry) {
        socket->writeDatagram(packet, serverAddr, serverPort);

        QElapsedTimer timer;
        timer.start();
        qint64 remainingTime = timeouts[retry];

        while (remainingTime > 0) {
            if (socket->hasPendingDatagrams() || socket->waitForReadyRead(remainingTime)) {

                while (socket->hasPendingDatagrams()) {
                    QNetworkDatagram datagram = socket->receiveDatagram();
                    QByteArray response = datagram.data();

                    if (response.size() >= 4) {
                        quint8 respSeq = (quint8)response.at(2);
                        if (respSeq == currentSeq) {
                            m_sequence++;
                            return response;
                        }
                    }
                }
            } else {
                break;
            }
            remainingTime = timeouts[retry] - timer.elapsed();
        }
        if (retry > 0) qWarning() << "!w" << "TNFS: Retry" << retry << "for CMD" << Qt::hex << cmd;
    }

    m_sequence++;
    qCritical() << "!e" << "TNFS: IO Error/Timeout. CMD:" << Qt::hex << cmd;
    return QByteArray();
}

bool TnfsClient::mount(const QString &remotePath)
{
    QByteArray payload;
    payload.append((char)0x01); payload.append((char)0x00);
    QString path = remotePath.isEmpty() ? "/" : remotePath;
    payload.append(path.toUtf8()); payload.append((char)0x00);
    payload.append("anonymous"); payload.append((char)0x00);
    payload.append((char)0x00);

    QByteArray response = sendCommand(CMD_MOUNT, payload);

    if (response.size() < 4) return false;
    if (response.size() > 4 && (quint8)response.at(4) != 0) return false;

    quint8 lowByte = (quint8)response.at(0);
    quint8 highByte = (quint8)response.at(1);
    m_sessionId = lowByte | (highByte << 8);
    return true;
}

quint8 TnfsClient::openFile(const QString &path) {
    QByteArray req;
    req.append((char)0x00); req.append((char)0x00);
    req.append((char)0x00); req.append((char)0x00);
    req.append(path.toUtf8()); req.append((char)0x00);

    QByteArray res = sendCommand(CMD_OPEN, req);
    if (res.size() >= 6 && (quint8)res.at(4) == 0x00) {
        return (quint8)res.at(5);
    }
    return 0xFF;
}

void TnfsClient::closeFile(quint8 handle) {
    if (handle != 0xFF) {
        sendCommand(CMD_CLOSE, QByteArray().append((char)handle));
    }
}

// ====================================================================================
// FILE DOWNLOAD PIPELINE
// ====================================================================================
QByteArray TnfsClient::readFile(quint8 handle, quint32 offset, quint32 size) {
    QMutexLocker locker(&netMutex);

    // [FIX 1] Hard limit to 512 bytes for universal older TNFS server compatibility
    const quint32 MAX_PAYLOAD = 512;

    if (size <= MAX_PAYLOAD) {
        QByteArray req;
        req.append((char)handle);
        req.append((char)(size & 0xFF));       req.append((char)((size >> 8) & 0xFF));
        req.append((char)(offset & 0xFF));     req.append((char)((offset >> 8) & 0xFF));
        req.append((char)((offset >> 16) & 0xFF)); req.append((char)((offset >> 24) & 0xFF));

        QByteArray res = sendCommand(CMD_READ, req);
        if (res.size() >= 7 && (quint8)res.at(4) == 0x00) return res.mid(7);
        return QByteArray();
    }

    while (socket->hasPendingDatagrams()) {
        socket->receiveDatagram();
    }

    QByteArray resultBuffer;
    resultBuffer.resize(size);
    quint32 actualTotalSize = size;

    struct Chunk {
        quint32 offset;
        quint32 length;
        quint8 seq;
        qint64 lastSent;
        int retries;
        bool done;
    };

    QVector<Chunk> chunks;
    quint32 currentOffset = offset;
    quint32 remaining = size;

    while (remaining > 0) {
        Chunk c;
        c.offset = currentOffset;
        c.length = qMin(remaining, MAX_PAYLOAD);
        c.seq = m_sequence++;
        c.lastSent = 0;
        c.retries = 0;
        c.done = false;
        chunks.append(c);

        currentOffset += c.length;
        remaining -= c.length;
    }

    int windowSize = 16;
    int chunksCompleted = 0;
    int totalChunks = chunks.size();

    QElapsedTimer timer;
    timer.start();

    while (chunksCompleted < totalChunks) {
        int inFlight = 0;
        int packetsSentThisLoop = 0;
        qint64 now = timer.elapsed();

        for (int i = 0; i < totalChunks; ++i) {
            if (chunks[i].done) continue;

            if (inFlight < windowSize) {
                int timeout = (chunks[i].retries == 0) ? 0 :
                                  (chunks[i].retries == 1 ? 50 :
                                       (chunks[i].retries == 2 ? 150 : 500));

                if (now - chunks[i].lastSent >= timeout) {
                    if (chunks[i].retries > 4) {
                        qCritical() << "!e" << "TNFS Pipeline Failure on chunk" << i << "(Offset:" << chunks[i].offset << ")";
                        resultBuffer.resize(chunksCompleted * MAX_PAYLOAD);
                        return resultBuffer;
                    }

                    QByteArray req;
                    req.reserve(13);
                    req.append(static_cast<char>(m_sessionId & 0xFF));
                    req.append(static_cast<char>((m_sessionId >> 8) & 0xFF));
                    req.append(static_cast<char>(chunks[i].seq));
                    req.append(static_cast<char>(CMD_READ));
                    req.append((char)handle);
                    req.append((char)(chunks[i].length & 0xFF));
                    req.append((char)((chunks[i].length >> 8) & 0xFF));
                    req.append((char)(chunks[i].offset & 0xFF));
                    req.append((char)((chunks[i].offset >> 8) & 0xFF));
                    req.append((char)((chunks[i].offset >> 16) & 0xFF));
                    req.append((char)((chunks[i].offset >> 24) & 0xFF));

                    socket->writeDatagram(req, serverAddr, serverPort);
                    chunks[i].lastSent = now;
                    chunks[i].retries++;
                    packetsSentThisLoop++;
                }
                inFlight++;
                if (packetsSentThisLoop >= 4) break;
            }
        }

        if (socket->waitForReadyRead(10) || socket->hasPendingDatagrams()) {
            while (socket->hasPendingDatagrams()) {
                QNetworkDatagram datagram = socket->receiveDatagram();
                QByteArray response = datagram.data();

                if (response.size() >= 5 && (quint8)response.at(3) == CMD_READ) {
                    quint8 respSeq = (quint8)response.at(2);
                    quint8 status = (quint8)response.at(4);

                    for (int i = 0; i < totalChunks; ++i) {
                        if (!chunks[i].done && chunks[i].seq == respSeq) {

                            // 0x00 = Success. Copy data into buffer.
                            if (status == 0x00 && response.size() >= 7) {
                                QByteArray payload = response.mid(7);
                                int localOffset = chunks[i].offset - offset;

                                memcpy(resultBuffer.data() + localOffset, payload.constData(), payload.size());

                                // Premature EOF Check
                                if ((quint32)payload.size() < chunks[i].length) {
                                    actualTotalSize = qMin(actualTotalSize, (quint32)(localOffset + payload.size()));
                                    for (int j = i + 1; j < totalChunks; ++j) {
                                        if (!chunks[j].done) {
                                            chunks[j].done = true;
                                            chunksCompleted++;
                                        }
                                    }
                                }
                            } else {
                                // Server explicitly rejected the packet (e.g. read past EOF).
                                // Gracefully abort this chunk and any trailing chunks.
                                actualTotalSize = qMin(actualTotalSize, chunks[i].offset - offset);
                                for (int j = i; j < totalChunks; ++j) {
                                    if (!chunks[j].done) {
                                        chunks[j].done = true;
                                        chunksCompleted++;
                                    }
                                }
                            }

                            chunks[i].done = true;
                            chunksCompleted++;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (actualTotalSize != size) {
        resultBuffer.resize(actualTotalSize);
    }
    return resultBuffer;
}

// ====================================================================================
// DIRECTORY BROWSING
// ====================================================================================
bool TnfsClient::beginListing(const QString &path)
{
    if (m_dirHandle != 0xFF) endListing();

    QByteArray req = path.toUtf8(); req.append((char)0x00);
    QByteArray response = sendCommand(CMD_OPENDIR, req);

    if (response.size() < 6 || (quint8)response.at(4) != 0) return false;

    m_dirHandle = (quint8)response.at(5);
    m_listingFinished = false;

    return true;
}

QList<TnfsClient::DirectoryEntry> TnfsClient::fetchNextBatch(int count)
{
    QList<DirectoryEntry> entries;
    if (m_dirHandle == 0xFF) return entries;

    for (int i = 0; i < count; i++) {
        QByteArray entryData = sendCommand(CMD_READDIR, QByteArray().append((char)m_dirHandle));

        if (entryData.size() < 6 || (quint8)entryData.at(4) != 0) {
            endListing();
            m_listingFinished = true;
            break;
        }

        QByteArray rawName = entryData.mid(5);
        if (rawName.isEmpty() || rawName.at(0) == '\0') {
            endListing();
            m_listingFinished = true;
            break;
        }

        QString name = QString::fromUtf8(rawName).trimmed();
        int nullPos = name.indexOf(QChar('\0'));
        if (nullPos != -1) name = name.left(nullPos);

        if (!name.isEmpty() && name != "." && name != "..") {
            DirectoryEntry entry;
            entry.name = name;
            bool hasSlash = name.endsWith("/");
            bool hasDot   = name.contains(".");
            entry.isDirectory = (hasSlash || !hasDot);
            if (hasSlash) entry.name.chop(1);
            entries.append(entry);
        }
    }
    return entries;
}

void TnfsClient::endListing()
{
    if (m_dirHandle != 0xFF) {
        sendCommand(CMD_CLOSEDIR, QByteArray().append((char)m_dirHandle));
        m_dirHandle = 0xFF;
    }
}

QList<TnfsClient::DirectoryEntry> TnfsClient::listDirectory(const QString &path) {
    QList<DirectoryEntry> entries;
    if (beginListing(path)) {
        while (!m_listingFinished) {
            entries.append(fetchNextBatch(100));
        }
    }
    return entries;
}

// ====================================================================================
// UTILITY ENGINE
// ====================================================================================
quint32 TnfsClient::getFileSize(const QString &path)
{
    QByteArray req = path.toUtf8();
    req.append((char)0x00);

    QByteArray res = sendCommand(CMD_STAT, req);

    if (res.size() < 13 || (quint8)res.at(4) != 0x00) {
        return 0;
    }

    // [FIX 2] Corrected the CMD_STAT offsets.
    // 0-3: Header
    // 4: Status
    // 5-6: Mode
    // 7: UID
    // 8: GID
    // 9-12: Size (32-bit LE)
    quint32 size = (quint8)res.at(9) |
                   ((quint8)res.at(10) << 8) |
                   ((quint8)res.at(11) << 16) |
                   ((quint8)res.at(12) << 24);

    return size;
}

quint32 TnfsClient::getFileSize(quint8 handle)
{
    QByteArray reqEnd;
    reqEnd.append((char)handle);
    reqEnd.append((char)TnfsSeekEnd);
    reqEnd.append((char)0x00); reqEnd.append((char)0x00); reqEnd.append((char)0x00); reqEnd.append((char)0x00);

    QByteArray resEnd = sendCommand(CMD_LSEEK, reqEnd);
    if (resEnd.size() < 5 || (quint8)resEnd.at(4) != 0x00) return 0;

    quint32 size = (quint8)resEnd.at(5) | ((quint8)resEnd.at(6) << 8) | ((quint8)resEnd.at(7) << 16) | ((quint8)resEnd.at(8) << 24);

    QByteArray reqSet;
    reqSet.append((char)handle);
    reqSet.append((char)TnfsSeekSet);
    reqSet.append((char)0x00); reqSet.append((char)0x00); reqSet.append((char)0x00); reqSet.append((char)0x00);

    sendCommand(CMD_LSEEK, reqSet);

    return size;
}
