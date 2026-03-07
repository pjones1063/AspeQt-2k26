/*
 * sioworker.cpp
 */

#include "sioworker.h"

#include "aspeqtsettings.h"
#include "rdevice.h"
#include <QFile>
#include <QDateTime>
#include <QtDebug>
#include <QCoreApplication>

/* SioDevice */
SioDevice::SioDevice(SioWorker *worker)
        :QObject()
{
    sio = worker;
    m_deviceNo = -1;
}

SioDevice::~SioDevice()
{
    if (m_deviceNo != -1) {
        sio->uninstallDevice(m_deviceNo);
    }
}

QString SioDevice::deviceName()
{
    return sio->deviceName(m_deviceNo);
}

/* SioWorker */

SioWorker::SioWorker()
        : QThread()
{
    // Use the dedicated class for recursive mutexes
    deviceMutex = new QRecursiveMutex();
    for (int i=0; i <= 255; i++) {
        devices[i] = 0;
    }
    mPort = 0;
}

SioWorker::~SioWorker()
{
    for (int i=0; i <= 255; i++) {
        if (devices[i]) {
            delete devices[i];
        }
    }
    delete deviceMutex;
}

bool SioWorker::wait(unsigned long time)
{
    mustTerminate = true;

    if (mPort) {
        mPort->cancel();
    }

    bool result = QThread::wait(time);

    if (mPort) {
        delete mPort;
        mPort = 0;
    }

    return result;
}

void SioWorker::start(Priority p)
{
    switch (aspeqtSettings->backend()) {
        case SERIAL_BACKEND_STANDARD:
            mPort = new StandardSerialPortBackend(0);
            break;
        case SERIAL_BACKEND_SIO_DRIVER:
            mPort = new AtariSioBackend(0);
            break;
    }

    mPort->setTraceEnabled(m_traceEnabled);
    connect(mPort, &AbstractSerialPortBackend::sioTrace, this, &SioWorker::sioTrace);

    QByteArray data;
    for (int i=0; i <= 255; i++)
    {
        if(devices[i])
        {
            data.append(i);
        }
    }
    mPort->setActiveSioDevices(data);

    mustTerminate = false;
    QThread::start(p);
}


void SioWorker::run()
{
    // Connect status signal (Existing)
    connect(mPort, SIGNAL(statusChanged(QString)), this, SIGNAL(statusChanged(QString)));
    /* Open serial port */
    if (!mPort->open()) {
        return;
    }
#ifdef HAS_LIBGPIOD
    if (aspeqtSettings->serialPortHardwareUart()){
        initHardwareInterrupts();
    }
#endif
    /* Process SIO commands until we're explicitly stopped */
    while (!mustTerminate) {

            // ====================================================================
            // NEW: Stream Mode Handler (High Priority)
            // ====================================================================
#ifdef HAS_LIBGPIOD
        if (aspeqtSettings->serialPortHardwareUart()){
            checkHardwareInterrupts();
        }
#endif
        if (m_isStreaming) {
            deviceMutex->lock();
            SioDevice *device = devices[0x50]; // Grab R: Device
            RDevice *rdev = qobject_cast<RDevice*>(device);

            // <--- CHANGED: DRAIN NETWORK BUFFER -> ATARI --->
            if (rdev) {
                QByteArray txData = rdev->dequeueNetworkData();
                if (!txData.isEmpty()) {
                    if (m_traceEnabled) emit sioTrace("TX (STRM)", txData);
                    mPort->writeRawFrame(txData);
                    emit txActivity();
                }
            }
            deviceMutex->unlock();

            // <--- CHANGED: READ UART -> NETWORK --->
            QByteArray rawData = mPort->readRawFrame(128, false);

            if (!rawData.isEmpty() && rdev) {
                // NEW: Trace Stream RX Data
                if (m_traceEnabled) emit sioTrace("RX (STRM)", rawData);
                deviceMutex->lock();
                rdev->processSerialData(rawData);
                deviceMutex->unlock();
                emit rxActivity();
            }
            usleep(100);
            continue;
        }

        QByteArray cmd = mPort->readCommandFrame();
        if (mustTerminate) {
            break;
        }
        if (cmd.isEmpty()) {
            qCritical() << "!e" << tr("Cannot read command frame.");
            break;
        }
        emit rxActivity();
        /* Decode the command */
        quint8 no = (quint8)cmd[0];
        quint8 command = (quint8)cmd[1];
        quint16 aux = ((quint8)cmd[2]) + ((quint8)cmd[3] * 256);

        /* Redirect the command to the appropriate device */
        deviceMutex->lock();
        if (devices[no]) {
            if (devices[no]->tryLock()) {
                devices[no]->handleCommand(command, aux);
                devices[no]->unlock();
                emit txActivity();
            } else {
                qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 ignored because the image explorer is open.")
                .arg(deviceName(no))
                    .arg(command, 2, 16, QChar('0'))
                    .arg(aux, 4, 16, QChar('0'));
            }
        } else {
            qDebug() << "!u" << tr("[%1] command: $%2, aux: $%3 ignored.")
            .arg(deviceName(no))
                .arg(command, 2, 16, QChar('0'))
                .arg(aux, 4, 16, QChar('0'));
        }
        deviceMutex->unlock();
        cmd.clear();
    }

    setHighSpeed(false);
    mPort->close();
}



