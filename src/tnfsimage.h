#ifndef TNFSIMAGE_H
#define TNFSIMAGE_H

#include "sioworker.h"
#include <QByteArray>
#include <QList>
#include <QString>
#include <QElapsedTimer>
#include <QCoreApplication>

// --- HELPER MACROS FOR ATX PARSING ---
#define VAPI_32(x, y) ((quint8)x[y] + ((quint8)x[y+1] << 8) + ((quint8)x[y+2] << 16) + ((quint8)x[y+3] << 24))
#define VAPI_16(x, y) ((quint8)x[y] + ((quint8)x[y+1] << 8))
#define VAPI_8(x, y) ((quint8)x[y])

// --- XEX CHUNK STRUCTURE ---
struct TnfsExeChunk {
    int address;
    QByteArray data;
};

class TnfsImage : public SioDevice
{
    Q_OBJECT
public:
    explicit TnfsImage(SioWorker *worker);
    virtual ~TnfsImage() override;

    // Downloads to RAM, Detects Format, Prepares Loader
    bool openUrl(const QString &url, volatile int *activeIdPtr = nullptr, int myId = 0);

    // Directly load a payload into RAM (For Web UI Drops)
    bool openFromMemory(const QString &fileName, const QByteArray &data);

    QString originalFileName() const { return m_originalFileName; }

    // SIO Interface
    virtual void handleCommand(quint8 command, quint16 aux) override;

    // Dynamic Name Tag
    virtual QString deviceName() override { return m_driveIdentity; }

signals:
    void downloadProgress(qint64 bytesRead, qint64 totalBytes);

private:
    // Core Data
    QByteArray m_imgData;
    QString m_originalFileName;
    QString m_driveIdentity;

    // Format Flags
    bool m_isAtx;
    bool m_isXex;

    // --- XEX SPECIFIC ---
    QByteArray m_bootSectors;
    QList<TnfsExeChunk> m_chunks;
    bool m_booterLoaded;

    // --- ATR SPECIFIC ---
    int m_headerSkip;
    int m_tnfsSectorSize;

    // --- ATX / VAPI DATA STRUCTURES ---
    struct atx_sector_list_header { quint32 size; quint8 type; };
    struct atx_sector {
        quint8 number;
        quint8 status;
        quint16 position;
        quint32 start;
        quint16 weakOffset;
    };
    struct atx_track_header {
        quint32 pos; quint32 next; quint16 type; quint8 track; quint16 numsectors; quint32 start;
        atx_sector_list_header sector_list_header; atx_sector *sectors;
    };
    struct atx_file { quint16 version; quint32 start; atx_track_header tracks[100]; };

    atx_file atx;
    quint8 wd1772status;

    // Hardware Emulation State
    int m_spt; // Sectors per track
    quint16 m_currentWeakOffset;
    quint16 m_targetAngularPosition;
    QElapsedTimer m_driveTimer;

    // --- METHODS ---
    void sendStatus();
    void cleanupAtx();

    // SIO Read Logic
    bool readSector(quint16 sector, QByteArray &data);

    // Format Parsers
    bool parseAtx();
    bool parseXex();

    // ATX Helpers
    bool seekToSectorAtx(quint16 sector, quint32 &offset);
    bool readSectorAtx(quint16 sector, QByteArray &data);
};

#endif
