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

RDevice::RDevice(SioWorker *worker, int portIndex) : SioDevice(worker), m_portIndex(portIndex) // <-- Add to initializer
{

    m_portIndex = portIndex;
    tcpSocket = new QTcpSocket(this);
    tcpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    m_ringPhase = false;

    connect(tcpSocket, &QTcpSocket::connected, this, &RDevice::onSocketConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &RDevice::onSocketDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &RDevice::onSocketReadyRead);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, &RDevice::onSocketError);

    m_ssh = new SshClient(this);
    connect(m_ssh, &SshClient::connected, this, &RDevice::onSshConnected);
    connect(m_ssh, &SshClient::disconnected, this, &RDevice::onSshDisconnected);
    connect(m_ssh, &SshClient::rxData, this, &RDevice::onSshDataReceived);
    connect(m_ssh, &SshClient::error, this, &RDevice::onSshError);

    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &RDevice::onNewConnection);
    pendingSocket = nullptr;

    m_ringTimer = new QTimer(this);
    m_ringTimer->setSingleShot(true);
    connect(m_ringTimer, &QTimer::timeout, this, &RDevice::onRingTimeout);
    m_escapeActionTimer = new QTimer(this);
    m_escapeActionTimer->setSingleShot(true);
    connect(m_escapeActionTimer, &QTimer::timeout, this, &RDevice::onEscapeTriggered);

    m_isEnabled = (aspeqtSettings && aspeqtSettings->modemTransportMode() == 0);

    bool shouldListen = aspeqtSettings->bbsListenerEnabled(m_portIndex) && m_isEnabled;
    int port = aspeqtSettings->modemListenPort(m_portIndex);

    if (m_isEnabled) loadPhonebook(aspeqtSettings->modemBridgePhonebookPath());

    updateListenerConfig();

    connect(this, &RDevice::dispatchToNetwork, this, [this](const QByteArray &data){
        if (m_isNetworkConnected) {
            // [FIX] Removed the duplicate parsing of macros and escape sequences!
            // We just pass the clean, pre-filtered data straight to the socket.
            if (m_isSshMode) m_ssh->write(data);
            else tcpSocket->write(data);
        }
        else {
            for (char c : data) {
                if (c == 0x0D || (quint8)c == 0x9B) { // Carriage Return
                    if (echoEnabled) {
                        QMutexLocker locker(&m_bufferMutex);
                        m_networkToSioBuffer.append(c); // Echo the CR cleanly
                    }
                    emit executeAtCommand(m_atCmdBuffer);
                    m_atCmdBuffer.clear();
                } else if (c == 8 || c == 126 || c == 127) { // Backspace or Delete
                    if (!m_atCmdBuffer.isEmpty()) {
                        m_atCmdBuffer.chop(1); // Remove from the internal command buffer
                        if (echoEnabled && !m_waitingForSshPassword) {
                            QMutexLocker locker(&m_bufferMutex);
                            // Visually erase the character: Backspace, Space, Backspace
                            m_networkToSioBuffer.append(char(8));
                            m_networkToSioBuffer.append(' ');
                            m_networkToSioBuffer.append(char(8));
                        }
                    }
                } else { // Normal typing
                    if (echoEnabled && !m_waitingForSshPassword) {
                        QMutexLocker locker(&m_bufferMutex);
                        m_networkToSioBuffer.append(c); // Echo normal characters
                    }
                    m_atCmdBuffer.append(c);
                }
            }
        }
    }, Qt::QueuedConnection);


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
        QMutexLocker locker(&m_bufferMutex);
        m_txBuffer.clear();
    } else {
        updateListenerConfig();
    }
}


