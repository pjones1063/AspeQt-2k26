#include "rdevice.h"
#include "rdevice_handler.h" // Binary blobs
#include "aspeqtsettings.h"
#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include <QDomDocument>

// --- Constants ---
#define RESULT_OK           0
#define RESULT_CONNECT      1
#define RESULT_RING         2
#define RESULT_NO_CARRIER   3
#define RESULT_ERROR        4

#define ESCAPE_GUARD_TIME   1000 // 1 second silence required

RDevice::RDevice(SioWorker *worker) : SioDevice(worker)
{
    // TCP Client
    tcpSocket = new QTcpSocket(this);
    tcpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    connect(tcpSocket, &QTcpSocket::connected, this, &RDevice::onSocketConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &RDevice::onSocketDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &RDevice::onSocketReadyRead);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, &RDevice::onSocketError);

    // [NEW] SSH Client
    m_ssh = new SshClient(this);
    connect(m_ssh, &SshClient::connected, this, &RDevice::onSshConnected);
    connect(m_ssh, &SshClient::disconnected, this, &RDevice::onSshDisconnected);
    connect(m_ssh, &SshClient::rxData, this, &RDevice::onSshDataReceived);
    connect(m_ssh, &SshClient::error, this, &RDevice::onSshError);

    // TCP Server
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &RDevice::onNewConnection);
    pendingSocket = nullptr;

    m_isEnabled = (aspeqtSettings && aspeqtSettings->enableRDevice());
    if (m_isEnabled) loadPhonebook(aspeqtSettings->modemBridgePhonebookPath());
}

RDevice::~RDevice()
{
    tcpSocket->abort();
    m_ssh->disconnectFromHost(); // [NEW] Cleanup SSH
    tcpServer->close();
}

void RDevice::setEnabled(bool enable)
{
    m_isEnabled = enable;
    if (!enable) {
        tcpSocket->disconnectFromHost();
        m_ssh->disconnectFromHost(); // [NEW]
        tcpServer->close();
        state = ModemState::CommandMode;
        m_txBuffer.clear();
    }
}

// --------------------------------------------------------------------------
// SIO Dispatcher (Unchanged)
// --------------------------------------------------------------------------
void RDevice::handleCommand(quint8 command, quint16 aux)
{
    if (!m_isEnabled) {
        return;
    }

    quint8 aux1 = (aux & 0xFF);
    quint8 aux2 = (aux >> 8) & 0xFF;

    switch (command) {
    case CMD_POLL_TYPE1: handlePollType1(); break;
    case CMD_POLL_TYPE3: handlePollType3(aux1, aux2); break;
    case CMD_RELOCATOR:  handleDownloadRelocator(); break;
    case CMD_DOWNLOAD:   handleDownloadDriver(); break;
    case CMD_STATUS:     handleStatus(); break;
    case CMD_WRITE:      handleWrite(aux); break;
    case CMD_READ:       handleRead(aux); break;
    case CMD_CONTROL:    handleControl(aux); break;
    case CMD_STREAM:     handleStream(); break;
    case CMD_LISTEN:     handleListen(aux); break;
    case CMD_UNLISTEN:   sio->port()->writeCommandAck(); tcpServer->close(); sio->port()->writeComplete(); break;
    default:             sio->port()->writeCommandNak(); break;
    }
}

// --------------------------------------------------------------------------
// Stream Mode
// --------------------------------------------------------------------------

void RDevice::handleStream()
{
    if (!sio->port()->writeCommandAck()) return;

    // 1. Send POKEY Table to Atari (Standard 19200 table for compatibility)
    const char table[] = {0x28, (char)0xA0, 0x00, (char)0xA0, 0x28, (char)0xA0, 0x00, (char)0xA0, 0x78};
    QByteArray payload(table, 9);

    SioWorker::usleep(5000);
    sio->port()->writeDataFrame(payload);
    sio->port()->writeComplete();

    // 2. Switch State
    state = ModemState::StreamMode;
    m_escapeTimer.start();
    m_plusCount = 0;

    // 3. Tell Hardware to Switch Speed (assuming 19200)
    emit requestBaudRateChange(19200);

    qDebug() << "[RDevice] Entered Stream Mode (Event Driven)";
}

