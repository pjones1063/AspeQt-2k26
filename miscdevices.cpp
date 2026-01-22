
/*
 * miscdevices.cpp
 */

#include "qprocess.h"
#ifdef Q_OS_WIN
#include "windows.h"
#endif

#include "miscdevices.h"
#include "aspeqtsettings.h"
#include "mainwindow.h"
#include <QDateTime>
#include <QtDebug>
#include <QDesktopServices>
#include <QUrl>
#include <QLocale>

extern char g_rclSlotNo;
bool conversionMsgdisplayedOnce;

QString imageFileName;
QByteArray  commandOutput;

QHash <quint8, QString> files;


// ==========================================
// RS232 Implementation (Dual Mode: Physical & Telnet)
// ==========================================

Rs232::Rs232(SioWorker *worker) : SioDevice(worker)
{
    // Init Physical
    m_serialPort = new QSerialPort(this);
#ifdef Q_OS_LINUX
    m_portName = "ttyUSB";
#else
    m_portName = "COM";
#endif

    // Init Telnet
    m_tcpSocket = new QTcpSocket(this);
    m_isTcpConnected = false;

    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &Rs232::onSocketReadyRead);
    connect(m_tcpSocket, &QTcpSocket::connected, this, &Rs232::onSocketConnected);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &Rs232::onSocketDisconnected);
    // Note: using QTcpSocket::errorOccurred for Qt 6 compatibility
    connect(m_tcpSocket, &QTcpSocket::errorOccurred, this, &Rs232::onSocketError);
}

Rs232::~Rs232()
{
    if (m_serialPort->isOpen()) m_serialPort->close();
    m_tcpSocket->abort();
}

// --------------------------------------------------------------------------
// THE MAIN TRIGGER
// --------------------------------------------------------------------------
void Rs232::handleCommand(quint8 command, quint16 aux)
{
    // Determine which mode we are in for this device (R1, R2, etc.)
    int index = m_deviceNo - RS232_BASE_CDEVIC;
    if (index < 0) index = 0;

    int mode = respeqtSettings->rs232Mode(index); // 0 = Physical, 1 = Telnet

    // Log the incoming command for debugging
    qDebug() << "!n" << tr("[%1] Command: $%2 Aux: $%3 Mode: %4")
                            .arg(deviceName())
                            .arg(command, 2, 16, QChar('0'))
                            .arg(aux, 4, 16, QChar('0'))
                            .arg(mode == 1 ? "Telnet" : "Physical");

    if (mode == 1) {
        // --- TELNET MODE ---
        if (m_serialPort->isOpen()) m_serialPort->close();
        handleTelnet(command, aux);
    }
    else {
        // --- PHYSICAL MODE ---
        if (m_tcpSocket->state() != QAbstractSocket::UnconnectedState)
            m_tcpSocket->abort();
        handlePhysical(command, aux);
    }
}