void RDevice::handleCommand(quint8 command, quint16 aux)
{
    if (!m_isEnabled) return;

    quint8 aux1 = (aux & 0xFF);
    quint8 aux2 = (aux >> 8) & 0xFF;

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
    case CMD_CONFIGURE:  handleConfigure(aux1, aux2); break;
    case CMD_CONTROL:    handleControl(aux); break;
    case CMD_AUTOANSWER:
        if (!sio->port()->writeCommandAck()) return;
        sio->port()->writeComplete();
        break;
    case CMD_STREAM:
        handleStream();
        break;
    case CMD_LISTEN:
        handleListen(aux);
        break;
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

void RDevice::handleConfigure(quint8 aux1, quint8 aux2) {
    Q_UNUSED(aux2);
    if (!sio->port()->writeCommandAck()) return;

    quint8 speedCode = aux1 & 0x0F;
    switch (speedCode) {
    case 0x08: m_currentBaudRate = 300; break;
    case 0x09: m_currentBaudRate = 600; break;
    case 0x0A: m_currentBaudRate = 1200; break;
    case 0x0B: m_currentBaudRate = 1800; break;
    case 0x0C: m_currentBaudRate = 2400; break;
    case 0x0D: m_currentBaudRate = 4800; break;
    case 0x0E: m_currentBaudRate = 9600; break;
    case 0x0F: m_currentBaudRate = 19200; break;
    default:   m_currentBaudRate = 19200; break;
    }

    qDebug() << "!d" << "[RDevice] Configure: Requested Baud Rate set to:" << m_currentBaudRate;
    sio->port()->writeComplete();
}

void RDevice::sendDataToAtari(const QByteArray &data)
{
    SioWorker::usleep(2000);
    sio->port()->writeComplete();
    SioWorker::usleep(2000);
    sio->port()->writeDataFrame(data);
}

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
    status[1] = 0xC0 | 0x30;

    // [FIX] We now use the thread-safe atomic flag instead of touching tcpSocket
    if (m_isNetworkConnected) {
        status[1] |= 0x0C;
    }

    sendDataToAtari(status);
}

void RDevice::handleRead(quint16 len) {
    if (!sio->port()->writeCommandAck()) return;

    int readLen = (len > 0) ? len : 128;
    QByteArray chunk;

    // [FIX] Protect the extraction of data to prevent Main Thread crashes
    {
        QMutexLocker locker(&m_bufferMutex);
        chunk = m_txBuffer.left(readLen);
        m_txBuffer.remove(0, chunk.size());
    }

    while (chunk.size() < readLen) chunk.append((char)0x00);

    sendDataToAtari(chunk);
}

void RDevice::handleStream() {
    if (!sio->port()->writeCommandAck()) return;

    QByteArray response;
    response.resize(9);
    response[0] = 0x28; response[1] = 0xA0; response[2] = 0x00;
    response[3] = 0xA0; response[4] = 0x28; response[5] = 0xA0;
    response[6] = 0x00; response[7] = 0xA0; response[8] = 0x78;

    switch (m_currentBaudRate) {
    case 300:   response[0] = response[4] = 0xA0; response[2] = response[6] = 0x0B; break;
    case 600:   response[0] = response[4] = 0xCC; response[2] = response[6] = 0x05; break;
    case 1200:  response[0] = response[4] = 0xE3; response[2] = response[6] = 0x02; break;
    case 1800:  response[0] = response[4] = 0xEA; response[2] = response[6] = 0x01; break;
    case 2400:  response[0] = response[4] = 0x6E; response[2] = response[6] = 0x01; break;
    case 4800:  response[0] = response[4] = 0xB3; response[2] = response[6] = 0x00; break;
    case 9600:  response[0] = response[4] = 0x56; response[2] = response[6] = 0x00; break;
    case 19200: break;
    }

    sendDataToAtari(response);

    {
        QMutexLocker locker(&m_bufferMutex);
        m_networkToSioBuffer.clear();
        m_txBuffer.clear();
    }

    state = ModemState::StreamMode;
    m_escapeTimer.start();
    m_plusCount = 0;

    sio->onChangeBaudRate(m_currentBaudRate);

    SioWorker::usleep(1100);
    if (sio->port()) {
        sio->port()->readRawFrame(256, false);
    }

    qDebug() << "!d" << "[RDevice] Stream active" << m_currentBaudRate;
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

    for (int i = 0; i < logicalLen && i < data.size(); i++) {
        char c = data.at(i);
        if (c == 0x0D || (quint8)c == 0x9B) {
            emit executeAtCommand(m_atCmdBuffer);
            m_atCmdBuffer.clear();
        }
        else if (c == 8 || c == 126 || c == 127) {
            if (!m_atCmdBuffer.isEmpty()) {
                m_atCmdBuffer.chop(1); // Silently remove from the actual command buffer
            }
        }
        else {
            m_atCmdBuffer.append(c);
        }
    }
    sio->port()->writeComplete();
}

