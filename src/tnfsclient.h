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
        CMD_OPEN     = 0x29, // Corrected from 0x24
        CMD_LSEEK    = 0x24  // 0x24 is actually LSEEK
    };

    bool connectToHost(const QString &host, quint16 port = 16384);
    bool mount(const QString &remotePath);
    QList<DirectoryEntry> listDirectory(const QString &path);

    quint8 openFile(const QString &path);
    Q_INVOKABLE QByteArray readFile(quint8 handle, quint32 offset, quint16 size);
    Q_INVOKABLE void closeFile(quint8 handle);

private:
    QUdpSocket *socket;
    QHostAddress serverAddr;
    quint16 serverPort;
    QMutex netMutex;

    // The Session ID assigned by the server
    quint16 m_sessionId = 0;
    // Sequence number for UDP packet tracking
    quint8 m_sequence = 0;

    QByteArray sendCommand(quint8 cmd, const QByteArray &data);
};

#endif
