#ifndef RDEVICE_H
#define RDEVICE_H

#include "sioworker.h"
#include <QTcpSocket>
#include <QTcpServer>
#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QList>
#include "bbsdata.h" // Assumed available in project

// SIO Command Constants
#define CMD_RELOCATOR    0x21 // '!' - Download Relocator
#define CMD_DOWNLOAD     0x26 // '&' - Download Handler
#define CMD_POLL_TYPE1   0x3F // '?' - Boot Poll
#define CMD_POLL_TYPE3   0x40 // '@' - Add Device Poll
#define CMD_CONTROL      0x41 // 'A' - Control
#define CMD_CONFIGURE    0x42 // 'B' - Configure
#define CMD_LISTEN       0x4C // 'L' - Listen (ATPORT)
#define CMD_UNLISTEN     0x4D // 'M' - Unlisten
#define CMD_AUTOANSWER   0x4F // 'O' - Auto Answer
#define CMD_STATUS       0x53 // 'S' - Status
#define CMD_WRITE        0x57 // 'W' - Output
#define CMD_READ         0x52 // 'R' - Input
#define CMD_STREAM       0x58 // 'X' - Concurrent Mode

class RDevice : public SioDevice
{
    Q_OBJECT

public:
    explicit RDevice(SioWorker *worker);
    ~RDevice() override;

    void handleCommand(quint8 command, quint16 aux) override;
    QString deviceName() override { return "R: Device (850 Emulation)"; }

    // Toggle R: functionality
    void setEnabled(bool enable);
    bool isEnabled() const { return m_isEnabled; }

private slots:
    // TCP Client Slots
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

    // TCP Server Slots
    void onNewConnection();

private:
    enum class ModemState { CommandMode, StreamMode };

    // --- State Variables ---
    ModemState state = ModemState::CommandMode;
    bool m_isEnabled;
    QByteArray rxBuffer;          // Buffer for data PC -> Atari
    QString atCmdAccumulator;     // Buffer for AT commands Atari -> PC

    // --- Networking ---
    QTcpSocket *tcpSocket;        // Outgoing/Active connection
    QTcpServer *tcpServer;        // Incoming connection listener
    QTcpSocket *pendingSocket;    // Waiting for Answer (ATA)

    // --- Modem Registers ---
    bool echoEnabled = true;      // ATE0/1
    bool verboseResponses = true; // ATV0/1
    bool autoAnswer = false;      // ATS0=1
    int listenPort = 0;           // ATPORT

    // --- Timing & Escape Sequence ---
    QElapsedTimer lastActivityTimer;
    QElapsedTimer lastRingTimer;
    int plusCount = 0;
    bool possibleEscape = false;

    // --- Phonebook & Macros ---
    QList<BbsEntry> m_phonebook;
    BbsEntry m_currentConnection; // Stores info for the active session (Login/Pass)
    bool m_escPressed = false;    // Tracks ESC key for Macros inside Stream Loop

    void loadPhonebook(const QString &path);

    // --- Core Logic Helpers ---
    void processStreamLoop();     // The Blocking Data Pump for Mode $58
    void processAtCommand(const QString &cmd);
    void processIncomingData(QByteArray data); // Telnet IAC stripping

    // --- SIO Command Handlers ---
    void handlePollType1();             // $3F
    void handlePollType3(quint8 aux1, quint8 aux2); // $40
    void handleDownloadRelocator();     // $21
    void handleDownloadDriver();        // $26
    void handleStatus();                // $53
    void handleControl(quint16 aux);    // $41
    void handleWrite(quint16 len);      // $57
    void handleRead(quint16 len);       // $52
    void handleStream();                // $58
    void handleListen(quint16 aux);     // $4C
    void handleUnlisten();              // $4D

    // --- AT Helpers ---
    void sendResultCode(int code);
    void sendAtResponse(const QString &text);
    void at_handle_dial(const QString &target);
    void at_handle_answer();
    void at_handle_hangup();
    void checkRing();

    // Helper for timing
    void shortDelay();
};

#endif // RDEVICE_H