void RDevice::handleControl(quint16 aux) {
    if (!sio->port()->writeCommandAck()) return;

    quint8 ctrl = aux & 0xFF;

    // Atari 850 CMD_CONTROL (0x41) Aux1 bits:
    // Bit 7 (0x80) = DTR (Data Terminal Ready).
    // If BBS Express clears this bit, it physically drops the line to hang up.
    if ((ctrl & 0x80) == 0) {
        qDebug() << "!i [RDevice] DTR drop detected via CMD_CONTROL. Forcing hangup.";
        QMetaObject::invokeMethod(this, "hangup", Qt::QueuedConnection);
    }

    sio->port()->writeComplete();
}

void RDevice::handleListen(quint16 aux) {
    if (tcpServer->isListening()) {
        // [FIX] If BBS Listener is already active, acknowledge silently
        sio->port()->writeCommandAck();
        sio->port()->writeComplete();
    } else if (tcpServer->listen(QHostAddress::Any, aux)) {
        sio->port()->writeCommandAck();
        sio->port()->writeComplete();
    } else {
        sio->port()->writeCommandNak();
    }
}


void RDevice::processSerialData(const QByteArray &data) {
    if (state != ModemState::StreamMode) return;

    static bool escPending = false;
    QByteArray finalDataToNetwork;

    for (int i = 0; i < data.size(); ++i) {
        char c = data[i];

        // Convert Atari Backspace to Standard Backspace here
        if (c == 126 || c == 127) c = 8;

        if (escPending) {
            escPending = false;

            if (c == 'U' || c == 'u') {
                QString inject = m_currentConnection.login + "\r";
                emit dispatchToNetwork(inject.toUtf8());
                continue;
            }
            if (c == 'P' || c == 'p') {
                QString inject = m_currentConnection.password + "\r";
                emit dispatchToNetwork(inject.toUtf8());
                continue;
            }
            if (c == 'H' || c == 'h') {
                QMetaObject::invokeMethod(this, "hangup", Qt::QueuedConnection);
                continue;
            }

            finalDataToNetwork.append(0x1B);
            finalDataToNetwork.append(c);
        } else if (c == 0x1B) {
            escPending = true;

        } else {

            // --- TIES: Pass-Through Logic ---
            if (m_isNetworkConnected) {
                if (c == '+') {
                    m_escapeBuffer.append(c);
                    finalDataToNetwork.append(c);
                    if (m_escapeBuffer.length() > 3) {
                        m_escapeBuffer.clear();
                    } else if (m_escapeBuffer.length() == 3) {
                        QMetaObject::invokeMethod(m_escapeActionTimer, "start", Qt::QueuedConnection, Q_ARG(int, 1000));
                    }
                } else {
                    m_escapeBuffer.clear();
                    QMetaObject::invokeMethod(m_escapeActionTimer, "stop", Qt::QueuedConnection);
                    finalDataToNetwork.append(c);
                }
            } else {
                finalDataToNetwork.append(c);
            }
        }
      }

    // Only dispatch to the network lambda if there is actually data left!
    if (!finalDataToNetwork.isEmpty()) {
        emit dispatchToNetwork(finalDataToNetwork);
    }
}


void RDevice::onEscapeTriggered() {
    if (m_escapeBuffer == "+++") {
        m_isNetworkConnected = false;
        qDebug() << "!i [RDevice] +++ Escape Sequence triggered. Dropping to AT Mode.";
        sendResultCode(RESULT_OK);
    }
    // No flushing needed, just clear the tracker
    m_escapeBuffer.clear();
}



void RDevice::onSocketReadyRead() {
    if (m_isSshMode) return;
    QByteArray data = tcpSocket->readAll();

    // [FIX] Wrap all background modifications to the Tx buffer in a mutex lock
    QMutexLocker locker(&m_bufferMutex);
    parseTelnet(data);

    if (state == ModemState::StreamMode) {
        if (!m_txBuffer.isEmpty()) {
            m_networkToSioBuffer.append(m_txBuffer);
            m_txBuffer.clear();
        }
    }
}

void RDevice::onSshDataReceived(const QByteArray &data) {
    QMutexLocker locker(&m_bufferMutex);
    if (state == ModemState::StreamMode) {
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
            // <-- The rejection logic belongs out here in the main state switch!
            if (m_telnetState == TelnetState::Will || m_telnetState == TelnetState::Do) {
                QByteArray reject;
                reject.append((char)0xFF);
                reject.append(m_telnetState == TelnetState::Will ? (char)0xFE : (char)0xFC); // DONT or WONT
                reject.append((char)byte);
                if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
                    tcpSocket->write(reject);
                }
            }
            m_telnetState = TelnetState::Normal;
            break;
        case TelnetState::SubNegotiation:
            if (byte == 0xFF) m_telnetState = TelnetState::SubIac;
            break;
        case TelnetState::SubIac:
            if (byte == 0xF0) m_telnetState = TelnetState::Normal;
            else if (byte != 0xFF) m_telnetState = TelnetState::SubNegotiation;
            break;
        }
    }
}

