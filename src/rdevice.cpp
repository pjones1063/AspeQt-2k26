#include "rdevice.h"
#include "rdevice_handler.h"
#include "aspeqtsettings.h"
#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include <QDomDocument>
#include <QThread>
#include <QMutexLocker>

#define RESULT_OK           0
#define RESULT_CONNECT      1
#define RESULT_RING         2
#define RESULT_NO_CARRIER   3
#define RESULT_ERROR        4

#define ESCAPE_GUARD_TIME   1000

RDevice::RDevice(SioWorker *worker) : SioDevice(worker)
{
    // Initialize standard Telnet socket
    tcpSocket = new QTcpSocket(this);
    tcpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    connect(tcpSocket, &QTcpSocket::connected, this, &RDevice::onSocketConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &RDevice::onSocketDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &RDevice::onSocketReadyRead);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, &RDevice::onSocketError);

    // Initialize SSH client for secure BBS/Linux connections
    m_ssh = new SshClient(this);
    connect(m_ssh, &SshClient::connected, this, &RDevice::onSshConnected);
    connect(m_ssh, &SshClient::disconnected, this, &RDevice::onSshDisconnected);
    connect(m_ssh, &SshClient::rxData, this, &RDevice::onSshDataReceived);
    connect(m_ssh, &SshClient::error, this, &RDevice::onSshError);

    // Initialize inbound listener server (for Answer Mode)
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &RDevice::onNewConnection);
    pendingSocket = nullptr;

    // Load initial settings and phonebook
    m_isEnabled = (aspeqtSettings && aspeqtSettings->enableRDevice());
    if (m_isEnabled) loadPhonebook(aspeqtSettings->modemBridgePhonebookPath());

    // --- UPDATED DISPATCH LOGIC: Smart Routing ---
    connect(this, &RDevice::dispatchToNetwork, this, [this](const QByteArray &data){
        bool isConnected = (m_isSshMode && m_ssh->isConnected()) ||
                           (!m_isSshMode && tcpSocket->state() == QAbstractSocket::ConnectedState);

        if (isConnected) {
            // We are ONLINE. Route all stream data straight to the remote BBS/Server.
            if (m_isSshMode) m_ssh->write(data);
            else tcpSocket->write(data);
        }
        else {
            // We are OFFLINE. (Even if the Atari is physically in Concurrent/Stream Mode!)
            // Route typing to the local AT Command Parser so the user can dial.
            for (char c : data) {
                // Handle local echo for terminal typing
                if (echoEnabled) {
                    QMutexLocker locker(&m_bufferMutex);
                    m_networkToSioBuffer.append(c);
                }

                // Translate ATASCII Return (0x9B) to standard Return (0x0D) for the parser
                if (c == 0x0D || (quint8)c == 0x9B) {
                    emit executeAtCommand(m_atCmdBuffer);
                    m_atCmdBuffer.clear();
                } else {
                    m_atCmdBuffer.append(c);
                }
            }
        }
    }, Qt::QueuedConnection);

    // Route parsed AT commands back to the Main Thread for safe socket operations
    connect(this, &RDevice::executeAtCommand, this, &RDevice::processAtCommand, Qt::QueuedConnection);
}


RDevice::~RDevice()
{
    tcpSocket->abort();
    m_ssh->disconnectFromHost();
    tcpServer->close();
}

void RDevice::setEnabled(bool enable)
{
    m_isEnabled = enable;
    if (!enable) {
        tcpSocket->disconnectFromHost();
        m_ssh->disconnectFromHost();
        tcpServer->close();
        state = ModemState::CommandMode;
        m_txBuffer.clear();
    }
}



