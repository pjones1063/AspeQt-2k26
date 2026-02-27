#include "modembridge.h"
#include <QDebug>
#include <QFile>
#include <QDomDocument>

ModemBridge::ModemBridge(QObject *parent) : QObject(parent),
    m_serial(new QSerialPort(this)),
    m_socket(new QTcpSocket(this)),
    m_ssh(new SshClient(this)), // [NEW] Init SSH
    m_isActive(false),
    m_isConnected(false),
    m_isSshMode(false), // Default to Telnet
    m_escapeTimer(new QTimer(this))
{
    m_localEcho = true;

    m_escapeTimer->setSingleShot(true);
    connect(m_escapeTimer, &QTimer::timeout, this, &ModemBridge::checkEscapeSequence);

    // Serial
    connect(m_serial, &QSerialPort::readyRead, this, &ModemBridge::onSerialDataReceived);

    // TCP (Telnet)
    connect(m_socket, &QTcpSocket::readyRead, this, &ModemBridge::onSocketDataReceived);
    connect(m_socket, &QTcpSocket::connected, this, &ModemBridge::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &ModemBridge::onSocketDisconnected);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &ModemBridge::onSocketError);

    // [NEW] SSH Connections
    connect(m_ssh, &SshClient::connected, this, &ModemBridge::onSshConnected);
    connect(m_ssh, &SshClient::disconnected, this, &ModemBridge::onSshDisconnected);
    connect(m_ssh, &SshClient::rxData, this, &ModemBridge::onSshDataReceived);
    connect(m_ssh, &SshClient::error, this, &ModemBridge::onSshError);
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
    } else {
        emit errorOccurred("Modem Bridge: Failed to open serial port.");
    }
}

void ModemBridge::stop() {
    if (m_serial->isOpen()) m_serial->close();

    // Close both types of connections
    if (m_socket->state() != QAbstractSocket::UnconnectedState) m_socket->disconnectFromHost();
    if (m_ssh->isConnected()) m_ssh->disconnectFromHost();

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
        for (char c : data) {
            // Atari Backspace Fix (126/127 -> 8)
            if (c == 126 || c == 127) c = 8;

            // 1. ESCAPE SEQUENCE (+++)
            if (c == '+') {
                m_escapeBuffer.append(c);
                if (m_escapeBuffer.length() > 3) {
                    // Not an escape sequence, flush it out
                    if (m_isSshMode) m_ssh->write(m_escapeBuffer);
                    else             m_socket->write(m_escapeBuffer);
                    m_escapeBuffer.clear();
                } else if (m_escapeBuffer.length() == 3) {
                    if (m_escapeTimer) m_escapeTimer->start(1000);
                }
                continue;
            } else {
                if (!m_escapeBuffer.isEmpty()) {
                    if (m_isSshMode) m_ssh->write(m_escapeBuffer);
                    else             m_socket->write(m_escapeBuffer);
                    m_escapeBuffer.clear();
                }
                if (m_escapeTimer) m_escapeTimer->stop();
            }

            // 2. MACROS
            if (m_escPressed) {
                m_escPressed = false;
                if (c == 'u' || c == 'U') { injectMacro('u'); continue; }
                if (c == 'p' || c == 'P') { injectMacro('p'); continue; }
                if (c == 'h' || c == 'H') { hangup(); continue; }

                // Pass literal ESC char if not a macro
                QByteArray esc(1, 0x1B);
                esc.append(c);
                if (m_isSshMode) m_ssh->write(esc);
                else             m_socket->write(esc);
            }
            else if (c == 0x1B) {
                m_escPressed = true;
            }
            else {
                // Normal Character
                if (m_isSshMode) m_ssh->write(QByteArray(1, c));
                else             m_socket->write(QByteArray(1, c));
            }
        }
    }
    // --- MODE 2: COMMAND MODE ---
    else {
        if (m_localEcho) m_serial->write(data);

        for (char c : data) {
            if (c == '\r' || c == 155) {
                m_serial->write("\n");
                processAtCommand(m_serialBuffer);
                m_serialBuffer.clear();
            } else if (c != '\n') {
                if (c == 126 || c == 127 || c == 8) {
                    if (!m_serialBuffer.isEmpty()) {
                        m_serialBuffer.chop(1);
                        m_serial->write(QByteArray(1, 8)); // BS
                        m_serial->write(" ");              // Space
                        m_serial->write(QByteArray(1, 8)); // BS
                    }
                } else {
                    m_serialBuffer.append(c);
                }
            }
        }
    }
}

