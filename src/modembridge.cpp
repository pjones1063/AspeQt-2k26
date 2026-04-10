#include "modembridge.h"
#include "aspeqtsettings.h"
#include <QDebug>
#include <QFile>
#include <QDomDocument>

ModemBridge::ModemBridge(QObject *parent) : QObject(parent),
    m_serial(new QSerialPort(this)),
    m_socket(new QTcpSocket(this)),
    m_ssh(new SshClient(this)),
    m_tcpServer(new QTcpServer(this)),
    m_pendingSocket(nullptr),
    m_isActive(false),
    m_isConnected(false),
    m_isSshMode(false) // Default to Telnet
{
    m_localEcho = true;

    // [NEW] Setup the trailing pause timer
    m_escapeActionTimer = new QTimer(this);
    m_escapeActionTimer->setSingleShot(true);
    connect(m_escapeActionTimer, &QTimer::timeout, this, &ModemBridge::onEscapeTriggered);

    m_escapeTimer.start(); // Start elapsed timer

    // Serial
    connect(m_serial, &QSerialPort::readyRead, this, &ModemBridge::onSerialDataReceived);

    // TCP (Telnet)
    connect(m_socket, &QTcpSocket::readyRead, this, &ModemBridge::onSocketDataReceived);
    connect(m_socket, &QTcpSocket::connected, this, &ModemBridge::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &ModemBridge::onSocketDisconnected);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &ModemBridge::onSocketError);

    // SSH Connections
    connect(m_ssh, &SshClient::connected, this, &ModemBridge::onSshConnected);
    connect(m_ssh, &SshClient::disconnected, this, &ModemBridge::onSshDisconnected);
    connect(m_ssh, &SshClient::rxData, this, &ModemBridge::onSshDataReceived);
    connect(m_ssh, &SshClient::error, this, &ModemBridge::onSshError);

    connect(m_tcpServer, &QTcpServer::newConnection, this, &ModemBridge::onNewConnection);

    m_ringTimer = new QTimer(this);
    m_ringTimer->setSingleShot(true);
    connect(m_ringTimer, &QTimer::timeout, this, &ModemBridge::onRingTimeout);
}

ModemBridge::~ModemBridge() {
    stop();
}

void ModemBridge::setSerialPort(const QString &portName, int baudRate) {
    if (m_isActive) stop();
    m_serial->setPortName(portName);
    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);
}

void ModemBridge::setFlowControl(bool enable) {
    m_flowControl = enable;
    if (m_serial->isOpen()) {
        m_serial->setFlowControl(enable ? QSerialPort::HardwareControl : QSerialPort::NoFlowControl);
    }
}

void ModemBridge::setLocalEcho(bool enable) {
    m_localEcho = enable;
}

void ModemBridge::setTcpMode(bool enableSsh) {
    Q_UNUSED(enableSsh);
    // Deprecated or can be used to set a global default if needed
}

void ModemBridge::start() {
    if (m_serial->open(QIODevice::ReadWrite)) {
        m_isActive = true;
        emit statusMessage("Modem Bridge: Serial port opened.");
        m_serial->setDataTerminalReady(true);
        m_serial->setRequestToSend(true);
        updateListenerConfig();
    } else {
        emit errorOccurred("Modem Bridge: Failed to open serial port.");
    }
}

void ModemBridge::stop() {
    if (m_serial->isOpen()) m_serial->close();

    // Close both types of connections
    if (m_socket->state() != QAbstractSocket::UnconnectedState) m_socket->disconnectFromHost();
    if (m_ssh->isConnected()) m_ssh->disconnectFromHost();

    m_tcpServer->close();
    if (m_pendingSocket) {
        m_pendingSocket->disconnectFromHost();
        m_pendingSocket->deleteLater();
        m_pendingSocket = nullptr;
    }
    m_ringPhase = false;
    m_isActive = false;
    m_isConnected = false;
}


