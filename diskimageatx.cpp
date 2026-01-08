#include "diskimageatx.h"
#include <time.h>
#include <sys/time.h>

#include <QtDebug>

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
    return false;
}

bool DiskImageAtx::open(const QString &fileName, FileTypes::FileType /* type */)
{
    phantomflip = 0;
    if (m_originalImageType == FileTypes::Atr) {
        sourceFile = new QFile(fileName);
    } else {
        sourceFile = new GzFile(fileName);
    }

    // Try to open the source file
    if (!sourceFile->open(QFile::Unbuffered | QFile::ReadOnly)) {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(sourceFile->errorString());
        delete sourceFile;
        return false;
    }

    // Try to read the header
    QByteArray header;
    header = sourceFile->read(48);
    if (header.size() != 48) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Cannot read the header: %1.").arg(sourceFile->errorString()));
        delete sourceFile;
        return false;
    }

    // Validate the magic number
    //quint16 magic = ((quint8)header[0]) * 256 + ((quint8)header[1]);
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
    while (start < sourceFile->size())
    {
        sourceFile->seek(start);
        header = sourceFile->read(32);
        atx.tracks[track].pos     = start;
        atx.tracks[track].next    = VAPI_32(header,  0);
        atx.tracks[track].type    = VAPI_16(header,  4);
        atx.tracks[track].track   = VAPI_8 (header,  8);
        atx.tracks[track].numsectors = VAPI_16(header, 10);
        atx.tracks[track].start   = VAPI_32(header, 20);

        sourceFile->seek(start+atx.tracks[track].start);
        header = sourceFile->read(8);
        atx.tracks[track].sector_list_header.size = VAPI_32(header, 0);
        atx.tracks[track].sector_list_header.type = VAPI_8 (header, 4);

        atx.tracks[track].sectors = new atx_sector[atx.tracks[track].numsectors];
        for (sector = 0; sector < atx.tracks[track].numsectors; sector++)
        {
            header = sourceFile->read(8);
            atx.tracks[track].sectors[sector].number   =  VAPI_8 (header, 0);
            atx.tracks[track].sectors[sector].status   = ~VAPI_8 (header, 1);
            atx.tracks[track].sectors[sector].position =  VAPI_16(header, 2);
            atx.tracks[track].sectors[sector].start    =  VAPI_32(header, 4);

            qDebug() << "!d" << tr("  sector number=%1 status=%2 position=%3 start=%4")
                .arg(atx.tracks[track].sectors[sector].number)
                .arg(atx.tracks[track].sectors[sector].status)
                .arg(atx.tracks[track].sectors[sector].position)
                .arg(atx.tracks[track].sectors[sector].start);
        }

        sectorcount += atx.tracks[track].numsectors;
        start += atx.tracks[track].next;
        track++;
    }
    qDebug() << "!i" << tr("Tracks=%1 Sectors=%2")
        .arg(track)
        .arg(sectorcount);

    // Validate disk geometry
    DiskGeometry geometry;
    geometry.initialize(0, track, sectorcount/track, 128);
    if (geometry.sectorCount() > 65535) {
        qCritical() << "!e" << tr("Cannot open '%1': %2")
                      .arg(fileName)
                      .arg(tr("Too many sectors in the image (%1).").arg(geometry.sectorCount()));
        file.close();
        delete sourceFile;
        return false;
    }

    // Update the image information
    m_geometry.initialize(geometry);
    refreshNewGeometry();
    m_isReadOnly = true;
    m_originalFileName = fileName;
    m_originalFileHeader = header;
    m_isModified = false;
    m_isUnmodifiable = true;
    m_isUnnamed = false;

    return true;
}

bool DiskImageAtx::seekToSector(quint16 sector)
{
    quint8 track, tracksector, trackindex, tracktemp;
    trackindex = 0; // Ray A.
    if (sector < 1 || sector > m_geometry.sectorCount()) {
        qCritical() << "!e" << tr("[%1] Cannot seek to sector %2: %3")
                       .arg(deviceName())
                       .arg(sector)
           .arg(tr("Sector number is out of bounds."));
    }

    track = (sector-1)/18; //m_geometry.sectorsPerTrack();
    tracksector = (sector-1)%18; //m_geometry.sectorsPerTrack();

    // because we don't calc timings we return first and last sector on each read. KP
    for (tracktemp = 0; tracktemp <= 17; tracktemp++) {
      if (atx.tracks[track].sectors[tracktemp].number == (tracksector+1))
      {
    trackindex = tracktemp;
    if (phantomflip)
      break;
      }
    }

    phantomflip = !phantomflip;
    qint64 pos = (atx.tracks[track].pos + atx.tracks[track].sectors[trackindex].start);
    wd1772status = atx.tracks[track].sectors[trackindex].status;
    lastsector = sector;

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

    if (wd1772status != 0xff)
    {
        qDebug() << "!e" << tr("Bad sector");

    data = sourceFile->read(m_geometry.bytesPerSector(sector));
    // return random data on error like in Atari800 - boots Zorro. :) KP
    int i;
    if (wd1772status == 0xB7) {
        for (i=0;i<128;i++) {
        //qDebug() << "!e" << tr("0x%02x").arg(data[i]);
                if (data[i] == '\x33')
            data[i] = rand() & 0xFF;
        }
        return true;
    }
    return false;
    }

    data = sourceFile->read(m_geometry.bytesPerSector(sector));
    if (data.size() != m_geometry.bytesPerSector(sector)) {
        qCritical() << "!e" << tr("[%1] Cannot read from sector %2: %3.")
                       .arg(deviceName())
                       .arg(sector)
                       .arg(sourceFile->errorString());
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
