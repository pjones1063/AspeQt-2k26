#include "rdevice.h"
#include "rdevice_handler.h" // Contains driver_850 and relocator_stub
#include "aspeqtsettings.h"
#include <QDebug>
#include <QThread>

// --- SIO Command Constants ---
#define CMD_RELOCATOR    0x21 // '!' - Download Relocator
#define CMD_DOWNLOAD     0x26 // '&' - Download Handler
#define CMD_POLL_TYPE1   0x3F // '?' - Boot Poll
#define CMD_CONTROL      0x41 // 'A' - Control
#define CMD_CONFIGURE    0x42 // 'B' - Configure
#define CMD_STATUS       0x53 // 'S' - Status
#define CMD_WRITE        0x57 // 'W' - Output
#define CMD_READ         0x52 // 'R' - Input
#define CMD_STREAM       0x58 // 'X' - Stream Mode

RDevice::RDevice(SioWorker *worker) : SioDevice(worker)
{
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::connected, this, &RDevice::onSocketConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &RDevice::onSocketDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &RDevice::onSocketReadyRead);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, &RDevice::onSocketError);
    m_isEnabled = aspeqtSettings->enableRDevice();
    state = ModemState::CommandMode;
}

RDevice::~RDevice()
{
    if (tcpSocket) tcpSocket->close();
}