void RDevice::handleCommand(quint8 command, quint16 aux)
{
    if (!m_isEnabled) return;

    quint8 aux1 = (aux & 0xFF);
    quint8 aux2 = (aux >> 8) & 0xFF;

    // Filter out the high-frequency polling/status commands to keep the logs readable
    if (command != CMD_STATUS && command != CMD_POLL_TYPE1 &&
        command != CMD_POLL_TYPE3 && command != CMD_RELOCATOR &&
        command != CMD_DOWNLOAD && command != CMD_STREAM) {
        qDebug() << "!d" << "[RDevice] Cmd:" << QString::number(command, 16).toUpper()
        << "Aux1:" << QString::number(aux1, 16).toUpper()
        << "Aux2:" << QString::number(aux2, 16).toUpper();
    }

    switch (command) {
    case CMD_POLL_TYPE1: handlePollType1(); break;
    case CMD_POLL_TYPE3: handlePollType3(aux1, aux2); break;
    case CMD_RELOCATOR:  handleDownloadRelocator(); break;
    case CMD_DOWNLOAD:   handleDownloadDriver(); break;
    case CMD_STATUS:     handleStatus(); break;
    case CMD_WRITE:      handleWrite(aux); break;
    case CMD_READ:       handleRead(aux); break;

        // --- HANDSHAKING HANDLERS ---
        // Catch these (XIO 34, 36, 38) so they don't leak into the stream.
    case CMD_CONTROL:    // 0x41 ('A') - DTR/RTS toggle
    case CMD_CONFIGURE:  // 0x42 ('B') - Baud/Protocol
    case CMD_AUTOANSWER: // 0x4F ('O') - Translation
        if (!sio->port()->writeCommandAck()) return;
        sio->port()->writeComplete();
        break;

        // --- MODE SWITCHING ---
    case CMD_STREAM:     // 0x58 ('X') - Enter Concurrent Mode
        handleStream();
        break;

    case CMD_LISTEN:     handleListen(aux); break;
    case CMD_UNLISTEN:
        sio->port()->writeCommandAck();
        tcpServer->close();
        sio->port()->writeComplete();
        break;

    default:
        sio->port()->writeCommandNak();
        break;
    }
}


// --------------------------------------------------------------------------
// SIO Core Helper (READ from Peripheral to Host)
// --------------------------------------------------------------------------
void RDevice::sendDataToAtari(const QByteArray &data)
{
    SioWorker::usleep(2000);
    sio->port()->writeComplete();
    SioWorker::usleep(2000);
    sio->port()->writeDataFrame(data);
}

// --------------------------------------------------------------------------
// SIO Handlers
// --------------------------------------------------------------------------

void RDevice::handlePollType1() {
    if (!sio->port()->writeCommandAck()) return;

    QByteArray bootBlock(12, 0);
    bootBlock[0]=0x50; // DDEVIC
    bootBlock[1]=0x01; // DUNIT
    bootBlock[2]=0x21; // DCOMND = '!' (boot relocator)
    bootBlock[3]=0x40; // DSTATS (Read)
    bootBlock[4]=0x00; // DBUFLO
    bootBlock[5]=0x05; // DBUFHI = $0500
    bootBlock[6]=0x08; // DTIMLO = 8 vblanks
    bootBlock[8]=sizeof(relocator_stub)&0xFF;
    bootBlock[9]=sizeof(relocator_stub)>>8;

    sendDataToAtari(bootBlock);
}

void RDevice::handlePollType3(quint8 aux1, quint8 aux2) {
    if ((aux1 == 0x52 && aux2 == 0x01) || aux1 == 0x00) {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray resp(4, 0);
        resp[0] = sizeof(driver_850)&0xFF;
        resp[1] = sizeof(driver_850)>>8;
        resp[2] = 0x50;
        resp[3] = 0x00;

        sendDataToAtari(resp);
    }
}

void RDevice::handleDownloadRelocator() {
    if (!sio->port()->writeCommandAck()) return;
    QByteArray payload((const char*)relocator_stub, sizeof(relocator_stub));
    sendDataToAtari(payload);
}

void RDevice::handleDownloadDriver() {
    if (!sio->port()->writeCommandAck()) return;
    QByteArray payload((const char*)driver_850, sizeof(driver_850));
    sendDataToAtari(payload);
}

