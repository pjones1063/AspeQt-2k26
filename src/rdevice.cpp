#include "rdevice.h"
#include "rdevice_handler.h" // Must contain driver_850[] array
#include <QDebug>
#include <QThread>

// --- SIO Command Constants ---
#define CMD_POLL_TYPE1   0x3F // '?'
#define CMD_DOWNLOAD     0x26 // '&'
#define CMD_CONTROL      0x41 // 'A'
#define CMD_CONFIGURE    0x42 // 'B'
#define CMD_STATUS       0x53 // 'S'
#define CMD_WRITE        0x57 // 'W'
#define CMD_READ         0x52 // 'R'
#define CMD_STREAM       0x58 // 'X'

// --- SIO Protocol Bytes ---
#define SIO_ACK  0x41
#define SIO_NAK  0x42
#define SIO_CMP  0x43
#define SIO_ERR  0x45

// --- 850 Status Bits ---
#define STATUS_DATA_READY 0x01 // Bit 0
#define STATUS_CTS        0x10 // Bit 4
#define STATUS_DSR        0x40 // Bit 6

RDevice::RDevice(SioWorker *worker) : SioDevice(worker)
{
    // The socket must live in the same thread as the SioWorker
    tcpSocket = new QTcpSocket(this);

    connect(tcpSocket, &QTcpSocket::connected, this, &RDevice::onSocketConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &RDevice::onSocketDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &RDevice::onSocketReadyRead);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, &RDevice::onSocketError);

    // Default state
    state = ModemState::CommandMode;
    echoEnabled = true;
    verboseResponses = true;
}

RDevice::~RDevice()
{
    if (tcpSocket) {
        tcpSocket->close();
    }
}

// --------------------------------------------------------------------------
// Main Command Dispatcher
// --------------------------------------------------------------------------
void RDevice::handleCommand(quint8 command, quint16 aux)
{
    switch (command) {
    case CMD_POLL_TYPE1:
        handlePollType1();
        break;
    case CMD_DOWNLOAD:
        handleDownloadDriver();
        break;
    case CMD_STATUS:
        handleStatus();
        break;
    case CMD_WRITE:
        handleWrite(aux); // Aux is the length of data to write
        break;
    case CMD_READ:
        handleRead(aux);  // Aux is the length of data to read
        break;
    case CMD_CONTROL:
    case CMD_CONFIGURE:
    case CMD_STREAM:
        // Accept configuration changes but don't enforce them on PC side
        sendAck();
        sendComplete();
        break;
    default:
        sendNak();
        break;
    }
}

// --------------------------------------------------------------------------
// SIO Protocol Helpers
// --------------------------------------------------------------------------

void RDevice::sendAck()
{
    QByteArray b;
    b.append((char)SIO_ACK);
    sio->port()->writeRawFrame(b);
}

void RDevice::sendNak()
{
    QByteArray b;
    b.append((char)SIO_NAK);
    sio->port()->writeRawFrame(b);
}

void RDevice::sendComplete()
{
    QByteArray b;
    b.append((char)SIO_CMP);
    sio->port()->writeRawFrame(b);
}

void RDevice::sendFrame(const QByteArray &data)
{
    QByteArray packet = data;
    packet.append(calcChecksum(data));
    sio->port()->writeRawFrame(packet);
}

quint8 RDevice::calcChecksum(const QByteArray &data)
{
    quint16 sum = 0;
    for (char c : data) {
        sum += (quint8)c;
        if (sum > 0xFF) {
            sum = (sum & 0xFF) + 1;
        }
    }
    return (quint8)sum;
}

// --------------------------------------------------------------------------
// Command Logic
// --------------------------------------------------------------------------

void RDevice::handlePollType1()
{
    sendAck();
}

void RDevice::handleDownloadDriver()
{
    sendAck();
    QByteArray payload((const char*)driver_850, sizeof(driver_850));
    sendFrame(payload);
}

void RDevice::handleStatus()
{
    sendAck();

    QByteArray statusData;
    statusData.resize(4);

    quint8 bits = 0x00;
    if (!rxBuffer.isEmpty()) bits |= STATUS_DATA_READY;
    bits |= STATUS_CTS;
    if (tcpSocket->state() == QAbstractSocket::ConnectedState) bits |= STATUS_DSR;

    statusData[0] = 0x00; // Error Byte
    statusData[1] = bits; // Interface Bits
    statusData[2] = 0xE0; // Timeout default
    statusData[3] = 0x00; // Unused

    sendFrame(statusData);
    sendComplete();
}