void RDevice::processAtCommand(const QString &rawCmd) {

    if (m_waitingForSshPassword) {
        m_waitingForSshPassword = false;
        m_currentConnection.password = rawCmd.trimmed();
        executeInteractiveSshDial();
        return;
    }

    QString cmd = rawCmd.trimmed().toUpper();
    if (cmd.startsWith("AT")) cmd.remove(0, 2);

    if (cmd.contains("E0")) {
        echoEnabled = false; cmd.replace("E0", "");
    }
    else if (cmd.contains("E1")) {
        echoEnabled = true; cmd.replace("E1", "");
    }
    if (cmd.contains("V0")) {
        verboseResponses = false; cmd.replace("V0", "");
    }
    else if (cmd.contains("V1")) {
        verboseResponses = true; cmd.replace("V1", "");
    }

    else if (cmd == "A" || cmd.startsWith("A ")) {
        if (m_ringPhase && pendingSocket) {
            m_ringTimer->stop();
            tcpSocket->disconnect(this); // [FIX] Safe disconnect
            tcpSocket->deleteLater();
            tcpSocket = pendingSocket;
            pendingSocket = nullptr;
            m_ringPhase = false;
            m_isSshMode = false;
            m_isNetworkConnected = true;

            connect(tcpSocket, &QTcpSocket::connected, this, &RDevice::onSocketConnected);
            connect(tcpSocket, &QTcpSocket::disconnected, this, &RDevice::onSocketDisconnected);
            connect(tcpSocket, &QTcpSocket::readyRead, this, &RDevice::onSocketReadyRead);
            connect(tcpSocket, &QTcpSocket::errorOccurred, this, &RDevice::onSocketError);

            sendResultCode(RESULT_CONNECT);
        } else {
            sendResultCode(RESULT_ERROR);
        }
    }

    // --- S0 REGISTER (Auto-Answer) ---
    else if (cmd.startsWith("S0=")) {
        bool ok;
        int val = cmd.mid(3).trimmed().toInt(&ok);
        if (ok && val >= 0 && val <= 255) {
            m_s0Register = val;
            sendResultCode(RESULT_OK);
        } else {
            sendResultCode(RESULT_ERROR);
        }
    }
    else if (cmd == "S0?") {
        QString valStr = QString("%1\r\n").arg(m_s0Register, 3, 10, QChar('0'));
        sendAtResponse(valStr);
        sendResultCode(RESULT_OK);
    }

    else if (cmd.startsWith("DT")) {
        int dtIndex = rawCmd.toUpper().indexOf("DT");
        QString target = rawCmd.mid(dtIndex + 2).trimmed();
        at_handle_dial(target);
    }


    else if (cmd == "H") {
        hangup();
    }
    else if (cmd == "Z") {
        {
            QMutexLocker locker(&m_bufferMutex);
            m_txBuffer.clear();
            m_networkToSioBuffer.clear();
            m_atCmdBuffer.clear();
        }
        tcpSocket->abort();
        if (m_isSshMode && m_ssh) m_ssh->disconnectFromHost();

        if (pendingSocket) {
            pendingSocket->disconnect(this);
            pendingSocket->disconnectFromHost();
            pendingSocket->deleteLater();
            pendingSocket = nullptr;
            m_ringPhase = false;
            m_ringTimer->stop();
        }

        sendResultCode(RESULT_OK);
    }

    // --- RETURN TO ONLINE (ATO) ---
    else if (cmd == "O" || cmd.startsWith("O0") || cmd.startsWith("O ")) {

        // BULLETPROOF CHECK: Interrogate the actual physical sockets
        bool hasActiveConnection = (tcpSocket->state() == QAbstractSocket::ConnectedState) ||
                                   (m_isSshMode && m_ssh->isConnected());

        if (hasActiveConnection) {
            m_isNetworkConnected = true; // Restore routing to the network

            // Reset the escape sequence timers
            m_plusCount = 0;
            m_escapeTimer.restart();
            if (m_escapeActionTimer->isActive()) m_escapeActionTimer->stop();

            // SIO StreamMode was never interrupted, so we just say CONNECT
            sendResultCode(RESULT_CONNECT);
        } else {
            m_isNetworkConnected = false;
            sendResultCode(RESULT_NO_CARRIER); // Authentic Hayes failure response
        }
    }


    else if(cmd.isEmpty()) {
        sendResultCode(RESULT_OK);
    }
    else {
            qDebug() << "!w Unrecognized AT command:" << cmd;
    }
}


