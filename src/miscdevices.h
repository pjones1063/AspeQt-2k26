/*
 * miscdevices.h
 */

#ifndef MISCDEVICES_H
#define MISCDEVICES_H

#include "sioworker.h"
#include <QAction>
#include <QSerialPort>
#include <QTcpSocket>
#include <QTextToSpeech> // <-- ADD THIS FOR THE A: DEVICE

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

signals:
    void requestClipSet(QString text);

};

// ==========================================
// VOICE DEVICE (A:)
// ==========================================
class VoiceDevice : public SioDevice
{
    Q_OBJECT
public:
    VoiceDevice(SioWorker *worker);
    void handleCommand(quint8 command, quint16 aux) override;

private:
    QTextToSpeech *m_speech;
    QString m_accumulator;
};

#endif // MISCDEVICES_H