// --------------------------------------------------------------------------
// PHYSICAL PORT LOGIC
// --------------------------------------------------------------------------
void Rs232::handlePhysical(quint8 command, quint16 aux)
{
    // FIX: Always handle STATUS ('S') command, even if port is closed.
    if (command == 0x53)
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray status(4, 0);
        quint8 bits = 0x0F; // Default idle bits

        if (m_serialPort->isOpen()) {
            QSerialPort::PinoutSignals pins = m_serialPort->pinoutSignals();
            if (pins & QSerialPort::DataSetReadySignal) bits |= 0x10; // DSR
            if (pins & QSerialPort::ClearToSendSignal)  bits |= 0x20; // CTS
            if (pins & QSerialPort::RingIndicatorSignal) bits |= 0x40; // RI
            if (pins & QSerialPort::DataCarrierDetectSignal) bits |= 0x80; // DCD
        } else {
            // Fake "Ready" status so driver loads even if port not yet open
            // DSR ($10) + CTS ($20) + Idle ($0F) = $3F
            bits = 0x3F;
        }

        status[0] = bits; // CORRECTED: Bits go in Byte 0
        status[1] = 0;
        status[2] = 0;
        status[3] = 0;

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(status);
        return;
    }

    // Lazy Initialization for Read/Write commands
    if (!m_serialPort->isOpen()) {
        int index = m_deviceNo - RS232_BASE_CDEVIC;
        if (index < 0) index = 0;

        QString portName = respeqtSettings->rs232PortName(index);

        if (portName.isEmpty() || portName == "None") {
            sio->port()->writeCommandNak();
            return;
        }

        m_serialPort->setPortName(portName);

        if (!m_serialPort->open(QIODevice::ReadWrite)) {
            qCritical() << "!e" << tr("[%1] Failed to open PC serial port %2: %3")
            .arg(deviceName()).arg(portName).arg(m_serialPort->errorString());
            sio->port()->writeCommandNak();
            return;
        }

        // Defaults
        m_serialPort->setBaudRate(9600);
        m_serialPort->setDataBits(QSerialPort::Data8);
        m_serialPort->setParity(QSerialPort::NoParity);
        m_serialPort->setStopBits(QSerialPort::OneStop);
        m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
    }

    switch(command)
    {
    case 0x57: // 'W' Write
    {
        if (!sio->port()->writeCommandAck()) return;
        QByteArray data = sio->port()->readDataFrame(aux);
        if (data.isEmpty()) { sio->port()->writeDataNak(); return; }

        m_serialPort->write(data);
        m_serialPort->waitForBytesWritten(10);

        sio->port()->writeDataAck();
        sio->port()->writeComplete();
        break;
    }
    case 0x52: // 'R' Read
    {
        if (!sio->port()->writeCommandAck()) return;
        QByteArray data = m_serialPort->read(aux);
        if (data.size() < aux) {
            m_serialPort->waitForReadyRead(20);
            data.append(m_serialPort->read(aux - data.size()));
        }
        if (data.size() < aux) data.append(QByteArray(aux - data.size(), 0));

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(data);
        break;
    }
    case 0x53: // 'S' Status (Handled above, but kept for completeness)
        break;

    case 0x43: // 'C' Control
        configurePort(aux & 0xFF, aux >> 8);
        sio->port()->writeCommandAck();
        sio->port()->writeComplete();
        break;

    // --- STUB COMMANDS TO SATISFY DRIVERS ---
    case 0x58: // 'X' Start Stream
    case 0x41: // 'A' Translate
    case 0x3F: // '?' Probe / Identify
    case 0xF3: // Alternative Probe
        if (!sio->port()->writeCommandAck()) return;

        // Safety: If the driver asked for data (aux > 0), send zeros back.
        if (aux > 0) {
            QByteArray emptyData(aux, 0);
            sio->port()->writeComplete();
            sio->port()->writeDataFrame(emptyData);
        } else {
            sio->port()->writeComplete();
        }
        break;

    default:
        // --- DIAGNOSTIC HACK: ACK EVERYTHING ---
        // Instead of NAKing unknown commands, we ACK them.
        // If the driver loads now, check the logs to see what Command ID we missed!
        qCritical() << "!!! UNKNOWN COMMAND DETECTED: $"
                    << QString::number(command, 16).toUpper()
                    << " (Aux: $" << QString::number(aux, 16).toUpper() << ") !!!";

        qCritical() << "!!! FAKING 'ACK' TO FORCE DRIVER LOAD !!!";

        sio->port()->writeCommandAck();

        // If the unknown command wanted data (Aux > 0), we MUST send it or SIO hangs.
        if (aux > 0) {
            QByteArray fakeData(aux, 0);
            sio->port()->writeComplete();
            sio->port()->writeDataFrame(fakeData);
        } else {
            sio->port()->writeComplete();
        }
        break;
    }
}

void Rs232::configurePort(quint16 val1, quint16 /*val2*/)
{
    qint32 baud = 9600;
    switch (val1 & 0x0F) {
    case 0: baud = 300; break;
    case 8: baud = 300; break;
    case 9: baud = 600; break;
    case 10: baud = 1200; break;
    case 12: baud = 2400; break;
    case 13: baud = 4800; break;
    case 14: baud = 9600; break;
    case 15: baud = 19200; break;
    default: baud = 9600;
    }
    m_serialPort->setBaudRate(baud);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);
}

