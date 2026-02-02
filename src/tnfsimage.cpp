#include "tnfsimage.h"
#include "tnfsclient.h"
#include <QUrl>
#include <QDebug>
#include <QApplication>
#include <QFile>
#include <QtGlobal>
#include <QRandomGenerator>

TnfsImage::TnfsImage(SioWorker *worker) : SioDevice(worker)
{
    m_headerSkip = 0;
    m_tnfsSectorSize = 128;
    m_isAtx = false;
    m_isXex = false;

    // Initialize ATX pointers
    for(int i=0; i<100; i++) atx.tracks[i].sectors = nullptr;
}

TnfsImage::~TnfsImage()
{
    cleanupAtx();
}

void TnfsImage::cleanupAtx()
{
    for(int i=0; i<100; i++) {
        if(atx.tracks[i].sectors) {
            delete[] atx.tracks[i].sectors;
            atx.tracks[i].sectors = nullptr;
        }
    }
}


bool TnfsImage::openUrl(const QString &url)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QUrl qurl(url);
    QString fullPath = qurl.path(QUrl::ComponentFormattingOption::FullyDecoded);
    QString host = qurl.host();

    // --- RESET STATE ---
    m_imgData.clear();
    m_bootSectors.clear();
    m_chunks.clear();
    m_isAtx = false;
    m_isXex = false;
    cleanupAtx();

    // 1. Force UI Update immediately to show "Connecting..."
    qDebug() << "!n" << "TNFS: Connecting to" << host << "...";
    // Drain event queue to ensure the log line renders NOW
    QCoreApplication::processEvents();

    TnfsClient client;
    if (!client.connectToHost(host)) {
        qWarning() << "!e" << "TNFS: Host Connection Failed:" << host;
        QApplication::restoreOverrideCursor();
        return false;
    }

    if (!client.mount("/")) {
        qWarning() << "!e" << "TNFS: Mount Session Failed";
        QApplication::restoreOverrideCursor();
        return false;
    }

    QString pathNoSlash = fullPath.startsWith("/") ? fullPath.mid(1) : fullPath;
    QString pathWithSlash = fullPath.startsWith("/") ? fullPath : "/" + fullPath;

    // --- STRATEGY 1: Try STAT (Preferred) ---
    // Try both formats because servers differ on leading slash handling
    quint32 totalSize = client.getFileSize(pathNoSlash);
    if (totalSize == 0) totalSize = client.getFileSize(pathWithSlash);

    // --- OPEN FILE ---
    quint8 handle = client.openFile(pathNoSlash);
    if (handle == 0xFF) handle = client.openFile(pathWithSlash);

    if (handle == 0xFF) {
        qWarning() << "!e" << "TNFS: Failed to open:" << fullPath;
        QApplication::restoreOverrideCursor();
        return false;
    }

    // --- STRATEGY 2: Try LSEEK (Fallback) ---
    // If STAT failed, try seeking to end of file
    if (totalSize == 0) {
        totalSize = client.getFileSize(handle);
    }

    // --- LOGGING & UI SETUP ---
    if (totalSize > 0) {
        qDebug() << "!i" << "TNFS: Downloading" << pathNoSlash << "(" << totalSize << "bytes)...";
    } else {
        qDebug() << "!i" << "TNFS: Downloading" << pathNoSlash << "(Stream mode)...";
    }

    // FORCE PAINT: Ensure the log window updates before we enter the loop
    QCoreApplication::sendPostedEvents();
    QCoreApplication::processEvents();

    this->m_originalFileName = url;
    m_imgData.clear();
    if (totalSize > 0) m_imgData.reserve(totalSize);

    quint32 offset = 0;
    QElapsedTimer progressTimer;
    progressTimer.start();

    // Initial Signal: Send 0, but if totalSize is 0, this tells UI to go "Busy Mode"
    emit downloadProgress(0, totalSize);
    QCoreApplication::processEvents();

    while (true) {
        // Download in 1KB chunks
        QByteArray chunk = client.readFile(handle, offset, 1024);

        if (chunk.isEmpty()) break;

        m_imgData.append(chunk);
        offset += chunk.size();

        // Emit Progress
        emit downloadProgress(offset, totalSize);

        // UI Refresh Logic
        // Keep this low (50ms) to prevent "Application Not Responding" ghosting
        if (progressTimer.elapsed() > 50) {
            QCoreApplication::processEvents();
            progressTimer.restart();
        }

        if (m_imgData.size() > 16 * 1024 * 1024) {
            qWarning() << "!e" << "TNFS: File too large (>16MB). Aborting.";
            client.closeFile(handle);
            QApplication::restoreOverrideCursor();
            return false;
        }
    }

    // Final 100% update (Only if we knew the size)
    if (totalSize > 0) {
        emit downloadProgress(totalSize, totalSize);
    } else {
        // If size was unknown, we are done, so maybe hide the bar or set to 100% now
        emit downloadProgress(m_imgData.size(), m_imgData.size());
    }

    QCoreApplication::processEvents();

    client.closeFile(handle);
    qDebug() << "!n" << "TNFS: Download Complete. Size:" << m_imgData.size();

    // ... (rest of parsing logic: ATX, XEX, ATR) ...
    // Copy the rest of your format detection code here

    // 1. ATX Format (Copy Protected)
    if (fullPath.endsWith(".atx", Qt::CaseInsensitive)) {
        if (!parseAtx()) {
            qWarning() << "!e" << "TNFS: Invalid ATX Header or Corrupt File.";
            QApplication::restoreOverrideCursor();
            return false;
        }
        m_isAtx = true;
        qDebug() << "!n" << "TNFS: ATX Protection Loaded.";
    }
    // 2. XEX Format (Executable)
    else if (fullPath.endsWith(".xex", Qt::CaseInsensitive) || fullPath.endsWith(".exe", Qt::CaseInsensitive)) {
        if (!parseXex()) {
            qWarning() << "!e" << "TNFS: Invalid XEX Executable.";
            QApplication::restoreOverrideCursor();
            return false;
        }
        m_isXex = true;
        m_booterLoaded = false;
        qDebug() << "!n" << "TNFS: XEX Loader Prepared.";
    }
    // 3. Standard ATR Format
    else {
        m_headerSkip = 0;
        m_tnfsSectorSize = 128;

        // Validate Magic Number (0x96 0x02)
        if (m_imgData.size() >= 16) {
            quint16 magic = (quint8)m_imgData[0] + ((quint8)m_imgData[1] << 8);
            quint16 secSz = (quint8)m_imgData[4] + ((quint8)m_imgData[5] << 8);

            if (magic == 0x0296) {
                m_headerSkip = 16;
                m_tnfsSectorSize = secSz;
                qDebug() << "!n" << "TNFS: ATR Header Valid. Sector Size:" << m_tnfsSectorSize;
            }
        }

        if (m_imgData.size() < 128) {
            qWarning() << "!e" << "TNFS: Image too small!";
            QApplication::restoreOverrideCursor();
            return false;
        }
    }

    QApplication::restoreOverrideCursor();
    return true;
}


 bool TnfsImage::parseXex()
{
    // Load the internal AspeQt loader binary
    // Try High Speed first if available, otherwise standard
    QFile boot(":/binaries/atari/autoboot/autoboot.bin");

    if (!boot.open(QFile::ReadOnly)) {
        qCritical() << "!e" << "TNFS: Missing internal resource 'autoboot.bin'!";
        return false;
    }
    m_bootSectors = boot.readAll();
    boot.close();

    int cursor = 0;
    int size = m_imgData.size();

    // Check Header (0xFF 0xFF)
    if (size < 2) return false;
    int start = (quint8)m_imgData[0] + (quint8)m_imgData[1] * 256;
    cursor += 2;

    if (start != 0xFFFF) {
        qCritical() << "!e" << "TNFS: XEX missing $FFFF header.";
        return false;
    }

    // Read First Segment Start
    if (cursor + 2 > size) return false;
    start = (quint8)m_imgData[cursor] + (quint8)m_imgData[cursor+1] * 256;
    cursor += 2;

    // Segment Loop
    while (true) {
        // Read Segment End
        if (cursor + 2 > size) break;
        int end = (quint8)m_imgData[cursor] + (quint8)m_imgData[cursor+1] * 256;
        cursor += 2;

        if (end < start) break; // Invalid

        int segLen = end - start + 1;
        if (cursor + segLen > size) break;

        // Split large segments into SIO-friendly chunks (max 1024)
        QByteArray segData = m_imgData.mid(cursor, segLen);
        cursor += segLen;

        int maxChunkSize = 1024;
        for (int i = 0; i < segData.size(); i += maxChunkSize) {
            TnfsExeChunk ch;
            // Use qMin to avoid overrun
            int len = 0;
            if (i + maxChunkSize > segData.size()) len = segData.size() - i;
            else len = maxChunkSize;

            ch.data = segData.mid(i, len);
            ch.address = start;
            start += len;
            m_chunks.append(ch);
        }

        if (cursor >= size) break;

        // Read Next Segment Header
        if (cursor + 2 > size) break;
        start = (quint8)m_imgData[cursor] + (quint8)m_imgData[cursor+1] * 256;
        cursor += 2;

        // Handle optional internal $FFFF headers
        if (start == 0xFFFF) {
            if (cursor + 2 > size) break;
            start = (quint8)m_imgData[cursor] + (quint8)m_imgData[cursor+1] * 256;
            cursor += 2;
        }
    }

    return true;
}