void SioWorker::setHighSpeed(bool enabled)
{
    if (!mPort) return;

    if (enabled) {
        // Happy Warp Speed is approx 52,631 bps
        mPort->setSpeed(52631);
    } else {
        // Standard Atari SIO speed
        mPort->setSpeed(19200);
    }
}

void SioWorker::installDevice(quint8 no, SioDevice *device)
{
    deviceMutex->lock();
    if (devices[no]) {
        delete devices[no];
    }
    devices[no] = device;
    device->setDeviceNo(no);
    deviceMutex->unlock();
    if(mPort)
    {
        QByteArray data;
        for (int i=0; i <= 255; i++)
        {
            if(devices[i])
            {
                data.append(i);
            }
        }
        mPort->setActiveSioDevices(data);
    }
}

void SioWorker::uninstallDevice(quint8 no)
{
    deviceMutex->lock();
    if (devices[no]) {
        devices[no]->setDeviceNo(-1);
    }
    devices[no] = 0;
    deviceMutex->unlock();
    if(mPort)
    {
        QByteArray data;
        for (int i=0; i <= 255; i++)
        {
            if(devices[i])
            {
                data.append(i);
            }
        }
        mPort->setActiveSioDevices(data);
    }
}

void SioWorker::swapDevices(quint8 d1, quint8 d2)
{
    SioDevice *t1, *t2;

    deviceMutex->lock();
    t1 = devices[d1];
    t2 = devices[d2];
    uninstallDevice(d1);
    uninstallDevice(d2);
    if (t2) {
        installDevice(d1, t2);
    }
    if (t1) {
        installDevice(d2, t1);
    }
    deviceMutex->unlock();
}

SioDevice* SioWorker::getDevice(quint8 no)
{
    SioDevice *result;
    deviceMutex->lock();
    result = devices[no];
    deviceMutex->unlock();
    return result;
}

void SioWorker::setHappyMode(int deviceId, bool enabled)
{
    // Adjust deviceId if it's passed as 0-indexed (0 for D1:)
    int index = deviceId - DISK_BASE_CDEVIC;

    deviceMutex->lock();
    if (index >= 0 && index < DISK_COUNT) {
        happyMode[index] = enabled;
    }
    deviceMutex->unlock();
}



