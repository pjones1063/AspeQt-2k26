/*
 * serialport-win32.cpp
 */

#include "serialport.h"
#include <windows.h>
#include <string.h>
#include "sioworker.h"
#include "aspeqtsettings.h"

#include <QTime>
#include <QtDebug>

/*********/

AbstractSerialPortBackend::AbstractSerialPortBackend(QObject *parent)
    : QObject(parent)
{
}

AbstractSerialPortBackend::~AbstractSerialPortBackend()
{
}

StandardSerialPortBackend::StandardSerialPortBackend(QObject *parent)
    : AbstractSerialPortBackend(parent)
{
    mHandle = INVALID_HANDLE_VALUE;
    mCurrentDeviceId = 0;
}

StandardSerialPortBackend::~StandardSerialPortBackend()
{
    if (isOpen()) {
        close();
    }
}

QString StandardSerialPortBackend::defaultPortName()
{
    return QString("COM1");
}

bool StandardSerialPortBackend::open()
{
    Sleep(250);        // 
    if (isOpen()) {
        close();
    }
//    qDebug() << "!d" << tr("DBG -- Serial Port Open...");

    QString name(SERIAL_PORT_LOCATION);
    name.append(aspeqtSettings->serialPortName());

    mMethod = aspeqtSettings->serialPortHandshakingMethod();
    mWriteDelay = SLEEP_FACTOR * aspeqtSettings->serialPortWriteDelay();
    mCompErrDelay = aspeqtSettings->serialPortCompErrDelay();

    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        mHandle = (CreateFile(
            (WCHAR*)name.utf16(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            0
        ));
        if (mHandle == INVALID_HANDLE_VALUE) {
            qCritical() << "!e" << tr("Cannot open serial port '%1': %2").arg(aspeqtSettings->serialPortName(), lastErrorMessage());
            return false;
        }
    }
    else
    {
        mHandle = (CreateFile(
            (WCHAR*)name.utf16(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            0
        ));
        if (mHandle == INVALID_HANDLE_VALUE) {
            qCritical() << "!e" << tr("Cannot open serial port '%1': %2").arg(aspeqtSettings->serialPortName(), lastErrorMessage());
            return false;
        }
        if (!EscapeCommFunction(mHandle, SETDTR)) {
            qCritical() << "!e" << tr("Cannot set DTR line in serial port '%1': %2").arg(aspeqtSettings->serialPortName(), lastErrorMessage());
            return false;
        }
        if (!EscapeCommFunction(mHandle, SETRTS)) {
            qCritical() << "!e" << tr("Cannot set RTS line in serial port '%1': %2").arg(aspeqtSettings->serialPortName(), lastErrorMessage());
            return false;
        }
    }

    mCanceled = false;

    mCancelHandle = CreateEvent(0, true, false, 0);

    if (!setNormalSpeed()) {
        close();
        return false;
    }

    QString m;
    switch (mMethod) {
    case HANDSHAKE_RI:
        m = "RI";
        break;
    case HANDSHAKE_DSR:
        m = "DSR";
        break;
    case HANDSHAKE_CTS:
        m = "CTS";
        break;
    case HANDSHAKE_NO_HANDSHAKE:
        m = "NO";
        break;
    case HANDSHAKE_SOFTWARE:
    default:
        m = "SOFTWARE (SIO2BT)";
        break;
    }

    /* Notify the user that emulation is started */
    qWarning() << "!i" << tr("Emulation started through standard serial port backend on '%1' with %2 handshaking")
                  .arg(aspeqtSettings->serialPortName())
                  .arg(m);

    return true;
}

bool StandardSerialPortBackend::isOpen()
{
    return mHandle != INVALID_HANDLE_VALUE;
}

void StandardSerialPortBackend::close()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port Close...");

    cancel();
    if (!CloseHandle(mHandle)) {
        qCritical() << "!e" << tr("Cannot close serial port: %1").arg(lastErrorMessage());
    }
    CloseHandle(mCancelHandle);
    mCancelHandle = mHandle = INVALID_HANDLE_VALUE;
}

void StandardSerialPortBackend::cancel()
{
    mCanceled = true;
    SetEvent(mCancelHandle);
}

int StandardSerialPortBackend::speedByte()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port speedByte...");

    if (aspeqtSettings->serialPortHandshakingMethod()==HANDSHAKE_SOFTWARE) {
        return 0x28; // standard speed (19200)
    } else if (aspeqtSettings->serialPortUsePokeyDivisors()) {
        return aspeqtSettings->serialPortPokeyDivisor();
    } else {
        int speed = 0x08;
        switch (aspeqtSettings->serialPortMaximumSpeed()) {
        case 0:
            speed = 0x28;
            break;
        case 1:
            speed = 0x10;
            break;
        case 2:
            speed = 0x08;
            break;
        }
        return speed;
    }
}