void RDevice::sendResultCode(int code) {
    QByteArray resp;
    if (verboseResponses) {
        if (code == RESULT_OK) resp = "\r\nOK\r\n";
        else if (code == RESULT_CONNECT) resp = "\r\nCONNECT\r\n";
        else if (code == RESULT_RING) resp = "\r\nRING\r\n";
        else if (code == RESULT_NO_CARRIER) resp = "\r\nNO CARRIER\r\n";
        else if (code == RESULT_ERROR) resp = "\r\nERROR\r\n";
    } else {
        resp = QString("%1\r").arg(code).toLatin1();
    }

    QMutexLocker locker(&m_bufferMutex);
    if (state == ModemState::StreamMode) {
        m_networkToSioBuffer.append(resp);
    } else {
        m_txBuffer.append(resp);
    }
}

void RDevice::at_handle_dial(const QString &target) {

    if (target.startsWith("SSH:", Qt::CaseInsensitive) && target.contains("@")) {
        parseInteractiveSshTarget(target);
        return;
    }

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

    if (proto.startsWith("SSH") || port == 22) {
        m_isSshMode = true;
        if (tcpSocket->state() != QAbstractSocket::UnconnectedState) tcpSocket->abort();

        if (proto == "SSH-AUTH") {
            QString user = m_currentConnection.login.isEmpty() ? "guest" : m_currentConnection.login;
            m_ssh->connectToHost(host, port, user, m_currentConnection.password);
        } else {
            m_ssh->connectToHost(host, port, "", "");
        }
    } else {
        m_isSshMode = false;
        if (m_ssh->isConnected()) m_ssh->disconnectFromHost();
        tcpSocket->connectToHost(host, port);
    }
}

void RDevice::sendAtResponse(const QString &text) {
    QMutexLocker locker(&m_bufferMutex);
    if (state == ModemState::StreamMode) {
        m_networkToSioBuffer.append(text.toLatin1());
    } else {
        m_txBuffer.append(text.toLatin1());
    }
}

