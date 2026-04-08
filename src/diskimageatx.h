#ifndef DISKIMAGEATX_H
#define DISKIMAGEATX_H

#include <QAction>
#include "diskimage.h"
#include <QElapsedTimer>

class DiskImageAtx : public SimpleDiskImage
{
    Q_OBJECT

public:
    DiskImageAtx(SioWorker *worker): SimpleDiskImage(worker) {}
    ~DiskImageAtx();

    void close();
    bool open(const QString &fileName, FileTypes::FileType /* type */);
    bool seekToSector(quint16 sector);
    bool readSector(quint16 sector, QByteArray &data);
    bool writeSector(quint16 sector, const QByteArray &data);
    void getStatus(QByteArray &status);
    bool format(quint16 aSectorCount, quint16 aSectorSize);

protected:
    struct atx_sector_list_header
    {
        quint32 size;
        quint8 type;
    };
    struct atx_sector
    {
        quint8 number;
        quint8 status;
        quint16 position;
        quint32 start;
        quint16 weakOffset; // Added for dynamic fuzzy sector support
    };
    struct atx_track_header
    {
        quint32 pos;
        quint32 next;
        quint16 type;
        quint8  track;
        quint16 numsectors;
        quint32 start;
        atx_sector_list_header sector_list_header;
        atx_sector *sectors;
    };
    struct atx_file
    {
        quint16 version;
        quint32 start;
        atx_track_header tracks[100];
    };

    QFile *sourceFile;
    quint8 count[1040];
    quint8 wd1772status;
    quint16 m_currentWeakOffset; // Stores weak offset of currently seeked sector
    quint16 m_targetAngularPosition; // Replaces the old duplicate/sequential counters
    atx_file atx;
    QElapsedTimer m_driveTimer;
};

#endif // DISKIMAGEATX_H