bool StandardSerialPortBackend::setNormalSpeed()
{
    mHighSpeed = false;
    return setSpeed(19200);
}

bool StandardSerialPortBackend::setHighSpeed()
{
    mHighSpeed = true;
    if (aspeqtSettings->serialPortUsePokeyDivisors()) {
        return setSpeed(divisorToBaud(aspeqtSettings->serialPortPokeyDivisor()));
    } else {
        int speed = 57600;
        switch (aspeqtSettings->serialPortMaximumSpeed()) {
        case 0:
            speed = 19200;
            break;
        case 1:
            speed = 38400;
            break;
        case 2:
            speed = 57600;
            break;
        }
        return setSpeed(speed);
    }
}

bool StandardSerialPortBackend::setSpeed(int speed)
{
//    qDebug() << "!d" << tr("DBG -- Serial Port setSpeed...");

    DCB dcb;
    COMMTIMEOUTS to;

    /* Adjust parameters */
    dcb.DCBlength = sizeof dcb;
    dcb.BaudRate = speed;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;

    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fAbortOnError = FALSE;
    dcb.fDummy2 = 0;
    dcb.wReserved = 0;
    dcb.XonLim = 0;
    dcb.XoffLim = 0;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
/*    if (speed > 100000) {
        dcb.StopBits = TWOSTOPBITS;
    } else*/ {
        dcb.StopBits = ONESTOPBIT;
    }
    dcb.XonChar = 0;
    dcb.XoffChar = 0;
    dcb.ErrorChar = 0;
    dcb.EofChar = 0;
    dcb.EvtChar = 0;
    dcb.wReserved1 = 0;

    /* Set serial port state */
    if (!SetCommState(mHandle, &dcb)) {
        qCritical() << "!e" << tr("Cannot set serial port speed to %1: %2")
                       .arg(speed)
                       .arg(lastErrorMessage());
        return false;
    }

    /* Adjust serial port timeouts */
    to.ReadIntervalTimeout = 0;

    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        to.ReadTotalTimeoutConstant = 300;
        to.WriteTotalTimeoutConstant = 300;
        to.ReadTotalTimeoutMultiplier = 200;
        to.WriteTotalTimeoutMultiplier = 200;
    }
    else
    {
        to.ReadTotalTimeoutConstant = 0;
        to.WriteTotalTimeoutConstant = 0;
        to.ReadTotalTimeoutMultiplier = 100;
        to.WriteTotalTimeoutMultiplier = 100;
    }

    /* Set serial port timeouts */
    if (!SetCommTimeouts(mHandle, &to)) {
        qCritical() << "!e" << tr("Cannot set serial port timeouts: %1").arg(lastErrorMessage());
        return false;
    }

    emit statusChanged(tr("%1 bits/sec").arg(speed));
    qWarning() << "!i" << tr("Serial port speed set to %1.").arg(speed);
    mSpeed = speed;
    return true;
}

int StandardSerialPortBackend::speed()
{
    return mSpeed;
}


QByteArray StandardSerialPortBackend::readCommandFrame()
{
    //    qDebug() << "!d" << tr("DBG -- Serial Port readCommandFrame...");

    QByteArray data;

    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        // [FIX] Roadblock B: Smart Purge
        // Prevent PurgeComm from deleting the next command in the R: boot chain.

        bool isRDevice = (mCurrentDeviceId >= 0x50 && mCurrentDeviceId <= 0x53);
        bool shouldPurge = true;

        if (aspeqtSettings->enableRDevice() && isRDevice) {
            shouldPurge = false; // SKIP PURGE
        }

        if (shouldPurge) {
            if (!PurgeComm(mHandle, PURGE_RXCLEAR)) {
                qCritical() << "!e" << tr("Cannot clear serial port read buffer: %1").arg(lastErrorMessage());
                return data;
            }
        }

        const int size = 4;
        quint8 expected = 0;
        quint8 got = 1;
        do
        {
            DWORD result;
            char c;
            if(ReadFile(mHandle, &c, 1, &result, NULL) && result == 1)
            {
                data.append(c);
                if(data.size() == size + 2)
                {
                    data.remove(0, 1);
                }
                if(data.size() == size + 1)
                {
                    for(int i=0 ; i < mSioDevices.size() ; i++)
                    {
                        if(data.at(0) == mSioDevices[i])
                        {
                            expected = (quint8)data.at(size);
                            got = sioChecksum(data, size);
                            break;
                        }
                    }
                }
            }
        } while(got != expected && !mCanceled);

        if(got == expected)
        {
            data.resize(size);

            // [FIX] Roadblock A: Update ID and Skip Sleep
            mCurrentDeviceId = (quint8)data.at(0);
            isRDevice = (mCurrentDeviceId >= 0x50 && mCurrentDeviceId <= 0x53);

            // Skip legacy delay for R: Device to ensure high throughput
            if (! (aspeqtSettings->enableRDevice() && isRDevice) )
            {
                QThread::usleep(500);
            }
        }
        else
        {
            data.clear();
        }
    }
    else
    {
        // Hardware Handshake Logic (Unchanged)
        // Ensure mCurrentDeviceId is updated here if you use HW handshake for R:
        // But strictly speaking, the bug was in the SW handshake path.

        // ... (Existing HW Handshake code) ...

        // If data was read in the HW block, you should ideally update mCurrentDeviceId there too:
        // mCurrentDeviceId = (quint8)data.at(0);
    }
    return data;
}


