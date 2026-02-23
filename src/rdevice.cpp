#include "rdevice.h"
#include "rdevice_handler.h" // Assumed to contain 'driver_850' and 'relocator_stub' arrays
#include "aspeqtsettings.h"  // For checking settings
#include <QDebug>
#include <QThread>
#include <QCoreApplication>
#include <QFile>
#include <QDomDocument>

// Result Codes
#define RESULT_OK           0
#define RESULT_CONNECT      1
#define RESULT_RING         2
#define RESULT_NO_CARRIER   3
#define RESULT_ERROR        4

#define GUARD_TIME_MS       1000
#define RING_INTERVAL_MS    3000

RDevice::RDevice(SioWorker *worker) : SioDevice(worker)
{
    // Client Socket
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket, &QTcpSocket::connected, this, &RDevice::onSocketConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &RDevice::onSocketDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &RDevice::onSocketReadyRead);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, &RDevice::onSocketError);

    // Server Socket
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &RDevice::onNewConnection);
    pendingSocket = nullptr;

    // Initial State
    // Note: User must call setEnabled(true) or logic should check settings
    m_isEnabled = (aspeqtSettings && aspeqtSettings->enableRDevice());
    state = ModemState::CommandMode;

    lastActivityTimer.start();
    lastRingTimer.start();

    if (m_isEnabled) {
        loadPhonebook(aspeqtSettings->modemBridgePhonebookPath());
    }
}

RDevice::~RDevice()
{
    if (tcpSocket) tcpSocket->close();
    if (tcpServer) tcpServer->close();
}

void RDevice::setEnabled(bool enable)
{
    m_isEnabled = enable;
    if (!m_isEnabled) {
        if (tcpSocket->state() != QAbstractSocket::UnconnectedState) {
            tcpSocket->disconnectFromHost();
        }
        tcpServer->close();
        state = ModemState::CommandMode;
        rxBuffer.clear();
        atCmdAccumulator.clear();
        m_phonebook.clear();
    } else {
        // Reload phonebook when enabling
        loadPhonebook(aspeqtSettings->modemBridgePhonebookPath());
    }
}

void RDevice::loadPhonebook(const QString &path) {
    if (path.isEmpty()) return;
    m_phonebook.clear();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QDomDocument doc;
    if (!doc.setContent(&file)) {
        file.close();
        return;
    }
    file.close();

    QDomElement root = doc.documentElement(); // <EtherTerm>
    QDomElement pb = root.firstChildElement("Phonebook");
    QDomNodeList list = pb.elementsByTagName("BBS");

    for (int i = 0; i < list.count(); i++) {
        QDomElement e = list.at(i).toElement();
        BbsEntry bbs;
        bbs.name = e.attribute("name");
        bbs.ip = e.attribute("ip");
        bbs.port = e.attribute("port").toInt();
        bbs.protocol = e.attribute("protocol");
        bbs.login = e.attribute("login");
        bbs.password = e.attribute("password");
        m_phonebook.append(bbs);
    }
    qDebug() << "[RDevice] Loaded" << m_phonebook.count() << "entries from phonebook.";
}

void RDevice::shortDelay()
{
    // Slight delay to allow SIO bus handling (approx 5ms)
    SioWorker::usleep(5000);
}

// --------------------------------------------------------------------------
// SIO Dispatcher
// --------------------------------------------------------------------------