// --- ATX PARSER ---
bool TnfsImage::parseAtx()
{
    if (m_imgData.size() < 48) return false;

    // Check Header "AT8X"
    if (m_imgData[0] != 'A' || m_imgData[1] != 'T' || m_imgData[2] != '8' || m_imgData[3] != 'X') return false;

    atx.version = VAPI_16(m_imgData, 4);
    atx.start = VAPI_32(m_imgData, 28);

    quint32 start = atx.start;
    quint8 track = 0;

    while (start < (quint32)m_imgData.size() && track < 100) {
        if (start + 32 > (quint32)m_imgData.size()) break;

        atx.tracks[track].pos     = start;
        atx.tracks[track].next    = VAPI_32(m_imgData, start + 0);
        atx.tracks[track].type    = VAPI_16(m_imgData, start + 4);
        atx.tracks[track].track   = VAPI_8 (m_imgData, start + 8);
        atx.tracks[track].numsectors = VAPI_16(m_imgData, start + 10);
        atx.tracks[track].start   = VAPI_32(m_imgData, start + 20);

        quint32 sectorListPos = start + atx.tracks[track].start;
        if (sectorListPos + 8 > (quint32)m_imgData.size()) break;

        atx.tracks[track].sector_list_header.size = VAPI_32(m_imgData, sectorListPos);
        atx.tracks[track].sector_list_header.type = VAPI_8 (m_imgData, sectorListPos + 4);

        atx.tracks[track].sectors = new atx_sector[atx.tracks[track].numsectors];
        quint32 currentSectorPos = sectorListPos + 8;

        for (int s = 0; s < atx.tracks[track].numsectors; s++) {
            if (currentSectorPos + 8 > (quint32)m_imgData.size()) break;

            atx.tracks[track].sectors[s].number   = VAPI_8 (m_imgData, currentSectorPos + 0);
            atx.tracks[track].sectors[s].status   = ~VAPI_8 (m_imgData, currentSectorPos + 1);
            atx.tracks[track].sectors[s].position = VAPI_16(m_imgData, currentSectorPos + 2);
            atx.tracks[track].sectors[s].start    = VAPI_32(m_imgData, currentSectorPos + 4);

            currentSectorPos += 8;
        }

        start += atx.tracks[track].next;
        track++;
    }

    phantomflip = 0;
    return true;
}