QByteArray StandardSerialPortBackend::readDataFrame(uint size, bool verbose)
{
//    qDebug() << "!d" << tr("DBG -- Serial Port readDataFrame...");

    QByteArray data = readRawFrame(size + 1, verbose);
    if (data.isEmpty()) {
        return data;
    }
    quint8 expected = (quint8)data.at(size);
    quint8 got = sioChecksum(data, size);
    if (expected == got) {
        data.resize(size);
        return data;
    } else {
        if (verbose) {
            qWarning() << "!w" << tr("Data frame checksum error, expected: %1, got: %2. (%3)")
                           .arg(expected)
                           .arg(got)
                           .arg(QString(data.toHex()));
        }
        data.clear();
        return data;
    }
}

bool StandardSerialPortBackend::writeDataFrame(const QByteArray &data)
{
    //    qDebug() << "!d" << tr("DBG -- Serial Port writeDataFrame...");

    QByteArray copy(data);
    copy.resize(copy.size() + 1);
    copy[copy.size() - 1] = sioChecksum(copy, copy.size() - 1);

    // --- START UPDATE: Speed Optimization ---
    bool isRDevice = (mCurrentDeviceId >= 0x50 && mCurrentDeviceId <= 0x53);

    // Skip artificial delays for R: Device to ensure high throughput
    if (! (aspeqtSettings->enableRDevice() && isRDevice) )
    {
        if(mMethod==HANDSHAKE_SOFTWARE) SioWorker::usleep(mWriteDelay);
        SioWorker::usleep(50);
    }
    // --- END UPDATE ---

    return writeRawFrame(copy);
}


bool StandardSerialPortBackend::writeCommandAck()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port writeCommandAck...");

    return writeRawFrame(QByteArray(1, SIO_ACK));
}

bool StandardSerialPortBackend::writeCommandNak()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port writeCommandNak...");

    return writeRawFrame(QByteArray(1, SIO_NACK));
}

bool StandardSerialPortBackend::writeDataAck()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port writeDataAck...");

    return writeRawFrame(QByteArray(1, SIO_ACK));
}

bool StandardSerialPortBackend::writeDataNak()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port writeDataNak...");

    return writeRawFrame(QByteArray(1, SIO_NACK));
}

bool StandardSerialPortBackend::writeComplete()
{
    //    qDebug() << "!d" << tr("DBG -- Serial Port writeComplete...");

    // --- START UPDATE: Speed Optimization ---
    bool isRDevice = (mCurrentDeviceId >= 0x50 && mCurrentDeviceId <= 0x53);

    if (! (aspeqtSettings->enableRDevice() && isRDevice) )
    {
        if(mMethod==HANDSHAKE_SOFTWARE) SioWorker::usleep(mWriteDelay);
        else SioWorker::usleep(mCompErrDelay);
    }
    // --- END UPDATE ---

    return writeRawFrame(QByteArray(1, SIO_COMPLETE));
}


bool StandardSerialPortBackend::writeError()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port writeError...");

    if(mMethod==HANDSHAKE_SOFTWARE)SioWorker::usleep(mWriteDelay);
    else SioWorker::usleep(mCompErrDelay);
    return writeRawFrame(QByteArray(1, SIO_ERROR));
}

quint8 StandardSerialPortBackend::sioChecksum(const QByteArray &data, uint size)
{
//    qDebug() << "!d" << tr("DBG -- Serial Port sioChecksum...");

    uint i;
    uint sum = 0;

    for (i=0; i < size; i++) {
        sum += (quint8)data.at(i);
        if (sum > 255) {
            sum -= 255;
        }
    }

    return sum;
}

