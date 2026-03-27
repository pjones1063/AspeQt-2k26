#include "xeximage.h"
#include <QFile>
#include <QDebug>

XexImage::XexImage(SioWorker *worker) : SioDevice(worker)
{
}

XexImage::~XexImage()
{
}

bool XexImage::openLocalFile(const QString &filePath)
{
    m_originalFileName = filePath;
    m_imgData.clear();
    m_bootSectors.clear();
    m_chunks.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "!e" << "XEX Booter: Could not read local file:" << filePath;
        return false;
    }

    m_imgData = file.readAll();
    file.close();

    if (!parseXex()) {
        qWarning() << "!e" << "XEX Booter: Invalid XEX Executable.";
        return false;
    }

    qDebug() << "!i" << "XEX Booter: Loaded" << filePath << "headlessly.";

    return true;
}

bool XexImage::parseXex()
{
    // Load the internal AspeQt loader binary
    QFile boot(":/binaries/autoboot.bin");
    if (!boot.open(QFile::ReadOnly)) {
        qCritical() << "!e" << "XEX Booter: Missing internal resource 'autoboot.bin'!";
        return false;
    }
    m_bootSectors = boot.readAll();
    boot.close();

    int cursor = 0;
    int size = m_imgData.size();

    // Check Header (0xFFFF)
    if (size < 2) return false;
    int start = (quint8)m_imgData[0] + (quint8)m_imgData[1] * 256;
    cursor += 2;

    if (start != 0xFFFF) {
        qCritical() << "!e" << "XEX Booter: Missing $FFFF header.";
        return false;
    }

    // Read First Segment Start
    if (cursor + 2 > size) return false;
    start = (quint8)m_imgData[cursor] + (quint8)m_imgData[cursor+1] * 256;
    cursor += 2;

    // Segment Loop
    while (true) {
        if (cursor + 2 > size) break;
        int end = (quint8)m_imgData[cursor] + (quint8)m_imgData[cursor+1] * 256;
        cursor += 2;

        if (end < start) break;
        int segLen = end - start + 1;
        if (cursor + segLen > size) break;

        QByteArray segData = m_imgData.mid(cursor, segLen);
        cursor += segLen;

        int maxChunkSize = 1024;
        for (int i = 0; i < segData.size(); i += maxChunkSize) {
            LocalExeChunk ch;
            int len = (i + maxChunkSize > segData.size()) ? (segData.size() - i) : maxChunkSize;
            ch.data = segData.mid(i, len);
            ch.address = start;
            start += len;
            m_chunks.append(ch);
        }

        if (cursor >= size) break;
        if (cursor + 2 > size) break;

        start = (quint8)m_imgData[cursor] + (quint8)m_imgData[cursor+1] * 256;
        cursor += 2;

        if (start == 0xFFFF) {
            if (cursor + 2 > size) break;
            start = (quint8)m_imgData[cursor] + (quint8)m_imgData[cursor+1] * 256;
            cursor += 2;
        }
    }
    return true;
}

void XexImage::handleCommand(quint8 command, quint16 aux)
{
    switch (command) {
    case 0x3F: // Speed Poll
        if (!sio->port()->writeCommandAck()) return;
        sio->port()->writeComplete();
        {
            QByteArray speed(1, sio->port()->speedByte());
            sio->port()->writeDataFrame(speed);
        }
        break;

    case 0xFE: // Get Chunk Data
        if (aux >= m_chunks.size()) { sio->port()->writeCommandNak(); return; }
        sio->port()->writeCommandAck();
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(m_chunks.at(aux).data);
        qDebug() << "!n" << "XEX Booter: Chunk" << aux << "sent.";
        break;

    case 0xFF: // Get Chunk Info
        if (aux >= m_chunks.size()) { sio->port()->writeCommandNak(); return; }
        sio->port()->writeCommandAck();
        {
            QByteArray info(6, 0);
            info[0] = m_chunks.at(aux).address % 256;
            info[1] = m_chunks.at(aux).address / 256;
            info[2] = 1;
            info[3] = (aux + 1 < m_chunks.size());
            info[4] = m_chunks.at(aux).data.size() % 256;
            info[5] = m_chunks.at(aux).data.size() / 256;
            sio->port()->writeComplete();
            sio->port()->writeDataFrame(info);
        }
        break;

    case 0xFD: // Loader Done
        sio->port()->writeCommandAck();
        sio->port()->writeComplete();
        qDebug() << "!n" << "XEX Booter: Execution Complete.";
        break;

    case 0x53: // STATUS
        sio->port()->writeCommandAck();
        {
            QByteArray status(4, 0);
            status[0] = 0x10; status[1] = 0xFF; status[2] = 0xE0; status[3] = 0x00;
            sio->port()->writeComplete();
            sio->port()->writeDataFrame(status);
        }
        break;

    case 0x52: // READ (Serve the Boot Sectors)
        sio->port()->writeCommandAck();
        {
            quint32 offset = (aux - 1) * 128;
            QByteArray data;
            if (offset >= (quint32)m_bootSectors.size()) {
                data.fill(0, 128);
            } else {
                int bytesToRead = qMin(128, (int)m_bootSectors.size() - (int)offset);
                data = m_bootSectors.mid(offset, bytesToRead);
                if (data.size() < 128) data.append(QByteArray(128 - data.size(), 0));
            }
            sio->port()->writeComplete();
            sio->port()->writeDataFrame(data);
        }
        break;

    default:
        sio->port()->writeCommandNak();
        break;
    }
}