// --- SIO COMMAND HANDLER ---
void TnfsImage::handleCommand(quint8 command, quint16 aux)
{
    // --- SPECIAL COMMANDS (Speed Poll / XEX Loader) ---
    switch (command) {
    case 0x3F: // Speed Poll
    {
        if (!sio->port()->writeCommandAck()) return;
        sio->port()->writeComplete();
        QByteArray speed(1, 0);
        speed[0] = sio->port()->speedByte();
        sio->port()->writeDataFrame(speed);
        return;
    }

        // --- XEX PROTOCOL ---
    case 0xFE: // Get Chunk Data
    {
        if (!m_isXex || aux >= m_chunks.count()) {
            sio->port()->writeCommandNak();
            return;
        }
        sio->port()->writeCommandAck();
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(m_chunks.at(aux).data);
        qDebug() << "!n" << "TNFS: XEX Chunk" << aux << "sent.";
        return;
    }
    case 0xFF: // Get Chunk Info
    {
        if (!m_isXex || aux >= m_chunks.count()) {
            sio->port()->writeCommandNak();
            return;
        }
        sio->port()->writeCommandAck();

        QByteArray info(6, 0);
        info[0] = m_chunks.at(aux).address % 256;
        info[1] = m_chunks.at(aux).address / 256;
        info[2] = 1;
        info[3] = (aux + 1 < m_chunks.size());
        info[4] = m_chunks.at(aux).data.size() % 256;
        info[5] = m_chunks.at(aux).data.size() / 256;

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(info);
        return;
    }
    case 0xFD: // Loader Done
    {
        if (!m_isXex) { sio->port()->writeCommandNak(); return; }
        sio->port()->writeCommandAck();
        sio->port()->writeComplete();
        qDebug() << "!n" << "TNFS: XEX Boot Complete.";
        return;
    }
    }

    // --- STANDARD SIO (ATR / ATX / BOOT SECTORS) ---
    switch (command) {
    case 0x53: // STATUS
        sio->port()->writeCommandAck();
        sendStatus();
        break;

    case 0x52: // READ
    {
        sio->port()->writeCommandAck();
        QByteArray data;
        if (readSector(aux, data)) {
            sio->port()->writeComplete();
            sio->port()->writeDataFrame(data);
        } else {
            sio->port()->writeError();
        }
        break;
    }

    default:
        sio->port()->writeCommandNak();
        break;
    }
}

