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

    // SioDevice Override
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

    bool echoEnabled = true;
    bool verboseResponses = true;

    // Command Handlers
    void handlePollType1();      // $3F
    void handleDownloadDriver(); // $26
    void handleStatus();         // $53
    void handleWrite(quint16 len);
    void handleRead(quint16 len);

    // AT Logic
    void processAtCommand(const QString &cmd);
    void sendAtResponse(const QString &text);
    void sendResultCode(int code);
};

#endif // RDEVICE_H