// ============================================================================
// DATA ROUTING (Serial -> Internet)
// ============================================================================
void ModemBridge::onSerialDataReceived() {
    QByteArray data = m_serial->readAll();
    if (!m_isActive) return;

    emit txActivity();

    // --- MODE 1: CONNECTED (Data Mode) ---
    if (m_isConnected) {
        QByteArray filteredData;

        for (char c : data) {
            // Atari Backspace Fix
            if (c == 126 || c == 127) c = 8;

            // MACROS & ESCAPES
            if (m_escPressed) {
                m_escPressed = false;
                if (c == 'u' || c == 'U') { injectMacro('u'); continue; }
                if (c == 'p' || c == 'P') { injectMacro('p'); continue; }
                if (c == 'h' || c == 'H') { hangup(); continue; }

                filteredData.append(0x1B);
                filteredData.append(c);
            }
            else if (c == 0x1B) {
                m_escPressed = true;
            }
            else {
                filteredData.append(c);
            }
        }

        if (!filteredData.isEmpty()) {
            emit traceData(m_isSshMode ? "TX (SSH)" : "TX (TCP)", filteredData);
            checkEscapeSequence(filteredData);

            if (m_isSshMode) m_ssh->write(filteredData);
            else             m_socket->write(filteredData);
        }
    }
    // --- MODE 2: COMMAND MODE ---
    else {
        // --- Trace AT Commands so you can see what you type ---
        emit traceData("TX (CMD)", data);

        for (char c : data) {
            // 1. Handle Carriage Returns (ASCII or ATASCII)
            if (c == '\r' || (quint8)c == 155) {
                if (m_localEcho) {
                    sendToSerial(QByteArray(1, c)); // ECHO THE EXACT NATIVE BYTE!
                }
                processAtCommand(m_serialBuffer);
                m_serialBuffer.clear();
            }
            // 2. Handle Backspaces / Deletes
            else if (c == 8 || c == 126 || c == 127) {
                if (!m_serialBuffer.isEmpty()) {
                    m_serialBuffer.chop(1);
                    if (m_localEcho && !m_waitingForSshPassword) {
                        // Visually erase the character: Backspace, Space, Backspace
                        sendToSerial(QByteArray(1, 8));
                        sendToSerial(" ");
                        sendToSerial(QByteArray(1, 8));
                    }
                }
            }
            // 3. Normal Typing (Stop filtering \n natively)
            else {
                if (m_localEcho && !m_waitingForSshPassword) {
                    sendToSerial(QByteArray(1, c)); // Echo the typed character
                }
                m_serialBuffer.append(c);
            }
        }
    }
}



void ModemBridge::checkEscapeSequence(const QByteArray &data) {
    for (char c : data) {
        if (c == '+') {
            if (m_plusCount == 0) {
                if (m_escapeTimer.elapsed() >= 1000) m_plusCount = 1;
            } else if (m_plusCount < 3) {
                m_plusCount++;
            }

            if (m_plusCount == 3) {
                m_escapeActionTimer->start(1000); // Wait for trailing pause
            }
        } else {
            m_plusCount = 0;
            m_escapeTimer.restart();
            if (m_escapeActionTimer->isActive()) {
                m_escapeActionTimer->stop();
            }
        }
    }
}

void ModemBridge::onEscapeTriggered() {
    m_plusCount = 0;
    emit statusMessage("Modem Bridge: +++ Escape Sequence detected. Dropping to Command Mode.");
    m_isConnected = false;
    sendResultCode(0); // OK
}


