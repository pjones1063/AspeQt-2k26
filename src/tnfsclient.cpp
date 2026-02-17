#include "tnfsclient.h"
#include <QHostInfo>
#include <QDebug>
#include <algorithm>
#include <QElapsedTimer> // Added for accurate timeout handling

TnfsClient::TnfsClient(QObject *parent) : QObject(parent) {
    socket = new QUdpSocket(this);
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

QByteArray TnfsClient::sendCommand(quint8 cmd, const QByteArray &data) {
    QMutexLocker locker(&netMutex);

    // --- FIX 1: FLUSH THE BUFFER ---
    // Drain any "Ghost Packets" left over from previous timeouts.
    while (socket->hasPendingDatagrams()) {
        socket->readDatagram(nullptr, socket->pendingDatagramSize());
    }

    const int MAX_RETRIES = 4;
    const int TIMEOUT_MS = 250;

    quint8 currentSeq = m_sequence;

    // --- OPTIMIZATION 1: Reserve Memory ---
    QByteArray packet;
    packet.reserve(4 + data.size()); // Header (4) + Data
    packet.append(static_cast<char>(m_sessionId & 0xFF));
    packet.append(static_cast<char>((m_sessionId >> 8) & 0xFF));
    packet.append(static_cast<char>(currentSeq));
    packet.append(static_cast<char>(cmd));
    packet.append(data);

    for (int retry = 0; retry < MAX_RETRIES; ++retry) {
        socket->writeDatagram(packet, serverAddr, serverPort);

        // --- OPTIMIZATION 2: Smart Receive Loop ---
        // Don't retransmit immediately if we get a stale packet.
        // Keep listening until the specific timeout for this attempt expires.

        QElapsedTimer timer;
        timer.start();
        qint64 remainingTime = TIMEOUT_MS;

        while (remainingTime > 0) {
            // Check for data immediately (Fast Path) or Wait (Slow Path)
            if (socket->hasPendingDatagrams() || socket->waitForReadyRead(remainingTime)) {

                while (socket->hasPendingDatagrams()) {
                    // Peek size to avoid extra allocation if possible, or just read.
                    qint64 pendingSize = socket->pendingDatagramSize();
                    QByteArray response;
                    response.resize(pendingSize);
                    socket->readDatagram(response.data(), pendingSize);

                    // Validate Header (4 bytes min)
                    if (response.size() >= 4) {
                        quint8 respSeq = (quint8)response.at(2);

                        // Match sequence
                        if (respSeq == currentSeq) {
                            m_sequence++;
                            return response; // SUCCESS
                        } else {
                            // Stale packet found. Log it, but DO NOT return.
                            // We loop back to keep waiting for the *correct* packet.
                            // qWarning() << "!w" << "TNFS: Stale packet ignored. Seq:" << respSeq << "Expected:" << currentSeq;
                        }
                    }
                }
            } else {
                // Real Timeout occurred (waitForReadyRead returned false)
                break;
            }

            // Update remaining time so we don't wait full 250ms again if we just read a stale packet
            remainingTime = TIMEOUT_MS - timer.elapsed();
        }

        // Log retries so we know if the network is struggling
        if (retry > 0) {
            qWarning() << "!w" << "TNFS: Retry" << retry << "for CMD" << Qt::hex << cmd;
        }
    }

    // If we failed after all retries, increment sequence anyway to avoid getting stuck
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

QByteArray TnfsClient::readFile(quint8 handle, quint32 offset, quint16 size) {
    QByteArray req;
    req.append((char)handle);
    req.append((char)(size & 0xFF));       req.append((char)((size >> 8) & 0xFF));
    req.append((char)(offset & 0xFF));     req.append((char)((offset >> 8) & 0xFF));
    req.append((char)((offset >> 16) & 0xFF)); req.append((char)((offset >> 24) & 0xFF));

    QByteArray res = sendCommand(CMD_READ, req);

    if (res.size() >= 7 && (quint8)res.at(4) == 0x00) {
        return res.mid(7);
    }
    return QByteArray();
}

void TnfsClient::closeFile(quint8 handle) {
    if (handle != 0xFF) {
        sendCommand(CMD_CLOSE, QByteArray().append((char)handle));
    }
}

QList<TnfsClient::DirectoryEntry> TnfsClient::listDirectory(const QString &path) {
    QList<DirectoryEntry> entries;
    QByteArray req = path.toUtf8(); req.append((char)0x00);
    QByteArray response = sendCommand(CMD_OPENDIR, req);

    if (response.size() < 6 || (quint8)response.at(4) != 0) return entries;

    quint8 handle = (quint8)response.at(5);
    int safety = 0;
    while (safety++ < 2048) {
        QByteArray entryData = sendCommand(CMD_READDIR, QByteArray().append((char)handle));
        if (entryData.size() < 6 || (quint8)entryData.at(4) != 0) break;

        QByteArray rawName = entryData.mid(5);
        if (rawName.isEmpty() || rawName.at(0) == '\0') break;

        QString name = QString::fromUtf8(rawName).trimmed();
        int nullPos = name.indexOf(QChar('\0'));
        if (nullPos != -1) name = name.left(nullPos);

        if (!name.isEmpty() && name != "." && name != "..") {
            DirectoryEntry entry;
            entry.name = name;
            entry.isDirectory = (name.endsWith("/"));
            if (entry.isDirectory) entry.name.chop(1);
            entries.append(entry);
        }
    }
    sendCommand(CMD_CLOSEDIR, QByteArray().append((char)handle));
    std::sort(entries.begin(), entries.end(), [](const DirectoryEntry &a, const DirectoryEntry &b) {
        return a.name.toLower() < b.name.toLower();
    });
    return entries;
}


bool TnfsClient::beginListing(const QString &path)
{
    // Close previous if exists
    if (m_dirHandle != 0xFF) endListing();

    QByteArray req = path.toUtf8(); req.append((char)0x00);
    QByteArray response = sendCommand(CMD_OPENDIR, req);

    if (response.size() < 6 || (quint8)response.at(4) != 0) return false;

    m_dirHandle = (quint8)response.at(5);

    // *** RESET THE FLAG ***
    m_listingFinished = false;

    return true;
}

QList<TnfsClient::DirectoryEntry> TnfsClient::fetchNextBatch(int count)
{
    QList<DirectoryEntry> entries;
    if (m_dirHandle == 0xFF) return entries;

    for (int i = 0; i < count; i++) {
        QByteArray entryData = sendCommand(CMD_READDIR, QByteArray().append((char)m_dirHandle));

        // Check for EOF (Status != 0) or Empty Packet
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

            // Logic: It is a directory if it ends with '/' OR has no extension
            bool hasSlash = name.endsWith("/");
            bool hasDot   = name.contains(".");

            entry.isDirectory = (hasSlash || !hasDot);

            // Cleanup: Remove trailing slash for display
            if (hasSlash) {
                entry.name.chop(1);
            }

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



quint32 TnfsClient::getFileSize(const QString &path)
{
    // Use CMD_STAT (0x20) instead of LSEEK.
    // Packet: [Cmd] [Path] [0x00]
    QByteArray req = path.toUtf8();
    req.append((char)0x00);

    QByteArray res = sendCommand(CMD_STAT, req);

    // Response Structure (Indices include 4-byte header):
    // 0-3: Header
    // 4: Status (0x00 = OK)
    // 5-6: Mode
    // 7-8: UID
    // 9-10: GID
    // 11-14: Size (Little Endian)

    if (res.size() < 15 || (quint8)res.at(4) != 0x00) {
        return 0; // Error or File Not Found
    }

    // Extract 32-bit Size from offset 11
    quint32 size = (quint8)res.at(11) |
                   ((quint8)res.at(12) << 8) |
                   ((quint8)res.at(13) << 16) |
                   ((quint8)res.at(14) << 24);

    return size;
}



quint32 TnfsClient::getFileSize(quint8 handle)
{
    // 1. Seek to END
    QByteArray reqEnd;
    reqEnd.append((char)handle);
    reqEnd.append((char)TnfsSeekEnd); // <--- Usage
    reqEnd.append((char)0x00); reqEnd.append((char)0x00); reqEnd.append((char)0x00); reqEnd.append((char)0x00);

    QByteArray resEnd = sendCommand(CMD_LSEEK, reqEnd);
    if (resEnd.size() < 5 || (quint8)resEnd.at(4) != 0x00) return 0;

    quint32 size = (quint8)resEnd.at(5) | ((quint8)resEnd.at(6) << 8) | ((quint8)resEnd.at(7) << 16) | ((quint8)resEnd.at(8) << 24);

    // 2. Rewind to START
    QByteArray reqSet;
    reqSet.append((char)handle);
    reqSet.append((char)TnfsSeekSet); // <--- Usage
    reqSet.append((char)0x00); reqSet.append((char)0x00); reqSet.append((char)0x00); reqSet.append((char)0x00);

    sendCommand(CMD_LSEEK, reqSet);

    return size;
}
