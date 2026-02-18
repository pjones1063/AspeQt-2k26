#ifndef MODEMBRIDGE_H
#define MODEMBRIDGE_H

#include <QObject>
#include <QSerialPort>
#include <QTcpSocket>
#include <QTimer>
#include "bbsdata.h"

class ModemBridge : public QObject
{
    Q_OBJECT
public:
    explicit ModemBridge(QObject *parent = nullptr);
    ~ModemBridge();

    // Configuration
    void setSerialPort(const QString &portName, int baudRate);
    void setTcpMode(bool enableSsh);
    void setFlowControl(bool enable);
    void setLocalEcho(bool enable);
    void dial(const QString &target);
    void setPhonebookPath(const QString &path);

public slots:
    void start();
    void stop();

signals:
    void statusMessage(const QString &msg); // To log to the AspeQt log window
    void errorOccurred(const QString &err);

private slots:
    void onSerialDataReceived();
    void onSocketDataReceived();
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError socketError);
    void checkEscapeSequence(); // Check for "+++" timing


private:
    QSerialPort *m_serial;
    QTcpSocket *m_socket;

    bool m_isActive;
    bool m_isConnected; // True = Online (Data Mode), False = Command Mode
    QByteArray m_serialBuffer; // Buffer for incoming AT commands
    QByteArray m_escapeBuffer; // Buffer for tracking "+++"
    QTimer *m_escapeTimer;     // Guard timer for "+++"
    bool m_flowControl = true;
    bool m_localEcho = false;
    bool m_isTelnetMode = true; // Default to true for port 23
    int m_telnetState = 0;      // 0=Normal, 1=IAC Received, 2=Command Received
    bool m_suppressCarrierMessage = false;

    void processAtCommand(const QByteArray &cmd);
    void sendToSerial(const QByteArray &data);

    QList<BbsEntry> m_phonebook;
    BbsEntry m_currentConnection; // Stores info for the active session
    bool m_escPressed = false;    // Tracks ESC state

    void loadPhonebook(const QString &path);
    BbsEntry findBbsByName(const QString &name);
};

#endif // MODEMBRIDGE_H
