#ifndef RDEVICE_H
#define RDEVICE_H

#include "sioworker.h"
#include <QTcpSocket>
#include <QTcpServer>
#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QQueue>
#include <QMutex>
#include "bbsdata.h"
#include "sshclient.h"

#define CMD_RELOCATOR    0x21
#define CMD_DOWNLOAD     0x26
#define CMD_POLL_TYPE1   0x3F
#define CMD_POLL_TYPE3   0x40
#define CMD_CONTROL      0x41
#define CMD_CONFIGURE    0x42
#define CMD_LISTEN       0x4C
#define CMD_UNLISTEN     0x4D
#define CMD_AUTOANSWER   0x4F
#define CMD_STATUS       0x53
#define CMD_WRITE        0x57
#define CMD_READ         0x52
#define CMD_STREAM       0x58

class RDevice : public SioDevice
{
    Q_OBJECT

public:
    explicit RDevice(SioWorker *worker);
    ~RDevice() override;

    void handleCommand(quint8 command, quint16 aux) override;
    QString deviceName() override { return "R: Device (850 Emulation)"; }
    void setEnabled(bool enable);
    bool isEnabled() const { return m_isEnabled; }
    void loadPhonebook(const QString &path);
    void forceCommandMode();
    QByteArray dequeueNetworkData();
    void processSerialData(const QByteArray &data);

    // --- THE MISSING SIGNALS BLOCK ---
signals:
    void dispatchToNetwork(const QByteArray &data);
    void executeAtCommand(const QString &cmd);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError);
    void onNewConnection();

    void onSshConnected();
    void onSshDisconnected();
    void onSshDataReceived(const QByteArray &data);
    void onSshError(const QString &msg);

private:
    enum class ModemState { CommandMode, StreamMode };
    enum class TelnetState { Normal, IacReceived, Will, Wont, Do, Dont, SubNegotiation, SubIac };

    ModemState state = ModemState::CommandMode;
    TelnetState m_telnetState = TelnetState::Normal;
    bool m_isEnabled;
    bool m_isSshMode = false;

    QByteArray m_txBuffer;
    QString m_atCmdBuffer;

    QByteArray m_networkToSioBuffer;
    QMutex m_bufferMutex;

    QTcpSocket *tcpSocket;
    QTcpServer *tcpServer;
    QTcpSocket *pendingSocket;
    SshClient  *m_ssh;

    bool echoEnabled = true;
    bool verboseResponses = true;
    bool autoAnswer = false;
    int listenPort = 0;

    QElapsedTimer m_escapeTimer;
    int m_plusCount = 0;

    QList<BbsEntry> m_phonebook;
    BbsEntry m_currentConnection;

    void processAtCommand(const QString &cmd);
    void sendResultCode(int code);
    void sendAtResponse(const QString &text);
    void parseTelnet(const QByteArray &data);
    void checkEscapeSequence(const QByteArray &data);

    // Core helper to handle SIO Reads (Device to Host)
    void sendDataToAtari(const QByteArray &data);

    // SIO Handlers
    void handlePollType1();
    void handlePollType3(quint8 aux1, quint8 aux2);
    void handleDownloadRelocator();
    void handleDownloadDriver();
    void handleStatus();
    void handleRead(quint16 len);
    void handleWrite(quint16 aux);
    void handleControl(quint16 aux);
    void handleListen(quint16 aux);
    void handleStream();

    void at_handle_dial(const QString &target);
};

#endif // RDEVICE_H
