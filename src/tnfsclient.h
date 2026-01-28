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

    enum TnfsCommands {
        CMD_MOUNT    = 0x00, // <--- REVERT THIS TO 0x00
        CMD_UMOUNT   = 0x01, // Unmount is 0x01
        CMD_OPENDIR  = 0x10,
        CMD_READDIR  = 0x11,
        CMD_CLOSEDIR = 0x12,
        CMD_OPEN     = 0x24,
        CMD_READ     = 0x25,
        CMD_CLOSE    = 0x26, // Close file (often 0x26 or 0x27)
        CMD_STAT     = 0x27
    };


    bool connectToHost(const QString &host, quint16 port = 16384);
    bool mount(const QString &remotePath);
    QList<DirectoryEntry> listDirectory(const QString &path);

    quint8 openFile(const QString &path);
    QByteArray readFile(quint8 handle, quint32 offset, quint16 size);
    void closeFile(quint8 handle);

private:
    QUdpSocket *socket;
    QHostAddress serverAddr;
    quint16 serverPort;
    QMutex netMutex;

    quint16 m_sessionId = 0;
    quint8 m_sequence = 0;

    QByteArray sendCommand(quint8 cmd, const QByteArray &data);
};

#endif