void RDevice::handleCommand(quint8 command, quint16 aux)
{
    if (aspeqtSettings) {
        bool settingEnabled = aspeqtSettings->enableRDevice();
        if (m_isEnabled != settingEnabled) setEnabled(settingEnabled);
    }

    if (!m_isEnabled) {
        sio->port()->writeCommandNak();
        return;
    }

    quint8 aux1 = (aux & 0xFF);
    quint8 aux2 = (aux >> 8) & 0xFF;

    switch (command) {
    // Boot & Load
    case CMD_POLL_TYPE1: handlePollType1(); break;
    case CMD_POLL_TYPE3: handlePollType3(aux1, aux2); break;
    case CMD_RELOCATOR:  handleDownloadRelocator(); break;
    case CMD_DOWNLOAD:   handleDownloadDriver(); break;

        // Operation
    case CMD_STATUS:     handleStatus(); break;
    case CMD_WRITE:      handleWrite(aux); break;
    case CMD_READ:       handleRead(aux); break;
    case CMD_CONTROL:    handleControl(aux); break;
    case CMD_STREAM:     handleStream(); break;

    // Server / Extended
    case CMD_LISTEN:     handleListen(aux); break;
    case CMD_UNLISTEN:   handleUnlisten(); break;

    // Config Stubs
    case CMD_CONFIGURE:
    case CMD_AUTOANSWER:
        sio->port()->writeCommandAck();
        sio->port()->writeComplete();
        break;

    default:
        sio->port()->writeCommandNak();
        break;
    }
}

// --------------------------------------------------------------------------
// Boot & Handlers
// --------------------------------------------------------------------------

void RDevice::handlePollType1()
{
    if (!sio->port()->writeCommandAck()) return;

    quint16 relSize = sizeof(relocator_stub);
    QByteArray bootBlock;
    bootBlock.resize(12);
    bootBlock[0] = 0x50;
    bootBlock[1] = 0x01;
    bootBlock[2] = 0x21;
    bootBlock[3] = 0x40;
    bootBlock[4] = 0x00;
    bootBlock[5] = 0x05;
    bootBlock[6] = 0x08;
    bootBlock[7] = 0x00;
    bootBlock[8] = (char)(relSize & 0xFF);
    bootBlock[9] = (char)((relSize >> 8) & 0xFF);
    bootBlock[10] = 0x00;
    bootBlock[11] = 0x00;

    shortDelay();
    sio->port()->writeDataFrame(bootBlock);
}

void RDevice::handlePollType3(quint8 aux1, quint8 aux2)
{
    bool respond = (m_deviceNo == 0x50);
    if (aux1 == 0x52 && aux2 == 0x01) respond = true;

    if (!respond) return;

    if (!sio->port()->writeCommandAck()) return;

    quint16 fsize = sizeof(driver_850);
    QByteArray response(4, 0);
    response[0] = (quint8)(fsize & 0xFF);
    response[1] = (quint8)((fsize >> 8) & 0xFF);
    response[2] = 0x50;
    response[3] = 0x00;

    shortDelay();
    sio->port()->writeDataFrame(response);
}

void RDevice::handleDownloadRelocator()
{
    if (!sio->port()->writeCommandAck()) return;
    QByteArray payload((const char*)relocator_stub, sizeof(relocator_stub));
    shortDelay();
    sio->port()->writeDataFrame(payload);
}

void RDevice::handleDownloadDriver()
{
    if (!sio->port()->writeCommandAck()) return;
    QByteArray payload((const char*)driver_850, sizeof(driver_850));
    shortDelay();
    sio->port()->writeDataFrame(payload);
}

// --------------------------------------------------------------------------
// Standard SIO Operations
// --------------------------------------------------------------------------

void RDevice::handleStatus()
{
    if (!sio->port()->writeCommandAck()) return;

    quint8 bits = 0x10; // CTS Always Ready
    bits |= 0x80;       // DSR Always Ready

    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
        bits |= 0x40; // Carrier Detect (CD)
    }

    if (!rxBuffer.isEmpty()) bits |= 0x01;

    QByteArray statusData(4, 0x00);
    statusData[1] = bits;
    statusData[2] = 0xE0; // Timeout

    sio->port()->writeComplete();
    sio->port()->writeDataFrame(statusData);
}