void RDevice::loadPhonebook(const QString &path) {
    m_phonebook.clear();

    if (path.isEmpty()) return;

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

void RDevice::forceCommandMode(bool sendAlert) {
    if (state == ModemState::StreamMode) {
        if (sendAlert) {
            QByteArray alert;
            alert.append(0x9B);
            alert.append("*** VIRTUAL MODEM POWERED OFF ***");
            alert.append(0x9B);
            alert.append("*** SIO IS OUT OF SYNC ***");
            alert.append(0x9B);

            if (sio && sio->port()) {
                sio->port()->writeRawFrame(alert);
                SioWorker::usleep(5000);
            }
        }

        state = ModemState::CommandMode;
        {
            QMutexLocker locker(&m_bufferMutex);
            m_txBuffer.clear();
            m_networkToSioBuffer.clear();
        }

        sio->onStreamFinished();

        qDebug() << "!d" << "[RDevice] Exited Stream Mode. Signaled SioWorker to restore SIO state.";
    }
}

// [FIX] Update Atomic State flag on connect/disconnect
void RDevice::onSocketConnected() {
    m_isNetworkConnected = true;
    sendResultCode(RESULT_CONNECT);
}
void RDevice::onSocketDisconnected() {
    m_isNetworkConnected = false;
    if (m_isSshMode) return;
    sendResultCode(RESULT_NO_CARRIER);
}

void RDevice::onSocketError(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    if (m_isSshMode) return;

    QString errorMsg = tcpSocket->errorString();

    // If we aren't connected yet (e.g. dialing failed), show the verbose error
    if (!m_isNetworkConnected) {
        sendAtResponse("\r\nERROR: " + errorMsg + "\r\n");
        sendResultCode(RESULT_NO_CARRIER);
    } else {
        // If an error happens while connected
        sendResultCode(RESULT_ERROR);
    }
}
void RDevice::onNewConnection() {
    QTcpSocket *client = tcpServer->nextPendingConnection();

    // Busy check: Are we already connected, dialing, or already ringing?
    if (m_isNetworkConnected || tcpSocket->state() != QAbstractSocket::UnconnectedState || m_isSshMode || pendingSocket) {
        qDebug() << "!i [RDevice] Rejected inbound call (Busy).";
        client->disconnectFromHost();
        client->deleteLater();
        return;
    }

    // Park the caller and trigger the Ring Phase
    pendingSocket = client;
    if (m_s0Register > 0) {
        int ringDelay = m_s0Register * 2000;
        QTimer::singleShot(ringDelay, this, &RDevice::onAutoAnswerTriggered);
    } else {
        m_ringTimer->start(30000);
    }
    m_ringPhase = true;
    connect(pendingSocket, &QTcpSocket::disconnected, this, &RDevice::onPendingSocketDisconnected);

    sendResultCode(RESULT_RING);
}

void RDevice::onRingTimeout() {
    qDebug() << "!w [RDevice] Ring timeout: No ATA received within 30s. Dropping caller.";
    if (pendingSocket) pendingSocket->disconnectFromHost(); // Cleanly trigger the disconnect signal
}

void RDevice::onPendingSocketDisconnected() {
    if (pendingSocket) {
        pendingSocket->disconnect(this); // [FIX] Safe disconnect
        pendingSocket->deleteLater();
        pendingSocket = nullptr;
        m_ringTimer->stop();
        m_ringPhase = false;
        sendResultCode(RESULT_NO_CARRIER);
        qDebug() << "!i [RDevice] Caller disconnected before answer.";
    }
}

void RDevice::onSshConnected() {
    m_isNetworkConnected = true;
    sendResultCode(RESULT_CONNECT);
}
void RDevice::onSshDisconnected() {
    m_isNetworkConnected = false;
    if (!m_isSshMode) return;
    sendResultCode(RESULT_NO_CARRIER);
}

void RDevice::onSshError(const QString &msg) {
    if (!m_isSshMode) return;

    // If we aren't connected yet (e.g. auth failed, host unreachable), show the verbose error
    if (!m_isNetworkConnected) {
        sendAtResponse("\r\nERROR: SSH - " + msg + "\r\n");
        sendResultCode(RESULT_NO_CARRIER);
    } else {
        // If an error happens while connected
        sendResultCode(RESULT_ERROR);
    }
}

QByteArray RDevice::dequeueNetworkData() {
    QMutexLocker locker(&m_bufferMutex);
    QByteArray data = m_networkToSioBuffer;
    m_networkToSioBuffer.clear();
    return data;
}

void RDevice::dial(const BbsEntry &entry) {
    m_currentConnection = entry;

    // 1. Clean up any existing connections
    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
        tcpSocket->disconnectFromHost();
    }
    if (m_ssh->isConnected()) {
        m_ssh->disconnectFromHost();
    }

    // 2. Determine Protocol (Catch both SSH and SSH-AUTH)
    bool isSsh = (entry.protocol.compare("SSH", Qt::CaseInsensitive) == 0) ||
                 (entry.protocol.compare("SSH-AUTH", Qt::CaseInsensitive) == 0);

    // 3. Execute the Dial
    if (isSsh) {
        m_isSshMode = true;
        m_isNetworkConnected = false;

        qDebug() << "!i" << tr("[RDevice] Negotiating SSH with %1...").arg(entry.ip);

        // --- NEW: Pass the privateKey field to the SSH wrapper ---
        m_ssh->connectToHost(entry.ip, entry.port, entry.login, entry.password, entry.privateKey);

    } else {
        m_isSshMode = false;
        m_isNetworkConnected = false;

        qDebug() << "!i" << tr("[RDevice] Dialing Telnet %1:%2...").arg(entry.ip).arg(entry.port);
        tcpSocket->connectToHost(entry.ip, entry.port);
    }
}


void RDevice::hangup() {
    {
        QMutexLocker locker(&m_bufferMutex);
        m_txBuffer.clear();
        m_networkToSioBuffer.clear();
        m_atCmdBuffer.clear();
    }
    if (tcpSocket->state() == QAbstractSocket::ConnectedState) tcpSocket->disconnectFromHost();
    if (m_isSshMode && m_ssh->isConnected()) m_ssh->disconnectFromHost();
    // [FIX] Moved this cleanup from injectMacro to here where it belongs!
    if (pendingSocket) {
        pendingSocket->disconnect(this); // Prevent signal loops
        pendingSocket->disconnectFromHost();
        pendingSocket->deleteLater();
        pendingSocket = nullptr;
        m_ringTimer->stop();
        m_ringPhase = false;
    }
    sendResultCode(RESULT_OK);
}