// --------------------------------------------------------------------------
// TELNET MODEM LOGIC
// --------------------------------------------------------------------------
void Rs232::handleTelnet(quint8 command, quint16 aux)
{
    switch(command)
    {
    case 0x57: // 'W' Write (Atari -> Modem)
    {
        if (!sio->port()->writeCommandAck()) return;
        QByteArray data = sio->port()->readDataFrame(aux);
        if (data.isEmpty()) { sio->port()->writeDataNak(); return; }

        if (m_isTcpConnected) {
            // DATA MODE: Send to TCP
            m_tcpSocket->write(data);
        }
        else {
            // COMMAND MODE: Build AT command
            m_rxBuffer.append(data);

            for (char c : data) {
                if (c == '\r') {
                    QString cmd = QString::fromLatin1(m_atCommandBuffer).trimmed();
                    m_atCommandBuffer.clear();
                    processAtCommand(cmd);
                }
                else if (c == '\n') { /* ignore */ }
                else if (c == 0x7F || c == 0x08) { // Backspace
                    if (!m_atCommandBuffer.isEmpty()) m_atCommandBuffer.chop(1);
                }
                else {
                    m_atCommandBuffer.append(c);
                }
            }
        }

        sio->port()->writeDataAck();
        sio->port()->writeComplete();
        break;
    }

    case 0x52: // 'R' Read (Modem -> Atari)
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray chunk;
        if (m_rxBuffer.size() >= aux) {
            chunk = m_rxBuffer.left(aux);
            m_rxBuffer.remove(0, aux);
        } else {
            chunk = m_rxBuffer;
            m_rxBuffer.clear();
            // Pad if not enough data
            while (chunk.size() < aux) chunk.append((char)0);
        }

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(chunk);
        break;
    }

    case 0x53: // 'S' Status
    {
        if (!sio->port()->writeCommandAck()) return;
        QByteArray status(4, 0);

        // 850 STATUS BITS:
        // Bit 4 ($10): CTS
        // Bit 5 ($20): DSR
        // Bit 6 ($40): RI
        // Bit 7 ($80): DCD
        // Bit 0-3 ($0F): Idle

        // Force "All Good" status: $30 (DSR+CTS) | $80 (DCD) | $0F = $BF
        quint8 bits = 0xBF;

        status[0] = 0xFF; // CORRECTED: Bits go in Byte 0
        status[1] = 0;
        status[2] = 0;
        status[3] = 0;

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(status);
        break;
    }

    case 0x43: // 'C' Control
        sio->port()->writeCommandAck();
        sio->port()->writeComplete();
        break;

        // --- STUB COMMANDS ---
    case 0x58: // 'X' Start Stream
    case 0x41: // 'A' Translate
    case 0x3F: // '?' Probe / Identify
    case 0xF3: // Alternative Probe
        if (!sio->port()->writeCommandAck()) return;
        if (aux > 0) {
            QByteArray emptyData(aux, 0);
            sio->port()->writeComplete();
            sio->port()->writeDataFrame(emptyData);
        } else {
            sio->port()->writeComplete();
        }
        break;

    default:
        // --- DIAGNOSTIC HACK: ACK EVERYTHING ---
        // Instead of NAKing unknown commands, we ACK them.
        // If the driver loads now, check the logs to see what Command ID we missed!
        qCritical() << "!!! UNKNOWN COMMAND DETECTED: $"
                    << QString::number(command, 16).toUpper()
                    << " (Aux: $" << QString::number(aux, 16).toUpper() << ") !!!";

        qCritical() << "!!! FAKING 'ACK' TO FORCE DRIVER LOAD !!!";

        sio->port()->writeCommandAck();

        // If the unknown command wanted data (Aux > 0), we MUST send it or SIO hangs.
        if (aux > 0) {
            QByteArray fakeData(aux, 0);
            sio->port()->writeComplete();
            sio->port()->writeDataFrame(fakeData);
        } else {
            sio->port()->writeComplete();
        }
        break;
    }
}

// --- AT Command Parser ---
void Rs232::processAtCommand(QString cmd)
{
    cmd = cmd.toUpper();

    if (cmd == "AT") {
        sendToAtari("OK\r\n");
    }
    else if (cmd == "ATI") {
        sendToAtari("AspeQt Wifi Modem V1.0\r\nOK\r\n");
    }
    else if (cmd.startsWith("ATDT")) {
        QString target = cmd.mid(4).trimmed();
        int port = 23;
        QString host = target;

        if (target.contains(":")) {
            QStringList parts = target.split(":");
            host = parts[0];
            port = parts[1].toInt();
        }

        if (host.isEmpty()) {
            sendToAtari("ERROR\r\n");
        } else {
            sendToAtari("DIALING " + host + "...\r\n");
            m_tcpSocket->connectToHost(host, port);
        }
    }
    else if (cmd == "ATH") {
        if (m_isTcpConnected) m_tcpSocket->disconnectFromHost();
        else sendToAtari("OK\r\n");
    }
    else if (cmd == "ATZ") {
        if (m_isTcpConnected) m_tcpSocket->disconnectFromHost();
        sendToAtari("OK\r\n");
    }
    else {
        sendToAtari("ERROR\r\n");
    }
}

void Rs232::sendToAtari(QString text)
{
    m_rxBuffer.append(text.toLatin1());
}

// --- Socket Slots ---
void Rs232::onSocketConnected() {
    m_isTcpConnected = true;
    sendToAtari("CONNECT\r\n");
    qDebug() << "!n" << tr("[%1] Telnet Connected.").arg(deviceName());
}