// ============================================================================
// CONNECTION LOGIC
// ============================================================================
void ModemBridge::processAtCommand(const QByteArray &cmd) {

    if (m_waitingForSshPassword) {
        m_waitingForSshPassword = false;
        m_currentConnection.password = QString::fromLatin1(cmd).trimmed();
        executeInteractiveSshDial();
        return;
    }

    QString upperCmd = QString::fromLatin1(cmd).trimmed().toUpper();
    if (upperCmd.startsWith("AT")) upperCmd = upperCmd.mid(2);
    upperCmd = upperCmd.trimmed();

    // --- VERBOSE SETTINGS (V0 / V1) ---
    if (upperCmd.contains("V0")) {
        m_verboseResponses = false; upperCmd.replace("V0", "");
    }
    else if (upperCmd.contains("V1")) {
        m_verboseResponses = true; upperCmd.replace("V1", "");
    }

    // --- ANSWER (ATA) ---
    if (upperCmd == "A" || upperCmd.startsWith("A ")) {
        if (m_ringPhase && m_pendingSocket) {
            if (m_ringTimer->isActive()) m_ringTimer->stop();

            m_socket->disconnect(this);
            m_socket->deleteLater();

            m_socket = m_pendingSocket;
            m_pendingSocket = nullptr;

            m_ringPhase = false;
            m_isSshMode = false;
            m_isConnected = true;
            m_telnetState = TelnetState::Normal;

            connect(m_socket, &QTcpSocket::readyRead, this, &ModemBridge::onSocketDataReceived);
            connect(m_socket, &QTcpSocket::connected, this, &ModemBridge::onSocketConnected);
            connect(m_socket, &QTcpSocket::disconnected, this, &ModemBridge::onSocketDisconnected);
            connect(m_socket, &QAbstractSocket::errorOccurred, this, &ModemBridge::onSocketError);

            sendToSerial("\r\nCONNECT 19200\r\n"); // Left untouched to preserve current baud display logic
            emit statusMessage("Modem Bridge: Inbound call answered.");

        } else {
            sendResultCode(4); // ERROR
        }
    }

    // --- DIAL (ATDT) ---
    else if (upperCmd.startsWith("D")) {
        QString originalCmd = QString::fromLatin1(cmd).trimmed();
        int dIndex = originalCmd.toUpper().indexOf("D");
        QString target = originalCmd.mid(dIndex + 1).trimmed();

        if (target.toUpper().startsWith("T")) target = target.mid(1).trimmed();

        if (target.startsWith("SSH:", Qt::CaseInsensitive) && target.contains("@")) {
            parseInteractiveSshTarget(target);
            return;
        }

        QString host = target;
        int port = 23;
        bool found = false;

        for (const BbsEntry &entry : m_phonebook) {
            if (entry.name.compare(target, Qt::CaseInsensitive) == 0) {
                host = entry.ip;
                port = entry.port;
                m_currentConnection = entry;
                found = true;
                break;
            }
        }

        if (!found) {
            m_currentConnection = BbsEntry();
            if (host.startsWith("SSH:")) {
                m_currentConnection.protocol = "SSH";
                host = host.mid(4);
                port = 22;
            }
            QStringList parts = host.split(':');
            host = parts[0];
            if (parts.size() > 1) port = parts[1].toInt();

            m_currentConnection.ip = host;
            m_currentConnection.port = port;
        }

        connectTo(host, port);
    }

    // --- RETURN TO ONLINE (ATO) ---
    else if (upperCmd == "O" || upperCmd.startsWith("O ")) {
        if (m_socket->state() == QAbstractSocket::ConnectedState || m_ssh->isConnected()) {
            m_isConnected = true;

            m_plusCount = 0;
            m_escapeTimer.restart();
            if (m_escapeActionTimer->isActive()) m_escapeActionTimer->stop();

            sendToSerial("\r\nCONNECT 19200\r\n"); // Left untouched
            emit statusMessage("Modem Bridge: Returned to Online Data Mode.");
        } else {
            sendResultCode(3); // NO CARRIER
        }
    }

    // --- S0 REGISTER (Auto-Answer) ---
    else if (upperCmd.startsWith("S0=")) {
        bool ok;
        int val = upperCmd.mid(3).trimmed().toInt(&ok);
        if (ok && val >= 0 && val <= 255) {
            m_s0Register = val;
            sendResultCode(0); // OK
        } else {
            sendResultCode(4); // ERROR
        }
    }
    else if (upperCmd == "S0?") {
        QString valStr = QString("%1\r\n").arg(m_s0Register, 3, 10, QChar('0'));
        sendToSerial(valStr.toUtf8());
        sendResultCode(0); // OK
    }

    // --- HANGUP (ATH) ---
    else if (upperCmd.startsWith("H")) {
        hangup();
        sendResultCode(0); // OK
    }

    // --- RESET (ATZ) ---
    else if (upperCmd.startsWith("Z")) {
        m_suppressCarrierMessage = true;
        m_serialBuffer.clear();
        if (m_serial->isOpen()) {
            m_serial->clear(QSerialPort::Output);
        }
        m_socket->abort();
        m_ssh->disconnectFromHost();

        if (m_pendingSocket) {
            m_pendingSocket->disconnect(this);
            m_pendingSocket->disconnectFromHost();
            m_pendingSocket->deleteLater();
            m_pendingSocket = nullptr;
        }

        m_ringPhase = false;
        m_plusCount = 0;
        m_escapeTimer.restart();
        if (m_escapeActionTimer->isActive()) m_escapeActionTimer->stop();
        m_isConnected = false;
        m_currentConnection = BbsEntry();
        sendResultCode(0); // OK
        m_suppressCarrierMessage = false;
    }

    else if (upperCmd.isEmpty()) {
            sendResultCode(0); // OK
    }

    else {
            qDebug() << "!w Unrecognized AT command swallowed:" << upperCmd;
    }
}