void RDevice::handleStatus() {
    if (!sio->port()->writeCommandAck()) return;

    QByteArray status(4, 0);

    // Set DSR (0xC0) and CTS (0x30) to Always High so Ice-T is allowed to transmit.
    status[1] = 0xC0 | 0x30; // 0xF0

    // If we have an active Telnet/SSH socket, raise the Carrier Detect (CRX) bit.
    if (tcpSocket->state() == QAbstractSocket::ConnectedState || (m_isSshMode && m_ssh->isConnected())) {
        status[1] |= 0x0C; // 0x0C is '11' for bits 3 and 2.
    }

    sendDataToAtari(status);
}

void RDevice::handleRead(quint16 len) {
    if (!sio->port()->writeCommandAck()) return;

    int readLen = (len > 0) ? len : 128;
    QByteArray chunk = m_txBuffer.left(readLen);
    m_txBuffer.remove(0, chunk.size());

    while (chunk.size() < readLen) chunk.append((char)0x00);

    sendDataToAtari(chunk);
}


void RDevice::handleStream() {
    // 1. Acknowledge the SIO command block
    if (!sio->port()->writeCommandAck()) return;

    // 2. Send the 9-byte POKEY divisor payload for 19200 baud!
    QByteArray response;
    response.resize(9);
    response[0] = 0x28; response[1] = 0xA0; response[2] = 0x00;
    response[3] = 0xA0; response[4] = 0x28; response[5] = 0xA0;
    response[6] = 0x00; response[7] = 0xA0; response[8] = 0x78;

    // This helper method sends the 2ms delay, the COMPLETE byte ('C'),
    // the 9 bytes of data, and the hardware SIO Checksum!
    sendDataToAtari(response);

    // 3. The Hardware Drain Guard
    // Wait 40ms to ensure the OS physically transmits the 12 bytes
    // (ACK + COMPLETE + 9 Data + Checksum) down the copper wire before we touch the baud rate.
    SioWorker::usleep(40000);

    // 4. Purge buffers immediately so the stream starts clean
    {
        QMutexLocker locker(&m_bufferMutex);
        m_networkToSioBuffer.clear();
        m_txBuffer.clear();
    }

    // 5. Transition to Stream Mode Safely!
    // NO QTimer! Because we are running linearly inside the SioWorker thread,
    // changing the baud rate here is 100% thread-safe and prevents the QByteArray crash.
    state = ModemState::StreamMode;
    m_escapeTimer.start();
    m_plusCount = 0;

    sio->onChangeBaudRate(19200);

    qDebug() << "!d" << "[RDevice] Stream active - Handing over to Pi 5 Hardware UART at 19200.";
}


void RDevice::handleWrite(quint16 aux) {
    if (!sio->port()->writeCommandAck()) return;

    quint8 logicalLen = aux & 0xFF;
    QByteArray data = sio->port()->readDataFrame(64);

    if (data.isEmpty()) {
        sio->port()->writeError();
        return;
    }

    sio->port()->writeDataAck();

    QString debugStr;
    for (int i = 0; i < logicalLen && i < data.size(); i++) {
        char c = data.at(i);
        if (c >= 32 && c <= 126) {
            debugStr.append(c);
        } else {
            debugStr.append(QString("<%1>").arg((quint8)c, 2, 16, QLatin1Char('0')).toUpper());
        }
    }

    for (int i = 0; i < logicalLen && i < data.size(); i++) {
        char c = data.at(i);
        if (c == 0x0D || (quint8)c == 0x9B) {
            emit executeAtCommand(m_atCmdBuffer);
            m_atCmdBuffer.clear();
        } else {
            m_atCmdBuffer.append(c);
        }
    }

    sio->port()->writeComplete();
}

void RDevice::handleControl(quint16 aux) {
    if (!sio->port()->writeCommandAck()) return;
    sio->port()->writeComplete();
}

void RDevice::handleListen(quint16 aux) {
    if (tcpServer->listen(QHostAddress::Any, aux)) {
        sio->port()->writeCommandAck();
        sio->port()->writeComplete();
    } else {
        sio->port()->writeCommandNak();
    }
}

// --------------------------------------------------------------------------
// Boilerplate Networking / Phonebook / Parsing
// --------------------------------------------------------------------------