// Called by SioWorker when it receives bytes from Atari
void RDevice::processSerialData(const QByteArray &data)
{
    if (state != ModemState::StreamMode) return;

    // 1. Check for Escape Sequence (+++)
    checkEscapeSequence(data);

    // 2. Send to Network (SSH or TCP)
    // [NEW] Check active protocol
    if (m_isSshMode) {
        if (m_ssh->isConnected()) m_ssh->write(data);
    } else {
        if (tcpSocket->state() == QAbstractSocket::ConnectedState) tcpSocket->write(data);
    }
}

void RDevice::checkEscapeSequence(const QByteArray &data)
{
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
        qDebug() << "[RDevice] Escape Sequence Detected";
        state = ModemState::CommandMode;
        emit streamModeFinished();
        sendResultCode(RESULT_OK);
        m_plusCount = 0;
    }
}

// Called by Qt when TCP data arrives (Atari <- Net)
void RDevice::onSocketReadyRead()
{
    if (m_isSshMode) return; // Ignore TCP if we are in SSH mode

    QByteArray data = tcpSocket->readAll();

    if (state == ModemState::StreamMode) {
        // Parse Telnet Options (Strip IAC)
        parseTelnet(data);

        // If we have clean data, send it to SioWorker
        if (!m_txBuffer.isEmpty()) {
            emit sendSerialData(m_txBuffer);
            m_txBuffer.clear();
        }
    } else {
        m_txBuffer.append(data);
    }
}

// [NEW] SSH Ready Read
void RDevice::onSshDataReceived(const QByteArray &data)
{
    // SSH usually already handles PTY/Telnet negotiation internally via libssh
    // so we just pass raw data through.

    if (state == ModemState::StreamMode) {
        emit sendSerialData(data);
    } else {
        m_txBuffer.append(data);
    }
}

// --------------------------------------------------------------------------
// Telnet State Machine (Only used for TCP)
// --------------------------------------------------------------------------
void RDevice::parseTelnet(const QByteArray &data)
{
    // ... (Keep existing implementation unchanged) ...
    for (char c : data) {
        unsigned char byte = (unsigned char)c;

        switch (m_telnetState) {
        case TelnetState::Normal:
            if (byte == 0xFF) { m_telnetState = TelnetState::IacReceived; }
            else { m_txBuffer.append(c); }
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
            if (byte == 0xFF) { m_telnetState = TelnetState::SubIac; }
            break;
        case TelnetState::SubIac:
            if (byte == 0xF0) { m_telnetState = TelnetState::Normal; }
            else if (byte == 0xFF) { m_telnetState = TelnetState::SubNegotiation; }
            else { m_telnetState = TelnetState::SubNegotiation; }
            break;
        }
    }
}

// --------------------------------------------------------------------------
// Command Handlers
// --------------------------------------------------------------------------

void RDevice::handleWrite(quint16 len)
{
    if (!sio->port()->writeCommandAck()) return;

    QByteArray data = sio->port()->readDataFrame(len > 0 ? len : 128);
    sio->port()->writeDataAck();

    for (char c : data) {
        if (c == 0x0D || (quint8)c == 0x9B) {
            processAtCommand(m_atCmdBuffer);
            m_atCmdBuffer.clear();
        } else {
            m_atCmdBuffer.append(c);
        }
    }

    sio->port()->writeComplete();
}

void RDevice::handleRead(quint16 len)
{
    if (!sio->port()->writeCommandAck()) return;

    int readLen = (len > 0) ? len : 128;
    QByteArray chunk = m_txBuffer.left(readLen);
    m_txBuffer.remove(0, chunk.size());

    while (chunk.size() < readLen) chunk.append((char)0x00);

    sio->port()->writeComplete();
    sio->port()->writeDataFrame(chunk);
}