void Rs232::onSocketDisconnected() {
    m_isTcpConnected = false;
    sendToAtari("\r\nNO CARRIER\r\n");
    qDebug() << "!n" << tr("[%1] Telnet Disconnected.").arg(deviceName());
}

void Rs232::onSocketReadyRead() {
    QByteArray data = m_tcpSocket->readAll();
    m_rxBuffer.append(data);
}

void Rs232::onSocketError(QAbstractSocket::SocketError) {
    qDebug() << "!e" << tr("[%1] Socket Error: %2").arg(deviceName()).arg(m_tcpSocket->errorString());
    if (!m_isTcpConnected) sendToAtari("BUSY\r\n");
}

// ==========================================
// PRINTER IMPLEMENTATION
// ==========================================

void Printer::handleCommand(quint8 command, quint16 aux)
{
    if(respeqtSettings->printerEmulation()) {
        switch(command) {
        case 0x53:
        {
            if (!sio->port()->writeCommandAck()) return;
            QByteArray status(4, 0);
            status[0] = 0; status[1] = m_lastOperation; status[2] = 3; status[3] = 0;
            sio->port()->writeComplete();
            sio->port()->writeDataFrame(status);
            qDebug() << "!n" << tr("[%1] Get status.").arg(deviceName());
            break;
        }
        case 0x57:
        {
            int aux2 = aux % 256;
            int len;
            switch (aux2) {
            case 0x4e: len = 40; break;
            case 0x53: len = 29; break;
            case 0x44: len = 21; break;
            default:
                sio->port()->writeCommandNak();
                return;
            }
            if (!conversionMsgdisplayedOnce) {
                qDebug() << "!n" << tr("[%1] Converting Inverse Video Characters for ASCII viewing").arg(deviceName()).arg(len);
                conversionMsgdisplayedOnce = true;
            }
            sio->port()->writeCommandAck();

            QByteArray data = sio->port()->readDataFrame(len);
            if (data.isEmpty()) {
                sio->port()->writeDataNak();
                return;
            }
            sio->port()->writeDataAck();
            qDebug() << "!n" << tr("[%1] Print (%2 chars)").arg(deviceName()).arg(len);
            int n = data.indexOf('\x9b');
            if (n == -1) n = len;
            data.resize(n);
            data.replace('\n', '\x9b');
            if (n < len) data.append("\n");
            emit print(QString::fromLatin1(data));
            sio->port()->writeComplete();
            break;
        }
        default:
            sio->port()->writeCommandNak();
        }
    } else {
        qDebug() << "!u" << tr("[%1] ignored").arg(deviceName());
    }
}

// ==========================================
// SMART DEVICE IMPLEMENTATION
// ==========================================

void SmartDevice::handleCommand(quint8 command, quint16 aux)
{
    switch(command)
    {
    case 0x93: // Get APE Time
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray data(6, 0);
        QDateTime dateTime = QDateTime::currentDateTime();
        data[0] = dateTime.date().day();
        data[1] = dateTime.date().month();
        data[2] = dateTime.date().year() % 100;
        data[3] = dateTime.time().hour();
        data[4] = dateTime.time().minute();
        data[5] = dateTime.time().second();

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(data);

        qDebug() << "!n" << tr("[%1] Read date/time (%2).")
                                .arg(deviceName())
                                .arg(QLocale::system().toString(dateTime, QLocale::ShortFormat));
        break;
    }
    case 0x55: // Submit URL
    {
        if(respeqtSettings->isURLSubmitEnabled() && aux!=0 && aux<=2000)
        {
            if (!sio->port()->writeCommandAck()) return;
            QByteArray data = sio->port()->readDataFrame(aux);
            if (data.isEmpty()) {
                sio->port()->writeDataNak();
                sio->port()->writeError();
                return;
            }
            sio->port()->writeDataAck();
            sio->port()->writeComplete();

            QString urlstr(data);
            QDesktopServices::openUrl(QUrl(urlstr));
            qDebug() << "!n" << tr("URL [%1] submitted").arg(urlstr);
        }
        else
        {
            sio->port()->writeCommandNak();
            return;
        }
        break;
    }
    default:
        sio->port()->writeCommandNak();
        break;
    }
}

// ==========================================
// MNU (ASPEQT CLIENT) IMPLEMENTATION
// ==========================================