void RDevice::processSerialData(const QByteArray &data) {
    if (state != ModemState::StreamMode) return;

    static bool escPending = false;
    QByteArray filteredData;

    // Trap Atari Keyboard Macros (ESC U / ESC P / ESC H) in the SIO stream
    for (int i = 0; i < data.size(); ++i) {
        char c = data[i];

        if (escPending) {
            escPending = false;

            if (c == 'U' || c == 'u') {
                // Inject UserID directly into the network stream
                QString inject = m_currentConnection.login + "\r";
                emit dispatchToNetwork(inject.toUtf8());
                continue;
            }
            if (c == 'P' || c == 'p') {
                // Inject Password directly into the network stream
                QString inject = m_currentConnection.password + "\r";
                emit dispatchToNetwork(inject.toUtf8());
                continue;
            }
            if (c == 'H' || c == 'h') {
                // Hangup Macro: Drop carrier instantly
                if (tcpSocket->state() == QAbstractSocket::ConnectedState) tcpSocket->disconnectFromHost();
                if (m_isSshMode && m_ssh->isConnected()) m_ssh->disconnectFromHost();
                continue;
            }

            // Not a macro, put the ESC byte and the character back
            filteredData.append(0x1B);
            filteredData.append(c);
        } else if (c == 0x1B) {
            // 0x1B is the ESC key. Hold it and wait for the next byte.
            escPending = true;
        } else {
            filteredData.append(c);
        }
    }

    // Only process the '+++' check and send to network if there is standard typing left
    if (!filteredData.isEmpty()) {
        checkEscapeSequence(filteredData);

        // Safely jump to the Main Thread via queued signal
        emit dispatchToNetwork(filteredData);
    }
}


void RDevice::checkEscapeSequence(const QByteArray &data) {
    for (char c : data) {
        if (c == '+') {
            if (m_escapeTimer.elapsed() > ESCAPE_GUARD_TIME) {
                if (m_plusCount == 0) m_plusCount = 1;
                else m_plusCount++;
            } else {
                if (m_plusCount > 0) m_plusCount++;
                else m_plusCount = 0;
            }
        } else {
            m_plusCount = 0;
            m_escapeTimer.restart();
        }
    }

    if (m_plusCount >= 3 && m_escapeTimer.elapsed() > ESCAPE_GUARD_TIME) {
        // --- FIX 4: DECOUPLE SIO STATE FROM MODEM STATE ---
        // Drop the remote connection, but KEEP the Atari SIO stream alive
        if (tcpSocket->state() == QAbstractSocket::ConnectedState) tcpSocket->disconnectFromHost();
        if (m_isSshMode && m_ssh->isConnected()) m_ssh->disconnectFromHost();

        sendResultCode(RESULT_OK); // Tell Ice-T the modem is listening
        m_plusCount = 0;
    }
}

void RDevice::onSocketReadyRead() {
    if (m_isSshMode) return;
    QByteArray data = tcpSocket->readAll();

    // Always strip telnet control codes first
    parseTelnet(data);

    if (state == ModemState::StreamMode) {
        if (!m_txBuffer.isEmpty()) {
            QMutexLocker locker(&m_bufferMutex);
            m_networkToSioBuffer.append(m_txBuffer);
            m_txBuffer.clear();
        }
    }
    // If in Command Mode, parseTelnet already safely placed the
    // filtered text into m_txBuffer for the Atari to read later!
}


void RDevice::onSshDataReceived(const QByteArray &data) {
    if (state == ModemState::StreamMode) {
        QMutexLocker locker(&m_bufferMutex);
        m_networkToSioBuffer.append(data);
    } else {
        m_txBuffer.append(data);
    }
}