void RDevice::processAtCommand(const QString &rawCmd)
{
    QString cmd = rawCmd.trimmed().toUpper();
    if (cmd.startsWith("AT")) cmd.remove(0, 2);

    if (cmd.startsWith("DT")) {
        QString target = cmd.mid(2).trimmed();
        at_handle_dial(target);
    }
    else if (cmd == "H") {
        // [NEW] Disconnect both
        tcpSocket->disconnectFromHost();
        m_ssh->disconnectFromHost();
        // onSocketDisconnected/onSshDisconnected will send NO CARRIER
    }
    else if (cmd == "Z") {
        tcpSocket->abort();
        m_ssh->disconnectFromHost();
        state = ModemState::CommandMode;
        sendResultCode(RESULT_OK);
    }
    else if (cmd == "O") {
        // Check connection on either
        bool active = (tcpSocket->state() == QAbstractSocket::ConnectedState) || m_ssh->isConnected();

        if (active) {
            sendResultCode(RESULT_CONNECT);
            state = ModemState::StreamMode;
            emit streamModeFinished();
        } else {
            sendResultCode(RESULT_ERROR);
        }
    }
    else {
        sendResultCode(RESULT_OK);
    }
}

void RDevice::sendResultCode(int code)
{
    if (verboseResponses) {
        if (code == RESULT_OK) m_txBuffer.append("\r\nOK\r\n");
        else if (code == RESULT_CONNECT) m_txBuffer.append("\r\nCONNECT\r\n");
        else if (code == RESULT_NO_CARRIER) m_txBuffer.append("\r\nNO CARRIER\r\n");
        else if (code == RESULT_ERROR) m_txBuffer.append("\r\nERROR\r\n");
    } else {
        m_txBuffer.append(QString("%1\r").arg(code).toLatin1());
    }
}

// --------------------------------------------------------------------------
// Dialing Logic (Updated for SSH)
// --------------------------------------------------------------------------
void RDevice::at_handle_dial(const QString &target)
{
    QString host = target;
    int port = 23;
    bool found = false;

    // 1. Phonebook Lookup
    for (const BbsEntry &entry : m_phonebook) {
        if (entry.name.compare(target, Qt::CaseInsensitive) == 0) {
            host = entry.ip;
            port = entry.port;
            m_currentConnection = entry; // Stores protocol, login, pass
            found = true;
            break;
        }
    }

    // 2. Raw Parse
    if (!found) {
        m_currentConnection = BbsEntry();

        // Check SSH Prefix
        if (host.startsWith("SSH:")) {
            m_currentConnection.protocol = "SSH";
            host = host.mid(4);
            port = 22;
        }
        else if (target.contains(":")) {
            QStringList parts = target.split(":");
            host = parts[0];
            port = parts[1].toInt();
        }

        m_currentConnection.ip = host;
        m_currentConnection.port = port;
    }

    sendAtResponse("DIALING " + host + "...\r\n");

    // 3. Switch Protocols
    if (m_currentConnection.protocol == "SSH" || port == 22) {
        m_isSshMode = true;
        // Kill TCP if active
        if (tcpSocket->state() != QAbstractSocket::UnconnectedState) tcpSocket->abort();

        QString user = m_currentConnection.login.isEmpty() ? "guest" : m_currentConnection.login;
        m_ssh->connectToHost(host, port, user, m_currentConnection.password);
    }
    else {
        m_isSshMode = false;
        // Kill SSH if active
        if (m_ssh->isConnected()) m_ssh->disconnectFromHost();

        tcpSocket->connectToHost(host, port);
    }
}

void RDevice::sendAtResponse(const QString &text)
{
    m_txBuffer.append(text.toLatin1());
}

