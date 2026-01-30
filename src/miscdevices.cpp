/*
 * miscdevices.cpp
 * DEBUG VERSION - HEAVY LOGGING ENABLED
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
    int index = m_deviceNo - RS232_BASE_CDEVIC;
    if (index < 0) index = 0;

    int mode = respeqtSettings->rs232Mode(index); 

    // LOG COMMAND ARRIVAL
    qDebug() << "!n" << tr("[DEBUG-R1] Command Arrival: $%1 Aux: $%2 Mode: %3")
                            .arg(command, 2, 16, QChar('0'))
                            .arg(aux, 4, 16, QChar('0'))
                            .arg(mode == 1 ? "Telnet" : "Physical");

    if (mode == 1) {
        if (m_serialPort->isOpen()) m_serialPort->close();
        handleTelnet(command, aux);
    }
    else {
        if (m_tcpSocket->state() != QAbstractSocket::UnconnectedState)
            m_tcpSocket->abort();
        handlePhysical(command, aux);
    }
}

void Rs232::handlePhysical(quint8 command, quint16 aux)
{
    // Keeping Physical logic brief to focus on Telnet debugging
    if (command == 0x53)
    {
        if (!sio->port()->writeCommandAck()) return;
        QByteArray status(4, 0);
        quint8 bits = 0x0F; 
        if (!m_serialPort->isOpen()) bits = 0x3F; // Fake Ready
        status[0] = bits; 
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(status);
        return;
    }
    // ... (Lazy init logic omitted for brevity in this debug paste, standard implementation applies) ...
    sio->port()->writeCommandNak(); // Fallback if used by accident
}

void Rs232::configurePort(quint16 val1, quint16 /*val2*/)
{
    // ... Standard implementation ...
}

// --------------------------------------------------------------------------
// TELNET MODEM LOGIC (DEBUGGED)
// --------------------------------------------------------------------------
void Rs232::handleTelnet(quint8 command, quint16 aux)
{
    switch(command)
    {
    case 0x26: // Download Handler (The Bootloader)
    case 0x3C: // Alternative Boot Command
    {
        qDebug() << "[R:] Booting Driver to Atari...";
        if (!sio->port()->writeCommandAck()) return;

        // [CRITICAL] You must insert your compiled 6502 binary here.
        // This is just a placeholder example.
        QByteArray handlerBinary;
        handlerBinary.append((char)0x00); // ... Insert 500+ bytes of 6502 code

        // Send the driver payload
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(handlerBinary);
        break;
    }

    case 0x20: // Start Concurrent Mode (Stream)
        qDebug() << "[R:] Entering Concurrent Mode...";
        if (!sio->port()->writeCommandAck()) return;
        sio->port()->writeComplete();

        // This function will BLOCK until the mode ends
        enterConcurrentMode();
        break;

    case 0x57: // Write (Packet Mode - Keep your existing logic here)
        // ... (Your existing 0x57 code) ...
        break;

    case 0x52: // Read (Packet Mode - Keep your existing logic here)
        // ... (Your existing 0x52 code) ...
        break;

        // ... Keep Status (0x53) and others ...
    }
}

