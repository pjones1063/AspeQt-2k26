/*
 * serialport-win32.h
 */

#ifndef SERIALPORTWIN32_H
#define SERIALPORTWIN32_H

#include "serialport.h"

class StandardSerialPortBackend : public AbstractSerialPortBackend
{
    Q_OBJECT

public:
    StandardSerialPortBackend(QObject *parent = 0);
    ~StandardSerialPortBackend();

    static QString defaultPortName();

    bool open();
    bool isOpen();
    void close();
    void cancel();
    int speedByte();
    QByteArray readCommandFrame();
    QByteArray readDataFrame(uint size, bool verbose = true);
    bool writeDataFrame(const QByteArray &data);
    bool writeCommandAck();
    bool writeCommandNak();
    bool writeDataAck();
    bool writeDataNak();
    bool writeComplete();
    bool writeError();
    bool setSpeed(int speed);
    bool writeRawFrame(const QByteArray &data);
    QByteArray readRawFrame(uint size, bool verbose = true);
    void setActiveSioDevices(const QByteArray &data);

    virtual void setStreamMode(bool stream) { m_isStreamMode = stream; }
    virtual bool isStreamMode() const { return m_isStreamMode; }
    bool isCommandLineAsserted();

private:
    bool mCanceled;
    bool mHighSpeed;
    bool m_isStreamMode = false;
    void *mHandle, *mCancelHandle;
    int mSpeed;
    int mMethod;
    int mWriteDelay;
    int mCompErrDelay;
    QByteArray mSioDevices;

    bool setNormalSpeed();
    bool setHighSpeed();
    int speed();
    quint8 sioChecksum(const QByteArray &data, uint size);

    QString lastErrorMessage();
};

/* Dummy AtariSIO backend */

class AtariSioBackend : public AbstractSerialPortBackend
{
    Q_OBJECT

public:
    static QString defaultPortName();

    AtariSioBackend(QObject *parent = 0);
    ~AtariSioBackend();
    bool open();
    bool isOpen();
    void close();
    void cancel();
    int speedByte();
    QByteArray readCommandFrame();
    QByteArray readDataFrame(uint size, bool verbose = true);
    bool writeDataFrame(const QByteArray &data);
    bool writeCommandAck();
    bool writeCommandNak();
    bool writeDataAck();
    bool writeDataNak();
    bool writeComplete();
    bool writeError();
    bool setSpeed(int speed);
    bool writeRawFrame(const QByteArray &data);
    QByteArray readRawFrame(uint size, bool verbose = true);
    void setActiveSioDevices(const QByteArray &data);
};

#endif // SERIALPORTWIN32_H
