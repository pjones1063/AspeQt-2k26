#include "modembridge.h"
#include <QDebug>
#include <QFile>
#include <QDomDocument>

ModemBridge::ModemBridge(QObject *parent) : QObject(parent),
    m_serial(new QSerialPort(this)),
    m_socket(new QTcpSocket(this)),
    m_isActive(false),
    m_isConnected(false),
   m_escapeTimer(new QTimer(this))

{
    m_localEcho = true;

    m_escapeTimer->setSingleShot(true);
    connect(m_escapeTimer, &QTimer::timeout, this, &ModemBridge::checkEscapeSequence);

    connect(m_serial, &QSerialPort::readyRead, this, &ModemBridge::onSerialDataReceived);
    connect(m_socket, &QTcpSocket::readyRead, this, &ModemBridge::onSocketDataReceived);
    connect(m_socket, &QTcpSocket::connected, this, &ModemBridge::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &ModemBridge::onSocketDisconnected);
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &ModemBridge::onSocketError);

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
    if (m_socket->state() == QAbstractSocket::ConnectedState) m_socket->disconnectFromHost();
    m_isActive = false;
    m_isConnected = false;
}

void ModemBridge::onSerialDataReceived() {
    QByteArray data = m_serial->readAll();
    if (!m_isActive) return;

    // ----------------------------------------
    // MODE 1: CONNECTED (Data Mode)
    // ----------------------------------------
    if (m_isConnected) {
        for (char c : data) {

            // [FIX] ATASCII DELETE -> ASCII BACKSPACE
            // Atari sends 126 (0x7E) or 127 (0x7F). BBS wants 8 (0x08).
            if (c == 126 || c == 127) {
                c = 8;
            }

            // 1. ESCAPE SEQUENCE (+++)
            if (c == '+') {
                m_escapeBuffer.append(c);
                if (m_escapeBuffer.length() > 3) {
                    m_socket->write(m_escapeBuffer);
                    m_escapeBuffer.clear();
                } else if (m_escapeBuffer.length() == 3) {
                    if (m_escapeTimer) m_escapeTimer->start(1000);
                }
                continue;
            } else {
                if (!m_escapeBuffer.isEmpty()) {
                    m_socket->write(m_escapeBuffer);
                    m_escapeBuffer.clear();
                }
                if (m_escapeTimer) m_escapeTimer->stop();
            }

            // 2. MACROS
            if (m_escPressed) {
                m_escPressed = false;
                if (c == 'u' || c == 'U') {
                    if (!m_currentConnection.login.isEmpty()) {
                        m_socket->write(m_currentConnection.login.toUtf8());
                        m_socket->write("\r");
                    }
                    continue;
                }
                else if (c == 'p' || c == 'P') {
                    if (!m_currentConnection.password.isEmpty()) {
                        m_socket->write(m_currentConnection.password.toUtf8());
                        m_socket->write("\r");
                    }
                    continue;
                }
                else if (c == 'h' || c == 'H') { // Hangup Hotkey
                    emit statusMessage("Modem Bridge: ESC-H detected. Hanging up.");
                    if (m_socket->state() != QAbstractSocket::UnconnectedState) m_socket->disconnectFromHost();
                    m_isConnected = false;
                    continue;
                }
                m_socket->write(QByteArray(1, 0x1B));
                m_socket->write(QByteArray(1, c));
            }
            else if (c == 0x1B) {
                m_escPressed = true;
            }
            else {
                m_socket->write(QByteArray(1, c));
            }
        }
    }
    // ----------------------------------------
    // MODE 2: COMMAND MODE
    // ----------------------------------------
    else {
        if (m_localEcho) m_serial->write(data);

        for (char c : data) {
            // [FIX] Handle Atari EOL (155) in Command Mode too?
            // Usually Ice-T handles this, but mapping 155->13 is safe.
            if (c == '\r' || c == 155) {
                m_serial->write("\n");
                processAtCommand(m_serialBuffer);
                m_serialBuffer.clear();
            } else if (c != '\n') {
                // Fix Backspace in Command Mode too (so you can fix typos in AT commands)
                if (c == 126 || c == 127 || c == 8) {
                    if (!m_serialBuffer.isEmpty()) {
                        m_serialBuffer.chop(1); // Remove last char from buffer
                        // Echo a real backspace to screen to erase char
                        // (Backspace + Space + Backspace)
                        m_serial->write(QByteArray(1, 8));
                        m_serial->write(" ");
                        m_serial->write(QByteArray(1, 8));
                    }
                } else {
                    m_serialBuffer.append(c);
                }
            }
        }
    }
}


