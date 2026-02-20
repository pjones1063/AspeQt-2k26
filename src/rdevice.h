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

    // --- ADDED THESE MISSING DECLARATIONS ---
    bool echoEnabled = true;
    bool verboseResponses = true;
    void sendResultCode(int code);
    // ----------------------------------------

    // SIO Helpers
    void sendAck();
    void sendNak();
    void sendComplete();
    void sendFrame(const QByteArray &data);
    quint8 calcChecksum(const QByteArray &data);

    // Command Handlers
    void handlePollType1();
    void handleDownloadDriver();
    void handleStatus();
    void handleWrite(quint16 len);
    void handleRead(quint16 len);

    // AT Command Logic
    void processAtCommand(const QString &cmd);
    void sendAtResponse(const QString &text);
};

#endif // RDEVICE_H
