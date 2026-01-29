#include "tnfsclient.h"
#include <QHostInfo>
#include <QDebug>
#include <algorithm>

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
    // Before sending a new command, drain any "Ghost Packets" left over
    // from previous timeouts. This ensures we only read the response to *this* command.
    while (socket->hasPendingDatagrams()) {
        socket->readDatagram(nullptr, socket->pendingDatagramSize());
    }

    // --- FIX 2: TUNE TIMEOUTS ---
    // Atari SIO times out very fast. 250ms is a better balance than 1000ms.
    const int MAX_RETRIES = 4;
    const int TIMEOUT_MS = 250;

    quint8 currentSeq = m_sequence;

    QByteArray packet;
    packet.append(static_cast<char>(m_sessionId & 0xFF));
    packet.append(static_cast<char>((m_sessionId >> 8) & 0xFF));
    packet.append(static_cast<char>(currentSeq));
    packet.append(static_cast<char>(cmd));
    packet.append(data);

    for (int retry = 0; retry < MAX_RETRIES; ++retry) {
        socket->writeDatagram(packet, serverAddr, serverPort);

        if (socket->waitForReadyRead(TIMEOUT_MS)) {
            while (socket->hasPendingDatagrams()) {
                QByteArray response;
                response.resize(socket->pendingDatagramSize());
                socket->readDatagram(response.data(), response.size());

                // Validate Header (4 bytes min)
                if (response.size() >= 4) {
                    quint8 respSeq = (quint8)response.at(2);

                    // Match sequence
                    if (respSeq == currentSeq) {
                        m_sequence++;
                        return response;
                    } else {
                        qWarning() << "!w" << "TNFS: Stale packet ignored. Seq:" << respSeq << "Expected:" << currentSeq;
                    }
                }
            }
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