void RDevice::handleControl(quint16 aux)
{
    if (!sio->port()->writeCommandAck()) return;

    if (aux & 0x80) {
        bool dtr = (aux & 0x40);
        if (!dtr && tcpSocket->state() == QAbstractSocket::ConnectedState) {
            tcpSocket->disconnectFromHost();
            state = ModemState::CommandMode;
        }
    }
    sio->port()->writeComplete();
}

void RDevice::handleStream()
{
    if (!sio->port()->writeCommandAck()) return;

    // Send POKEY Table
    QByteArray payload;
    const char table[] = {0x28, (char)0xA0, 0x00, (char)0xA0, 0x28, (char)0xA0, 0x00, (char)0xA0, 0x78};
    payload.append(table, 9);

    shortDelay();
    sio->port()->writeDataFrame(payload);
    sio->port()->writeComplete();

    qDebug() << "[RDevice] Entering Concurrent Stream Mode";
    state = ModemState::StreamMode;
    processStreamLoop();
}

// --------------------------------------------------------------------------
// Stream Loop (The Bridge)
// --------------------------------------------------------------------------

void RDevice::processStreamLoop()
{
    // Reset ESC state logic on entry
    m_escPressed = false;

    while (state == ModemState::StreamMode) {
        QCoreApplication::processEvents();

        if (sio->isInterruptionRequested()) break;

        // 1. TCP -> Serial (PC to Atari)
        if (!rxBuffer.isEmpty()) {
            sio->port()->writeRawFrame(rxBuffer);
            rxBuffer.clear();
        }

        // 2. Serial -> TCP (Atari to PC)
        QByteArray chunk = sio->port()->readRawFrame(1, false);

        if (!chunk.isEmpty()) {
            char c = chunk.at(0);

            // --- Escape Sequence +++ Logic ---
            qint64 silenceDuration = lastActivityTimer.elapsed();
            lastActivityTimer.restart();

            if (c == '+') {
                if (silenceDuration > GUARD_TIME_MS && plusCount == 0) plusCount = 1;
                else if (plusCount > 0) plusCount++;
            } else {
                plusCount = 0;
            }

            // --- MACRO Logic (ESC-U/P/H) ---
            bool charConsumed = false;

            if (m_escPressed) {
                m_escPressed = false;
                charConsumed = true; // We consume this char as part of the sequence

                if (c == 'u' || c == 'U') {
                    if (!m_currentConnection.login.isEmpty()) {
                        if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
                            tcpSocket->write(m_currentConnection.login.toUtf8());
                            tcpSocket->write("\r");
                            qDebug() << "[RDevice] Sent Login Macro";
                        }
                    }
                }
                else if (c == 'p' || c == 'P') {
                    if (!m_currentConnection.password.isEmpty()) {
                        if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
                            tcpSocket->write(m_currentConnection.password.toUtf8());
                            tcpSocket->write("\r");
                            qDebug() << "[RDevice] Sent Password Macro";
                        }
                    }
                }
                else if (c == 'h' || c == 'H') {
                    qDebug() << "[RDevice] Manual Hangup via ESC-H";
                    tcpSocket->disconnectFromHost();
                    state = ModemState::CommandMode;
                    sendResultCode(RESULT_OK); // Or NO CARRIER? Usually OK then back to command
                    return; // Break out of stream loop
                }
                else {
                    // Not a valid macro, send the ESC we swallowed + this char
                    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
                        char esc = 0x1B;
                        tcpSocket->write(&esc, 1);
                        tcpSocket->write(&c, 1);
                    }
                }
            }
            else if (c == 0x1B) {
                m_escPressed = true;
                charConsumed = true; // Don't send ESC yet, wait for next char
            }

            // Write to Socket if not consumed by Macro logic
            if (!charConsumed && tcpSocket->state() == QAbstractSocket::ConnectedState) {
                tcpSocket->write(chunk);
            }

        } else {
            // No data received (Timeout). Check if we completed an escape sequence.
            if (plusCount == 3 && lastActivityTimer.elapsed() > GUARD_TIME_MS) {
                qDebug() << "[RDevice] Escape Sequence +++ Detected";
                state = ModemState::CommandMode;
                sendResultCode(RESULT_OK);
                plusCount = 0;
            }
        }
    }

    qDebug() << "[RDevice] Exited Concurrent Stream Mode";
}