void ModemBridge::dial(const BbsEntry &entry) {

    // FIX: Store the FULL entry into your existing class variable so
    // injectMacro() has access to m_currentConnection.login and password!
    m_currentConnection = entry;

    QString proto = entry.protocol.toUpper();
    m_isSshMode = proto.startsWith("SSH");

    if (m_isSshMode) {
        // CRITICAL FIX: The SSH Protocol strictly requires a username string.
        QString safeUser = entry.login.isEmpty() ? "guest" : entry.login;

        if (proto == "SSH-AUTH") {
            qDebug() << "!i [ModemBridge] Dialing" << entry.name << "via Authenticated SSH...";
            m_ssh->connectToHost(entry.ip, entry.port, safeUser, entry.password);
        } else {
            qDebug() << "!i [ModemBridge] Dialing" << entry.name << "via Anonymous BBS SSH...";
            // Pass safeUser but a BLANK password to let the BBS show its own ANSI login
            m_ssh->connectToHost(entry.ip, entry.port, safeUser, "");
        }
    } else {
        qDebug() << "!i [ModemBridge] Dialing" << entry.name << "via Telnet/TCP...";
        connectTo(entry.ip, entry.port);
    }
}


void ModemBridge::dial(const QString &target) {
    processAtCommand(("ATDT " + target).toUtf8());
}

void ModemBridge::connectTo(const QString &host, int port) {
    // 1. Cleanup existing connections
    if (m_socket->state() != QAbstractSocket::UnconnectedState) m_socket->abort();
    if (m_ssh->isConnected()) m_ssh->disconnectFromHost();
    m_isConnected = false;

    emit statusMessage(QString("Modem Bridge: Dialing %1:%2...").arg(host).arg(port));
    m_serial->write("\r\nDIALING...\r\n");

    // 2. Decide Protocol
    QString proto = m_currentConnection.protocol.toUpper();
    bool useSsh = proto.startsWith("SSH") || (port == 22);

    if (useSsh) {
        m_isSshMode = true;

        // CRITICAL FIX: Apply the safe username rule here too
        QString safeUser = m_currentConnection.login.isEmpty() ? "guest" : m_currentConnection.login;

        if (proto == "SSH-AUTH") {
            m_ssh->connectToHost(host, port, safeUser, m_currentConnection.password);
        } else {
            // Anonymous BBS mode
            m_ssh->connectToHost(host, port, safeUser, "");
        }
    } else {
        m_isSshMode = false;
        m_socket->connectToHost(host, port);
    }
}

// ============================================================================
// TCP HANDLERS
// ============================================================================
void ModemBridge::onSocketConnected() {
    m_isConnected = true;
    sendToSerial("CONNECT 57600\r\n");
    emit statusMessage("Modem Bridge: Telnet Connected.");
}


void ModemBridge::onSocketDataReceived() {
    QByteArray data = m_socket->readAll();
    emit traceData("RX (TCP)", data);
    parseTelnet(data); // Route through filter instead of direct to serial
}