void RDevice::handleCommand(quint8 command, quint16 aux)
{
    bool shouldBeEnabled = aspeqtSettings->enableRDevice();
    if (m_isEnabled != shouldBeEnabled) {
        setEnabled(shouldBeEnabled);
    }

    if (!m_isEnabled) return;

    switch (command) {
    // --- Boot Sequence ---
    case CMD_POLL_TYPE1: handlePollType1(); break;
    case CMD_RELOCATOR:  handleDownloadRelocator(); break;
    case CMD_DOWNLOAD:   handleDownloadDriver(); break;

        // --- Normal Operation ---
    case CMD_STATUS:     handleStatus(); break;
    case CMD_WRITE:      handleWrite(aux); break;
    case CMD_READ:       handleRead(aux); break;
    case CMD_CONTROL:    handleControl(aux); break;

    default:
        // Generic ACK for configuration commands we don't strictly enforce yet
        if (command == CMD_CONFIGURE || command == CMD_STREAM) {
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
// SIO Boot Loader Logic (Adapted from FujiNet modem.cpp)
// --------------------------------------------------------------------------

void RDevice::handlePollType1()
{
    // LOGIC FROM: modem.cpp :: sio_poll_1()
    // The Atari sends '?' ($3F). We must reply with a 12-byte Boot Block.
    // This tells the OS: "Load my relocator code to $0500".

    if (!sio->port()->writeCommandAck()) return;

    // Defines the size of the relocator stub (from rdevice_handler.h)
    quint16 relSize = sizeof(relocator_stub);

    // Construct the 12-byte Boot Block
    // Byte 0: DDEVIC ($50 for R:)
    // Byte 1: DUNIT (1)
    // Byte 2: DCOMND ($21 = The command we want the Atari to send next)
    // Byte 3: DSTATS ($40 = Read/Download)
    // Byte 4/5: Buffer Address ($0500 - Fixed boot location)
    // Byte 6: Timeout
    // Byte 7: Unused
    // Byte 8/9: Byte Count (Size of the Relocator binary)
    // Byte 10/11: Aux (0)

    QByteArray bootBlock;
    bootBlock.resize(12);
    bootBlock[0] = 0x50;
    bootBlock[1] = 0x01;
    bootBlock[2] = 0x21; // This triggers CMD_RELOCATOR next
    bootBlock[3] = 0x40;
    bootBlock[4] = 0x00; // Low byte of $0500
    bootBlock[5] = 0x05; // High byte of $0500
    bootBlock[6] = 0x08;
    bootBlock[7] = 0x00;
    bootBlock[8] = (char)(relSize & 0xFF);
    bootBlock[9] = (char)((relSize >> 8) & 0xFF);
    bootBlock[10] = 0x00;
    bootBlock[11] = 0x00;

    // CRITICAL TIMING: FujiNet uses a delay here to let the Atari OS settle.
    QThread::msleep(10);

    // Send the block.
    // IMPORTANT: Do NOT send writeComplete() for boot blocks.
    sio->port()->writeDataFrame(bootBlock);

    qDebug() << "[RDevice] Boot Sequence Started: Sent Boot Block.";
}

void RDevice::handleDownloadRelocator()
{
    // LOGIC FROM: modem.cpp :: sio_send_firmware(0x21)
    // The Atari executes the command we gave it in the Boot Block ($21).
    // We send the Relocator binary.

    qDebug() << "[RDevice] Sending Relocator ($21)";

    if (!sio->port()->writeCommandAck()) return;

    // Get data from rdevice_handler.h
    QByteArray payload((const char*)relocator_stub, sizeof(relocator_stub));

    // CRITICAL TIMING
    QThread::msleep(5);

    // Send the binary
    sio->port()->writeDataFrame(payload);
}

void RDevice::handleDownloadDriver()
{
    // LOGIC FROM: modem.cpp :: sio_send_firmware(0x26)
    // The Relocator (now running on Atari) sends $26 to fetch the main driver.

    qDebug() << "[RDevice] Sending Main Driver ($26)";

    if (!sio->port()->writeCommandAck()) return;

    // Get data from rdevice_handler.h
    QByteArray payload((const char*)driver_850, sizeof(driver_850));

    // CRITICAL TIMING
    QThread::msleep(5);

    // Send the binary
    sio->port()->writeDataFrame(payload);
}

// --------------------------------------------------------------------------
// Standard SIO Operations
// --------------------------------------------------------------------------

void RDevice::handleStatus()
{
    // LOGIC FROM: modem.cpp :: sio_status()
    if (!sio->port()->writeCommandAck()) return;

    QByteArray statusData(4, 0x00);

    // Byte 0: Error bits (0 = OK)
    statusData[0] = 0x00;

    // Byte 1: Hardware Status bits
    // bit 7: DSR (Not usually emulated here, but set to 1 for "Connected")
    // bit 6: CTS (1 = Ready)
    // bit 4: Carrier Detect (1 = Connected)
    quint8 bits = 0x00;

    // Always indicate CTS/DSR Ready
    bits |= 0x10; // CTS

    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
        bits |= 0x40; // CD (Carrier Detect)
    }

    if (!rxBuffer.isEmpty()) {
        // bit 0: Data Ready? (Usually managed by IRQ, but good to set)
        bits |= 0x01;
    }

    statusData[1] = bits;
    statusData[2] = 0xE0; // Default Timeout
    statusData[3] = 0x00;

    sio->port()->writeComplete();
    sio->port()->writeDataFrame(statusData);
}

void RDevice::handleControl(quint16 aux)
{
    // LOGIC FROM: modem.cpp :: sio_control()
    // AUX1 controls DTR/RTS/XMT lines.

    if (!sio->port()->writeCommandAck()) return;

    // Extract DTR bit (Bit 7 enables change, Bit 6 is value)
    if (aux & 0x80) {
        bool dtr = (aux & 0x40);
        if (!dtr && tcpSocket->state() == QAbstractSocket::ConnectedState) {
            // Drop DTR = Hangup
            qDebug() << "[RDevice] DTR Dropped. Disconnecting.";
            tcpSocket->disconnectFromHost();
        }
    }

    sio->port()->writeComplete();
}

void RDevice::handleWrite(quint16 len)
{
    // LOGIC FROM: modem.cpp :: sio_write()
    if (!sio->port()->writeCommandAck()) return;

    int frameLen = (len > 0) ? len : 128; // Usually 0 in header means 128 or 256 depending on density, but R: is standard
    QByteArray data = sio->port()->readDataFrame(frameLen);

    if (data.isEmpty()) {
        sio->port()->writeDataNak();
        return;
    }

    sio->port()->writeDataAck();

    if (state == ModemState::CommandMode) {
        for (char c : data) {
            if (c == 0x0D || (quint8)c == 0x9B) { // CR or ATASCII EOL
                processAtCommand(atCmdAccumulator);
                atCmdAccumulator.clear();
            } else if (c == 0x7D || c == 0x08) { // Backspace
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

    int readLen = (len > 0) ? len : 128; // Usually SIO is fixed block
    QByteArray chunk;

    if (!rxBuffer.isEmpty()) {
        chunk = rxBuffer.left(readLen);
        rxBuffer.remove(0, chunk.size());
    }

    // Pad with 0x00 if we don't have enough data
    // (Real 850 might pad differently or timeout, but 0x00 is safe for text)
    while (chunk.size() < readLen) chunk.append((char)0x00);

    sio->port()->writeComplete();
    sio->port()->writeDataFrame(chunk);
}

// --------------------------------------------------------------------------
// AT Command Processing
// --------------------------------------------------------------------------

void RDevice::processAtCommand(const QString &cmd)
{
    QString upperCmd = cmd.trimmed().toUpper();
    qDebug() << "[RDevice] AT Command:" << upperCmd;

    if (upperCmd == "AT") {
        sendResultCode(0); // OK
    } else if (upperCmd.startsWith("ATDT")) {
        QString target = upperCmd.mid(4).trimmed();

        // Handle "Host:Port" logic
        QString host = target;
        int port = 23;

        if (target.contains(":")) {
            QStringList parts = target.split(":");
            host = parts[0];
            port = parts[1].toInt();
        }

        qDebug() << "[RDevice] Dialing:" << host << port;
        tcpSocket->connectToHost(host, port);
        // Note: We don't send CONNECT yet; we wait for the socket signal
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

void RDevice::setEnabled(bool enable)
{
    m_isEnabled = enable;
    if (!m_isEnabled) {
        if (tcpSocket->state() != QAbstractSocket::UnconnectedState) {
            tcpSocket->disconnectFromHost();
        }
        state = ModemState::CommandMode;
        rxBuffer.clear();
        atCmdAccumulator.clear();
    }
}

void RDevice::onSocketConnected() {
    state = ModemState::StreamMode;
    sendResultCode(1); // CONNECT
}

void RDevice::onSocketDisconnected() {
    state = ModemState::CommandMode;
    sendResultCode(3); // NO CARRIER
}

void RDevice::onSocketReadyRead() {
    rxBuffer.append(tcpSocket->readAll());
}

void RDevice::onSocketError(QAbstractSocket::SocketError) {
    sendResultCode(4); // ERROR
}
