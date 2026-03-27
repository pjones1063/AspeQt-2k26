#ifndef XEXIMAGE_H
#define XEXIMAGE_H

#include "sioworker.h"
#include <QByteArray>
#include <QList>
#include <QString>

// Structure to hold the executable chunks
struct LocalExeChunk {
    int address;
    QByteArray data;
};

class XexImage : public SioDevice
{
    Q_OBJECT
public:
    explicit XexImage(SioWorker *worker);
    virtual ~XexImage() override;

    // Opens a local file from the PC hard drive
    bool openLocalFile(const QString &filePath);
    QString originalFileName() const { return m_originalFileName; }

    // Standard SIO Interface overrides
    virtual void handleCommand(quint8 command, quint16 aux) override;
    virtual QString deviceName() override { return "Local XEX Booter"; }

signals:
    void downloadProgress(qint64 bytesRead, qint64 totalBytes);

private:
    QByteArray m_imgData;
    QString m_originalFileName;
    QByteArray m_bootSectors;      // The internal AspeQt "Micro-DOS" loader
    QList<LocalExeChunk> m_chunks; // The broken-down executable parts
    bool parseXex();

};

#endif // XEXIMAGE_H