void Mnu::handleCommand(quint8 command, quint16 aux)
{
    switch (command) {
    case 0x86 : // get server command
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray  fdata(255, 0);
        QString cmd = respeqtSettings->lastRclCommand();
        QByteArray cm = cmd.toUtf8();
        if(cm.length() < 1) cm = "";
        for(int i=0; i < 253; i++)
            fdata[i] = (cm.length() > i) ? (cm[i] & 0xff) : 0x00;

        qCritical() << "!i" << tr(" Get Cmd: [%1]").arg(cmd);
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(fdata);
        return;
    }
    case 0x87 : // Exec server command
    {
        if (!sio->port()->writeCommandAck()) return;

        QProcess process;
        QByteArray  fdata(255, 0);
        fdata = sio->port()->readDataFrame(32);
        QString cmd =  fdata;
        if (cmd.isEmpty()) cmd = respeqtSettings->lastRclCommand();

#if defined(Q_OS_WIN)
        process.start( "cmd ",  QStringList() <<"/c" << cmd  );
#else
        process.start( "sh", QStringList() <<"-c" << cmd  );
#endif
        respeqtSettings->setRclCommand(cmd);
        qCritical().noquote() << "!i" << tr("[%1]").arg(cmd);
        qCritical() << "!i" << tr("Command Complete [%1]").arg("--");

        process.waitForFinished(1000);

        QString co = (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) ?
                         (process.readAllStandardOutput()) :
                         (process.readAllStandardError());

        commandOutput = co.trimmed().toUtf8().replace(10, (char)155);

        sio->port()->writeDataAck();
        sio->port()->writeComplete();
        return;
    }
    case 0x88 : // get server command result
    {
        if (!sio->port()->writeCommandAck()) return;
        QByteArray  fdata(255, 0);

        if(commandOutput.length() < 1) commandOutput = " ";
        for(int i=0; i < 251; i++)
            fdata[i] = (commandOutput.length() > i) ? (commandOutput[i] & 0xff) : 0x00;

        if(commandOutput.length() > 250) {
            commandOutput = commandOutput.remove(0,251);
            fdata[251] = 0x00;
            fdata[252] = 0x01;
        }
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(fdata);
        return;
    }
    case 0x89 : // set list filter
    {
        if (!sio->port()->writeCommandAck()) return;
        QByteArray ddata = sio->port()->readDataFrame(32);
        if (ddata.isEmpty()) {
            sio->port()->writeDataNak();
            sio->port()->writeError();
            fFilter = "*";
            return;
        }
        sio->port()->writeDataAck();
        sio->port()->writeComplete();
        fFilter = ddata;
        qCritical() << "!i" << tr("[%1] List filter set: [%2]").arg(deviceName()).arg(fFilter);
        return;
    }
    case 0x90 : // get list option
    {
        if (!sio->port()->writeCommandAck()) return;
        quint8 cmdpPrm  = (aux  % 256);
        QByteArray  ddata(255, 0);

        if(!files.contains(cmdpPrm)) {
            ddata[0] = '@';
        } else {
            imageFileName = files.value(cmdpPrm);
            if(imageFileName.startsWith("+")) {
                imageFileName = imageFileName.mid(2, imageFileName.length()-3).trimmed();
                if(imageFileName == "home") fPath = "";
                else if(imageFileName == "up" ) fPath = fPath.left(fPath.lastIndexOf("/"));
                else fPath = fPath + "/"+ imageFileName;

                ddata[0] = '$';
                qCritical() << "!i" << tr("[%1] Set Path: [%2]").arg(deviceName()).arg(fPath);
            } else {
                ddata[0] = char(cmdpPrm);
            }
        }
        ddata[1] = (char)155;
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(ddata);
        return;
    }
    case 0x91 : // list folder
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray  ddata(255, 0);
        QString pth = respeqtSettings->lastRclDir() + fPath;
        if(pth.trimmed().isEmpty()) {
            QByteArray fn = QString("Home not set in Options>Emulation").toUtf8();
            for(int i=0; i < 253; i++)
                ddata[i] = (fn.length() > i) ? (fn[i] & 0xff) : 0x00;
            ddata[252] =  0x41; ddata[253] =  (1) / 256; ddata[254] =  (1) % 256;
            qCritical() << "!e" << tr("** AspeQT home folder not set - Goto Tools>Options>Emulation");
            sio->port()->writeComplete();
            sio->port()->writeDataFrame(ddata);
            return;
        }

        QDir dir(pth);
        QStringList filters;
        if(fFilter == "*" || fFilter == "") { filters << "*.*"; filters << "*"; }
        else { filters <<fFilter+"*.*"; filters <<fFilter+"*"; }

        dir.setNameFilters(filters);
        QFileInfoList list = dir.entryInfoList();
        quint8 index  = 0;
        ddata[index++] = (char)155;
        ddata[252] =  0; ddata[253] =  0; ddata[254] =  0;
        files.clear();

        for (quint16 i = aux; i < list.size() && i < 0xFFFA && i-aux < 0x10;  ++i) {
            QFileInfo fileInfo = list.at(i);
            QString dosfilname;
            fileInfo.isDir()? dosfilname = "+ " +fileInfo.fileName().trimmed(): dosfilname = fileInfo.fileName().trimmed();

            if(fileInfo.fileName().trimmed() == "." )  dosfilname = "+[home]";
            else if(fileInfo.fileName().trimmed() == ".." )  dosfilname = "+[up]";
            else if(fileInfo.isDir())   dosfilname = "+[" +fileInfo.fileName().trimmed()+"]";
            else dosfilname = fileInfo.fileName().trimmed();

            quint8 fileNum = i-aux+0x41;
            files.insert(fileNum, dosfilname);

            QString atariFilenum = QString(QChar::fromLatin1(fileNum));
            QString atariFileDsc = dosfilname.left(33);

            QByteArray fn  = (" "+atariFilenum+" "+atariFileDsc).toUtf8();
            if(index + fn.length() < 250) {
                for(int n = 0; n < fn.length(); n++)
                    ddata[index++] = fn[n] & 0xff;
                ddata[index++] = (char)155;
            } else  {
                break;
            }
            if(index > 0 ) ddata[252] =  0x41 + (i - aux);
            ddata[253] =  (i + 1) / 256;
            ddata[254] =  (i + 1) % 256;
        }

        for(int n = index; n < 252 ; n++) ddata[index++] = 0x00;
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(ddata);
        return;
    }
    case 0x92 : // get slots filename
    {
        if (!sio->port()->writeCommandAck()) return;
        QByteArray  fdata(31, 0);
        qint8 deviceNo = (aux  % 256);
        deviceNo = (deviceNo > 9) ? (deviceNo -16) :deviceNo;

        if (deviceNo >= 0x0 && deviceNo <= 15 ) {
            SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(deviceNo -1 +  DISK_BASE_CDEVIC));
            QString  filename = "";
            if (img) {
                int i = -1;
                i = img->originalFileName().lastIndexOf("/");
                if ((i != -1) || (img->originalFileName().mid(0, 14) == "Untitled image")) {
                    QString dosfilname =  img->originalFileName().right(img->originalFileName().size() - ++i);
                    filename = dosfilname.left(35);
                }
            }
            QByteArray fn = filename.toUtf8();
            for(int i=0; i < 32; i++)
                fdata[i] = (fn.length() > i) ? (fn[i] & 0xff) : 0x00;

            sio->port()->writeComplete();
            sio->port()->writeDataFrame(fdata);
            return;
        }
        sio->port()->writeDataNak();
    }
    break;

    case 0x93 : // Send Date/Time
    {
        if (!sio->port()->writeCommandAck()) return;
        QDateTime   dateTime = QDateTime::currentDateTime();
        QByteArray  data(5, 0);

        data[0] = dateTime.date().day(); data[1] = dateTime.date().month();
        data[2] = dateTime.date().year() % 100;
        data[3] = dateTime.time().hour(); data[4] = dateTime.time().minute();
        data[5] = dateTime.time().second();
        qDebug() << "!n" << tr("[%1] Date/time sent to client (%2).").arg(deviceName()).arg(QLocale::system().toString(dateTime, QLocale::ShortFormat));
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(data);
    }
    break;

    case 0x94 : // Swap Disks
    {
        if (!sio->port()->writeCommandAck()) return;
        qint8 swapDisk1 = aux /256 - 1;
        qint8 swapDisk2 = aux % 256 - 1;
        if (swapDisk1 > 9) swapDisk1 -= 16;
        if (swapDisk2 > 9) swapDisk2 -= 16;
        if (swapDisk1 >= 0 and swapDisk1 < 15 and swapDisk2 >=0 and swapDisk2 < 15 and swapDisk1 != swapDisk2) {
            sio->swapDevices(swapDisk1 + DISK_BASE_CDEVIC, swapDisk2 + DISK_BASE_CDEVIC);
            respeqtSettings->swapImages(swapDisk1, swapDisk2);
            qDebug() << "!n" << tr("[%1] Swapped disk %2 with disk %3.").arg(deviceName()).arg(swapDisk2 + 1).arg(swapDisk1 + 1);
        } else {
            sio->port()->writeCommandNak();
        }
        sio->port()->writeComplete();
    }
    break;

    case 0x95 : // Unmount Disk(s)
    {
        if (!sio->port()->writeCommandAck()) return;
        qint8 unmountDisk = aux /256;
        if (unmountDisk == -6) unmountDisk = 0;
        if (unmountDisk > 9)   unmountDisk -= 16;
        if (unmountDisk >= 0 and unmountDisk <= 15) {
            if (unmountDisk == 0) {
                // Eject All
                for (int i = 0; i <= 14; i++) {
                    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
                    if (img && img->isModified() && !img->isUnnamed()) img->save();
                }
                for (int i = 14; i >= 0; i--) {
                    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
                    sio->uninstallDevice(i + DISK_BASE_CDEVIC);
                    delete img;
                    respeqtSettings->unmountImage(i);
                }
                qDebug() << "!n" << tr("[%1] ALL images were remotely unmounted").arg(deviceName());
            } else {
                // Single Eject
                SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(unmountDisk - 1 + DISK_BASE_CDEVIC));
                if (img && img->isModified() && !img->isUnnamed()) img->save();
                sio->uninstallDevice(unmountDisk - 1 + DISK_BASE_CDEVIC);
                delete img;
                respeqtSettings->unmountImage(unmountDisk - 1);
                qDebug() << "!n" << tr("[%1] Remotely unmounted disk %2").arg(deviceName()).arg(unmountDisk);
            }
        } else {
            sio->port()->writeCommandNak();
        }
        sio->port()->writeComplete();
    }
    break;

    case 0x96 :   // Mount Disk Image
    case 0x97 :   // Create and Mount a new Disk Image
    {
        if (!sio->port()->writeCommandAck()) return;
        if(respeqtSettings->lastRclDir() == "") {
            qCritical() << "!e" << tr("[%1] AspeQt can't determine the folder where the image file must be created/mounted!").arg(deviceName());
            sio->port()->writeDataNak();
            sio->port()->writeError();
            return;
        }
        int len = (command == 0x96) ? 12 : 14;
        QByteArray data = sio->port()->readDataFrame(len);
        if (data.isEmpty()) {
            sio->port()->writeDataNak();
            sio->port()->writeError();
            return;
        }

        imageFileName = data;

        if (command == 0x97) { // Create new image
            int i, type;
            bool ok;
            i = imageFileName.lastIndexOf(".");
            type = imageFileName.mid(i+1).toInt(&ok, 10);
            if (ok && (type < 1 || type > 6)) ok = false;
            if(!ok) {
                sio->port()->writeDataNak();
                sio->port()->writeError();
                return;
            }
            imageFileName = imageFileName.left(i);
            QFile file(respeqtSettings->lastRclDir() + "/" + imageFileName);
            if (!file.open(QIODevice::WriteOnly)) {
                sio->port()->writeDataNak();
                sio->port()->writeError();
                return;
            }
            sio->port()->writeDataAck();

            int fileSize;
            QByteArray fileData;
            switch (type){
            case 1 : fileSize = 92160; fileData.resize(fileSize+16); fileData.fill(0); fileData[2]=0x80; fileData[3]=0x16; fileData[4]=0x80; break;
            case 2 : fileSize = 133120; fileData.resize(fileSize+16); fileData.fill(0); fileData[2]=0x80; fileData[3]=0x20; fileData[4]=0x80; break;
            case 3 : fileSize = 183936; fileData.resize(fileSize+16); fileData.fill(0); fileData[2]=0xE8; fileData[3]=0x2C; fileData[4]=0x00; fileData[5]=0x01; break;
            case 4 : fileSize = 368256; fileData.resize(fileSize+16); fileData.fill(0); fileData[2]=0xE8; fileData[3]=0x59; fileData[4]=0x00; fileData[5]=0x01; break;
            case 5 : fileSize = 16776576; fileData.resize(fileSize+16); fileData.fill(0); fileData[2]=0xD8; fileData[3]=0xFF; fileData[4]=0x00; fileData[5]=0x01; fileData[6]=0x0F; break;
            case 6 : fileSize = 33553576; fileData.resize(fileSize+16); fileData.fill(0); fileData[2]=0xE0; fileData[3]=0xFF; fileData[4]=0x00; fileData[5]=0x02; fileData[6]=0x1F; break;
            }
            fileData[0] = 0x96; fileData[1] = 0x02;
            file.write(fileData);
            file.close();
        }

        qint8 mountDisk = aux % 256 - 1;
        if (mountDisk > 9) mountDisk -= 16;
        if (mountDisk != -7 && (mountDisk <0 || mountDisk > 14)) {
            sio->port()->writeCommandNak();
            return;
        }

        sio->port()->writeDataAck();
        sio->port()->writeComplete();
        imageFileName = "*" + imageFileName;
        emit mountFile(mountDisk,imageFileName);
    }
    break;

    case 0x98 : // Auto-Commit toggle
    {
        if (!sio->port()->writeCommandAck()) return;
        qint8 commitDisk = aux % 256 - 1;
        bool commitOnOff = (aux/256)?false:true;
        if (commitDisk > 9) commitDisk -= 16;
        if (commitDisk != -7 && (commitDisk <0 || commitDisk > 14)) {
            sio->port()->writeCommandNak();
            return;
        }
        if (commitDisk == -7) {
            for (int i = 0; i < 15; i++) emit toggleAutoCommit(i, commitOnOff);
        } else {
            emit toggleAutoCommit(commitDisk, commitOnOff);
        }
        sio->port()->writeComplete();
    }
    break;

    case 0x99 : // save disks
    {
        if (!sio->port()->writeCommandAck()) return;
        int diskSaved = 0;
        qint8 deviceNo = aux /256;
        if (deviceNo == -6) deviceNo = 0;
        if (deviceNo > 9) deviceNo -= 16;
        if (deviceNo >= 0 and deviceNo <= 15) {
            if (deviceNo == 0) {
                // Save All
                for (int i = 0; i < 15; i++) {
                    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
                    if (img && img->isModified() && !img->isUnnamed()) {
                        img->save();
                        diskSaved++;
                    }
                }
            } else {
                // Save Single
                SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(deviceNo - 1 + DISK_BASE_CDEVIC));
                if (img && img->isModified() && !img->isUnnamed()) {
                    img->save();
                    diskSaved++;
                }
            }
            if (!diskSaved) sio->port()->writeCommandNak();
        } else {
            sio->port()->writeCommandNak();
        }
        sio->port()->writeComplete();
    }
    break;

    case 0x9A : // Mount slot and boot
    {
        if (!sio->port()->writeCommandAck()) return;
        qint8 mountDisk = aux % 256 - 1;
        if (mountDisk > 9) mountDisk -= 16;
        if (mountDisk != -7 && (mountDisk <0 || mountDisk > 14)) mountDisk = 0;

        if(respeqtSettings->lastRclDir() == "") {
            sio->port()->writeDataNak();
            sio->port()->writeError();
            return;
        }

        QByteArray data = sio->port()->readDataFrame(12);
        if (data.isEmpty()) {
            sio->port()->writeDataNak();
            sio->port()->writeError();
            return;
        }

        sio->port()->writeDataAck();
        sio->port()->writeComplete();

        quint8 fileNum = (aux / 256);
        if(fileNum > 40 && files.contains(fileNum))
            imageFileName = fPath +"/"+ files.value(fileNum);
        else
            imageFileName = data;

        imageFileName = imageFileName.trimmed();
        bool isCasTmage = (imageFileName.toLower().endsWith("cas")) ? true: false;
        bool isExeTmage = (imageFileName.toLower().endsWith("xex") || imageFileName.toLower().endsWith("exe") || imageFileName.toLower().endsWith("com")) ? true: false;

        if (isCasTmage) {
            emit bootCas(imageFileName);
        } else if(isExeTmage) {
            emit bootExe(imageFileName);
        } else {
            imageFileName = "*" + imageFileName;
            emit mountFile(mountDisk,imageFileName);
        }
    }
    break;

    case 0x9B : // toggle printer
    {
        if (!sio->port()->writeCommandAck()) return;
        bool enable = (aux/256)?true:false;
        sio->port()->writeComplete();
        emit togglePrinterServer(enable);
    }
    break;

    case 0x9C : // get RCL path
    {
        if (!sio->port()->writeCommandAck()) return;
        QByteArray  fdata(255, 0);
        QString pth = respeqtSettings->lastRclDir() + fPath;
        QByteArray fn = pth.toUtf8();
        if(fn.length() < 5) {
            qCritical() << "!e" << tr("** AspeQT home folder not set - Goto Tools>Options>Emulation");
            fn = QString("Home not set in Options >Emulation").toUtf8();
        }
        for(int i=0; i < 253; i++)
            fdata[i] = (fn.length() > i) ? (fn[i] & 0xff) : 0x00;
        qCritical() << "!i" << tr(" Get Path: [%1]") .arg(pth);
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(fdata);
        return;
    }
    break;

    default :
        sio->port()->writeCommandNak();
        qWarning() << "!e" << tr("[%1] command: $%2, aux: $%3 NAKed.").arg(deviceName()).arg(command, 2, 16, QChar('0')).arg(aux, 4, 16, QChar('0'));
    }
}

void Mnu::gotNewSlot(int slot)
{
    g_rclSlotNo = slot;
    emit mountFile(slot, imageFileName);
}

void Mnu::fileMounted(bool mounted)
{
    if (mounted) {
        sio->port()->writeComplete();
        qDebug() << "!n" << tr("[%1] Image %2 mounted").arg(deviceName()).arg(imageFileName.mid(1,imageFileName.size()-1));
    } else {
        sio->port()->writeDataNak();
    }
}
