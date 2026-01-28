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
    if (info.error() != QHostInfo::NoError || info.addresses().isEmpty()) return false;
    serverAddr = info.addresses().first();
    m_sessionId = 0;
    m_sequence = 0;
    return true;
}

QByteArray TnfsClient::sendCommand(quint8 cmd, const QByteArray &data) {
    QMutexLocker locker(&netMutex);

    QByteArray packet;
    packet.append(static_cast<char>(m_sessionId & 0xFF));
    packet.append(static_cast<char>((m_sessionId >> 8) & 0xFF));
    packet.append(static_cast<char>(m_sequence++));
    packet.append(static_cast<char>(cmd));
    packet.append(data);

    socket->writeDatagram(packet, serverAddr, serverPort);

    if (socket->waitForReadyRead(2000)) {
        QByteArray response;
        response.resize(socket->pendingDatagramSize());
        socket->readDatagram(response.data(), response.size());

        if (response.size() >= 4) {
            // Latch Session ID (Bytes 0-1)
            quint16 incomingSid = (quint8)response.at(0) | ((quint8)response.at(1) << 8);
            if (incomingSid != 0 && incomingSid != m_sessionId) {
                m_sessionId = incomingSid;
                qDebug() << "TNFS: Session Update:" << Qt::hex << m_sessionId;
            }
            return response;
        }
    }
    return QByteArray();
}

bool TnfsClient::mount(const QString &remotePath)
{
    QByteArray payload;
    payload.append((char)0x01); payload.append((char)0x00);
    QString path = remotePath.isEmpty() ? "/" : remotePath;
    payload.append(path.toUtf8());
    payload.append((char)0x00);
    payload.append("anonymous"); payload.append((char)0x00);
    payload.append((char)0x00);

    QByteArray response = sendCommand(CMD_MOUNT, payload);

    // FIX: Check Status at Index 4 (Byte 4)
    // [Header:4] [Status:1]
    if (response.size() < 5 || (quint8)response.at(4) != 0) {
        qDebug() << "TNFS: Mount failed. Status:" << (response.size() >= 5 ? (quint8)response.at(4) : -1);
        return false;
    }
    return true;
}

QList<TnfsClient::DirectoryEntry> TnfsClient::listDirectory(const QString &path) {
    QList<DirectoryEntry> entries;
    QByteArray req = path.toUtf8(); req.append((char)0x00);

    QByteArray response = sendCommand(CMD_OPENDIR, req); // CMD_OPENDIR = 0x10

    // DEBUGGER TRUTH: [Header:4] [Status:1] [Handle:1] = 6 Bytes Min
    if (response.size() < 6) return entries;

    // Byte 4 is Status (Must be 0). Byte 5 is Handle.
    if ((quint8)response.at(4) != 0) {
        qDebug() << "TNFS: OpenDir Error Status:" << (quint8)response.at(4);
        return entries;
    }
    quint8 handle = (quint8)response.at(5); // <--- CORRECTED OFFSET

    int safety = 0;
    while (safety++ < 1024) {
        QByteArray entryData = sendCommand(CMD_READDIR, QByteArray().append((char)handle));

        // DEBUGGER TRUTH: [Header:4] [Status:1] [Name:1+] = 6 Bytes Min
        if (entryData.size() < 6) break;

        // Byte 4 is Status (Must be 0)
        if ((quint8)entryData.at(4) != 0) break;

        // Byte 5 is Start of Name
        QByteArray rawName = entryData.mid(5); // <--- CORRECTED OFFSET

        if (rawName.isEmpty() || rawName.at(0) == '\0') break;

        QString name = QString::fromUtf8(rawName).trimmed();
        int nullPos = name.indexOf(QChar('\0'));
        if (nullPos != -1) name = name.left(nullPos);

        if (!name.isEmpty() && name != "." && name != "..") {
            DirectoryEntry entry;
            entry.name = name;
            entry.isDirectory = !name.contains(".");
            entries.append(entry);
        }
    }
    sendCommand(CMD_CLOSEDIR, QByteArray().append((char)handle));

    // 4. RESTORED: Sort folders to top, then alphabetical
    std::sort(entries.begin(), entries.end(), [](const DirectoryEntry &a, const DirectoryEntry &b) {
        if (a.isDirectory != b.isDirectory) return a.isDirectory;
        return a.name.toLower() < b.name.toLower();
    });

    // ... (Keep sorting logic) ...
    return entries;
}

quint8 TnfsClient::openFile(const QString &path) {
    QByteArray req;
    req.append((char)0x00); req.append((char)0x00); // O_RDONLY (Fix applied)
    req.append((char)0x00); req.append((char)0x00); // Mode
    req.append(path.toUtf8()); req.append((char)0x00);

    QByteArray res = sendCommand(CMD_OPEN, req);

    // DEBUG: See what we got
    // qDebug() << "TNFS OPEN Size:" << res.size();

    // FIX: Check Status at Index 4 (Must be 0)
    // FIX: Handle is at Index 5
    if (res.size() >= 6 && (quint8)res.at(4) == 0x00) {
        return (quint8)res.at(5); // <--- CHANGE FROM 4 TO 5
    }

    qDebug() << "TNFS Open Failed. Status:" << (res.size() > 4 ? (quint8)res.at(4) : -1);
    return 0xFF;
}

QByteArray TnfsClient::readFile(quint8 handle, quint32 offset, quint16 size) {
    // DEBUG LOG: Prove we entered readFile
    // qDebug() << "TNFS: Reading Handle" << handle << "Offset" << offset << "Size" << size;

    QByteArray req;
    req.append((char)handle);
    req.append((char)(size & 0xFF)); req.append((char)((size >> 8) & 0xFF));
    req.append((char)(offset & 0xFF)); req.append((char)((offset >> 8) & 0xFF));
    req.append((char)((offset >> 16) & 0xFF)); req.append((char)((offset >> 24) & 0xFF));

    QByteArray res = sendCommand(CMD_READ, req);

    // TIMEOUT CHECK
    if (res.isEmpty()) {
        qCritical() << "TNFS: Read Timeout or Empty Response!";
        return QByteArray();
    }

    // STATUS CHECK
    // Index 4 must be 0x00.
    if (res.size() > 5) {
        quint8 status = (quint8)res.at(4);
        if (status != 0x00) {
            qCritical() << "TNFS: Read Error from Server. Status:" << Qt::hex << status;
            return QByteArray();
        }
        // SUCCESS
        return res.mid(5);
    }

    qCritical() << "TNFS: Read Response too short. Size:" << res.size();
    return QByteArray();
}


void TnfsClient::closeFile(quint8 handle) {
    if (handle != 0xFF) sendCommand(CMD_CLOSE, QByteArray().append((char)handle));
}
