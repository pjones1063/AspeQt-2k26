#include "rdevice.h"
#include "rdevice_handler.h"
#include <QDebug>
#include <QThread>

// --- SIO Command Constants ---
#define CMD_POLL_TYPE1   0x3F // '?' - Boot Poll
#define CMD_DOWNLOAD     0x26 // '&' - Handler Download
#define CMD_STATUS       0x53 // 'S' - Status
#define CMD_WRITE        0x57 // 'W' - Output
#define CMD_READ         0x52 // 'R' - Input

RDevice::RDevice(SioWorker *worker) : SioDevice(worker)
{
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::connected, this, &RDevice::onSocketConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &RDevice::onSocketDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &RDevice::onSocketReadyRead);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, &RDevice::onSocketError);

    state = ModemState::CommandMode;
}

RDevice::~RDevice()
{
    if (tcpSocket) tcpSocket->close();
}

void RDevice::handleCommand(quint8 command, quint16 aux)
{
    switch (command) {
    case CMD_POLL_TYPE1: handlePollType1(); break;
    case CMD_DOWNLOAD:   handleDownloadDriver(); break;
    case CMD_STATUS:     handleStatus(); break;
    case CMD_WRITE:      handleWrite(aux); break;
    case CMD_READ:       handleRead(aux); break;
    default:
        // ACK standard configuration pokes ($41, $42)
        if (command == 0x41 || command == 0x42) {
            if (sio->port()->writeCommandAck()) {
                sio->port()->writeComplete();
            }
        } else {
            sio->port()->writeCommandNak();
        }
        break;
    }
}

// --------------------------------------------------------------------------
// Command Logic
// --------------------------------------------------------------------------

void RDevice::handlePollType1()
{
    // FIX: A poll must return ACK -> COMPLETE -> 4-byte status
    if (!sio->port()->writeCommandAck()) return;

    QByteArray idFrame(4, 0x00); // 850 ID is usually all zeros or $00,$00,$01,$00
    sio->port()->writeComplete();
    sio->port()->writeDataFrame(idFrame);

    qDebug() << "!i [RDevice] Responded to Boot Poll ($3F)";
}

void RDevice::handleDownloadDriver()
{
    qDebug() << "!i [RDevice] Handling Download ($26)";

    // 1. Handshake
    if (!sio->port()->writeCommandAck()) return;

    // 2. Prepare Payload with Length Header
    QByteArray payload;
    quint16 len = sizeof(driver_850);
    payload.append((char)(len & 0xFF));         // LSB
    payload.append((char)((len >> 8) & 0xFF));  // MSB
    payload.append((const char*)driver_850, len);

    // 3. FIX: DO NOT send writeComplete() here. The 850 sends Data immediately after ACK.
    sio->port()->writeDataFrame(payload);

    qDebug() << "!n [RDevice] Sent Handler (" << payload.size() << " bytes)";
}

void RDevice::handleStatus()
{
    if (!sio->port()->writeCommandAck()) return;

    QByteArray statusData(4, 0x00);
    quint8 bits = 0x10; // CTS Active
    if (!rxBuffer.isEmpty()) bits |= 0x01; // RX Data Ready
    if (tcpSocket->state() == QAbstractSocket::ConnectedState) bits |= 0x40; // Carrier Detect

    statusData[1] = bits;
    statusData[2] = 0xE0; // Timeout

    sio->port()->writeComplete();
    sio->port()->writeDataFrame(statusData);
}

void RDevice::handleWrite(quint16 len)
{
    if (!sio->port()->writeCommandAck()) return;

    int frameLen = (len > 0) ? len : 128;
    QByteArray data = sio->port()->readDataFrame(frameLen);

    if (data.isEmpty()) {
        sio->port()->writeDataNak();
        return;
    }

    sio->port()->writeDataAck();

    if (state == ModemState::CommandMode) {
        for (char c : data) {
            if (c == 0x0D || (quint8)c == 0x9B) {
                processAtCommand(atCmdAccumulator);
                atCmdAccumulator.clear();
            } else if (c == 0x7D || c == 0x08) {
                if (!atCmdAccumulator.isEmpty()) atCmdAccumulator.chop(1);
            } else {
                atCmdAccumulator.append(c);
            }
        }
    } else {
        if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
            tcpSocket->write(data);
        }
    }

    sio->port()->writeComplete();
}

void RDevice::handleRead(quint16 len)
{
    if (!sio->port()->writeCommandAck()) return;

    int readLen = (len > 0) ? len : 128;
    QByteArray chunk;
    if (!rxBuffer.isEmpty()) {
        chunk = rxBuffer.left(readLen);
        rxBuffer.remove(0, chunk.size());
    }

    while (chunk.size() < readLen) chunk.append((char)0x00);

    sio->port()->writeComplete();
    sio->port()->writeDataFrame(chunk);
}



// --------------------------------------------------------------------------
// AT Command / Networking Logic (Same as before)
// --------------------------------------------------------------------------

void RDevice::processAtCommand(const QString &cmd)
{
    QString upperCmd = cmd.trimmed().toUpper();
    if (upperCmd == "AT") {
        sendResultCode(0); // OK
    } else if (upperCmd.startsWith("ATDT")) {
        QString target = upperCmd.mid(4).trimmed();
        tcpSocket->connectToHost(target, 23);
    } else if (upperCmd == "ATH") {
        tcpSocket->disconnectFromHost();
        sendResultCode(0);
    } else {
        sendResultCode(4); // ERROR
    }
}

void RDevice::sendResultCode(int code)
{
    if (verboseResponses) {
        switch(code) {
        case 0: sendAtResponse("OK\r\n"); break;
        case 1: sendAtResponse("CONNECT\r\n"); break;
        case 3: sendAtResponse("NO CARRIER\r\n"); break;
        case 4: sendAtResponse("ERROR\r\n"); break;
        }
    } else {
        sendAtResponse(QString("%1\r").arg(code));
    }
}

void RDevice::sendAtResponse(const QString &text)
{
    rxBuffer.append(text.toLatin1());
}

void RDevice::onSocketConnected() { state = ModemState::StreamMode; sendResultCode(1); }
void RDevice::onSocketDisconnected() { state = ModemState::CommandMode; sendResultCode(3); }
void RDevice::onSocketReadyRead() { rxBuffer.append(tcpSocket->readAll()); }
void RDevice::onSocketError(QAbstractSocket::SocketError) { sendResultCode(4); }
