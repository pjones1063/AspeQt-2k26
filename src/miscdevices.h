/*
 * miscdevices.h
 */

#ifndef MISCDEVICES_H
#define MISCDEVICES_H

#include "sioworker.h"
#include <QAction>
#include <QSerialPort>
#include <QTcpSocket>


class SmartDevice: public SioDevice
{
    Q_OBJECT
public:
    SmartDevice(SioWorker *worker): SioDevice(worker) {}
    void handleCommand(quint8 command, quint16 aux);
};



class ClipboardDevice : public SioDevice
{
    Q_OBJECT
public:
    ClipboardDevice(SioWorker *worker) : SioDevice(worker), m_clipPos(0) {}
    void handleCommand(quint8 command, quint16 aux) override;

private:
    QByteArray m_clipBuffer;
    QString    m_writeAccumulator;
    int m_clipPos;

signals: // Define these signals
     void requestClipSet(QString text); // The "Commit" signal

};

#endif // MISCDEVICES_H