QString SioWorker::deviceName(int device)
{
    QString result;
    switch (device) {
        case -1:
            // It must be because of the piggy-backed autoboot
            result = tr("Disk 1 (below autoboot)");
            break;
        case DISK_BASE_CDEVIC+0x0:
        case DISK_BASE_CDEVIC+0x1:
        case DISK_BASE_CDEVIC+0x2:
        case DISK_BASE_CDEVIC+0x3:
        case DISK_BASE_CDEVIC+0x4:
        case DISK_BASE_CDEVIC+0x5:
        case DISK_BASE_CDEVIC+0x6:
        case DISK_BASE_CDEVIC+0x7:
        case DISK_BASE_CDEVIC+0x8:
        case DISK_BASE_CDEVIC+0x9:
        case DISK_BASE_CDEVIC+0xA:
        case DISK_BASE_CDEVIC+0xB:
        case DISK_BASE_CDEVIC+0xC:
        case DISK_BASE_CDEVIC+0xD:
        case DISK_BASE_CDEVIC+0xE:
            result = tr("Disk %1").arg(device & 0x0F);
            break;
        case PRINTER_BASE_CDEVIC+0:
        case PRINTER_BASE_CDEVIC+1:
        case PRINTER_BASE_CDEVIC+2:
        case PRINTER_BASE_CDEVIC+3:
            result = tr("Printer %1").arg((device & 0x0F) + 1);
            break;
        case SMART_CDEVIC:
            result = tr("Smart device (APE time + URL)");
            break;
        case ASPEQT_CLIENT_CDEVIC:
            result = tr("AspeQt Client");
            break;
        case RS232_BASE_CDEVIC+0:
        case RS232_BASE_CDEVIC+1:
        case RS232_BASE_CDEVIC+2:
        case RS232_BASE_CDEVIC+3:
            result = tr("RS232 %1").arg((device & 0x0F) +1);
            break;
        case PCLINK_CDEVIC:
            result = tr("PCLINK");
            break;
        default:
            result = tr("Device $%1").arg(device, 2, 16, QChar('0'));
            break;
    }
    return result;
}

/* CassetteWorker */

CassetteWorker::CassetteWorker()
    : QThread()
{
    mPort = 0;
    mustTerminate.lock();
}

CassetteWorker::~CassetteWorker()
{
}

bool CassetteWorker::loadCasImage(const QString &fileName)
{
    mRecords.clear();

    QFile casFile(fileName);

    if (!casFile.open(QFile::ReadOnly)) {
        qCritical() << "!e" << tr("Cannot open '%1': %2").arg(fileName).arg(casFile.errorString());
        return false;
    }

    QByteArray header, data;
    uint magic;
    int length, aux;

    header = casFile.read(8);

    if (header.length() != 8) {
        qCritical() << "!e" << tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
        return false;
    }

    magic = (quint8)header.at(0) + (quint8)header.at(1) * 256 + (quint8)header.at(2) * 65536 + (quint8)header.at(3) * 16777216;
    length = (quint8)header.at(4) + (quint8)header.at(5) * 256;
    aux = (quint8)header.at(6) + (quint8)header.at(7) * 256;


    data = casFile.read(length);
    if (data.length() != length) {
        qCritical() << "!e" << tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
        return false;
    }

    /* Verify the header */
    if (magic != 0x494a5546) { // "FUJI"
        qCritical() << "!e" << tr("Cannot open '%1': The header does not match.").arg(fileName);
        return false;
    }

    if (!data.isEmpty()) {
        qDebug() << "!n" << tr("[Cassette]: File description '%2'.").arg(QString::fromLatin1(data));
    }

    int lastBaud = 600;
    mTotalDuration = 0;

    /* Read the cas file */
    do {
        header = casFile.read(8);

        if (header.length() != 8) {
            qCritical() << "!e" << tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
            return false;
        }

        magic = (quint8)header.at(0) + (quint8)header.at(1) * 256 + (quint8)header.at(2) * 65536 + (quint8)header.at(3) * 16777216;
        length = (quint8)header.at(4) + (quint8)header.at(5) * 256;
        aux = (quint8)header.at(6) + (quint8)header.at(7) * 256;

        data = casFile.read(length);
        if (data.length() != length) {
            qCritical() << "!e" << tr("Cannot read '%1': %2").arg(fileName).arg(casFile.errorString());
            return false;
        }

        /* Verify the header */
        if (magic == 0x64756162) {          // "baud"
            if (aspeqtSettings->useCustomCasBaud()) {
                lastBaud = aspeqtSettings->customCasBaud();
            } else {
                lastBaud = aux;
            }
        } else if (magic == 0x61746164) {   // "data"
            CassetteRecord record;
            record.baudRate = lastBaud;
            record.data = data;
            record.gapDuration = aux;
            record.totalDuration = aux + (length * 10000 + (lastBaud/2))/lastBaud;
            mTotalDuration += record.totalDuration;
            mRecords.append(record);
        } else {
            qCritical() << "!e" << tr("Cannot open '%1': Unknown chunk header %2.").arg(fileName).arg(magic);
            return false;
        }

    } while (!casFile.atEnd());

    return true;
}

