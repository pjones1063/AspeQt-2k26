#include "diskimageatx.h"
#include <QtDebug>
#include <QThread>
#include <cmath>
#include <QRandomGenerator>

#define VAPI_32(x, y) ((quint8)x[y] + ((quint8)x[y+1] << 8) + ((quint8)x[y+2] << 16) + ((quint8)x[y+3] << 24))
#define VAPI_16(x, y) ((quint8)x[y] + ((quint8)x[y+1] << 8))
#define VAPI_8(x, y) ((quint8)x[y])

DiskImageAtx::~DiskImageAtx()
{
    close();
}

void DiskImageAtx::close()
{
}

bool DiskImageAtx::format(quint16, quint16)
{
    // ATX images typically cannot be formatted via SIO
    return false;
}

bool DiskImageAtx::open(const QString &fileName, FileTypes::FileType /* type */)
{
    m_currentWeakOffset = 0xFFFF;
    m_targetAngularPosition = 0;

    if (m_originalImageType == FileTypes::Atr) {
        sourceFile = new QFile(fileName);
    } else {
        sourceFile = new GzFile(fileName);
    }

    if (!sourceFile->open(QFile::Unbuffered | QFile::ReadOnly)) {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(sourceFile->errorString());
        delete sourceFile;
        return false;
    }

    QByteArray header = sourceFile->read(48);
    if (header.size() != 48) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
        .arg(fileName)
            .arg(tr("Cannot read the header: %1.").arg(sourceFile->errorString()));
        delete sourceFile;
        return false;
    }

    if (header[0] != 'A' || header[1] != 'T' || header[2] != '8' || header[3] != 'X') {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(tr("Not a valid ATX file."));
        delete sourceFile;
        return false;
    }

    atx.version = VAPI_16(header, 4);
    atx.start = VAPI_32(header, 28);
    qDebug() << "!i" << tr("VAPI version %1: %2").arg(atx.version).arg(atx.start);

    quint32 start = atx.start;
    quint8 track = 0;
    quint8 sector;
    quint16 sectorcount = 0;

    while (start < sourceFile->size() && track < 100)
    {
        sourceFile->seek(start);
        QByteArray chunkHeader = sourceFile->read(32);
        atx.tracks[track].pos     = start;
        atx.tracks[track].next    = VAPI_32(chunkHeader,  0);
        atx.tracks[track].type    = VAPI_16(chunkHeader,  4);
        atx.tracks[track].track   = VAPI_8 (chunkHeader,  8);
        atx.tracks[track].numsectors = VAPI_16(chunkHeader, 10);
        atx.tracks[track].start   = VAPI_32(chunkHeader, 20);

        sourceFile->seek(start+atx.tracks[track].start);
        chunkHeader = sourceFile->read(8);
        atx.tracks[track].sector_list_header.size = VAPI_32(chunkHeader, 0);
        atx.tracks[track].sector_list_header.type = VAPI_8 (chunkHeader, 4);

        atx.tracks[track].sectors = new atx_sector[atx.tracks[track].numsectors];
        for (sector = 0; sector < atx.tracks[track].numsectors; sector++)
        {
            chunkHeader = sourceFile->read(8);
            atx.tracks[track].sectors[sector].number   =  VAPI_8 (chunkHeader, 0);
            atx.tracks[track].sectors[sector].status   = ~VAPI_8 (chunkHeader, 1);
            atx.tracks[track].sectors[sector].position =  VAPI_16(chunkHeader, 2);
            atx.tracks[track].sectors[sector].start    =  VAPI_32(chunkHeader, 4);
            atx.tracks[track].sectors[sector].weakOffset = 0xFFFF;
        }

        quint32 currentChunkPos = start + atx.tracks[track].start + atx.tracks[track].sector_list_header.size;
        quint32 endOfTrack = start + atx.tracks[track].next;

        while (currentChunkPos < endOfTrack) {
            sourceFile->seek(currentChunkPos);
            chunkHeader = sourceFile->read(8);
            if (chunkHeader.size() < 8) break;

            quint32 chunkSize = VAPI_32(chunkHeader, 0);
            quint8 chunkType = VAPI_8(chunkHeader, 4);

            if (chunkType == 0x08) {
                for (sector = 0; sector < atx.tracks[track].numsectors; sector++) {
                    QByteArray extData = sourceFile->read(8);
                    if (extData.size() == 8) {
                        quint16 wOffset = VAPI_16(extData, 6);
                        atx.tracks[track].sectors[sector].weakOffset = wOffset;

                        // Hardware Accuracy: ATX stores status INVERTED.
                        // Setting bit 6 (0x40) in real FDC means clearing it here.
                        if (wOffset != 0xFFFF) {
                            atx.tracks[track].sectors[sector].status &= ~0x40;
                        }
                    }
                }
                break;
            }
            if (chunkSize == 0) break;
            currentChunkPos += chunkSize;
        }

        sectorcount += atx.tracks[track].numsectors;
        start += atx.tracks[track].next;
        track++;
    }

    if (track == 0) track = 1;

    qDebug() << "!i" << tr("Tracks=%1 Sectors=%2")
                            .arg(track)
                            .arg(sectorcount);

    // CRITICAL FIX: Read explicit density from ATX Header Byte 18
    // 0 = Single, 1 = Enhanced, 2 = Double
    quint8 density = VAPI_8(header, 18);
    int spt = (density == 1) ? 26 : 18;
    int bps = (density == 2) ? 256 : 128;

    DiskGeometry geometry;
    geometry.initialize(0, track, spt, bps);

    if (geometry.sectorCount() > 65535) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
        .arg(fileName)
            .arg(tr("Too many sectors in the image (%1).").arg(geometry.sectorCount()));
        delete sourceFile;
        return false;
    }

    m_geometry.initialize(geometry);
    refreshNewGeometry();
    m_isReadOnly = true;
    m_originalFileName = fileName;
    m_originalFileHeader = header;
    m_isModified = false;
    m_isUnmodifiable = true;
    m_isUnnamed = false;

    m_driveTimer.start();

    return true;
}