// --------------------------------------------------------------------------
// Read / Write (Command Mode)
// --------------------------------------------------------------------------

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

    // In Command Mode, we parse AT commands
    for (char c : data) {
        if (c == 0x0D || (quint8)c == 0x9B) {
            if (echoEnabled) {
                rxBuffer.append(0x0D);
                rxBuffer.append(0x0A);
            }
            processAtCommand(atCmdAccumulator);
            atCmdAccumulator.clear();
        }
        else if (c == 0x08 || c == 0x7D || c == 0x7F) {
            if (!atCmdAccumulator.isEmpty()) {
                atCmdAccumulator.chop(1);
                if (echoEnabled) rxBuffer.append(c);
            }
        }
        else {
            atCmdAccumulator.append(c);
            if (echoEnabled) rxBuffer.append(c);
        }
    }

    sio->port()->writeComplete();
}

void RDevice::handleRead(quint16 len)
{
    if (!sio->port()->writeCommandAck()) return;

    checkRing();

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
// Server / Listen
// --------------------------------------------------------------------------

void RDevice::handleListen(quint16 aux)
{
    listenPort = aux;
    if (listenPort > 0) {
        if (tcpServer->isListening()) tcpServer->close();
        if (tcpServer->listen(QHostAddress::Any, listenPort)) {
            sio->port()->writeCommandAck();
            sio->port()->writeComplete();
            return;
        }
    }
    sio->port()->writeCommandNak();
}

void RDevice::handleUnlisten()
{
    tcpServer->close();
    if (pendingSocket) {
        pendingSocket->disconnectFromHost();
        pendingSocket->deleteLater();
        pendingSocket = nullptr;
    }
    sio->port()->writeCommandAck();
    sio->port()->writeComplete();
}

void RDevice::onNewConnection()
{
    if (tcpServer->hasPendingConnections()) {
        QTcpSocket *client = tcpServer->nextPendingConnection();

        if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
            client->write("BUSY\r\n");
            client->disconnectFromHost();
            client->deleteLater();
        } else {
            if (pendingSocket) {
                pendingSocket->close();
                pendingSocket->deleteLater();
            }
            pendingSocket = client;

            if (autoAnswer) {
                at_handle_answer();
            }
        }
    }
}

void RDevice::checkRing()
{
    if (pendingSocket && !autoAnswer) {
        if (lastRingTimer.elapsed() > RING_INTERVAL_MS) {
            sendResultCode(RESULT_RING);
            lastRingTimer.restart();
        }
    }
}

// --------------------------------------------------------------------------
// AT Commands
// --------------------------------------------------------------------------

void RDevice::processAtCommand(const QString &rawCmd)
{
    QString cmd = rawCmd.trimmed().toUpper();
    if (cmd.startsWith("AT")) cmd.remove(0, 2);
    else return;

    qDebug() << "[RDevice] AT Command:" << cmd;

    if (cmd.isEmpty()) sendResultCode(RESULT_OK);
    else if (cmd == "A") at_handle_answer();
    else if (cmd.startsWith("D")) at_handle_dial(cmd.mid(1));
    else if (cmd == "H") at_handle_hangup();
    else if (cmd == "E0") { echoEnabled = false; sendResultCode(RESULT_OK); }
    else if (cmd == "E1") { echoEnabled = true; sendResultCode(RESULT_OK); }
    else if (cmd == "V0") { verboseResponses = false; sendResultCode(RESULT_OK); }
    else if (cmd == "V1") { verboseResponses = true; sendResultCode(RESULT_OK); }
    else if (cmd == "S0=0") { autoAnswer = false; sendResultCode(RESULT_OK); }
    else if (cmd == "S0=1") { autoAnswer = true; sendResultCode(RESULT_OK); }
    else if (cmd.startsWith("PORT")) {
        listenPort = cmd.mid(4).toInt();
        handleListen(listenPort);
        sendResultCode(RESULT_OK);
    }
    else sendResultCode(RESULT_ERROR);
}

