#ifndef MODEMBRIDGE_H
#define MODEMBRIDGE_H

#include <QObject>
#include <QSerialPort>
#include <QTcpSocket>
#include <QTimer>
#include <QTcpServer>
#include <QElapsedTimer>
#include "bbsdata.h"
#include "sshclient.h" // [NEW] Include SSH wrapper

class ModemBridge : public QObject
{
    Q_OBJECT
public:
    explicit ModemBridge(QObject *parent = nullptr, int portIndex = 0);
    ~ModemBridge();

    // Configuration
    void setSerialPort(const QString &portName, int baudRate);
    void setFlowControl(bool enable);
    void setLocalEcho(bool enable);
    void dial(const QString &target);
    void setPhonebookPath(const QString &path);
    void dial(const BbsEntry &entry);
    void hangup();
    void injectMacro(char macroType);

    // Placeholder for setTcpMode if you still use it elsewhere,
    // though protocol is now handled per-connection.
    void setTcpMode(bool enableSsh);

public slots:
    void start();
    void stop();
    void updateListenerConfig();


signals:
    void statusMessage(const QString &msg);
    void errorOccurred(const QString &err);
    void traceData(const QString &dir, const QByteArray &data);
    void rxActivity();
    void txActivity();

private slots:
    // Serial Port
    void onSerialDataReceived();

    // TCP / Telnet Slots
    void onSocketDataReceived();
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);

    // [NEW] SSH Slots
    void onSshConnected();
    void onSshDisconnected();
    void onSshDataReceived(const QByteArray &data);
    void onSshError(const QString &msg);

    void onNewConnection();
    void onPendingSocketDisconnected();
    void onRingTimeout();
    void onEscapeTriggered();
    void onAutoAnswerTriggered();


private:
    int m_portIndex;
    QSerialPort *m_serial;
    QTcpSocket *m_socket;
    SshClient *m_ssh;
    QTcpServer *m_tcpServer;
    QTcpSocket *m_pendingSocket;

    bool m_isActive;
    bool m_isConnected;
    bool m_isSshMode;
    enum class TelnetState { Normal, IacReceived, Will, Wont, Do, Dont, SubNegotiation, SubIac };
    TelnetState m_telnetState = TelnetState::Normal;
    void parseTelnet(const QByteArray &data);

    QByteArray m_serialBuffer;
    QByteArray m_escapeBuffer;
    QElapsedTimer m_escapeTimer;
    QTimer *m_escapeActionTimer;
    int m_plusCount = 0;

    QTimer *m_ringTimer;
    bool m_flowControl = true;
    bool m_localEcho = false;
    bool m_isTelnetMode = true;
    bool m_ringPhase;
    bool m_suppressCarrierMessage = false;
    int m_s0Register = 0;
    bool m_verboseResponses = true;
    bool m_escPressed = false;

    QString m_currentLogin;
    QString m_currentPassword;    
    BbsEntry m_currentConnection;

    void processAtCommand(const QByteArray &cmd);
    void sendToSerial(const QByteArray &data);
    void connectTo(const QString &host, int port);
    QList<BbsEntry> m_phonebook;
    void loadPhonebook(const QString &path);
    BbsEntry findBbsByName(const QString &name);
    bool m_waitingForSshPassword = false;
    void parseInteractiveSshTarget(const QString &target);
    void executeInteractiveSshDial();
    void sendResultCode(int code);

};

#endif // MODEMBRIDGE_H