bool DiskImageAtx::seekToSector(quint16 sector)
{
    quint8 track, tracksector, tracktemp;
    int matchedIndices[256];
    int matchCount = 0;

    if (sector < 1 || sector > m_geometry.sectorCount()) {
        qCritical() << "!e" << tr("[%1] Cannot seek to sector %2: %3")
        .arg(deviceName())
            .arg(sector)
            .arg(tr("Sector number is out of bounds."));
        return false;
    }

    quint16 spt = m_geometry.sectorsPerTrack();
    if (spt == 0) spt = 18;

    track = (sector - 1) / spt;
    tracksector = (sector - 1) % spt;

    if (track >= 100) {
        qCritical() << "!e" << tr("[%1] Track %2 out of bounds").arg(deviceName()).arg(track);
        return false;
    }

    int actualSectorsInTrack = atx.tracks[track].numsectors;

    for (tracktemp = 0; tracktemp < actualSectorsInTrack; tracktemp++) {
        if (atx.tracks[track].sectors[tracktemp].number == (tracksector + 1)) {
            matchedIndices[matchCount] = tracktemp;
            matchCount++;
        }
    }

    if (matchCount == 0) {
        return false;
    }

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

    quint8 trackindex = best_index;

    qint64 pos = (atx.tracks[track].pos + atx.tracks[track].sectors[trackindex].start);
    wd1772status = atx.tracks[track].sectors[trackindex].status;
    m_currentWeakOffset = atx.tracks[track].sectors[trackindex].weakOffset;

    m_targetAngularPosition = atx.tracks[track].sectors[trackindex].position;

    if (!sourceFile->seek(pos)) {
        qCritical() << "!e" << tr("[%1] Cannot seek to sector %2: %3")
        .arg(deviceName())
            .arg(sector)
            .arg(sourceFile->error());
        return false;
    }
    return true;
}

bool DiskImageAtx::readSector(quint16 sector, QByteArray &data)
{
    if (!seekToSector(sector)) {
        return false;
    }

    double rotations = m_driveTimer.elapsed() / 208.3333333;
    quint16 current_angular_pos = (quint16)(std::fmod(rotations, 1.0) * 26042.0);

    int distance = (m_targetAngularPosition >= current_angular_pos) ?
                       (m_targetAngularPosition - current_angular_pos) :
                       (m_targetAngularPosition + 26042 - current_angular_pos);

    int expectedDelayMs = (int)((distance / 26042.0) * 208.333);

    if (expectedDelayMs > 0) {
        QThread::msleep(expectedDelayMs);
    }

    data = sourceFile->read(m_geometry.bytesPerSector(sector));

    if (data.size() != m_geometry.bytesPerSector(sector)) {
        qCritical() << "!e" << tr("[%1] Cannot read from sector %2: %3.")
        .arg(deviceName())
            .arg(sector)
            .arg(sourceFile->errorString());
        return false;
    }

    bool isWeak = false;

    if (m_currentWeakOffset != 0xFFFF && m_currentWeakOffset < data.size()) {
        for (int i = m_currentWeakOffset; i < data.size(); i++) {
            data[i] = QRandomGenerator::global()->generate() % 0xFF;
        }
        isWeak = true;
    }
    else if (wd1772status == 0xB7) {
        for (int i=0; i<128; i++) {
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

bool DiskImageAtx::writeSector(quint16, const QByteArray &)
{
    return false;
}

void DiskImageAtx::getStatus(QByteArray &status)
{
    status[0] = m_isReadOnly * 8 |
                (m_newGeometry.bytesPerSector() == 256) * 32 |
                (m_newGeometry.bytesPerSector() == 128 && m_newGeometry.sectorsPerTrack() == 26) * 128;
    status[1] = wd1772status;
    status[2] = 3;
    status[3] = 0;
}
