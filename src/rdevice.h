#ifndef RDEVICE_H
#define RDEVICE_H

#include "sioworker.h"
#include <QTcpSocket>
#include <QTcpServer>
#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QQueue>
#include "bbsdata.h"
#include "sshclient.h" // [NEW] Include SSH Client

// SIO Command Constants
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

    // SIO Interface
    void handleCommand(quint8 command, quint16 aux) override;
    QString deviceName() override { return "R: Device (850 Emulation)"; }
    void setEnabled(bool enable);
    bool isEnabled() const { return m_isEnabled; }
    void loadPhonebook(const QString &path);

    // Called by SIO Worker when raw bytes arrive during Stream Mode
    void processSerialData(const QByteArray &data);

signals:
    // Signal to SIO Worker to send bytes to Atari (Stream Mode)
    void sendSerialData(const QByteArray &data);

    // Signal to SIO Worker to change physical UART speed
    void requestBaudRateChange(int baudRate);

    // Signal to SIO Worker to exit Stream Mode (return to Command Mode)
    void streamModeFinished();

private slots:
    // TCP Sockets
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);
    void onNewConnection();

    // [NEW] SSH Slots
    void onSshConnected();
    void onSshDisconnected();
    void onSshDataReceived(const QByteArray &data);
    void onSshError(const QString &msg);

private:
    enum class ModemState { CommandMode, StreamMode };

    // Telnet FSM States
    enum class TelnetState {
        Normal,
        IacReceived,
        Will, Wont, Do, Dont,
        SubNegotiation,
        SubIac
    };

    // --- State Variables ---
    ModemState state = ModemState::CommandMode;
    TelnetState m_telnetState = TelnetState::Normal;
    bool m_isEnabled;
    bool m_isSshMode = false; // [NEW] Track active protocol

    QByteArray m_txBuffer;        // Data waiting to go to Atari
    QString m_atCmdBuffer;        // AT command accumulator

    // --- Networking ---
    QTcpSocket *tcpSocket;
    QTcpServer *tcpServer;
    QTcpSocket *pendingSocket;
    SshClient  *m_ssh;           // [NEW] SSH Handler

    // --- Modem Registers ---
    bool echoEnabled = true;
    bool verboseResponses = true;
    bool autoAnswer = false;
    int listenPort = 0;

    // --- Escape Sequence (+++) ---
    QElapsedTimer m_escapeTimer;
    int m_plusCount = 0;

    // --- Phonebook ---
    QList<BbsEntry> m_phonebook;
    BbsEntry m_currentConnection;


    // --- Helpers ---
    void processAtCommand(const QString &cmd);
    void sendResultCode(int code);
    void sendAtResponse(const QString &text);
    void parseTelnet(const QByteArray &data);
    void checkEscapeSequence(const QByteArray &data);


    // SIO Handlers
    void handlePollType1();
    void handlePollType3(quint8 aux1, quint8 aux2);
    void handleDownloadRelocator();
    void handleDownloadDriver();
    void handleStatus();
    void handleControl(quint16 aux);
    void handleWrite(quint16 len);
    void handleRead(quint16 len);
    void handleStream();
    void handleListen(quint16 aux);
    void at_handle_dial(const QString &target);

};

#endif // RDEVICE_H
