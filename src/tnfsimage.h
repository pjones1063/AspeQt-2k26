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

    // The entry point called by MainWindow
    bool openUrl(const QString &url);

    // --- EXACT OVERRIDES FROM diskimage.h ---
    // These must match line 98 of your uploaded file
    virtual bool readSector(quint16 sector, QByteArray &data) override;

    // deviceName() isn't in SimpleDiskImage, but it's usually in SioDevice
    virtual QString deviceName() override { return "TNFS"; }

private:
    TnfsClient m_client;
    quint8 m_fileHandle;
    QString m_host;
    SioWorker *m_worker;
};

#endif