// ============================================================================
// CONNECTION LOGIC
// ============================================================================
void ModemBridge::processAtCommand(const QByteArray &cmd) {
    QString upperCmd = QString::fromLatin1(cmd).trimmed().toUpper();
    if (upperCmd.startsWith("AT")) upperCmd = upperCmd.mid(2);

    // --- DIAL (ATDT) ---
    if (upperCmd.startsWith("D")) {
        QString target = upperCmd.mid(1).trimmed();
        if (target.startsWith("T")) target = target.mid(1).trimmed();

        QString host = target;
        int port = 23;
        bool found = false;

        // A. Phonebook Lookup
        for (const BbsEntry &entry : m_phonebook) {
            if (entry.name.compare(target, Qt::CaseInsensitive) == 0) {
                host = entry.ip;
                port = entry.port;
                m_currentConnection = entry; // Loads Protocol, Login, Pass
                found = true;
                break;
            }
        }

        // B. Manual Dial
        if (!found) {
            m_currentConnection = BbsEntry();

            // Check for "SSH:" prefix (e.g., ATDT SSH:192.168.1.50)
            if (host.startsWith("SSH:")) {
                m_currentConnection.protocol = "SSH";
                host = host.mid(4);
                port = 22; // Default SSH port
            }

            QStringList parts = host.split(':');
            host = parts[0];
            if (parts.size() > 1) port = parts[1].toInt();

            m_currentConnection.ip = host;
            m_currentConnection.port = port;
        }

        connectTo(host, port);
    }
    // --- HANGUP (ATH) ---
    else if (upperCmd.startsWith("H")) {
        hangup();
        m_serial->write("\r\nOK\r\n");
    }
    // --- RESET (ATZ) ---
    else if (upperCmd.startsWith("Z")) {
        m_suppressCarrierMessage = true;

        m_socket->abort();
        m_ssh->disconnectFromHost();

        m_isConnected = false;
        m_currentConnection = BbsEntry();
        m_escapeBuffer.clear();
        m_serial->write("\r\nOK\r\n");
        m_suppressCarrierMessage = false;
    }
    else {
        m_serial->write("\r\nOK\r\n");
    }
}

void ModemBridge::dial(const BbsEntry &entry) {
    m_currentConnection = entry;
    connectTo(entry.ip, entry.port);
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
    // If protocol is explicitly SSH, OR if port is 22 (and not explicitly Telnet)
    bool useSsh = (m_currentConnection.protocol.toUpper() == "SSH") || (port == 22);

    if (useSsh) {
        m_isSshMode = true;
        // Use saved user/pass or defaults
        QString user = m_currentConnection.login.isEmpty() ? "guest" : m_currentConnection.login;
        QString pass = m_currentConnection.password;
        m_ssh->connectToHost(host, port, user, pass);
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
    emit rxActivity();
    sendToSerial(data);
}

void ModemBridge::onSocketDisconnected() {
    if (m_isSshMode) return; // Ignore TCP signals if we are in SSH mode
    m_isConnected = false;
    if (!m_suppressCarrierMessage) sendToSerial("\r\nNO CARRIER\r\n");
    emit statusMessage("Modem Bridge: Disconnected.");
}

void ModemBridge::onSocketError(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    if (m_isSshMode) return;
    emit errorOccurred(m_socket->errorString());
    if (!m_isConnected) sendToSerial("\r\nNO CARRIER\r\n");
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
    emit rxActivity();
    sendToSerial(data);
}

void ModemBridge::onSshDisconnected() {
    if (!m_isSshMode) return;
    m_isConnected = false;
    if (!m_suppressCarrierMessage) sendToSerial("\r\nNO CARRIER\r\n");
    emit statusMessage("Modem Bridge: SSH Disconnected.");
}

void ModemBridge::onSshError(const QString &msg) {
    // Only report error if we are actively trying to use SSH
    if (m_isSshMode) {
        emit errorOccurred("SSH Error: " + msg);
        if (!m_isConnected) sendToSerial("\r\nNO CARRIER\r\n");
    }
}

// ============================================================================
// UTILITIES
// ============================================================================

void ModemBridge::hangup() {
    emit statusMessage("Modem Bridge: Hangup.");

    if (m_isSshMode) m_ssh->disconnectFromHost();
    else             m_socket->disconnectFromHost();

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

        if (m_isSshMode) m_ssh->write(bytes);
        else             m_socket->write(bytes);

        emit statusMessage(QString("Modem Bridge: Sent Macro %1").arg(macroType));
        emit txActivity();
    }
}

void ModemBridge::sendToSerial(const QByteArray &data) {
    if (m_serial->isOpen()) m_serial->write(data);
}

void ModemBridge::checkEscapeSequence() {
    if (m_escapeBuffer == "+++") {
        m_escapeBuffer.clear();
        m_isConnected = false; // Enter Command Mode
        m_serial->write("\r\nOK\r\n");
    } else {
        if (!m_escapeBuffer.isEmpty()) {
            if (m_isSshMode) m_ssh->write(m_escapeBuffer);
            else             m_socket->write(m_escapeBuffer);
        }
        m_escapeBuffer.clear();
    }
}

void ModemBridge::setPhonebookPath(const QString &path) {
    if (!path.isEmpty()) loadPhonebook(path);
}

void ModemBridge::loadPhonebook(const QString &path) {
    if (path.isEmpty()) return;
    m_phonebook.clear();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QDomDocument doc;
    if (!doc.setContent(&file)) { file.close(); return; }
    file.close();

    QDomNodeList list = doc.elementsByTagName("BBS");
    for (int i = 0; i < list.count(); i++) {
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
    emit statusMessage(QString("Modem Bridge: Loaded %1 entries.").arg(m_phonebook.count()));
}

BbsEntry ModemBridge::findBbsByName(const QString &name) {
    for (const BbsEntry &entry : m_phonebook) {
        if (entry.name.compare(name, Qt::CaseInsensitive) == 0) return entry;
    }
    return BbsEntry();
}