void RDevice::at_handle_dial(const QString &target)
{
    QString cleanTarget = target;
    if (cleanTarget.startsWith("T")) cleanTarget.remove(0, 1);

    QString host = cleanTarget;
    int port = 23;
    bool foundInPhonebook = false;

    // 1. Check Phonebook First
    for (const BbsEntry &entry : m_phonebook) {
        if (entry.name.compare(cleanTarget, Qt::CaseInsensitive) == 0) {
            host = entry.ip;
            port = entry.port;
            m_currentConnection = entry; // Save for Macros
            foundInPhonebook = true;
            qDebug() << "[RDevice] Phonebook Match:" << entry.name;
            break;
        }
    }

    if (!foundInPhonebook) {
        m_currentConnection = BbsEntry(); // Clear previous session data
        if (cleanTarget.contains(":")) {
            QStringList parts = cleanTarget.split(":");
            host = parts[0];
            port = parts[1].toInt();
        }
    }

    tcpSocket->connectToHost(host, port);
}

void RDevice::at_handle_answer()
{
    if (pendingSocket) {
        if (tcpSocket->state() == QAbstractSocket::ConnectedState) tcpSocket->disconnectFromHost();
        tcpSocket->deleteLater();
        tcpSocket = pendingSocket;
        pendingSocket = nullptr;

        connect(tcpSocket, &QTcpSocket::disconnected, this, &RDevice::onSocketDisconnected);
        connect(tcpSocket, &QTcpSocket::readyRead, this, &RDevice::onSocketReadyRead);
        connect(tcpSocket, &QTcpSocket::errorOccurred, this, &RDevice::onSocketError);

        sendResultCode(RESULT_CONNECT);
        state = ModemState::StreamMode;
    } else {
        sendResultCode(RESULT_ERROR);
    }
}

void RDevice::at_handle_hangup()
{
    tcpSocket->disconnectFromHost();
    sendResultCode(RESULT_OK);
    state = ModemState::CommandMode;
}

void RDevice::sendResultCode(int code)
{
    if (verboseResponses) {
        switch(code) {
        case RESULT_OK:         sendAtResponse("OK\r\n"); break;
        case RESULT_CONNECT:    sendAtResponse("CONNECT\r\n"); break;
        case RESULT_RING:       sendAtResponse("RING\r\n"); break;
        case RESULT_NO_CARRIER: sendAtResponse("NO CARRIER\r\n"); break;
        case RESULT_ERROR:      sendAtResponse("ERROR\r\n"); break;
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
// Sockets & Data
// --------------------------------------------------------------------------

void RDevice::onSocketConnected() {
    sendResultCode(RESULT_CONNECT);
}

void RDevice::onSocketDisconnected() {
    sendResultCode(RESULT_NO_CARRIER);
    state = ModemState::CommandMode;
}

void RDevice::onSocketReadyRead() {
    processIncomingData(tcpSocket->readAll());
}

void RDevice::processIncomingData(QByteArray data)
{
    // Basic Telnet Filter (Strip IAC)
    for (int i = 0; i < data.size(); ++i) {
        unsigned char c = (unsigned char)data.at(i);
        if (c == 0xFF) {
            if (i + 2 < data.size()) i += 2;
        } else {
            rxBuffer.append(c);
        }
    }
}

void RDevice::onSocketError(QAbstractSocket::SocketError) {
    if (state == ModemState::CommandMode) sendResultCode(RESULT_ERROR);
    else {
        sendResultCode(RESULT_NO_CARRIER);
        state = ModemState::CommandMode;
    }
}
