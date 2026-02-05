/*
 * miscdevices.h
 */

#ifndef MISCDEVICES_H
#define MISCDEVICES_H

#include "sioworker.h"
#include <QAction>
#include <QSerialPort>
#include <QTcpSocket>


// ... (Keep Printer, SmartDevice, and Mnu classes exactly as they were below) ...
class Printer: public SioDevice
{
    Q_OBJECT
private:
    int m_lastOperation;
public:
    Printer(SioWorker *worker): SioDevice(worker) {}
    void handleCommand(quint8 command, quint16 aux);
signals:
    void print(const QString &text);
};

class SmartDevice: public SioDevice
{
    Q_OBJECT
public:
    SmartDevice(SioWorker *worker): SioDevice(worker) {}
    void handleCommand(quint8 command, quint16 aux);
};

// --- NEW: K: Device (Clipboard) ---
class ClipboardDevice : public SioDevice
{
    Q_OBJECT
public:
    ClipboardDevice(SioWorker *worker) : SioDevice(worker), m_clipPos(0) {}
    void handleCommand(quint8 command, quint16 aux) override;

private:
    QByteArray m_clipBuffer;
    int m_clipPos;
};

#endif // MISCDEVICES_H
