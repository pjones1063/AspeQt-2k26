#ifndef DISKIMAGEPRO_H
#define DISKIMAGEPRO_H

#include "diskimage.h"
#include <QElapsedTimer>

class DiskImagePro : public SimpleDiskImage
{
    Q_OBJECT

public:
    DiskImagePro(SioWorker *worker): SimpleDiskImage(worker) {}
    ~DiskImagePro();

    void close();
    bool open(const QString &fileName, FileTypes::FileType /* type */);
    bool seekToSector(quint16 sector);
    bool readSector(quint16 sector, QByteArray &data);
    bool writeSector(quint16 sector, const QByteArray &data);
    void getStatus(QByteArray &status);
    bool format(const DiskGeometry& );
protected:
    QFile *sourceFile;
    quint8 count[2048]; // Expanded to prevent off-by-one crashes on 1040 ED sectors
    quint8 wd1772status;
    QElapsedTimer m_driveTimer;
    int m_lastRequestedSector;
};

#endif // DISKIMAGEPRO_H
