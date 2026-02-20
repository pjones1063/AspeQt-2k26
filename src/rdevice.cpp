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
// SIO Command Handlers
// --------------------------------------------------------------------------

void RDevice::handlePollType1()
{
    // FIX: Restore full handshake for Command $3F (Read).
    // The Atari expects ACK -> COMPLETE -> 4 Bytes of Data.
    if (!sio->port()->writeCommandAck()) return;

    // Standard 850 Status Response (4 bytes)
    // 00 00 01 00 = "Device Ready / Version 1"
    QByteArray idFrame;
    idFrame.append((char)0x00);
    idFrame.append((char)0x00);
    idFrame.append((char)0x01);
    idFrame.append((char)0x00);

    // Send "Complete" to signal data is ready
    sio->port()->writeComplete();

    // Send the Data Frame
    sio->port()->writeDataFrame(idFrame);

    qDebug() << "!i [RDevice] Responded to Boot Poll ($3F) with Valid Status.";
}

void RDevice::handleDownloadDriver()
{
    qDebug() << "!i [RDevice] Handling Download ($26)";

    if (!sio->port()->writeCommandAck()) return;

    // Payload: Length Header + Driver
    QByteArray payload;
    quint16 len = sizeof(driver_850);
    payload.append((char)(len & 0xFF));         // LSB
    payload.append((char)((len >> 8) & 0xFF));  // MSB
    payload.append((const char*)driver_850, len);

    // Command $26 Protocol: ACK -> Data (No Complete byte)
    sio->port()->writeDataFrame(payload);

    qDebug() << "!n [RDevice] Sent Driver (" << payload.size() << " bytes).";
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
// AT Command / Networking Logic
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
    } else if (upperCmd == "ATE0") {
        echoEnabled = false;
        sendResultCode(0);
    } else if (upperCmd == "ATE1") {
        echoEnabled = true;
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
