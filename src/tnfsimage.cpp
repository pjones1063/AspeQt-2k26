#include "tnfsimage.h"
#include "tnfsclient.h"
#include "ftpclient.h"
#include "inetworkclient.h"
#include <QUrl>
#include <QDebug>
#include <QApplication>
#include <QFile>
#include <QtGlobal>
#include <QThread>
#include <cmath>
#include <QRandomGenerator>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QEventLoop>

TnfsImage::TnfsImage(SioWorker *worker) : SioDevice(worker)
{
    m_headerSkip = 0;
    m_tnfsSectorSize = 128;
    m_isAtx = false;
    m_isXex = false;
    m_spt = 18;
    m_currentWeakOffset = 0xFFFF;
    m_targetAngularPosition = 0;
    m_driveIdentity = "RAM Drive";

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

bool TnfsImage::openUrl(const QString &url, volatile int *activeIdPtr, int myId)
{
    m_driveIdentity = "Network (RAM)";
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QUrl qurl(url);
    QString fullPath = qurl.path(QUrl::ComponentFormattingOption::FullyDecoded);
    QString host = qurl.host();

    // Grab the scheme AND credentials before we cross the thread boundary
    QString scheme = qurl.scheme().toLower();
    QString ftpUser = qurl.userName();
    QString ftpPass = qurl.password();

    m_imgData.clear();
    m_bootSectors.clear();
    m_chunks.clear();
    m_isAtx = false;
    m_isXex = false;
    cleanupAtx();

    qDebug() << "!n" << m_driveIdentity + ":" << "Connecting to" << host << "...";

    // Give the UI one last chance to paint the status before we lock the scope
    QCoreApplication::processEvents();

    if (activeIdPtr && *activeIdPtr != myId) {
        QApplication::restoreOverrideCursor();
        return false;
    }

    QString pathNoSlash = fullPath.startsWith("/") ? fullPath.mid(1) : fullPath;
    QString pathWithSlash = fullPath.startsWith("/") ? fullPath : "/" + fullPath;

    // ========================================================================
    // BACKGROUND DOWNLOAD ENGINE (Bypasses UI freezing and processEvents delay)
    // ========================================================================
    QFuture<QByteArray> future = QtConcurrent::run([=]() -> QByteArray {

        // 1. Create the correct Universal Client
        INetworkClient *client = nullptr;
        if (scheme == "ftp") {
            FtpClient* ftp = new FtpClient();
            // Inject the credentials into the background worker!
            if (!ftpUser.isEmpty()) {
                ftp->setCredentials(ftpUser, ftpPass);
            }
            client = ftp;
        } else {
            client = new TnfsClient();
        }

        if (!client->connectToHost(host)) {
            delete client;
            return QByteArray();
        }

        // 2. TNFS requires an explicit root mount command, FTP does not
        if (scheme != "ftp") {
            if (!static_cast<TnfsClient*>(client)->mount("/")) {
                delete client;
                return QByteArray();
            }
        }

        quint32 totalSize = client->getFileSize(pathNoSlash);
        if (totalSize == 0) totalSize = client->getFileSize(pathWithSlash);

        quint8 handle = client->openFile(pathNoSlash);
        if (handle == 0xFF) handle = client->openFile(pathWithSlash);

        if (handle == 0xFF) {
            delete client;
            return QByteArray();
        }

        if (totalSize == 0) totalSize = client->getFileSize(handle);

        if (totalSize > 0) qDebug() << "!i" << m_driveIdentity + ":" << "Downloading" << pathNoSlash << "(" << totalSize << "bytes)...";
        else qDebug() << "!i" << m_driveIdentity + ":" << "Downloading" << pathNoSlash << "(Stream mode)...";

        QByteArray data;
        if (totalSize > 0) data.reserve(totalSize);
        quint32 offset = 0;

        emit this->downloadProgress(0, totalSize);

        while (true) {
            // Check for user cancel
            if (activeIdPtr && *activeIdPtr != myId) {
                qDebug() << "!w" << m_driveIdentity + ":" << "Download aborted by user.";
                client->closeFile(handle);
                delete client;
                return QByteArray();
            }

            // [PIPELINE ACTIVATION] Ask for 32KB chunks natively!
            QByteArray chunk = client->readFile(handle, offset, 32768);
            if (chunk.isEmpty()) break;

            data.append(chunk);
            offset += chunk.size();

            // Emit safely across the thread boundary to update the UI
            emit this->downloadProgress(offset, totalSize);

            // Hard limit to prevent memory exhaustion
            if (data.size() > 16 * 1024 * 1024) {
                qWarning() << "!e" << m_driveIdentity + ":" << "File too large (>16MB). Aborting.";
                client->closeFile(handle);
                delete client;
                return QByteArray();
            }
        }

        if (totalSize > 0) emit this->downloadProgress(totalSize, totalSize);
        else emit this->downloadProgress(data.size(), data.size());

        client->closeFile(handle);
        delete client;
        return data;
    });

    // --- EVENT LOOP BRIDGE ---
    // This blocks openUrl from returning while the background thread runs,
    // BUT allows the Qt UI to continue processing paints and progress bar updates cleanly.
    QFutureWatcher<QByteArray> watcher;
    QEventLoop loop;
    connect(&watcher, &QFutureWatcherBase::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(future);
    loop.exec();

    // Retrieve the downloaded data from the background thread
    m_imgData = watcher.result();

    if (m_imgData.isEmpty()) {
        qWarning() << "!e" << m_driveIdentity + ":" << "Failed to open or download:" << fullPath;
        QApplication::restoreOverrideCursor();
        return false;
    }

    this->m_originalFileName = url;
    qDebug() << "!n" << m_driveIdentity + ":" << "Download Complete. Size:" << m_imgData.size();

    // 1. ATX Format
    if (fullPath.endsWith(".atx", Qt::CaseInsensitive)) {
        if (!parseAtx()) {
            qWarning() << "!e" << m_driveIdentity + ":" << "Invalid ATX Header or Corrupt File.";
            QApplication::restoreOverrideCursor();
            return false;
        }
        m_isAtx = true;
        qDebug() << "!n" << m_driveIdentity + ":" << "ATX Protection Loaded.";
    }
    // 2. XEX Format
    else if (fullPath.endsWith(".xex", Qt::CaseInsensitive) || fullPath.endsWith(".exe", Qt::CaseInsensitive)) {
        if (!parseXex()) {
            qWarning() << "!e" << m_driveIdentity + ":" << "Invalid XEX Executable.";
            QApplication::restoreOverrideCursor();
            return false;
        }
        m_isXex = true;
        m_booterLoaded = false;
        qDebug() << "!n" << m_driveIdentity + ":" << "XEX Loader Prepared.";
    }
    // 3. Standard ATR Format
    else {
        m_headerSkip = 0;
        m_tnfsSectorSize = 128;

        if (m_imgData.size() >= 16) {
            quint16 magic = (quint8)m_imgData[0] + ((quint8)m_imgData[1] << 8);
            quint16 secSz = (quint8)m_imgData[4] + ((quint8)m_imgData[5] << 8);

            if (magic == 0x0296) {
                m_headerSkip = 16;
                m_tnfsSectorSize = secSz;
                qDebug() << "!n" << m_driveIdentity + ":" << "ATR Header Valid. Sector Size:" << m_tnfsSectorSize;
            }
        }
        if (m_imgData.size() < 128) {
            qWarning() << "!e" << m_driveIdentity + ":" << "Image too small!";
            QApplication::restoreOverrideCursor();
            return false;
        }
    }

    QApplication::restoreOverrideCursor();
    return true;
}

bool TnfsImage::openFromMemory(const QString &fileName, const QByteArray &data)
{
    m_driveIdentity = "Web Drop (RAM)";

    m_imgData.clear();
    m_bootSectors.clear();
    m_chunks.clear();
    m_isAtx = false;
    m_isXex = false;
    cleanupAtx();

    m_imgData = data;
    m_originalFileName = fileName;

    if (fileName.endsWith(".atx", Qt::CaseInsensitive)) {
        if (!parseAtx()) {
            qWarning() << "!e" << m_driveIdentity + ":" << "Invalid ATX Header or Corrupt File.";
            return false;
        }
        m_isAtx = true;
        qDebug() << "!n" << m_driveIdentity + ":" << "ATX Protection Loaded.";
    }
    else if (fileName.endsWith(".xex", Qt::CaseInsensitive) || fileName.endsWith(".com", Qt::CaseInsensitive)) {
        if (!parseXex()) {
            qWarning() << "!e" << m_driveIdentity + ":" << "Invalid XEX Executable.";
            return false;
        }
        m_isXex = true;
        m_booterLoaded = false;
        qDebug() << "!n" << m_driveIdentity + ":" << "XEX Loader Prepared.";
    }
    else {
        m_headerSkip = 0;
        m_tnfsSectorSize = 128;
        if (m_imgData.size() >= 16 && (quint8)m_imgData[0] == 0x96 && (quint8)m_imgData[1] == 0x02) {
            m_headerSkip = 16;
            quint16 secSize = (quint8)m_imgData[4] | ((quint8)m_imgData[5] << 8);
            m_tnfsSectorSize = (secSize == 256) ? 256 : 128;
        }
    }

    return true;
}

bool TnfsImage::parseXex()
{
    QFile boot(":/binaries/autoboot.bin");

    if (!boot.open(QFile::ReadOnly)) {
        qCritical() << "!e" << m_driveIdentity + ":" << "Missing internal resource 'autoboot.bin'!";
        return false;
    }
    m_bootSectors = boot.readAll();
    boot.close();

    int cursor = 0;
    int size = m_imgData.size();

    if (size < 2) return false;
    int start = (quint8)m_imgData[0] + (quint8)m_imgData[1] * 256;
    cursor += 2;

    if (start != 0xFFFF) {
        qCritical() << "!e" << m_driveIdentity + ":" << "XEX missing $FFFF header.";
        return false;
    }

    if (cursor + 2 > size) return false;
    start = (quint8)m_imgData[cursor] + (quint8)m_imgData[cursor+1] * 256;
    cursor += 2;

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
            TnfsExeChunk ch;
            int len = 0;
            if (i + maxChunkSize > segData.size()) len = segData.size() - i;
            else len = maxChunkSize;

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

// --- ATX PARSER ---
bool TnfsImage::parseAtx()
{
    if (m_imgData.size() < 48) return false;

    if (m_imgData[0] != 'A' || m_imgData[1] != 'T' || m_imgData[2] != '8' || m_imgData[3] != 'X') return false;

    atx.version = VAPI_16(m_imgData, 4);
    atx.start = VAPI_32(m_imgData, 28);

    quint8 density = VAPI_8(m_imgData, 18);
    m_spt = (density == 1) ? 26 : 18;

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
            atx.tracks[track].sectors[s].weakOffset = 0xFFFF;

            currentSectorPos += 8;
        }

        // VAPI Proper: Scan remaining chunks in this track for Extended Sector Info (0x08)
        quint32 currentChunkPos = sectorListPos + atx.tracks[track].sector_list_header.size;
        quint32 endOfTrack = start + atx.tracks[track].next;

        while (currentChunkPos < endOfTrack) {
            if (currentChunkPos + 8 > (quint32)m_imgData.size()) break;

            quint32 chunkSize = VAPI_32(m_imgData, currentChunkPos);
            quint8 chunkType = VAPI_8(m_imgData, currentChunkPos + 4);

            if (chunkType == 0x08) {
                quint32 extDataPos = currentChunkPos + 8;
                for (int s = 0; s < atx.tracks[track].numsectors; s++) {
                    if (extDataPos + 8 > (quint32)m_imgData.size()) break;

                    quint16 wOffset = VAPI_16(m_imgData, extDataPos + 6);
                    atx.tracks[track].sectors[s].weakOffset = wOffset;

                    // Clear the inverted 0x40 bit flag
                    if (wOffset != 0xFFFF) {
                        atx.tracks[track].sectors[s].status &= ~0x40;
                    }
                    extDataPos += 8;
                }
                break;
            }
            if (chunkSize == 0) break;
            currentChunkPos += chunkSize;
        }

        start += atx.tracks[track].next;
        track++;
    }

    m_driveTimer.start(); // Begin continuous 288 RPM spin
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
        if (!m_isXex || aux >= m_chunks.size()) {
            sio->port()->writeCommandNak();
            return;
        }
        sio->port()->writeCommandAck();
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(m_chunks.at(aux).data);
        qDebug() << "!n" << m_driveIdentity + ":" << "XEX Chunk" << aux << "sent.";
        return;
    }
    case 0xFF: // Get Chunk Info
    {
        if (!m_isXex || aux >= m_chunks.size()) {
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
        qDebug() << "!n" << m_driveIdentity + ":" << "XEX Boot Complete.";
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

        if (offset >= (quint32)m_bootSectors.size()) {
            data.fill(0, 128);
            return true;
        }

        int bytesAvailable = m_bootSectors.size() - offset;
        int bytesToRead = qMin(128, bytesAvailable);

        data = m_bootSectors.mid(offset, bytesToRead);

        if (data.size() < 128) {
            data.append(QByteArray(128 - data.size(), 0));
        }
        return true;
    }

    // --- ATX MODE ---
    if (m_isAtx) {
        bool result = readSectorAtx(sector, data);
        if (result) qDebug() << "!n" << m_driveIdentity + ":" << "Read ATX Sector" << sector;
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
        qWarning() << "!e" << m_driveIdentity + ":" << "Read past EOF. Sector:" << sector;
        return false;
    }

    data = m_imgData.mid(offset, bytesToRead);
    qDebug() << "!n" << m_driveIdentity + ":" << "Read Sector" << sector;
    return true;
}

// --- ATX HELPERS ---
bool TnfsImage::seekToSectorAtx(quint16 sector, quint32 &offset)
{
    int track = (sector - 1) / m_spt;
    int tracksector = (sector - 1) % m_spt;

    if (track >= 100) return false;

    int actualSectors = atx.tracks[track].numsectors;
    int matchedIndices[256];
    int matchCount = 0;

    for (int i = 0; i < actualSectors; i++) {
        if (atx.tracks[track].sectors[i].number == (tracksector + 1)) {
            matchedIndices[matchCount] = i;
            matchCount++;
        }
    }

    if (matchCount == 0) return false;

    // Geographic Closest Sector Logic
    double rotations = m_driveTimer.elapsed() / 208.3333333;
    quint16 current_angular_pos = (quint16)(std::fmod(rotations, 1.0) * 26042.0);

    int best_index = matchedIndices[0];
    int min_distance = 26043;

    for (int i = 0; i < matchCount; i++) {
        quint16 sec_pos = atx.tracks[track].sectors[matchedIndices[i]].position;

        int dist = (sec_pos >= current_angular_pos) ?
                       (sec_pos - current_angular_pos) :
                       (sec_pos + 26042 - current_angular_pos);

        if (dist < min_distance) {
            min_distance = dist;
            best_index = matchedIndices[i];
        }
    }

    int trackindex = best_index;

    offset = atx.tracks[track].pos + atx.tracks[track].sectors[trackindex].start;
    wd1772status = atx.tracks[track].sectors[trackindex].status;
    m_currentWeakOffset = atx.tracks[track].sectors[trackindex].weakOffset;
    m_targetAngularPosition = atx.tracks[track].sectors[trackindex].position;

    return true;
}

bool TnfsImage::readSectorAtx(quint16 sector, QByteArray &data)
{
    quint32 offset = 0;
    if (!seekToSectorAtx(sector, offset)) return false;

    // 288 RPM Rotational Delay
    double rotations = m_driveTimer.elapsed() / 208.3333333;
    quint16 current_angular_pos = (quint16)(std::fmod(rotations, 1.0) * 26042.0);

    int distance = (m_targetAngularPosition >= current_angular_pos) ?
                       (m_targetAngularPosition - current_angular_pos) :
                       (m_targetAngularPosition + 26042 - current_angular_pos);

    int expectedDelayMs = (int)((distance / 26042.0) * 208.333);

    if (expectedDelayMs > 0) {
        QThread::msleep(expectedDelayMs);
    }

    // Determine payload size based on density
    int bytesToRead = (m_spt == 18 && m_imgData[18] == 2) ? 256 : 128;
    data = m_imgData.mid(offset, bytesToRead);

    bool isWeak = false;

    if (m_currentWeakOffset != 0xFFFF && m_currentWeakOffset < data.size()) {
        for (int i = m_currentWeakOffset; i < data.size(); i++) {
            data[i] = QRandomGenerator::global()->generate() % 0xFF;
        }
        isWeak = true;
    }
    else if (wd1772status == 0xB7) {
        for (int i=0; i<data.size(); i++) {
            if (data[i] == '\x33') {
                data[i] = QRandomGenerator::global()->generate() % 0xFF;
            }
        }
        isWeak = true;
    }

    if (wd1772status != 0xff && !isWeak) {
        return false;
    }

    return true;
}