void RDevice::parseTelnet(const QByteArray &data) {
    for (char c : data) {
        unsigned char byte = (unsigned char)c;
        switch (m_telnetState) {
        case TelnetState::Normal:
            if (byte == 0xFF) m_telnetState = TelnetState::IacReceived;
            else m_txBuffer.append(c);
            break;
        case TelnetState::IacReceived:
            switch (byte) {
            case 0xFF: m_txBuffer.append((char)0xFF); m_telnetState = TelnetState::Normal; break;
            case 0xFB: m_telnetState = TelnetState::Will; break;
            case 0xFC: m_telnetState = TelnetState::Wont; break;
            case 0xFD: m_telnetState = TelnetState::Do;   break;
            case 0xFE: m_telnetState = TelnetState::Dont; break;
            case 0xFA: m_telnetState = TelnetState::SubNegotiation; break;
            default: m_telnetState = TelnetState::Normal; break;
            }
            break;
        case TelnetState::Will:
        case TelnetState::Wont:
        case TelnetState::Do:
        case TelnetState::Dont:
            m_telnetState = TelnetState::Normal;
            break;
        case TelnetState::SubNegotiation:
            if (byte == 0xFF) m_telnetState = TelnetState::SubIac;
            break;
        case TelnetState::SubIac:
            if (byte == 0xF0) m_telnetState = TelnetState::Normal;
            else if (byte == 0xFF) m_telnetState = TelnetState::SubNegotiation;
            else m_telnetState = TelnetState::SubNegotiation;
            break;
        }
    }
}

void RDevice::processAtCommand(const QString &rawCmd) {
    QString cmd = rawCmd.trimmed().toUpper();
    if (cmd.startsWith("AT")) cmd.remove(0, 2);

    if (cmd.startsWith("DT")) {
        QString target = cmd.mid(2).trimmed();
        at_handle_dial(target);
    }
    else if (cmd == "H") {
        if (tcpSocket->state() == QAbstractSocket::ConnectedState) tcpSocket->disconnectFromHost();
        if (m_isSshMode && m_ssh->isConnected()) m_ssh->disconnectFromHost();
        sendResultCode(RESULT_OK);
    }
    else if (cmd == "Z") {
        tcpSocket->abort();
        if (m_isSshMode && m_ssh) m_ssh->disconnectFromHost();
        // REMOVED: state = ModemState::CommandMode; (Do not kill SIO stream)
        sendResultCode(RESULT_OK);
    }
    else if (cmd == "O") {
        bool active = (tcpSocket->state() == QAbstractSocket::ConnectedState) || (m_isSshMode && m_ssh->isConnected());
        if (active) {
            sendResultCode(RESULT_CONNECT);
        } else {
            sendResultCode(RESULT_ERROR);
        }
    }
    else {
        sendResultCode(RESULT_OK);
    }
}

void RDevice::sendResultCode(int code) {
    QByteArray resp;
    if (verboseResponses) {
        if (code == RESULT_OK) resp = "\r\nOK\r\n";
        else if (code == RESULT_CONNECT) resp = "\r\nCONNECT\r\n";
        else if (code == RESULT_NO_CARRIER) resp = "\r\nNO CARRIER\r\n";
        else if (code == RESULT_ERROR) resp = "\r\nERROR\r\n";
    } else {
        resp = QString("%1\r").arg(code).toLatin1();
    }

    // Crucial: Route to the active stream buffer if in Concurrent Mode
    if (state == ModemState::StreamMode) {
        QMutexLocker locker(&m_bufferMutex);
        m_networkToSioBuffer.append(resp);
    } else {
        m_txBuffer.append(resp);
    }
}