void TnfsImage::sendStatus()
{
    QByteArray status;
    status.append((char)0x10);
    status.append((char)0xFF);
    status.append((char)0xE0);
    status.append((char)0x00);

    // Inject WD1772 status if ATX
    if (m_isAtx) status[1] = wd1772status;

    sio->port()->writeComplete();
    sio->port()->writeDataFrame(status);
}

bool TnfsImage::readSector(quint16 sector, QByteArray &data)
{
    // --- XEX MODE (Serve Loader) ---
    if (m_isXex) {
        if (m_bootSectors.isEmpty()) return false;

        quint32 offset = (sector - 1) * 128;

        // --- FIX: Bounds Checking & Padding ---
        // If the start of the sector is beyond the file, just return zeros
        if (offset >= (quint32)m_bootSectors.size()) {
            data.fill(0, 128);
            return true;
        }

        // Calculate actual bytes available to read
        int bytesAvailable = m_bootSectors.size() - offset;
        int bytesToRead = qMin(128, bytesAvailable);

        data = m_bootSectors.mid(offset, bytesToRead);

        // Pad with zeros if we read less than 128 bytes
        if (data.size() < 128) {
            data.append(QByteArray(128 - data.size(), 0));
        }
        return true;
    }

    // --- ATX MODE ---
    if (m_isAtx) {
        bool result = readSectorAtx(sector, data);
        if (result) qDebug() << "!n" << "TNFS: Read ATX Sector" << sector;
        return result;
    }

    // --- ATR MODE ---
    quint16 bytesToRead = m_tnfsSectorSize;
    if (m_tnfsSectorSize == 256 && sector <= 3) bytesToRead = 128;

    quint32 offset;
    if (m_tnfsSectorSize == 256) {
        if (sector <= 3) offset = m_headerSkip + (sector - 1) * 128;
        else offset = m_headerSkip + 384 + (sector - 4) * 256;
    } else {
        offset = ((sector - 1) * 128) + m_headerSkip;
    }

    if (offset + bytesToRead > (quint32)m_imgData.size()) {
        qWarning() << "!e" << "TNFS: Read past EOF. Sector:" << sector;
        return false;
    }

    data = m_imgData.mid(offset, bytesToRead);
    qDebug() << "!n" << "TNFS: Read Sector" << sector;
    return true;
}

// --- ATX HELPERS ---
bool TnfsImage::seekToSectorAtx(quint16 sector, quint32 &offset)
{
    int track = (sector - 1) / 18;
    int tracksector = (sector - 1) % 18;
    int trackindex = 0;

    if (track >= 100) return false;

    int actualSectors = atx.tracks[track].numsectors;
    bool found = false;

    for (int i = 0; i < actualSectors; i++) {
        if (atx.tracks[track].sectors[i].number == (tracksector + 1)) {
            trackindex = i;
            found = true;
            if (phantomflip) break;
        }
    }

    if (found) phantomflip = !phantomflip;
    else return false;

    offset = atx.tracks[track].pos + atx.tracks[track].sectors[trackindex].start;
    wd1772status = atx.tracks[track].sectors[trackindex].status;

    return true;
}

bool TnfsImage::readSectorAtx(quint16 sector, QByteArray &data)
{
    quint32 offset = 0;
    if (!seekToSectorAtx(sector, offset)) return false;

    if (wd1772status != 0xff) {
        data = m_imgData.mid(offset, 128);
        // Zorro Hack
        if (wd1772status == 0xB7) {
            for (int i=0; i<data.size(); i++) {
                if (data[i] == '\x33') {
                    data[i] = QRandomGenerator::global()->generate() & 0xFF;
                }
            }
            return true;
        }
        return false;
    }

    data = m_imgData.mid(offset, 128);
    return true;
}
