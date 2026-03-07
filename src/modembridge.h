#ifndef MODEMBRIDGE_H
#define MODEMBRIDGE_H

#include <QObject>
#include <QSerialPort>
#include <QTcpSocket>
#include <QTimer>
#include "bbsdata.h"
#include "sshclient.h" // [NEW] Include SSH wrapper

class ModemBridge : public QObject
{
    Q_OBJECT
public:
    explicit ModemBridge(QObject *parent = nullptr);
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

signals:
    void statusMessage(const QString &msg);
    void errorOccurred(const QString &err);

private slots:
    // Serial Port
    void onSerialDataReceived();
    void checkEscapeSequence();

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

private:
    QSerialPort *m_serial;
    QTcpSocket *m_socket;
    SshClient *m_ssh; // [NEW] The SSH Controller

    bool m_isActive;
    bool m_isConnected;     // True = Online (Data Mode)
    bool m_isSshMode;       // [NEW] True = SSH, False = TCP/Telnet

    QByteArray m_serialBuffer;
    QByteArray m_escapeBuffer;
    QTimer *m_escapeTimer;
    bool m_flowControl = true;
    bool m_localEcho = false;
    bool m_isTelnetMode = true;
    bool m_suppressCarrierMessage = false;

    QString m_currentLogin;
    QString m_currentPassword;

    void processAtCommand(const QByteArray &cmd);
    void sendToSerial(const QByteArray &data);
    void connectTo(const QString &host, int port);

    QList<BbsEntry> m_phonebook;
    BbsEntry m_currentConnection;
    bool m_escPressed = false;

    void loadPhonebook(const QString &path);
    BbsEntry findBbsByName(const QString &name);
};

#endif // MODEMBRIDGE_H