bool CassetteWorker::wait (unsigned long time)
{
    if (mPort) {
        mPort->cancel();
    }

    mustTerminate.unlock();

    bool result = QThread::wait(time);

    if (mPort) {
        mPort->close();
        delete mPort;
        mPort = 0;
    }

    return result;
}

void CassetteWorker::run()
{
    /* Open serial port */
    if (!mPort->open()) {
        return;
    }

    int lastBaud = 0;
    int block = 1;
    int remainingTime = mTotalDuration;

    QTime tm = QTime::currentTime();

    foreach (CassetteRecord record, mRecords) {
        if (lastBaud != record.baudRate) {
            lastBaud = record.baudRate;
            if (!mPort->setSpeed(lastBaud)) {
                return;
            }
        }
        emit statusChanged(remainingTime);
        qDebug() << "!n" << tr("[Cassette] Playing record %1 of %2 (%3 ms of gap + %4 bytes of data)")
                .arg(block)
                .arg(mRecords.count())
                .arg(record.gapDuration)
                .arg(record.data.length());
        tm = tm.addMSecs(record.gapDuration);
        int w = QTime::currentTime().msecsTo(tm);
        if (w < 0) {
            w = 0;
        }
        if (mustTerminate.tryLock(w)) {
            return;
        }
        tm = QTime::currentTime();
        tm = tm.addMSecs((record.data.length() * 10000 + (lastBaud/2))/lastBaud);
        for (int i=0; i < record.data.length(); i+=10) {
            mPort->writeRawFrame(record.data.mid(i, 10));
            if (mustTerminate.tryLock()) {
                return;
            }
        }
        block++;
        remainingTime -= record.totalDuration;
    }
    // Wait until last written bytes are transferred and then some (FTDI bug)
    int w = QTime::currentTime().msecsTo(tm);
    if (w < 0) {
        w = 0;
    }
    msleep(w + 500);
    mPort->close();
}

void CassetteWorker::start(Priority p)
{
    switch (aspeqtSettings->backend()) {
        case SERIAL_BACKEND_STANDARD:
            mPort = new StandardSerialPortBackend(0);
            break;
        case SERIAL_BACKEND_SIO_DRIVER:
            mPort = new AtariSioBackend(0);
            break;
    }
    QThread::start(p);
}


void SioWorker::onChangeBaudRate(int baudRate)
{
    if (port()) {
        qDebug() << "[SioWorker] Changing Baud Rate to:" << baudRate;

        if (baudRate != 19200) {
            port()->setSpeed(baudRate);
        }

#ifdef HAS_LIBGPIOD
        // <--- ADDED: PURGE THE STALE EVENTS --->
        // Read any pending edges left over from the Command Block
        if (m_gpioRequest) {
            ::gpiod::edge_event_buffer purge_buffer(16);
            int purge_ret = poll(&m_gpioPollFd, 1, 0);
            if (purge_ret > 0) {
                m_gpioRequest->read_edge_events(purge_buffer);
                qDebug() << "!d" << "[SioWorker] Purged stale COMMAND edges from kernel queue.";
            }
        }
        m_streamGuardTimer.start();
#endif

        port()->setStreamMode(true);
        m_isStreaming = true;
    }
}



