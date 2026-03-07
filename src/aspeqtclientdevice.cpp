#include "aspeqtclientdevice.h"
#include "aspeqtsettings.h"
#include "mainwindow.h" // Needed for SimpleDiskImage definitions
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QtDebug>
#include <QLocale>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

AspeqtClientDevice::AspeqtClientDevice(SioWorker *worker)
    : SioDevice(worker), m_fFilter("*")
{
}

void AspeqtClientDevice::handleCommand(quint8 command, quint16 aux)
{
    switch (command) {

        // -------------------------------------------------------------------
        // Note: Commands $86, $87, and $88 (Host Code Execution) have been
        // removed for security. The SIO handler now begins at directory operations.
        // -------------------------------------------------------------------

    case 0x89 : // Set list filter
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray ddata = sio->port()->readDataFrame(32);
        if (ddata.isEmpty()) {
            qCritical() << "!e" << tr("[%1] Read data frame failed").arg(deviceName());
            sio->port()->writeDataNak();
            sio->port()->writeError();
            m_fFilter = "*";
            return;
        }
        sio->port()->writeDataAck();
        sio->port()->writeComplete();

        // FIX: Use .constData() to strip the Atari's null-byte SIO padding
        m_fFilter = QString::fromUtf8(ddata.constData()).trimmed();
        qCritical() << "!i" << tr("[%1] List filter set: [%2]").arg(deviceName()).arg(m_fFilter);
        return;
    }

    case 0x90 : // Get list option
    {
        if (!sio->port()->writeCommandAck()) return;

        quint8 cmdpPrm = (aux % 256);
        QByteArray ddata(255, 0);

        if(!m_files.contains(cmdpPrm)) {
            ddata[0] = '@';
        } else {
            m_imageFileName = m_files.value(cmdpPrm);
            if(m_imageFileName.startsWith("+")) {
                // Safety bound check
                if (m_imageFileName.length() >= 3) {
                    m_imageFileName = m_imageFileName.mid(2, m_imageFileName.length() - 3).trimmed();
                }

                if(m_imageFileName == "home")
                    m_fPath = "";
                else if(m_imageFileName == "up")
                    m_fPath = m_fPath.left(m_fPath.lastIndexOf("/"));
                else
                    m_fPath = m_fPath + "/" + m_imageFileName;

                ddata[0] = '$';
                qCritical() << "!i" << tr("[%1] Set Path: [%2]").arg(deviceName()).arg(m_fPath);
            } else {
                ddata[0] = char(cmdpPrm);
            }
        }
        ddata[1] = (char)155;
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(ddata);
        return;
    }

    case 0x91 : // List folder (up to 65000 files)
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray ddata(255, 0);
        QString pth = aspeqtSettings->lastRclDir() + m_fPath;

        if(pth.trimmed().isEmpty()) {
            QByteArray fn = QString("Home not set in Options>Emulation").toUtf8();
            for(int i = 0; i < 253; i++) {
                ddata[i] = (fn.length() > i) ? (fn[i] & 0xff) : 0x00;
            }
            ddata[252] = 0x41;
            ddata[253] = 1 / 256;
            ddata[254] = 1 % 256;
            qCritical() << "!e" << tr("** AspeQt home folder not set - Goto Tools>Options>Emulation");
            sio->port()->writeComplete();
            sio->port()->writeDataFrame(ddata);
            return;
        }

        QDir dir(pth);
        QStringList filters;
        if(m_fFilter == "*" || m_fFilter.isEmpty()) {
            filters << "*.*" << "*";
        } else {
            filters << m_fFilter + "*.*" << m_fFilter + "*";
        }

        dir.setNameFilters(filters);
        QFileInfoList list = dir.entryInfoList();
        quint8 index = 0;
        ddata[index++] = (char)155;
        ddata[252] = 0;
        ddata[253] = 0;
        ddata[254] = 0;
        m_files.clear();

        for (quint16 i = aux; i < list.size() && i < 0xFFFA && i - aux < 0x10; ++i) {
            QFileInfo fileInfo = list.at(i);
            QString dosfilname;

            if(fileInfo.fileName().trimmed() == ".") dosfilname = "+[home]";
            else if(fileInfo.fileName().trimmed() == "..") dosfilname = "+[up]";
            else if(fileInfo.isDir()) dosfilname = "+[" + fileInfo.fileName().trimmed() + "]";
            else dosfilname = fileInfo.fileName().trimmed();

            quint8 fileNum = i - aux + 0x41;
            m_files.insert(fileNum, dosfilname);

            QString atariFilenum = QString(QChar::fromLatin1(fileNum));
            QString atariFileDsc = dosfilname.left(33);

            QByteArray fn = (" " + atariFilenum + " " + atariFileDsc).toUtf8();
            if(index + fn.length() < 250) {
                for(int n = 0; n < fn.length(); n++) {
                    ddata[index++] = fn[n] & 0xff;
                }
                ddata[index++] = (char)155;
            } else {
                break;
            }

            if(index > 0) ddata[252] = 0x41 + (i - aux);
            ddata[253] = (i + 1) / 256;
            ddata[254] = (i + 1) % 256;
        }

        for(int n = index; n < 252; n++) ddata[index++] = 0x00;
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(ddata);
        return;
    }

    case 0x92 : // Get slots filename
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray fdata(32, 0); // Corrected boundary
        qint8 deviceNo = (aux % 256);
        deviceNo = (deviceNo > 9) ? (deviceNo - 16) : deviceNo;

        if (deviceNo >= 0x0 && deviceNo <= 15) {
            SimpleDiskImage *img = qobject_cast<SimpleDiskImage*>(sio->getDevice(deviceNo - 1 + DISK_BASE_CDEVIC));
            QString filename = "";
            if (img) {
                int i = img->originalFileName().lastIndexOf("/");
                if ((i != -1) || (img->originalFileName().startsWith("Untitled image"))) {
                    QString dosfilname = img->originalFileName().mid(i + 1);
                    filename = dosfilname.left(35);
                }
            }

            QByteArray fn = filename.toUtf8();
            for(int i = 0; i < 32; i++) {
                fdata[i] = (fn.length() > i) ? (fn[i] & 0xff) : 0x00;
            }

            sio->port()->writeComplete();
            sio->port()->writeDataFrame(fdata);
            return;
        }
        sio->port()->writeDataNak();
        break;
    }

    case 0x93 : // Send Date/Time
    {
        if (!sio->port()->writeCommandAck()) return;

        QDateTime dateTime = QDateTime::currentDateTime();
        QByteArray data(6, 0); // Corrected boundary

        data[0] = dateTime.date().day();
        data[1] = dateTime.date().month();
        data[2] = dateTime.date().year() % 100;
        data[3] = dateTime.time().hour();
        data[4] = dateTime.time().minute();
        data[5] = dateTime.time().second();

        qDebug() << "!n" << tr("[%1] Date/time sent to client (%2).")
                                .arg(deviceName())
                                .arg(QLocale::system().toString(dateTime, QLocale::ShortFormat));

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(data);
        break;
    }

    case 0x94 : // Swap Disks
    {
        if (!sio->port()->writeCommandAck()) return;

        qint8 swapDisk1 = aux / 256 - 1;
        qint8 swapDisk2 = aux % 256 - 1;
        if (swapDisk1 > 9) swapDisk1 -= 16;
        if (swapDisk2 > 9) swapDisk2 -= 16;

        if (swapDisk1 >= 0 && swapDisk1 < 15 && swapDisk2 >= 0 && swapDisk2 < 15 && swapDisk1 != swapDisk2) {
            sio->swapDevices(swapDisk1 + DISK_BASE_CDEVIC, swapDisk2 + DISK_BASE_CDEVIC);
            aspeqtSettings->swapImages(swapDisk1, swapDisk2);
            qDebug() << "!n" << tr("[%1] Swapped disk %2 with disk %3.")
                                    .arg(deviceName()).arg(swapDisk2 + 1).arg(swapDisk1 + 1);
        } else {
            sio->port()->writeCommandNak();
            qDebug() << "!e" << tr("[%1] Invalid swap request for drives: (%2)-(%3).")
                                    .arg(deviceName()).arg(swapDisk2 + 1).arg(swapDisk1 + 1);
        }
        sio->port()->writeComplete();
        break;
    }

    case 0x95 : // Unmount Disk(s)
    {
        if (!sio->port()->writeCommandAck()) return;

        qint8 unmountDisk = aux / 256;
        if (unmountDisk == -6) unmountDisk = 0; // All drives
        if (unmountDisk > 9) unmountDisk -= 16; // Drive 10-15

        if (unmountDisk >= 0 && unmountDisk <= 15) {
            if (unmountDisk == 0) {
                for (int i = 0; i <= 14; i++) {
                    SimpleDiskImage *img = qobject_cast<SimpleDiskImage*>(sio->getDevice(i + DISK_BASE_CDEVIC));
                    if (img && img->isModified() && !img->isUnnamed()) img->save();
                }
                for (int i = 14; i >= 0; i--) {
                    SimpleDiskImage *img = qobject_cast<SimpleDiskImage*>(sio->getDevice(i + DISK_BASE_CDEVIC));
                    sio->uninstallDevice(i + DISK_BASE_CDEVIC);
                    delete img;
                    aspeqtSettings->unmountImage(i);
                }
                qDebug() << "!n" << tr("[%1] ALL images were remotely unmounted").arg(deviceName());
            } else {
                SimpleDiskImage *img = qobject_cast<SimpleDiskImage*>(sio->getDevice(unmountDisk - 1 + DISK_BASE_CDEVIC));
                if (img && img->isModified() && !img->isUnnamed()) img->save();

                sio->uninstallDevice(unmountDisk - 1 + DISK_BASE_CDEVIC);
                delete img;
                aspeqtSettings->unmountImage(unmountDisk - 1);
                qDebug() << "!n" << tr("[%1] Remotely unmounted disk %2").arg(deviceName()).arg(unmountDisk);
            }
        } else {
            sio->port()->writeCommandNak();
            qDebug() << "!e" << tr("[%1] Invalid drive number: %2 for remote unmount").arg(deviceName()).arg(unmountDisk);
        }
        sio->port()->writeComplete();
        break;
    }

    case 0x96 : // Mount Disk Image
    case 0x97 : // Create and Mount a new Disk Image
    {
        if (!sio->port()->writeCommandAck()) return;

        if(aspeqtSettings->lastRclDir().isEmpty()) {
            qCritical() << "!e" << tr("[%1] AspeQt can't determine the folder where the image file must be created/mounted!").arg(deviceName());
            sio->port()->writeDataNak();
            sio->port()->writeError();
            return;
        }

        int len = (command == 0x96) ? 12 : 14;
        QByteArray data = sio->port()->readDataFrame(len);

        if (data.isEmpty()) {
            qCritical() << "!e" << tr("[%1] Read data frame failed").arg(deviceName());
            sio->port()->writeDataNak();
            sio->port()->writeError();
            return;
        }

        // FIX: Use .constData() to strip the Atari's null-byte padding
        m_imageFileName = QString::fromUtf8(data.constData()).trimmed();

        if (command == 0x97) {
            bool ok;
            int i = m_imageFileName.lastIndexOf(".");
            int type = m_imageFileName.mid(i + 1).toInt(&ok, 10);
            if (ok && (type < 1 || type > 6)) ok = false;

            if(!ok) {
                qCritical() << "!e" << tr("[%1] Invalid image file attribute: %2").arg(deviceName()).arg(type);
                sio->port()->writeDataNak();
                sio->port()->writeError();
                return;
            }

            m_imageFileName = m_imageFileName.left(i);
            QFile file(aspeqtSettings->lastRclDir() + "/" + m_imageFileName);
            if (!file.open(QIODevice::WriteOnly)) {
                qCritical() << "!e" << tr("[%1] Can not create PC File: %2").arg(deviceName()).arg(m_imageFileName);
                sio->port()->writeDataNak();
                sio->port()->writeError();
                return;
            }
            sio->port()->writeDataAck();

            int fileSize = 0;
            QByteArray fileData;
            switch (type){
            case 1: fileSize = 92160; fileData.resize(fileSize+16); fileData.fill(0); fileData[2] = 0x80; fileData[3] = 0x16; fileData[4] = 0x80; break;
            case 2: fileSize = 133120; fileData.resize(fileSize+16); fileData.fill(0); fileData[2] = 0x80; fileData[3] = 0x20; fileData[4] = 0x80; break;
            case 3: fileSize = 183936; fileData.resize(fileSize+16); fileData.fill(0); fileData[2] = 0xE8; fileData[3] = 0x2C; fileData[4] = 0x00; fileData[5] = 0x01; break;
            case 4: fileSize = 368256; fileData.resize(fileSize+16); fileData.fill(0); fileData[2] = 0xE8; fileData[3] = 0x59; fileData[4] = 0x00; fileData[5] = 0x01; break;
            case 5: fileSize = 16776576; fileData.resize(fileSize+16); fileData.fill(0); fileData[2] = 0xD8; fileData[3] = 0xFF; fileData[4] = 0x00; fileData[5] = 0x01; fileData[6] = 0x0F; break;
            case 6: fileSize = 33553576; fileData.resize(fileSize+16); fileData.fill(0); fileData[2] = 0xE0; fileData[3] = 0xFF; fileData[4] = 0x00; fileData[5] = 0x02; fileData[6] = 0x1F; break;
            }
            fileData[0] = 0x96;
            fileData[1] = 0x02;
            file.write(fileData);
            file.close();
        }

        sio->port()->writeDataAck();
        sio->port()->writeComplete();

        m_imageFileName = "*" + m_imageFileName;

        // RESOLVING THE PROTOCOL SCHISM:
        if (aux == 0) {
            // aspecl.asm Auto-Mount: Ask MainWindow to find the next empty slot dynamically
            emit findNewSlot(0, true);
        } else {
            // menu.asm Explicit Mount: Use the specific slot requested by the user
            qint8 mountDisk = (aux % 256) - 1;
            if (mountDisk > 9) mountDisk -= 16;

            if (mountDisk >= 0 && mountDisk <= 14) {
                emit mountFile(mountDisk, m_imageFileName);
            }
        }
        break;
    }

    case 0x98 : // Auto-Commit toggle
    {
        if (!sio->port()->writeCommandAck()) return;

        qint8 commitDisk = aux % 256 - 1;
        bool commitOnOff = (aux / 256) ? false : true;

        if (commitDisk > 9) commitDisk -= 16;
        if (commitDisk != -7 && (commitDisk < 0 || commitDisk > 14)) {
            sio->port()->writeCommandNak();
            return;
        }

        if (commitDisk == -7) {
            for (int i = 0; i < 15; i++) emit toggleAutoCommit(i, commitOnOff);
        } else {
            emit toggleAutoCommit(commitDisk, commitOnOff);
        }

        sio->port()->writeComplete();
        break;
    }

    case 0x99 : // Save disks
    {
        if (!sio->port()->writeCommandAck()) return;

        int diskSaved = 0;
        qint8 deviceNo = aux / 256;

        if (deviceNo == -6) deviceNo = 0; // All drives
        if (deviceNo > 9) deviceNo -= 16; // Drive 10-15

        if (deviceNo >= 0 && deviceNo <= 15) {
            if (deviceNo == 0) {
                for (int i = 0; i < 15; i++) {
                    SimpleDiskImage *img = qobject_cast<SimpleDiskImage*>(sio->getDevice(i + DISK_BASE_CDEVIC));
                    if (img && img->isModified() && !img->isUnnamed()) {
                        img->save();
                        qDebug() << "!n" << tr("[%1] Saved disk %2").arg(deviceName()).arg(i + 1);
                        diskSaved++;
                    }
                }
            } else {
                SimpleDiskImage *img = qobject_cast<SimpleDiskImage*>(sio->getDevice(deviceNo - 1 + DISK_BASE_CDEVIC));
                if (img && img->isModified() && !img->isUnnamed()) {
                    img->save();
                    qDebug() << "!n" << tr("[%1] Saved disk %2").arg(deviceName()).arg(deviceNo);
                    diskSaved++;
                }
            }
            if (!diskSaved) {
                sio->port()->writeCommandNak();
            }
        } else {
            sio->port()->writeCommandNak();
            qDebug() << "!e" << tr("[%1] Invalid drive number: %2 for remote save").arg(deviceName()).arg(deviceNo);
        }
        sio->port()->writeComplete();
        break;
    }

    case 0x9A : // Mount slot and boot
    {
        if (!sio->port()->writeCommandAck()) return;

        qint8 mountDisk = aux % 256 - 1;
        if (mountDisk > 9) mountDisk -= 16;
        if (mountDisk != -7 && (mountDisk < 0 || mountDisk > 14)) {
            mountDisk = 0;
        }

        if(aspeqtSettings->lastRclDir().isEmpty()) {
            qCritical() << "!e" << tr("[%1] AspeQt can't determine the folder where the image file must be created/mounted!").arg(deviceName());
            sio->port()->writeDataNak();
            sio->port()->writeError();
            return;
        }

        QByteArray data = sio->port()->readDataFrame(12);
        if (data.isEmpty()) {
            qCritical() << "!e" << tr("[%1] Read data frame failed").arg(deviceName());
            sio->port()->writeDataNak();
            sio->port()->writeError();
            return;
        }

        sio->port()->writeDataAck();
        sio->port()->writeComplete();

        quint8 fileNum = (aux / 256);
        if(fileNum > 40 && m_files.contains(fileNum)) {
            m_imageFileName = m_fPath + "/" + m_files.value(fileNum);
        } else {
            // FIX: Use .constData() to strip the Atari's null-byte padding
            m_imageFileName = QString::fromUtf8(data.constData()).trimmed();
        }

        m_imageFileName = m_imageFileName.trimmed();

        bool isCasImage = m_imageFileName.toLower().endsWith("cas");
        bool isExeImage = (m_imageFileName.toLower().endsWith("xex") ||
                           m_imageFileName.toLower().endsWith("exe") ||
                           m_imageFileName.toLower().endsWith("com"));

        if (isCasImage) {
            emit bootCas(m_imageFileName);
        } else if (isExeImage) {
            emit bootExe(m_imageFileName);
        } else {
            m_imageFileName = "*" + m_imageFileName;
            emit mountFile(mountDisk, m_imageFileName);
        }
        break;
    }

    case 0x9B : // Toggle printer
    {
        if (!sio->port()->writeCommandAck()) return;

        bool enable = (aux / 256) ? true : false;
        sio->port()->writeComplete();
        emit togglePrinterServer(enable);
        break;
    }

    case 0x9C : // Get RCL path
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray fdata(255, 0);
        QString pth = aspeqtSettings->lastRclDir() + m_fPath;
        QByteArray fn = pth.toUtf8();

        if(fn.length() < 5) {
            qCritical() << "!e" << tr("** AspeQt home folder not set - Goto Tools>Options>Emulation");
            fn = QString("Home not set in Options >Emulation").toUtf8();
        }

        for(int i = 0; i < 253; i++) {
            fdata[i] = (fn.length() > i) ? (fn[i] & 0xff) : 0x00;
        }

        qCritical() << "!i" << tr(" Get Path: [%1]").arg(pth);

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(fdata);
        return;
    }

    case 0x9D : // Get Last Mounted Drive Number (For aspecl.asm)
    {
        if (!sio->port()->writeCommandAck()) return;

        QByteArray data(1, 0);
        data[0] = m_lastSlotNo; // Populated by your gotNewSlot() function

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(data);
        break;
    }

    default :
        sio->port()->writeCommandNak();
        qWarning() << "!e" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                                  .arg(deviceName())
                                  .arg(command, 2, 16, QChar('0'))
                                  .arg(aux, 4, 16, QChar('0'));
    }
}

void AspeqtClientDevice::gotNewSlot(int slot)
{
    m_lastSlotNo = slot;
    emit mountFile(slot, m_imageFileName);
}

void AspeqtClientDevice::fileMounted(bool mounted)
{
    if (mounted) {
        sio->port()->writeComplete();
        qDebug() << "!n" << tr("[%1] Image %2 mounted").arg(deviceName()).arg(m_imageFileName.mid(1));
    } else {
        sio->port()->writeDataNak();
    }
}