void ModemBridge::parseTelnet(const QByteArray &data) {
    QByteArray cleanData;
    for (char c : data) {
        unsigned char byte = (unsigned char)c;
        switch (m_telnetState) {
        case TelnetState::Normal:
            if (byte == 0xFF) m_telnetState = TelnetState::IacReceived;
            else cleanData.append(c);
            break;
        case TelnetState::IacReceived:
            switch (byte) {
            case 0xFF: cleanData.append((char)0xFF); m_telnetState = TelnetState::Normal; break;
            case 0xFB: m_telnetState = TelnetState::Will; break;
            case 0xFC: m_telnetState = TelnetState::Wont; break;
            case 0xFD: m_telnetState = TelnetState::Do; break;
            case 0xFE: m_telnetState = TelnetState::Dont; break;
            case 0xFA: m_telnetState = TelnetState::SubNegotiation; break;
            default:   m_telnetState = TelnetState::Normal; break;
            }
            break;
        case TelnetState::Will:
        case TelnetState::Wont:
        case TelnetState::Do:
        case TelnetState::Dont:
            if (m_telnetState == TelnetState::Will || m_telnetState == TelnetState::Do) {
                QByteArray reject;
                reject.append((char)0xFF);
                reject.append(m_telnetState == TelnetState::Will ? (char)0xFE : (char)0xFC);
                reject.append((char)byte);
                if (m_socket->state() == QAbstractSocket::ConnectedState) m_socket->write(reject);
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
    if (!cleanData.isEmpty()) sendToSerial(cleanData);
}

void ModemBridge::onSocketDisconnected() {
    if (m_isSshMode) return;
    m_isConnected = false;
    if (!m_suppressCarrierMessage) sendResultCode(3); // NO CARRIER
    emit statusMessage("Modem Bridge: Disconnected.");
    if (m_serial->isOpen()) {
        m_serial->setDataTerminalReady(false); // Drop carrier
        QTimer::singleShot(1500, this, [this](){
            if (m_serial->isOpen()) m_serial->setDataTerminalReady(true); // Raise it for the next call
        });
    }
}


void ModemBridge::onSocketError(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    if (m_isSshMode) return;

    QString errorMsg = m_socket->errorString();
    emit errorOccurred(errorMsg);

    if (!m_isConnected) {
        // Error happened while trying to dial
        sendToSerial(("\r\nERROR: " + errorMsg + "\r\n").toUtf8());
        sendResultCode(3); // NO CARRIER
    } else {
        // Error happened mid-connection! Force the hardware drop.
        m_isConnected = false;
        sendResultCode(3); // NO CARRIER

        if (m_serial->isOpen()) {
            m_serial->setDataTerminalReady(false); // Slam DTR down
            QTimer::singleShot(1500, this, [this](){
                if (m_serial->isOpen()) m_serial->setDataTerminalReady(true); // Raise it back up
            });
        }
    }
}



// ============================================================================
// [NEW] SSH HANDLERS
// ============================================================================
void ModemBridge::onSshConnected() {
    m_isConnected = true;
    // Standard Atari modem CONNECT message
    sendToSerial("CONNECT 57600\r\n");
    emit statusMessage("Modem Bridge: SSH Connected (Secure).");
}

void ModemBridge::onSshDataReceived(const QByteArray &data) {
    emit traceData("RX (SSH)", data);
    sendToSerial(data); // RX Blinks automatically here now
}

void ModemBridge::onSshDisconnected() {
    if (!m_isSshMode) return;
    m_isConnected = false;
    if (!m_suppressCarrierMessage) sendResultCode(3); // NO CARRIER
    emit statusMessage("Modem Bridge: SSH Disconnected.");
    if (m_serial->isOpen()) {
        m_serial->setDataTerminalReady(false); // Drop carrier
        QTimer::singleShot(1500, this, [this](){
            if (m_serial->isOpen()) m_serial->setDataTerminalReady(true); // Raise it for the next call
        });
    }
}


void ModemBridge::onSshError(const QString &msg) {
    if (!m_isSshMode) return;

    emit errorOccurred("SSH Error: " + msg);

    if (!m_isConnected) {
        // Error happened while trying to authenticate/dial
        sendToSerial(("\r\nERROR: SSH - " + msg + "\r\n").toUtf8());
        sendResultCode(3); // NO CARRIER
    } else {
        // Error happened mid-connection! Force the hardware drop.
        m_isConnected = false;
        sendResultCode(3); // NO CARRIER

        if (m_serial->isOpen()) {
            m_serial->setDataTerminalReady(false); // Slam DTR down
            QTimer::singleShot(1500, this, [this](){
                if (m_serial->isOpen()) m_serial->setDataTerminalReady(true); // Raise it back up
            });
        }
    }
}



// ============================================================================
// UTILITIES
// ============================================================================

void ModemBridge::hangup() {
    emit statusMessage("Modem Bridge: Hangup.");
    m_serialBuffer.clear();
    if (m_serial->isOpen()) {
        m_serial->clear(QSerialPort::Output);
    }
    if (m_isSshMode) m_ssh->disconnectFromHost();
    else             m_socket->disconnectFromHost();

    if (m_pendingSocket) {
        m_pendingSocket->disconnect(this); // [FIX] Block signals before killing
        m_pendingSocket->disconnectFromHost();
        m_pendingSocket->deleteLater();
        m_pendingSocket = nullptr;
        m_ringPhase = false;
    }

    m_isConnected = false;
}

void ModemBridge::injectMacro(char macroType) {
    // Check connection first
    if (!m_isConnected) {
        emit errorOccurred("Cannot send macro - Not Connected.");
        return;
    }

    QString textToSend;
    if (macroType == 'U' || macroType == 'u') textToSend = m_currentConnection.login;
    else if (macroType == 'P' || macroType == 'p') textToSend = m_currentConnection.password;

    if (!textToSend.isEmpty()) {
        QByteArray bytes = textToSend.toUtf8() + "\r";

        emit traceData(m_isSshMode ? "TX (MACRO)" : "TX (MACRO)", bytes);
        emit txActivity();

        if (m_isSshMode) m_ssh->write(bytes);
        else             m_socket->write(bytes);

        emit statusMessage(QString("Modem Bridge: Sent Macro %1").arg(macroType));
    }
}

void ModemBridge::sendToSerial(const QByteArray &data) {
    if (m_serial->isOpen()) {
        m_serial->write(data);

        // --- NEW: Centralized RX Blinker ---
        emit rxActivity();

        // Let the Hex Dump see local responses like "OK" and "NO CARRIER"
        if (!m_isConnected) {
            emit traceData("RX (CMD)", data);
        }
    }
}


void ModemBridge::setPhonebookPath(const QString &path) {
    if (!path.isEmpty()) loadPhonebook(path);
}

void ModemBridge::loadPhonebook(const QString &path) {
    m_phonebook.clear();
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QDomDocument doc;
    if (!doc.setContent(&file)) { file.close(); return; }
    file.close();

    QDomNodeList list = doc.elementsByTagName("BBS");
    for (int i = 0; i < list.size(); i++) {
        QDomElement e = list.at(i).toElement();
        BbsEntry bbs;
        bbs.name = e.attribute("name");
        bbs.ip = e.attribute("ip");
        bbs.port = e.attribute("port").toInt();
        bbs.protocol = e.attribute("protocol"); // [NEW] Read Protocol
        bbs.login = e.attribute("login");
        bbs.password = e.attribute("password");
        m_phonebook.append(bbs);
    }
    emit statusMessage(QString("Modem Bridge: Loaded %1 entries.").arg(m_phonebook.size()));
}

BbsEntry ModemBridge::findBbsByName(const QString &name) {
    for (const BbsEntry &entry : m_phonebook) {
        if (entry.name.compare(name, Qt::CaseInsensitive) == 0) return entry;
    }
    return BbsEntry();
}

void ModemBridge::parseInteractiveSshTarget(const QString &target) {
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

    m_currentConnection = BbsEntry();
    m_currentConnection.protocol = "SSH-AUTH";
    m_currentConnection.login = user;
    m_currentConnection.ip = host;
    m_currentConnection.port = port;
    m_currentConnection.name = host;

    m_waitingForSshPassword = true;
    sendToSerial("\r\nPASSWORD: ");
}

void ModemBridge::executeInteractiveSshDial() {
    m_isSshMode = true;
    m_isConnected = false;

    if (m_socket->state() != QAbstractSocket::UnconnectedState) m_socket->abort();
    if (m_ssh->isConnected()) m_ssh->disconnectFromHost();

    emit statusMessage(QString("Modem Bridge: Dialing %1:%2...").arg(m_currentConnection.ip).arg(m_currentConnection.port));
    m_serial->write("\r\nDIALING...\r\n");

    m_ssh->connectToHost(m_currentConnection.ip, m_currentConnection.port, m_currentConnection.login, m_currentConnection.password);
}


void ModemBridge::onNewConnection() {
    QTcpSocket *client = m_tcpServer->nextPendingConnection();

    // Busy check: Are we already connected, or already ringing?
    if (m_isConnected || m_socket->state() != QAbstractSocket::UnconnectedState || m_ssh->isConnected() || m_pendingSocket) {
        emit statusMessage(QString("Modem Bridge: Rejected inbound call from %1 (Busy).").arg(client->peerAddress().toString()));
        client->disconnectFromHost();
        client->deleteLater();
        return;
    }

    emit statusMessage(QString("Modem Bridge: RING from %1...").arg(client->peerAddress().toString()));
    m_pendingSocket = client;
    m_ringPhase = true;
    connect(m_pendingSocket, &QTcpSocket::disconnected, this, &ModemBridge::onPendingSocketDisconnected);

    // Auto-Answer Logic Evaluation
    if (m_s0Register > 0) {
        // Calculate delay: 2 seconds per ring cycle
        int ringDelay = m_s0Register * 2000;
        QTimer::singleShot(ringDelay, this, &ModemBridge::onAutoAnswerTriggered);
    } else {
        m_ringTimer->start(30000); // Standard watchdog if auto-answer is off
    }
   sendResultCode(2); // RING
}


void ModemBridge::onRingTimeout() {
    emit statusMessage("Modem Bridge: Ring timeout (No ATA).");
    if (m_pendingSocket) m_pendingSocket->disconnectFromHost();
}


void ModemBridge::onPendingSocketDisconnected() {
    if (m_pendingSocket) {
        m_pendingSocket->disconnect(this); // [FIX] Safe disconnect
        m_pendingSocket->deleteLater();
        m_pendingSocket = nullptr;
        m_ringPhase = false;
        sendResultCode(3);
        emit statusMessage("Modem Bridge: Caller disconnected before answer.");
    }
}


void ModemBridge::onAutoAnswerTriggered() {
    if (m_ringPhase && m_pendingSocket) {
        emit statusMessage(QString("Modem Bridge: Auto-answering call (S0=%1)").arg(m_s0Register));
        m_socket->disconnect(this);
        m_socket->deleteLater();
        m_socket = m_pendingSocket;
        m_pendingSocket = nullptr;
        m_ringPhase = false;
        m_isSshMode = false;
        m_isConnected = true;
        connect(m_socket, &QTcpSocket::readyRead, this, &ModemBridge::onSocketDataReceived);
        connect(m_socket, &QTcpSocket::connected, this, &ModemBridge::onSocketConnected);
        connect(m_socket, &QTcpSocket::disconnected, this, &ModemBridge::onSocketDisconnected);
        connect(m_socket, &QTcpSocket::errorOccurred, this, &ModemBridge::onSocketError);
        sendToSerial("\r\nCONNECT 19200\r\n");
    }
}


void ModemBridge::updateListenerConfig() {
    if (!aspeqtSettings) return;

    // Only listen if the Modem Bridge serial port is actually open
    bool shouldListen = aspeqtSettings->bbsListenerEnabled() && m_isActive;
    int port = aspeqtSettings->modemListenPort();

    if (shouldListen) {
        if (m_tcpServer->isListening()) {
            if (m_tcpServer->serverPort() != port) {
                m_tcpServer->close();
                if (m_tcpServer->listen(QHostAddress::Any, port)) {
                    emit statusMessage(QString("Modem Bridge: BBS listener restarted on port %1").arg(port));
                }
            }
        } else {
            if (m_tcpServer->listen(QHostAddress::Any, port)) {
                emit statusMessage(QString("Modem Bridge: Listening for callers on port %1").arg(port));
            } else {
                emit errorOccurred("Modem Bridge: Failed to start BBS listener.");
            }
        }
    } else {
        if (m_tcpServer->isListening()) {
            m_tcpServer->close();
            emit statusMessage("Modem Bridge: BBS listener stopped.");
        }
    }
}
void ModemBridge::sendResultCode(int code) {
    QByteArray resp;
    if (m_verboseResponses) {
        if (code == 0) resp = "\r\nOK\r\n";
        else if (code == 1) resp = "\r\nCONNECT\r\n";
        else if (code == 2) resp = "\r\nRING\r\n";
        else if (code == 3) resp = "\r\nNO CARRIER\r\n";
        else if (code == 4) resp = "\r\nERROR\r\n";
    } else {
        resp = QString("%1\r").arg(code).toLatin1();
    }

    sendToSerial(resp);
}