void RDevice::handleWrite(quint16 len)
{
    sendAck();

    int bytesToRead = (len > 0 ? len : 128) + 1;
    QByteArray data;
    QElapsedTimer timeout;
    timeout.start();

    // Loop until we have the full frame or timeout
    while (data.size() < bytesToRead && timeout.elapsed() < 2000) {
        // Use the existing AspeQt method to pull bytes from the buffer
        QByteArray chunk = sio->port()->readCommandFrame();
        if (!chunk.isEmpty()) {
            data.append(chunk);
        } else {
            QThread::msleep(2);
        }
    }

    if (data.size() < bytesToRead) {
        sendNak();
        return;
    }

    if(data.size() > bytesToRead) data.resize(bytesToRead);

    QByteArray payload = data.left(data.size() - 1);
    if (calcChecksum(payload) != (quint8)data.at(data.size() - 1)) {
        sendNak();
        return;
    }

    sendAck();

    if (state == ModemState::CommandMode) {
        for (char c : payload) {
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
            tcpSocket->write(payload);
        }
    }
    sendComplete();
}


void RDevice::handleRead(quint16 len)
{
    sendAck();

    int readLen = (len > 0) ? len : 128;
    QByteArray chunk;

    if (!rxBuffer.isEmpty()) {
        chunk = rxBuffer.left(readLen);
        rxBuffer.remove(0, chunk.size());
    }

    while (chunk.size() < readLen) {
        chunk.append((char)0x00);
    }

    sendFrame(chunk);
    sendComplete();
}

// --------------------------------------------------------------------------
// AT Command Processing
// --------------------------------------------------------------------------

void RDevice::processAtCommand(const QString &cmd)
{
    QString upperCmd = cmd.trimmed().toUpper();

    if (upperCmd == "AT") {
        sendResultCode(0); // OK
        return;
    }

    if (upperCmd.startsWith("ATDT")) {
        QString target = upperCmd.mid(4).trimmed();
        QString host = target;
        int port = 23;

        int colonIndex = target.indexOf(':');
        if (colonIndex != -1) {
            host = target.left(colonIndex);
            port = target.mid(colonIndex + 1).toInt();
        }

        qDebug() << "[RDevice] Dialing" << host << port;
        tcpSocket->connectToHost(host, port);
        return;
    }

    if (upperCmd == "ATH") {
        tcpSocket->disconnectFromHost();
        sendResultCode(0);
        return;
    }

    if (upperCmd == "ATE0") {
        echoEnabled = false;
        sendResultCode(0);
        return;
    }

    if (upperCmd == "ATE1") {
        echoEnabled = true;
        sendResultCode(0);
        return;
    }

    sendResultCode(4); // ERROR
}

void RDevice::sendResultCode(int code)
{
    if (verboseResponses) {
        switch(code) {
        case 0: sendAtResponse("OK\r\n"); break;
        case 1: sendAtResponse("CONNECT\r\n"); break;
        case 2: sendAtResponse("RING\r\n"); break;
        case 3: sendAtResponse("NO CARRIER\r\n"); break;
        case 4: sendAtResponse("ERROR\r\n"); break;
        default: sendAtResponse("ERROR\r\n"); break;
        }
    } else {
        sendAtResponse(QString("%1\r").arg(code));
    }
}

void RDevice::sendAtResponse(const QString &text)
{
    rxBuffer.append(text.toLatin1());
}

// --------------------------------------------------------------------------
// Network Event Slots
// --------------------------------------------------------------------------

void RDevice::onSocketConnected()
{
    state = ModemState::StreamMode;
    sendResultCode(1); // CONNECT
}

void RDevice::onSocketDisconnected()
{
    state = ModemState::CommandMode;
    sendResultCode(3); // NO CARRIER
}

void RDevice::onSocketReadyRead()
{
    QByteArray incoming = tcpSocket->readAll();
    if (rxBuffer.size() < 4096) {
        rxBuffer.append(incoming);
    }
}

void RDevice::onSocketError(QAbstractSocket::SocketError err)
{
    state = ModemState::CommandMode;
    sendResultCode(4); // ERROR
}
