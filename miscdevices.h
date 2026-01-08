/*
 * miscdevices.h
 */

#ifndef MISCDEVICES_H
#define MISCDEVICES_H

#include "sioworker.h"

// SmartDevice (ApeTime + URL submit)
class Rs232: public SioDevice
{
    Q_OBJECT
public:
    Rs232(SioWorker *worker): SioDevice(worker) {}
    void handleCommand(quint8 command, quint16 aux);
};

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

// SmartDevice (ApeTime + URL submit)
class SmartDevice: public SioDevice
{
    Q_OBJECT
public:
    SmartDevice(SioWorker *worker): SioDevice(worker) {}
    void handleCommand(quint8 command, quint16 aux);
};

// AspeQt Time Server //
class Mnu: public SioDevice
{
    Q_OBJECT

public:
    QString fFilter, fPath;

    Mnu(SioWorker *worker): SioDevice(worker) {}
    void handleCommand(quint8 command, quint16 aux);
private:
    QString toAtariFileName(QString dosFileName);
    QString toAtariFileDesc(QString dosFileName, int len);
    QString toDosFileName(QString atariFileName);

public slots:
    void gotNewSlot (int slot);                         // 
    void fileMounted (bool mounted);                    // 

signals:
    void findNewSlot (int startFrom, bool createOne);
    void mountFile (int no, const QString fileName);
    void toggleAutoCommit (int no, bool st);
    void bootExe (const QString fileName);
    void bootCas (const QString fileName);
    void togglePrinterServer (bool enable);
};

#endif // MISCDEVICES_H
