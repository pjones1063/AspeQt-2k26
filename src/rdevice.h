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
#include <atomic>
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
    QByteArray dequeueNetworkData();
    void processSerialData(const QByteArray &data);
    void dial(const BbsEntry &entry);
    void injectMacro(char macroType);
    void forceCommandMode(bool sendAlert = false);
    void updateListenerConfig();

public slots:
    // [FIX] Moved to slots so SioWorker can safely invoke it across threads!
    void hangup();

signals:
    void dispatchToNetwork(const QByteArray &data);
    void executeAtCommand(const QString &cmd);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError);

    void onNewConnection();
    void onPendingSocketDisconnected();
    void onRingTimeout();
    void onEscapeTriggered();

    void onSshConnected();
    void onSshDisconnected();
    void onSshDataReceived(const QByteArray &data);
    void onSshError(const QString &msg);
    void onAutoAnswerTriggered();



private:
    enum class ModemState { CommandMode, StreamMode };
    enum class TelnetState { Normal, IacReceived, Will, Wont, Do, Dont, SubNegotiation, SubIac };

    ModemState state = ModemState::CommandMode;
    TelnetState m_telnetState = TelnetState::Normal;
    bool m_isEnabled;
    bool m_isSshMode = false;
    bool m_ringPhase = false;
    std::atomic<bool> m_isNetworkConnected{false};

    QByteArray m_txBuffer;
    QString m_atCmdBuffer;
    QByteArray m_networkToSioBuffer;
    QMutex m_bufferMutex;
    QTimer *m_ringTimer;
    QTimer *m_escapeActionTimer;

    QTcpSocket *tcpSocket;
    QTcpServer *tcpServer;
    QTcpSocket *pendingSocket;
    SshClient  *m_ssh;

    bool echoEnabled = true;
    bool verboseResponses = true;
    bool m_escPending = false;
    bool autoAnswer = false;
    bool m_waitingForSshPassword = false;
    int m_s0Register = 0;

    int listenPort = 0;
    int m_currentBaudRate = 19200;

    QElapsedTimer m_escapeTimer;
    int m_plusCount = 0;

    QList<BbsEntry> m_phonebook;
    BbsEntry m_currentConnection;

    void processAtCommand(const QString &cmd);
    void sendResultCode(int code);
    void sendAtResponse(const QString &text);
    void parseTelnet(const QByteArray &data);
    void checkEscapeSequence(const QByteArray &data);

    void sendDataToAtari(const QByteArray &data);

    void handlePollType1();
    void handlePollType3(quint8 aux1, quint8 aux2);
    void handleConfigure(quint8 aux1, quint8 aux2);
    void handleDownloadRelocator();
    void handleDownloadDriver();
    void handleStatus();
    void handleRead(quint16 len);
    void handleWrite(quint16 aux);
    void handleControl(quint16 aux);
    void handleListen(quint16 aux);
    void handleStream();
    void at_handle_dial(const QString &target);
    void parseInteractiveSshTarget(const QString &target);
    void executeInteractiveSshDial();

};

#endif // RDEVICE_H