void ModemBridge::processAtCommand(const QByteArray &cmd) {
    QString upperCmd = QString::fromLatin1(cmd).trimmed().toUpper();
    if (upperCmd.startsWith("AT")) upperCmd = upperCmd.mid(2);

    // --- DIAL (ATDT) ---
    if (upperCmd.startsWith("D")) {

        // [FIX] Auto-Disconnect if already online
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            emit statusMessage("Modem Bridge: Disconnecting previous session...");
            m_socket->abort(); // Immediate hard close
            m_isConnected = false;
        }

        QString target = upperCmd.mid(1).trimmed();
        if (target.startsWith("T")) target = target.mid(1).trimmed();

        QString host = target;
        int port = 23;

        // 1. Phonebook Lookup
        bool found = false;
        for (const BbsEntry &entry : m_phonebook) {
            if (entry.name.compare(target, Qt::CaseInsensitive) == 0) {
                host = entry.ip;
                port = entry.port;
                m_currentConnection = entry; // Save for Macros
                found = true;
                break;
            }
        }

        // 2. Raw Parse
        if (!found) {
            m_currentConnection = BbsEntry();
            QStringList parts = target.split(':');
            host = parts[0];
            if (parts.size() > 1) port = parts[1].toInt();
        }

        m_serial->write("\r\nDIALING...\r\n");
        m_socket->connectToHost(host, port);
    }
    // --- HANGUP (ATH) ---
    else if (upperCmd.startsWith("H")) {
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->disconnectFromHost();
            // The 'NO CARRIER' message will be sent by onSocketDisconnected
        } else {
            m_serial->write("\r\nOK\r\n");
        }
        m_isConnected = false;
    }
    // --- RESET (ATZ) ---
    else if (upperCmd.startsWith("Z")) {
        m_suppressCarrierMessage = true; // Prevent "NO CARRIER" spam
        m_socket->abort();
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


void ModemBridge::onSocketConnected() {
    m_isConnected = true;
    sendToSerial("CONNECT\r\n"); // The magic word the Atari is waiting for!
    emit statusMessage("Modem Bridge: Connected to remote host.");
}


void ModemBridge::onSocketDataReceived() {
    QByteArray data = m_socket->readAll();

    if (!m_isTelnetMode) {
        // Raw Mode (good for SSH later)
        sendToSerial(data);
        return;
    }

    sendToSerial(data);

    // Telnet Protocol Filter (Strip IAC sequences: FF xx xx)

/*    QByteArray filteredData;
    for (char c : data) {
        unsigned char byte = static_cast<unsigned char>(c);

        switch (m_telnetState) {
        case 0: // Normal Data
            if (byte == 255) { // IAC (Interpret As Command)
                m_telnetState = 1;
            } else {
                filteredData.append(c);
            }
            break;

        case 1: // Seen IAC (255)
            if (byte == 255) {
                // Double 255 means literal 255 data byte
                filteredData.append(static_cast<char>(255));
                m_telnetState = 0;
            } else if (byte >= 251 && byte <= 254) {
                // WILL, WONT, DO, DONT (Requires 1 more option byte)
                m_telnetState = 2;
            } else {
                // Other commands (NOP, DM, BRK, IP, etc) - No option byte needed
                m_telnetState = 0;
            }
            break;

        case 2: // Seen Command (WILL/WONT...), ignore Option byte
            m_telnetState = 0; // Reset to normal
            break;
        }
    }

    if (!filteredData.isEmpty()) {
        sendToSerial(filteredData);
    }
    */
}


void ModemBridge::onSocketDisconnected() {
    m_isConnected = false;
    sendToSerial("\r\nNO CARRIER\r\n");
    emit statusMessage("Modem Bridge: Remote host disconnected.");
}

void ModemBridge::sendToSerial(const QByteArray &data) {
    if (m_serial->isOpen()) {
        m_serial->write(data);
    }
}

/* modembridge.cpp - Update onSocketError */

void ModemBridge::onSocketError(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    QString err = m_socket->errorString();

    // Log to PC (AspeQt Log Window)
    emit errorOccurred(QString("Modem Bridge Error: %1").arg(err));

    // Tell the Atari the bad news
    // Only send if we are NOT connected (i.e. the dialing attempt failed)
    if (!m_isConnected) {
        // "BUSY" is sometimes better for connection refused,
        // but "NO CARRIER" is the universal Hayes standard for failure.
        sendToSerial("\r\nNO CARRIER\r\n");
    }
}


void ModemBridge::checkEscapeSequence() {
    // Timer fired! Use guard time silence to confirm.
    if (m_escapeBuffer == "+++") {
        m_escapeBuffer.clear();
        m_isConnected = false; // Switch to Command Mode (Do NOT disconnect socket)
        m_serial->write("\r\nOK\r\n");
    } else {
        // Should not happen if logic is right, but flush just in case
        if (!m_escapeBuffer.isEmpty()) m_socket->write(m_escapeBuffer);
        m_escapeBuffer.clear();
    }
}



void ModemBridge::setTcpMode(bool enableSsh) {
    // Placeholder: In the future, this flag could switch m_socket
    // from QTcpSocket to a QSshSocket wrapper.
    // For now, we just store it or ignore it if SSH isn't ready.
    Q_UNUSED(enableSsh);
    // m_isSshEnabled = enableSsh;
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

void ModemBridge::setPhonebookPath(const QString &path) {
    if (!path.isEmpty()) loadPhonebook(path);
}

void ModemBridge::dial(const QString &target) {
    // This allows MainWindow to just say dial("13th Leader BBS")
    // and reusing the logic in processAtCommand
    processAtCommand(("ATDT " + target).toUtf8());
}

void ModemBridge::loadPhonebook(const QString &path) {
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
    emit statusMessage(QString("Modem Bridge: Loaded %1 entries.").arg(m_phonebook.count()));
}

BbsEntry ModemBridge::findBbsByName(const QString &name) {
    for (const BbsEntry &entry : m_phonebook) {
        if (entry.name.compare(name, Qt::CaseInsensitive) == 0) {
            return entry;
        }
    }
    return BbsEntry();
}