void Rs232::enterConcurrentMode()
{
    m_isConcurrentMode = true;
    m_escapeBuffer.clear();
    // Use a separate buffer for AT commands while in concurrent mode
    QByteArray lineBuffer;
    m_lastCharTime = QDateTime::currentMSecsSinceEpoch();

    bool useHardwareCheck = (respeqtSettings->serialPortHandshakingMethod() != HANDSHAKE_SOFTWARE
                             && respeqtSettings->serialPortHandshakingMethod() != HANDSHAKE_NO_HANDSHAKE);

    while (m_isConcurrentMode)
    {
        QCoreApplication::processEvents();

        // 1. HARDWARE BREAK CHECK
        if (useHardwareCheck && checkHardwareBreak()) {
            qDebug() << "[R:] Hardware Break. Exiting.";
            m_isConcurrentMode = false;
            break;
        }

        // 2. READ FROM ATARI (User Typing)
         QByteArray dataFromAtari = sio->port()->readAll();   ** TO FIX 1 **

        if (!dataFromAtari.isEmpty()) {
            for (char c : dataFromAtari) {

                // A. Check for Escape Sequence (+++)
                if (checkForEscapeSequence(c)) {
                    m_isConcurrentMode = false;
                    sendToAtari("\r\nOK\r\n");
                    break;
                }

                // B. Logic: Are we Connected or Offline?
                if (m_isTcpConnected) {
                    // ONLINE: Send keystrokes to the BBS
                    m_tcpSocket->write(&c, 1);
                }
                else {
                    // OFFLINE: Buffer keystrokes to parse "AT" commands locally
                    // Echo back to user (Half Duplex/Local Echo) if needed
                    // sendToAtari(QString(c));

                    if (c == '\r' || c == (char)155) { // Return key hit
                        QString cmd = QString::fromLatin1(lineBuffer).trimmed();
                        lineBuffer.clear();

                        // Parse the Command (Reuse your existing processAtCommand)
                        // This handles the "ATDT bbs.com" logic!
                        processAtCommand(cmd);
                    }
                    else if (c == 0x7F || c == 0x08) { // Backspace
                        if (!lineBuffer.isEmpty()) lineBuffer.chop(1);
                    }
                    else {
                        lineBuffer.append(c);
                    }
                }
            }
        }

        // 3. READ FROM INTERNET (BBS Data)
        // Only valid if connected
        if (m_isTcpConnected && m_tcpSocket->bytesAvailable()) {
            QByteArray dataFromNet = m_tcpSocket->readAll();
          sio->port()->write(dataFromNet);     ** To Fix **
        }

        // 4. Prevent CPU Hogging
        QThread::msleep(1);
    }
}
bool Rs232::checkForEscapeSequence(char c)
{
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 delta = now - m_lastCharTime;
    m_lastCharTime = now;

    // Guard Time: Silence before the sequence (e.g., 1000ms)
    // If a character comes in too fast, reset buffer.
    if (m_escapeBuffer.isEmpty()) {
        if (c == '+' && delta > 1000) {
            m_escapeBuffer.append(c);
        }
    }
    else {
        // We are in the middle of a sequence
        if (c == '+' && delta < 1000) { // Must be typed relatively quickly
            m_escapeBuffer.append(c);
            if (m_escapeBuffer.size() == 3) {
                // We have "+++". Now we need to wait 1s for the trailing silence.
                // We can cheat here: Return true, but in the main loop,
                // wait 1s before processing any more data.
                return true;
            }
        } else {
            // Invalid char or too slow, reset
            m_escapeBuffer.clear();
        }
    }
    return false;
}

bool Rs232::checkHardwareBreak()
{
    QSerialPort::PinoutSignals pins = sio->port()->pinoutSignals();  ** TO FIX  2 **

    int method = respeqtSettings->serialPortHandshakingMethod();

    // Check logical alignment with Option Constants (check your aspeqtsettings.h)
    if (method == HANDSHAKE_CTS && (pins & QSerialPort::ClearToSendSignal)) return true;
    if (method == HANDSHAKE_DSR && (pins & QSerialPort::DataSetReadySignal)) return true;

    // Note: The logic is usually "Active Low" implies command mode.
    // You may need to invert this depending on your specific SIO2PC wiring.
    // Usually: If Pin is ACTIVE (Command Line Low), we break.

    return false;
}

// --- AT Command Parser ---
void Rs232::processAtCommand(QString cmd)
{
    qDebug() << "[DEBUG-R1] Executing AT Command Logic for: " << cmd;
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
    qDebug() << "[DEBUG-R1] Queuing response to Atari: " << text;
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

