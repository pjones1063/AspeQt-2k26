#ifndef TNFSIMAGE_H
#define TNFSIMAGE_H

#include "diskimage.h"
#include "tnfsclient.h"

class TnfsImage : public SimpleDiskImage
{
    Q_OBJECT
public:
    explicit TnfsImage(SioWorker *worker);
    virtual ~TnfsImage() override;

    bool openUrl(const QString &url);
    virtual bool readSector(quint16 sector, QByteArray &data) override;
    virtual QString deviceName() override { return "TNFS"; }

private:
    TnfsClient *m_client;
    quint8 m_fileHandle;
    QString m_host;
    SioWorker *m_worker;

    int m_headerSkip; // <--- ADD THIS VARIABLE
};

#endif
