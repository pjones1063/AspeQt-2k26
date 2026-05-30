/*
 * tnfsclient.h
 */
#ifndef TNFSCLIENT_H
#define TNFSCLIENT_H

#include "inetworkclient.h"
#include <QUdpSocket>
#include <QMutex>
#include <QList>

class TnfsClient : public INetworkClient
{
    Q_OBJECT
public:
    explicit TnfsClient(QObject *parent = nullptr);
    ~TnfsClient() override;

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

    bool connectToHost(const QString &host, quint16 port = 16384) override;

    // Mount is specific to TNFS, not in the universal interface, but kept for compatibility
    bool mount(const QString &remotePath);

    QList<DirectoryEntry> listDirectory(const QString &path);

    bool beginListing(const QString &path) override;
    QList<DirectoryEntry> fetchNextBatch(int count) override;
    void endListing() override;
    bool isListingFinished() const override { return m_listingFinished; }

    quint32 getFileSize(const QString &path) override;
    quint32 getFileSize(quint8 handle) override;
    quint8 openFile(const QString &path) override;

    Q_INVOKABLE QByteArray readFile(quint8 handle, quint32 offset, quint32 size) override;
    Q_INVOKABLE void closeFile(quint8 handle) override;

private:
    QUdpSocket *socket;
    QHostAddress serverAddr;
    quint16 serverPort;
    QMutex netMutex;

    quint16 m_sessionId = 0;
    quint8 m_sequence = 0;

    quint8 m_dirHandle = 0xFF;
    bool m_listingFinished = false;

    QByteArray sendCommand(quint8 cmd, const QByteArray &data);
};

#endif // TNFSCLIENT_H