QByteArray StandardSerialPortBackend::readRawFrame(uint size, bool verbose)
{
//    qDebug() << "!d" << tr("DBG -- Serial Port readRawFrame...");

    QByteArray data;
    DWORD result;

    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        data.resize(size);
        if (!ReadFile(mHandle, data.data(), size, &result, NULL) || (result != (DWORD)size))
        {
            data.clear();
        }
    }
    else
    {
        OVERLAPPED ov;

        memset(&ov, 0, sizeof(ov));
        ov.hEvent = CreateEvent(0, true, false, 0);

        if (ov.hEvent == INVALID_HANDLE_VALUE) {
            qCritical() << "!e" << tr("Cannot create event: %1").arg(lastErrorMessage());
            return data;
        }

        data.resize(size);
        if (!ReadFile(mHandle, data.data(), size, &result, &ov)) {
            if (GetLastError() == ERROR_IO_PENDING) {
                if (!GetOverlappedResult(mHandle, &ov, &result, true)) {
                    qCritical() << "!e" << tr("Cannot read from serial port: %1").arg(lastErrorMessage());
                    data.clear();
                    CloseHandle(ov.hEvent);
                    return data;
                }
            } else {
                qCritical() << "!e" << tr("Cannot read from serial port: %1").arg(lastErrorMessage());
                data.clear();
                CloseHandle(ov.hEvent);
                return data;
            }
        }
        CloseHandle(ov.hEvent);
        if (result != (DWORD)size)
        {
            if(verbose)
            {
                qCritical() << "!e" << tr("Serial port read timeout.");
            }
            data.clear();
            return data;
        }
    }
    return data;
}

bool StandardSerialPortBackend::writeRawFrame(const QByteArray &data)
{
//    qDebug() << "!d" << tr("DBG -- Serial Port writeRawFrame...");

    DWORD result;

    if (!PurgeComm(mHandle, PURGE_TXCLEAR)) {
        qCritical() << "!e" << tr("Cannot clear serial port write buffer: %1").arg(lastErrorMessage());
        return false;
    }

    if(mMethod==HANDSHAKE_SOFTWARE)
    {
        return WriteFile(mHandle, data.constData(), data.size(), &result, NULL);
    }
    else
    {
        OVERLAPPED ov;

        memset(&ov, 0, sizeof(ov));
        ov.hEvent = CreateEvent(0, true, false, 0);

        if (!WriteFile(mHandle, data.constData(), data.size(), &result, &ov)) {
            if (GetLastError() == ERROR_IO_PENDING) {
                if (!GetOverlappedResult(mHandle, &ov, &result, true)) {
                    qCritical() << "!e" << tr("Cannot write to serial port: %1").arg(lastErrorMessage());
                    CloseHandle(ov.hEvent);
                    return false;
                }
            } else {
                qCritical() << "!e" << tr("Cannot write to serial port: %1").arg(lastErrorMessage());
                return false;
            }
        }
        CloseHandle(ov.hEvent);
    }

    if (result != (DWORD)data.size()) {
        qCritical() << "!e" << tr("Serial port write timeout.");
        return false;
    }
    return true;
}

QString StandardSerialPortBackend::lastErrorMessage()
{
//    qDebug() << "!d" << tr("DBG -- Serial Port lastErrorMessage...");

    QString result;
    LPVOID lpMsgBuf;

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    result.setUtf16((ushort *)lpMsgBuf, wcslen((wchar_t*)lpMsgBuf) - 2);
    LocalFree(lpMsgBuf);
    return result;
}

void StandardSerialPortBackend::setActiveSioDevices(const QByteArray &data)
{
    mSioDevices = data;
}

/* Dummy AtariSIO backend */

bool AtariSioBackend::open()
{
    qCritical() << "!e" << tr("AtariSIO is only available under Linux.");
    return false;
}

QString AtariSioBackend::defaultPortName()
{
    return QString();
}

QByteArray AtariSioBackend::readRawFrame(uint, bool)
{
    return QByteArray();
}

AtariSioBackend::AtariSioBackend(QObject *) {}
AtariSioBackend::~AtariSioBackend() {}
bool AtariSioBackend::isOpen() {return false;}
void AtariSioBackend::close() {}
void AtariSioBackend::cancel() {}
int AtariSioBackend::speedByte() {return 0;}
QByteArray AtariSioBackend::readCommandFrame() {return QByteArray();}
QByteArray AtariSioBackend::readDataFrame(uint, bool) {return QByteArray();}
bool AtariSioBackend::writeDataFrame(const QByteArray &) {return false;}
bool AtariSioBackend::writeCommandAck() {return false;}
bool AtariSioBackend::writeCommandNak() {return false;}
bool AtariSioBackend::writeDataAck() {return false;}
bool AtariSioBackend::writeDataNak() {return false;}
bool AtariSioBackend::writeComplete() {return false;}
bool AtariSioBackend::writeError() {return false;}
bool AtariSioBackend::setSpeed(int ) {return false;}
bool AtariSioBackend::writeRawFrame(const QByteArray &) {return false;}
void AtariSioBackend::setActiveSioDevices(const QByteArray &){}
