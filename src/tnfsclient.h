/*
 * tnfsclient.h
 */
#ifndef TNFSCLIENT_H
#define TNFSCLIENT_H

#include <QObject>
#include <QUdpSocket>
#include <QMutex>
#include <QList>

class TnfsClient : public QObject
{
    Q_OBJECT
public:
    explicit TnfsClient(QObject *parent = nullptr);
    ~TnfsClient();

    struct DirectoryEntry {
        QString name;
        bool isDirectory;
    };

    enum SeekMode {
        TnfsSeekSet = 0x00,
        TnfsSeekCur = 0x01,
        TnfsSeekEnd = 0x02
    };

    // --- STANDARD TNFS PROTOCOL OPCODES ---
    enum TnfsCommands {
        CMD_MOUNT    = 0x00,
        CMD_UMOUNT   = 0x01,
        CMD_OPENDIR  = 0x10,
        CMD_READDIR  = 0x11,
        CMD_CLOSEDIR = 0x12,
        CMD_STAT     = 0x20,
        CMD_READ     = 0x21,
        CMD_WRITE    = 0x23,
        CMD_CLOSE    = 0x22,
        CMD_OPEN     = 0x29,
        CMD_LSEEK    = 0x24
    };

    bool connectToHost(const QString &host, quint16 port = 16384);
    bool mount(const QString &remotePath);

    // --- OLD METHOD (Can remove if you fully replaced it, or keep for compatibility) ---
    QList<DirectoryEntry> listDirectory(const QString &path);

    // --- NEW PAGINATION API ---
    bool beginListing(const QString &path);
    QList<DirectoryEntry> fetchNextBatch(int count);
    void endListing();
    bool isListingFinished() const { return m_listingFinished; }
    quint32 getFileSize(const QString &path);
    quint32 getFileSize(quint8 handle);
    quint8 openFile(const QString &path);
    Q_INVOKABLE QByteArray readFile(quint8 handle, quint32 offset, quint16 size);
    Q_INVOKABLE void closeFile(quint8 handle);

private:
    QUdpSocket *socket;
    QHostAddress serverAddr;
    quint16 serverPort;
    QMutex netMutex;

    quint16 m_sessionId = 0;
    quint8 m_sequence = 0;

    // --- PAGINATION STATE ---
    quint8 m_dirHandle = 0xFF;
    bool m_listingFinished = false; // *** THIS WAS MISSING ***

    QByteArray sendCommand(quint8 cmd, const QByteArray &data);
};

#endif