void RDevice::at_handle_dial(const QString &target) {
    QString host = target;
    int port = 23;
    bool found = false;

    for (const BbsEntry &entry : m_phonebook) {
        if (entry.name.compare(target, Qt::CaseInsensitive) == 0) {
            host = entry.ip; port = entry.port; m_currentConnection = entry; found = true; break;
        }
    }

    if (!found) {
        m_currentConnection = BbsEntry();

        // --- NEW: Allow manual SSH-AUTH or SSH prefixes via AT commands ---
        if (host.startsWith("SSH-AUTH:")) {
            m_currentConnection.protocol = "SSH-AUTH"; host = host.mid(9); port = 22;
        }
        else if (host.startsWith("SSH:")) {
            m_currentConnection.protocol = "SSH"; host = host.mid(4); port = 22;
        }
        else if (target.contains(":")) {
            QStringList parts = target.split(":"); host = parts[0]; port = parts[1].toInt();
        }
        m_currentConnection.ip = host; m_currentConnection.port = port;
    }

    sendAtResponse("DIALING " + host + "...\r\n");

    QString proto = m_currentConnection.protocol.toUpper();

    // --- SMART PROTOCOL LOGIC ---
    if (proto.startsWith("SSH") || port == 22) {
        m_isSshMode = true;
        if (tcpSocket->state() != QAbstractSocket::UnconnectedState) tcpSocket->abort();

        if (proto == "SSH-AUTH") {
            // Linux Box Mode: Strict cryptographic SSH authentication
            QString user = m_currentConnection.login.isEmpty() ? "guest" : m_currentConnection.login;
            m_ssh->connectToHost(host, port, user, m_currentConnection.password);
        } else {
            // Retro BBS Mode: Anonymous connection, bypass protocol auth
            m_ssh->connectToHost(host, port, "", "");
        }
    } else {
        // Telnet Mode
        m_isSshMode = false;
        if (m_ssh->isConnected()) m_ssh->disconnectFromHost();
        tcpSocket->connectToHost(host, port);
    }
}


void RDevice::sendAtResponse(const QString &text) {
    if (state == ModemState::StreamMode) {
        QMutexLocker locker(&m_bufferMutex);
        m_networkToSioBuffer.append(text.toLatin1());
    } else {
        m_txBuffer.append(text.toLatin1());
    }
}

void RDevice::loadPhonebook(const QString &path) {
    if (path.isEmpty()) return;
    m_phonebook.clear();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QDomDocument doc;
    if (!doc.setContent(&file)) { file.close(); return; }
    file.close();

    QDomElement root = doc.documentElement();
    QDomElement pb = root.firstChildElement("Phonebook");
    QDomNodeList list = pb.elementsByTagName("BBS");

    for (int i = 0; i < list.size(); i++) {
        QDomElement e = list.at(i).toElement();
        BbsEntry bbs;
        bbs.name = e.attribute("name"); bbs.ip = e.attribute("ip"); bbs.port = e.attribute("port").toInt();
        bbs.protocol = e.attribute("protocol"); bbs.login = e.attribute("login"); bbs.password = e.attribute("password");
        m_phonebook.append(bbs);
    }
}

void RDevice::forceCommandMode()
{
    if (state == ModemState::StreamMode) {
        state = ModemState::CommandMode;
        m_plusCount = 0;

        // Reset the SioWorker's stream flags and hardware block mode
        sio->onStreamFinished();

        qDebug() << "!d" << "[RDevice] Emulation stopped: Forced back to Command Mode.";
    }
}

void RDevice::onSocketConnected() { sendResultCode(RESULT_CONNECT); }
void RDevice::onSocketDisconnected() {
    if (m_isSshMode) return;
    sendResultCode(RESULT_NO_CARRIER);
    // REMOVED: state = ModemState::CommandMode; (Do not kill SIO stream)
}
void RDevice::onSocketError(QAbstractSocket::SocketError) {
    if (m_isSshMode) return;
    sendResultCode(RESULT_ERROR);
}
void RDevice::onNewConnection() { }

void RDevice::onSshConnected() { sendResultCode(RESULT_CONNECT); }
void RDevice::onSshDisconnected() {
    if (!m_isSshMode) return;
    sendResultCode(RESULT_NO_CARRIER);
    // REMOVED: state = ModemState::CommandMode; (Do not kill SIO stream)
}
void RDevice::onSshError(const QString &msg) {
    Q_UNUSED(msg);
    if (!m_isSshMode) return;
    sendResultCode(RESULT_ERROR);
}

// --- NEW METHOD FOR SIOWORKER TO CALL ---
QByteArray RDevice::dequeueNetworkData() {
    QMutexLocker locker(&m_bufferMutex);
    QByteArray data = m_networkToSioBuffer;
    m_networkToSioBuffer.clear();
    return data;
}