void SioWorker::onWriteRawData(const QByteArray &data)
{
    if (port() && m_isStreaming) {
        // Direct write without adding SIO checksums
        port()->writeRawFrame(data);
    }
}

void SioWorker::onStreamFinished()
{
    qDebug() << "[SioWorker] Stream Mode Finished. Restoring standard SIO.";
    m_isStreaming = false;

    if (port()) {
        // [CRITICAL FIX] Restore strict blocking rules for Disk Drives
        port()->setStreamMode(false);
    }

    restoreStandardBaudRate();
}


void SioWorker::restoreStandardBaudRate()
{
    if (port()) {
        // Standard SIO is usually 19200
        port()->setSpeed(19200);
    }
}


// ==========================================================================
// RASPBERRY PI 5: TRUE HARDWARE INTERRUPTS (SIO COMMAND PIN 7)
// ==========================================================================
#ifdef HAS_LIBGPIOD

void SioWorker::initHardwareInterrupts()
{
    try {
        // --- FIX 1: Prevent "Device or resource busy" ---
        // Release any existing lock on the GPIO pin before claiming it again
        if (m_gpioRequest) {
            m_gpioRequest.reset();
        }

        const std::string chip_path = "/dev/gpiochip4";
        const ::gpiod::line::offset line_offset = 18; // GPIO 18 (Physical Pin 12)

        m_gpioRequest = std::make_unique<::gpiod::line_request>(
            ::gpiod::chip(chip_path)
                .prepare_request()
                .set_consumer("AspeQt_Command_Watcher")
                .add_line_settings(
                    line_offset,
                    ::gpiod::line_settings()
                        .set_direction(::gpiod::line::direction::INPUT)
                        .set_edge_detection(::gpiod::line::edge::FALLING)
                        .set_bias(::gpiod::line::bias::PULL_UP)
                    )
                .do_request()
            );

        m_gpioPollFd.fd = m_gpioRequest->fd();
        m_gpioPollFd.events = POLLIN;

        qDebug() << "!i" << "[SioWorker] Hardware interrupts enabled on Pi 5 GPIO 18 (Physical Pin 12).";
    } catch (const std::exception& e) {
        qCritical() << "!e" << "[SioWorker] Failed to setup Pi 5 hardware interrupts:" << e.what();
        m_gpioRequest.reset();
    }
}


void SioWorker::checkHardwareInterrupts()
{
    if (!m_gpioRequest) return;

    int ret = poll(&m_gpioPollFd, 1, 0);

    if (ret > 0) {
        ::gpiod::edge_event_buffer buffer(16);
        m_gpioRequest->read_edge_events(buffer);

        // REMOVED: m_streamGuardTimer check (Blinded the emulator to instant retries)

        for (const auto& event : buffer) {
            if (event.type() == ::gpiod::edge_event::event_type::FALLING_EDGE) {

                if (m_isStreaming) {
                    // FIX: Atari command blocks at 19200 baud hold the line low for ~2.6ms.
                    // A 20ms debounce will ALWAYS fail to catch it.
                    // We use a light 100 microsecond debounce just to filter EMI noise.
                    int lowCount = 0;
                    for (int i = 0; i < 5; i++) {
                        usleep(20); // 20 microseconds
                        if (m_gpioRequest->get_values()[0] == ::gpiod::line::value::INACTIVE) {
                            lowCount++;
                        } else {
                            break; // Noise spike bounced high. Ignore.
                        }
                    }

                    if (lowCount == 5) {
                        qDebug() << "!d" << "[SioWorker] Atari asserted COMMAND line! Exiting Stream.";
                        deviceMutex->lock();

                        SioDevice *rdev = devices[0x50];
                        // Safely cast to RDevice to call forceCommandMode()
                        if (rdev) {
                            RDevice *r = qobject_cast<RDevice*>(rdev);
                            if (r) r->forceCommandMode();
                        }

                        deviceMutex->unlock();
                        break;
                    }
                }
            }
        }
    }
}

#endif
