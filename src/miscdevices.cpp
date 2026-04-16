/*
 * miscdevices.cpp
 * DEBUG VERSION - HEAVY LOGGING ENABLED
 */

#include "qprocess.h"
#ifdef Q_OS_WIN
#include "windows.h"
#endif

#include "miscdevices.h"
#include "rdevice_handler.h"
#include "aspeqtsettings.h"
#include <QDateTime>
#include <QtDebug>
#include <QDesktopServices>
#include <QUrl>
#include <QLocale>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QClipboard>
#include <QGuiApplication>

bool conversionMsgdisplayedOnce;

// ==========================================
// SMART DEVICE IMPLEMENTATION
// ==========================================

void SmartDevice::handleCommand(quint8 command, quint16 aux)
{
    switch(command)
    {

    case 0x3F: // --- CRITICAL: Type 3 Smart Poll ---
    {
        if (!sio->port()->writeCommandAck()) return;

        // Signature for Relocatable Loader Request (AUX LSB 0x70)
        if ((aux & 0xFF) == 0x70) {
            // Serve the relocator stub program from rdevice_handler.h
            QByteArray payload((const char*)relocator_stub, sizeof(relocator_stub));
            sio->port()->writeComplete();
            sio->port()->writeDataFrame(payload);
            qDebug() << "!i [SmartDevice] Sent Relocator Stub to Atari.";
        } else {
            sio->port()->writeCommandNak();
        }
        break;
    }


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

    default:
        sio->port()->writeCommandNak();
        break;
    }
}


void ClipboardDevice::handleCommand(quint8 command, quint16 aux)
{
    switch (command) {
    // =============================================================
    // COMMAND: OPEN
    // =============================================================
    case 0x4F:
    {
        if (!sio->port()->writeCommandAck()) return;

        quint8 mode = aux & 0xFF; // 4 = Read, 8 = Write

        if (mode == 8) {
            // --- WRITE MODE (LIST "Y:") ---
            // Clear the internal accumulator. We do NOT touch the OS clipboard yet.
            m_writeAccumulator.clear();
            qDebug() << "!n" << tr("[Y:] Open for Write (Accumulator Reset)");
        } else {
            // --- READ MODE (ENTER "Y:") ---
            //  Fetch text from OS immediately so we have data to send.

            QString clipText;
            // Use invokeMethod to safely get clipboard text from the Main Thread
            QMetaObject::invokeMethod(qApp, [&clipText](){
                QClipboard *cb = QGuiApplication::clipboard();
                if (cb) clipText = cb->text();
            }, Qt::BlockingQueuedConnection);

            // Normalize LF( \r\n -> \n -> \x9B ) and ensure we end with a LF
            clipText.replace("\r\n", "\n");
            clipText.replace('\n', '\x9B');

            if (!clipText.isEmpty() && !clipText.endsWith(QChar(0x9B))) {
                clipText.append(QChar(0x9B));
            }

            m_clipBuffer = clipText.toLatin1();
            m_clipPos = 0; // Reset read head

            qDebug() << "!n" << tr("[Y:] Open for Read (Buffered %1 bytes)").arg(m_clipBuffer.size());
        }

        sio->port()->writeComplete();
        break;
    }

        // -------------------------------------------------------------
        // COMMAND: READ
        // -------------------------------------------------------------
    case 0x52:
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray dataFrame(256, (char)0x00);

        int bytesRemaining = m_clipBuffer.size() - m_clipPos;

        if (bytesRemaining > 0) {
            int bytesToCopy = qMin(bytesRemaining, 256);
            QByteArray segment = m_clipBuffer.mid(m_clipPos, bytesToCopy);
            dataFrame.replace(0, bytesToCopy, segment);
            m_clipPos += bytesToCopy;
        } else {
            qDebug() << "!d" << tr("[Y:] Sent EOF");
        }

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(dataFrame);
        break;
    }

        // -------------------------------------------------------------
        // COMMAND: WRITE
        // -------------------------------------------------------------
    case 0x57:
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray dataFrame = sio->port()->readDataFrame(256);
        if (dataFrame.isEmpty()) {
            sio->port()->writeDataNak();
            return;
        }
        sio->port()->writeDataAck();
        QString chunkStr = QString::fromLatin1(dataFrame);
        chunkStr.replace(QChar(0x9B), QString("\n"));
        chunkStr.remove(QChar(0x00));
        m_writeAccumulator.append(chunkStr);
        sio->port()->writeComplete();

        break;
    }


        // =============================================================
        // COMMAND: CLOSE
        // =============================================================
    case 0x43:
    {
        if (!sio->port()->writeCommandAck()) return;

        // COMMIT: Now we overwrite the OS Clipboard with our complete string.
        if (!m_writeAccumulator.isEmpty()) {
            emit requestClipSet(m_writeAccumulator);
            qDebug() << "!n" << tr("[Y:] Close: Clipboard Updated (%1 chars).").arg(m_writeAccumulator.length());
        } else {
            qDebug() << "!n" << tr("[Y:] Close: Buffer empty, clipboard not touched.");
        }

        sio->port()->writeComplete();
        break;
    }

        // =============================================================
        // UNKNOWN COMMAND
        // =============================================================
    default:
        sio->port()->writeCommandNak();
        qWarning() << "!w" << tr("[Y:] Unknown Command: $%1").arg(command, 2, 16, QChar('0'));
        break;
    }
}
