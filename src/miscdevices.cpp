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
#include <QDateTime>
#include <QtDebug>
#include <QDesktopServices>
#include <QUrl>
#include <QLocale>
#include <QDir>
#include <QFileInfo>
#include "diskimage.h"


extern char g_rclSlotNo;
bool conversionMsgdisplayedOnce;

QString imageFileName;
QByteArray  commandOutput;

QHash <quint8, QString> files;

// ==========================================
// PRINTER IMPLEMENTATION
// ==========================================

void Printer::handleCommand(quint8 command, quint16 aux)
{
    if(aspeqtSettings->printerEmulation()) {
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
        if(aspeqtSettings->isURLSubmitEnabled() && aux!=0 && aux<=2000)
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

