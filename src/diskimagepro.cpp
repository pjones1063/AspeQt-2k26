#include "diskimagepro.h"
#include <QtDebug>
#include <QThread>

DiskImagePro::~DiskImagePro()
{
    close();
}

void DiskImagePro::close()
{
}

bool DiskImagePro::format(const DiskGeometry&)
{
    return false;
}

bool DiskImagePro::open(const QString &fileName, FileTypes::FileType /* type */)
{
    m_lastRequestedSector = -1;

    if (m_originalImageType == FileTypes::Atr) {
        sourceFile = new QFile(fileName);
    } else {
        sourceFile = new GzFile(fileName);
    }

    if (!sourceFile->open(QFile::Unbuffered | QFile::ReadOnly)) {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName, sourceFile->errorString());
        delete sourceFile;
        return false;
    }

    QByteArray header = sourceFile->read(16);
    if (header.size() != 16) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
        .arg(fileName)
            .arg(tr("Cannot read the header: %1.").arg(sourceFile->errorString()));
        delete sourceFile;
        return false;
    }

    quint16 magic = ((quint8)header[0]) * 256 + ((quint8)header[1]);
    quint16 maxPhysicalSectors = (sourceFile->size() - 16) / 140;

    if (magic != maxPhysicalSectors) {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName, tr("Not a valid PRO file."));
        delete sourceFile;
        return false;
    }

    // Lock logical geometry to standard sizes.
    // PRO files store duplicates physically past these logical boundaries.
    int logicalSectors = 720;
    int spt = 18;
    if (magic >= 1040) {
        logicalSectors = 1040;
        spt = 26;
    }

    DiskGeometry geometry;
    geometry.initialize(0, logicalSectors / spt, spt, 128);

    m_geometry.initialize(geometry);
    refreshNewGeometry();
    m_isReadOnly = true;
    m_originalFileName = fileName;
    m_originalFileHeader = header;
    m_isModified = false;
    m_isUnmodifiable = true;
    m_isUnnamed = false;

    for (int i=0; i<2048; i++) count[i] = 0;

    m_driveTimer.start();

    return true;
}

bool DiskImagePro::seekToSector(quint16 sector)
{
    // Bounds check against the PHYSICAL size of the file, not logical geometry.
    quint16 maxPhysicalSectors = (sourceFile->size() - 16) / 140;

    if (sector < 1 || sector > maxPhysicalSectors) {
        qCritical() << "!e" << tr("[%1] Cannot seek to sector %2: out of bounds.")
        .arg(deviceName()).arg(sector);
        return false;
    }

    qint64 pos = 16 + (sector - 1) * 140;
    if (!sourceFile->seek(pos)) {
        qCritical() << "!e" << tr("[%1] Cannot seek to sector %2: %3")
        .arg(deviceName())
            .arg(sector)
            .arg(sourceFile->errorString());
        return false;
    }
    return true;
}

bool DiskImagePro::readSector(quint16 sector, QByteArray &data)
{
    int previousSector = m_lastRequestedSector;
    m_lastRequestedSector = sector;

    if (!seekToSector(sector)) return false;

    QByteArray header = sourceFile->read(12);

    // PREVENT CRASH: Ensure header was successfully read before accessing indices
    if (header.size() < 12) return false;

    if (header[5] != 0 && sector < 2048) {
        quint8 dupnum = (count[sector] + 1) % ((quint8)header[5] + 1);
        count[sector] = dupnum;

        if (dupnum != 0) {
            quint16 dupSector = m_geometry.sectorCount() + (quint8)header[6 + dupnum];

            if (dupnum > 4 || dupSector <= 0 || dupSector > ((sourceFile->size() - 16) / 140)) {
                qCritical() << "!e" << tr("Error in .pro image: sector: %1 dupnum: %2").arg(sector).arg(dupnum);
                return false;
            }

            if (!seekToSector(dupSector)) return false;

            header = sourceFile->read(12);
            // PREVENT CRASH: Verify the duplicate sector header read
            if (header.size() < 12) return false;
        }
    }

    // 288 RPM Rotational Delay Emulation
    int spt = m_geometry.sectorsPerTrack();
    if (spt == 0) spt = 18;
    float msPerSector = 208.33f / spt;
    int sectorsPassed = 1;

    if (previousSector != -1) {
        sectorsPassed = sector - previousSector;
        if (sectorsPassed < 0) {
            sectorsPassed += spt;
        }
    }

    int expectedDelayMs = static_cast<int>(sectorsPassed * msPerSector);
    qint64 elapsed = m_driveTimer.elapsed();

    if (elapsed < expectedDelayMs) {
        QThread::msleep(expectedDelayMs - elapsed);
    }
    m_driveTimer.restart();

    wd1772status = header[1];

    // Read the data payload
    data = sourceFile->read(m_geometry.bytesPerSector(sector));

    if (data.size() != m_geometry.bytesPerSector(sector)) {
        qCritical() << "!e" << tr("[%1] Cannot read from sector %2: %3.")
        .arg(deviceName())
            .arg(sector)
            .arg(sourceFile->errorString());
        return false;
    }

    if (wd1772status != 0xff) {
        return false;
    }

    return true;
}

bool DiskImagePro::writeSector(quint16, const QByteArray &)
{
    return false;
}

void DiskImagePro::getStatus(QByteArray &status)
{
    status[0] = m_isReadOnly * 8 |
                (m_newGeometry.bytesPerSector() == 256) * 32 |
                (m_newGeometry.bytesPerSector() == 128 && m_newGeometry.sectorsPerTrack() == 26) * 128;
    status[1] = wd1772status;
    status[2] = 3;
    status[3] = 0;
}