void RDevice::loadPhonebook(const QString &path)
{
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

    for (int i = 0; i < list.count(); i++) {
        QDomElement e = list.at(i).toElement();
        BbsEntry bbs;
        bbs.name = e.attribute("name");
        bbs.ip = e.attribute("ip");
        bbs.port = e.attribute("port").toInt();
        bbs.protocol = e.attribute("protocol"); // [NEW] Added protocol
        bbs.login = e.attribute("login");
        bbs.password = e.attribute("password");
        m_phonebook.append(bbs);
    }
    qDebug() << "[RDevice] Loaded" << m_phonebook.count() << "phonebook entries.";
}

// --------------------------------------------------------------------------
// Socket / SSH Event Slots
// --------------------------------------------------------------------------
void RDevice::onSocketConnected() { sendResultCode(RESULT_CONNECT); }
void RDevice::onSocketDisconnected() {
    if (m_isSshMode) return;
    sendResultCode(RESULT_NO_CARRIER);
    state = ModemState::CommandMode;
    emit streamModeFinished();
}
void RDevice::onSocketError(QAbstractSocket::SocketError) {
    if (m_isSshMode) return;
    sendResultCode(RESULT_ERROR);
}
void RDevice::onNewConnection() { /* Handle incoming call */ }

// [NEW] SSH Callbacks
void RDevice::onSshConnected() {
    sendResultCode(RESULT_CONNECT);
}
void RDevice::onSshDisconnected() {
    if (!m_isSshMode) return;
    sendResultCode(RESULT_NO_CARRIER);
    state = ModemState::CommandMode;
    emit streamModeFinished();
}
void RDevice::onSshError(const QString &msg) {
    Q_UNUSED(msg);
    if (!m_isSshMode) return;
    sendResultCode(RESULT_ERROR);
}

// Boilerplate Handlers
void RDevice::handlePollType1() {
    if (!sio->port()->writeCommandAck()) return;
    QByteArray bootBlock(12, 0);
    bootBlock[0]=0x50; bootBlock[1]=0x01; bootBlock[2]=0x21; bootBlock[3]=0x40;
    bootBlock[4]=0x00; bootBlock[5]=0x05; bootBlock[8]=sizeof(relocator_stub)&0xFF; bootBlock[9]=sizeof(relocator_stub)>>8;
    SioWorker::usleep(5000);
    sio->port()->writeDataFrame(bootBlock);
}

void RDevice::handlePollType3(quint8 aux1, quint8 aux2) {
    bool respond = (aux1 == 0x52 && aux2 == 0x01); // Respond to R1:
    if (respond) {
        if (!sio->port()->writeCommandAck()) return;
        QByteArray resp(4, 0);
        resp[0] = sizeof(driver_850)&0xFF;
        resp[1] = sizeof(driver_850)>>8;
        resp[2] = 0x50;
        SioWorker::usleep(5000);
        sio->port()->writeDataFrame(resp);
    }
}

void RDevice::handleDownloadRelocator() {
    if (!sio->port()->writeCommandAck()) return;
    QByteArray payload((const char*)relocator_stub, sizeof(relocator_stub));
    SioWorker::usleep(5000);
    sio->port()->writeDataFrame(payload);
}

void RDevice::handleDownloadDriver() {
    if (!sio->port()->writeCommandAck()) return;
    QByteArray payload((const char*)driver_850, sizeof(driver_850));
    SioWorker::usleep(5000);
    sio->port()->writeDataFrame(payload);
}

void RDevice::handleStatus() {
    if (!sio->port()->writeCommandAck()) return;
    QByteArray status(4, 0);
    status[1] = 0x10 | 0x80; // CTS | DSR
    // Check either connection
    if (tcpSocket->state() == QAbstractSocket::ConnectedState || (m_isSshMode && m_ssh->isConnected()))
        status[1] |= 0x40; // CD
    sio->port()->writeComplete();
    sio->port()->writeDataFrame(status);
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
