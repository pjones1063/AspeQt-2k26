#ifndef RDEVICE_H
#define RDEVICE_H

#include "sioworker.h"
#include <QTcpSocket>
#include <QByteArray>

class RDevice : public SioDevice
{
    Q_OBJECT

public:
    explicit RDevice(SioWorker *worker);
    ~RDevice() override;
    void handleCommand(quint8 command, quint16 aux) override;
    void setEnabled(bool enable);
    bool isEnabled() const { return m_isEnabled; }
    QString deviceName() override { return "R: Device (850 Emulation)"; }

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    enum class ModemState { CommandMode, StreamMode };

    ModemState state = ModemState::CommandMode;
    QByteArray rxBuffer;
    QString    atCmdAccumulator;
    QTcpSocket *tcpSocket;

    bool echoEnabled = true;
    bool verboseResponses = true;
    bool m_isEnabled;

    // --- SIO Command Handlers ---
    void handlePollType1();      // $3F - Boot Poll (The Trigger)
    void handleDownloadRelocator(); // $21 - Relocator (The Loader)
    void handleDownloadDriver(); // $26 - Handler (The Driver)

    // Standard R: Operations
    void handleStatus();         // $53
    void handleWrite(quint16 len);
    void handleRead(quint16 len);
    void handleControl(quint16 aux); // $41 - Control (DTR/RTS)

    // AT Logic
    void processAtCommand(const QString &cmd);
    void sendAtResponse(const QString &text);
    void sendResultCode(int code);
};

#endif // RDEVICE_H