void RDevice::injectMacro(char macroType) {
    if (!m_isNetworkConnected) return; // Ignore if offline

    QString textToSend;
    if (macroType == 'U' || macroType == 'u') textToSend = m_currentConnection.login;
    else if (macroType == 'P' || macroType == 'p') textToSend = m_currentConnection.password;

    if (!textToSend.isEmpty()) {
        QByteArray bytes = textToSend.toUtf8() + "\r";
        if (m_isSshMode) m_ssh->write(bytes);
        else tcpSocket->write(bytes);
    }
}


void RDevice::parseInteractiveSshTarget(const QString &target) {
    // Format: SSH:user@host:port OR SSH:user@host
    QString connectionStr = target.mid(4);
    int atIndex = connectionStr.indexOf('@');
    QString user = connectionStr.left(atIndex);
    QString hostPort = connectionStr.mid(atIndex + 1);

    QString host = hostPort;
    int port = 22;
    if (hostPort.contains(":")) {
        QStringList parts = hostPort.split(":");
        host = parts[0];
        port = parts[1].toInt();
    }

    // Stage the connection details
    m_currentConnection = BbsEntry();
    m_currentConnection.protocol = "SSH-AUTH";
    m_currentConnection.login = user;
    m_currentConnection.ip = host;
    m_currentConnection.port = port;
    m_currentConnection.name = host;

    m_waitingForSshPassword = true;
    sendAtResponse("\r\nPASSWORD: ");
}


void RDevice::executeInteractiveSshDial() {
    sendAtResponse("\r\nDIALING " + m_currentConnection.ip + "...\r\n");
    m_isSshMode = true;

    if (tcpSocket->state() != QAbstractSocket::UnconnectedState) tcpSocket->abort();
    if (m_ssh->isConnected()) m_ssh->disconnectFromHost();

    m_ssh->connectToHost(m_currentConnection.ip, m_currentConnection.port, m_currentConnection.login, m_currentConnection.password);
}


void RDevice::updateListenerConfig() {
    if (!aspeqtSettings) return;

    // Only listen if both the R: Device AND the BBS Listener are enabled
    bool shouldListen = aspeqtSettings->bbsListenerEnabled(m_portIndex) && m_isEnabled;
    int port = aspeqtSettings->modemListenPort(m_portIndex);

    if (shouldListen) {
        if (tcpServer->isListening()) {
            // If it's already listening but the user changed the port, restart it
            if (tcpServer->serverPort() != port) {
                tcpServer->close();
                if (tcpServer->listen(QHostAddress::Any, port)) {
                    qDebug() << "!i [RDevice] BBS listener restarted on new port:" << port;
                }
            }
        } else {
            // Start listening
            if (tcpServer->listen(QHostAddress::Any, port)) {
                qDebug() << "!i [RDevice] BBS listener started on port:" << port;
            } else {
                qDebug() << "!e [RDevice] Failed to start BBS listener on port:" << port;
            }
        }
    } else {
        // If the setting is toggled off, kill the server
        if (tcpServer->isListening()) {
            tcpServer->close();
            qDebug() << "!i [RDevice] BBS listener stopped.";
        }
    }
}


void RDevice::onAutoAnswerTriggered() {
    if (m_ringPhase && pendingSocket) {
        qDebug() << "!i [RDevice] Auto-answering call (S0=" << m_s0Register << ")";

        tcpSocket->disconnect(this);
        tcpSocket->deleteLater();

        tcpSocket = pendingSocket;
        pendingSocket = nullptr;
        m_ringPhase = false;
        m_isSshMode = false;
        m_isNetworkConnected = true;

        connect(tcpSocket, &QTcpSocket::connected, this, &RDevice::onSocketConnected);
        connect(tcpSocket, &QTcpSocket::disconnected, this, &RDevice::onSocketDisconnected);
        connect(tcpSocket, &QTcpSocket::readyRead, this, &RDevice::onSocketReadyRead);
        connect(tcpSocket, &QTcpSocket::errorOccurred, this, &RDevice::onSocketError);

        sendResultCode(RESULT_CONNECT);
    }
}
