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
    tcpSocket = new QTcpSocket(this);
    tcpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);

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

    m_isEnabled = (aspeqtSettings && aspeqtSettings->isRDeviceEnabled());
    if (m_isEnabled) loadPhonebook(aspeqtSettings->modemBridgePhonebookPath());

    connect(this, &RDevice::dispatchToNetwork, this, [this](const QByteArray &data){
        if (m_isNetworkConnected) {
            QByteArray filteredData;

            for (char c : data) {
                if (m_escPending) {
                    m_escPending = false;
                    if (c == 'U' || c == 'u') {
                        QString inject = m_currentConnection.login + "\r";
                        filteredData.append(inject.toUtf8());
                        continue;
                    }
                    if (c == 'P' || c == 'p') {
                        QString inject = m_currentConnection.password + "\r";
                        filteredData.append(inject.toUtf8());
                        continue;
                    }
                    if (c == 'H' || c == 'h') {
                        hangup();
                        continue;
                    }
                    filteredData.append(0x1B);
                    filteredData.append(c);
                } else if (c == 0x1B) {
                    m_escPending = true;
                } else {
                    filteredData.append(c);
                }
            }

            checkEscapeSequence(filteredData);

            if (!filteredData.isEmpty()) {
                if (m_isSshMode) m_ssh->write(filteredData);
                else tcpSocket->write(filteredData);
            }
        }
        else {
            for (char c : data) {
                if (echoEnabled) {
                    QMutexLocker locker(&m_bufferMutex);
                    m_networkToSioBuffer.append(c);
                }

                if (c == 0x0D || (quint8)c == 0x9B) {
                    emit executeAtCommand(m_atCmdBuffer);
                    m_atCmdBuffer.clear();
                }    else if (c == 8 || c == 126 || c == 127) { // Backspace or Delete
                        if (!m_atCmdBuffer.isEmpty()) {
                            m_atCmdBuffer.chop(1); // Remove from the internal command buffer
                            if (echoEnabled) {
                                QMutexLocker locker(&m_bufferMutex);
                                // Visually erase the character: Backspace, Space, Backspace
                                m_networkToSioBuffer.append(char(8));
                                m_networkToSioBuffer.append(' ');
                                m_networkToSioBuffer.append(char(8));
                        }
                    }

                } else {
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
    case CMD_CONTROL:
    case CMD_AUTOANSWER:
        if (!sio->port()->writeCommandAck()) return;
        sio->port()->writeComplete();
        break;
    case CMD_STREAM:
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

    qDebug() << "!d" << "[RDevice] Configure: Ice-T requested Baud Rate set to:" << m_currentBaudRate;
    sio->port()->writeComplete();
}

void RDevice::sendDataToAtari(const QByteArray &data)
{
    // SioWorker::usleep(2000);
    sio->port()->writeComplete();
    // SioWorker::usleep(2000);
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

    qDebug() << "!d" << "[RDevice] Stream active - Handing over to Pi 5 Hardware UART at" << m_currentBaudRate;
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
                    m_atCmdBuffer.chop(1); // Remove from the actual command buffer
                    if (echoEnabled) {
                        // Visually erase the character: Backspace, Print Space, Backspace
                        sendAtResponse(QByteArray(1, 8) + " " + QByteArray(1, 8));
                    }
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

void RDevice::processSerialData(const QByteArray &data) {
    if (state != ModemState::StreamMode) return;

    static bool escPending = false;
    QByteArray filteredData;

    for (int i = 0; i < data.size(); ++i) {
        char c = data[i];

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
                // [FIX] Use Thread-Safe invokeMethod to call the hangup slot
                QMetaObject::invokeMethod(this, "hangup", Qt::QueuedConnection);
                continue;
            }

            filteredData.append(0x1B);
            filteredData.append(c);
        } else if (c == 0x1B) {
            escPending = true;
        } else {
            filteredData.append(c);
        }
    }

    if (!filteredData.isEmpty()) {
        checkEscapeSequence(filteredData);
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
        // [FIX] Use Thread-Safe invokeMethod to call the hangup slot
        QMetaObject::invokeMethod(this, "hangup", Qt::QueuedConnection);
        m_plusCount = 0;
    }
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
    else if (cmd.startsWith("DT")) {
        QString target = cmd.mid(2).trimmed();
        at_handle_dial(target);
    }
    else if (cmd == "H") {
        hangup();
    }
    else if (cmd == "Z") {
        tcpSocket->abort();
        if (m_isSshMode && m_ssh) m_ssh->disconnectFromHost();
        sendResultCode(RESULT_OK);
    }
    else if (cmd == "O") {
        if (m_isNetworkConnected) {
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
void RDevice::onSocketError(QAbstractSocket::SocketError) {
    if (m_isSshMode) return;
    sendResultCode(RESULT_ERROR);
}

void RDevice::onNewConnection() {
    if (!pendingSocket) {
        pendingSocket = tcpServer->nextPendingConnection();
        sendResultCode(RESULT_RING);
    } else {
        QTcpSocket *temp = tcpServer->nextPendingConnection();
        temp->disconnectFromHost();
        temp->deleteLater();
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
    Q_UNUSED(msg);
    if (!m_isSshMode) return;
    sendResultCode(RESULT_ERROR);
}

QByteArray RDevice::dequeueNetworkData() {
    QMutexLocker locker(&m_bufferMutex);
    QByteArray data = m_networkToSioBuffer;
    m_networkToSioBuffer.clear();
    return data;
}

void RDevice::dial(const BbsEntry &entry) {
    m_currentConnection = entry;
    QString proto = entry.protocol.toUpper();
    m_isSshMode = proto.startsWith("SSH");

    if (tcpSocket->state() != QAbstractSocket::UnconnectedState) tcpSocket->abort();
    if (m_ssh->isConnected()) m_ssh->disconnectFromHost();

    sendAtResponse("\r\nDIALING " + entry.name + "...\r\n");

    if (m_isSshMode) {
        QString safeUser = entry.login.isEmpty() ? "guest" : entry.login;
        if (proto == "SSH-AUTH") {
            m_ssh->connectToHost(entry.ip, entry.port, safeUser, entry.password);
        } else {
            m_ssh->connectToHost(entry.ip, entry.port, safeUser, "");
        }
    } else {
        tcpSocket->connectToHost(entry.ip, entry.port);
    }
}

void RDevice::hangup() {
    if (tcpSocket->state() == QAbstractSocket::ConnectedState) tcpSocket->disconnectFromHost();
    if (m_isSshMode && m_ssh->isConnected()) m_ssh->disconnectFromHost();
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
