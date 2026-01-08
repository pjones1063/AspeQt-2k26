/*
 * miscdevices.cpp
 */

#include "qprocess.h"
#ifdef Q_WS_WIN
#include "windows.h"
#endif

#include "miscdevices.h"
#include "aspeqtsettings.h"
#include "mainwindow.h"
#include <QDateTime>
#include <QtDebug>
#include <QDesktopServices>
#include <QUrl>

extern char g_rclSlotNo;
bool conversionMsgdisplayedOnce;

QString imageFileName;
QByteArray  commandOutput;


QHash <quint8, QString> files;


void Rs232::handleCommand(quint8 command, quint16 aux)
{
    switch(command)
    {
    // Submit URL
    case 0x99:
    {
        if(respeqtSettings->isURLSubmitEnabled() && aux!=0 && aux<=2000)
        {
            if (!sio->port()->writeCommandAck())
            {
                return;
            }

            QByteArray data(aux, 0);
            data = sio->port()->readDataFrame(aux);
            if (data.isEmpty())
            {
                qCritical() << "!e" << tr("[%1] Read data frame failed").arg(deviceName());
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
            qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                           .arg(deviceName())
                           .arg(command, 2, 16, QChar('0'))
                           .arg(aux, 4, 16, QChar('0'));
            return;
        }
        break;
    }

    default:
    {
        qDebug() << "!W" <<  tr("rS232 - command: $%1, aux: $%2 ignored.")
                       .arg(command, 2, 16, QChar('0'))
                       .arg(aux, 4, 16, QChar('0'));
        break;
    }

    }
}


void Printer::handleCommand(quint8 command, quint16 aux)
{
    if(respeqtSettings->printerEmulation()) {  // Ignore printer commands  if Emulation turned OFF)    //
        switch(command) {
        case 0x53:
            {
                // Get status
                if (!sio->port()->writeCommandAck()) {
                    return;
                }

                QByteArray status(4, 0);
                status[0] = 0;
                status[1] = m_lastOperation;
                status[2] = 3;
                status[3] = 0;
                sio->port()->writeComplete();
                sio->port()->writeDataFrame(status);
                qDebug() << "!n" << tr("[%1] Get status.").arg(deviceName());
                break;
            }
        case 0x57:
            {
                // Write
                int aux2 = aux % 256;
                int len;
                switch (aux2) {
                case 0x4e:
                    // Normal
                    len = 40;
                    break;
                case 0x53:
                    // Sideways
                    len = 29;
                    break;
                case 0x44:
                    // Double width
                    len = 21;
                    break;
                default:
                    sio->port()->writeCommandNak();
                    qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                                   .arg(deviceName())
                                   .arg(command, 2, 16, QChar('0'))
                                   .arg(aux, 4, 16, QChar('0'));
                    return;
                }
                // Display Info message once  //
                if (!conversionMsgdisplayedOnce) {
                    qDebug() << "!n" << tr("[%1] Converting Inverse Video Characters for ASCII viewing").arg(deviceName()).arg(len);
                    conversionMsgdisplayedOnce = true;
                }
                sio->port()->writeCommandAck();

                QByteArray data = sio->port()->readDataFrame(len);
                if (data.isEmpty()) {
                    qCritical() << "!e" << tr("[%1] Print: data frame failed")
                                  .arg(deviceName());
                    sio->port()->writeDataNak();
                    return;
                }
                sio->port()->writeDataAck();
                qDebug() << "!n" << tr("[%1] Print (%2 chars)").arg(deviceName()).arg(len);
                int n = data.indexOf('\x9b');
                if (n == -1) {
                    n = len;
                }
                data.resize(n);
                data.replace('\n', '\x9b');
                if (n < len) {
                    data.append("\n");
                }
                emit print(QString::fromLatin1(data));
                sio->port()->writeComplete();
                break;
            }
        default:
            sio->port()->writeCommandNak();
            qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                           .arg(deviceName())
                           .arg(command, 2, 16, QChar('0'))
                           .arg(aux, 4, 16, QChar('0'));
        }
    } else {
        qDebug() << "!u" << tr("[%1] ignored").arg(deviceName());
    }
}

//SmartDevice (ApeTime + URL submit)

void SmartDevice::handleCommand(quint8 command, quint16 aux)
{
    switch(command)
    {
    // Get APE Time
    case 0x93:
    {
        if (!sio->port()->writeCommandAck())
        {
            return;
        }

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
                       .arg(dateTime.toString(Qt::SystemLocaleShortDate));
        break;
    }

    // Submit URL
    case 0x55:
    {
        if(respeqtSettings->isURLSubmitEnabled() && aux!=0 && aux<=2000)
        {
            if (!sio->port()->writeCommandAck())
            {
                return;
            }

            QByteArray data(aux, 0);
            data = sio->port()->readDataFrame(aux);
            if (data.isEmpty())
            {
                qCritical() << "!e" << tr("[%1] Read data frame failed").arg(deviceName());
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
            qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                           .arg(deviceName())
                           .arg(command, 2, 16, QChar('0'))
                           .arg(aux, 4, 16, QChar('0'));
            return;
        }
        break;
    }

    default:
    {
        sio->port()->writeCommandNak();
        qWarning() << "!w" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                       .arg(deviceName())
                       .arg(command, 2, 16, QChar('0'))
                       .arg(aux, 4, 16, QChar('0'));
        break;
    }

    }
}

// AspeQt Client ()

void Mnu::handleCommand(quint8 command, quint16 aux)
{

    switch (command) {

    case 0x86 : // get server command
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }

        QByteArray  fdata(255, 0);
        QString cmd = respeqtSettings->lastRclCommand();
        QByteArray cm = cmd.toUtf8();


        if(cm.length() < 1)
        {
            cm = "";
        }

        for(int i=0; i < 253; i++)
            fdata[i] = (cm.length() > i) ? (cm[i] & 0xff) : 0x00;

        qCritical() << "!i" << tr(" Get Cmd: [%1]").arg(cmd);

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(fdata);
        return;
    }
        break;


    case 0x87 : // Exec server command
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }

        QProcess process;
        QByteArray  fdata(255, 0);
        fdata = sio->port()->readDataFrame(32);
        QString cmd =  fdata;

        if (cmd.isEmpty())
            cmd = respeqtSettings->lastRclCommand();

#if defined(Q_OS_WIN)
        process.start( "cmd ",  QStringList() <<"/c" << cmd  );
#else
        process.start( "sh", QStringList() <<"-c" << cmd  );
#endif
        respeqtSettings->setRclCommand(cmd);
        qCritical().noquote() << "!i" << tr("[%1]").arg(cmd);
        qCritical() << "!i" << tr("Command Complete [%1]").arg("--");

        process.waitForFinished(1000);

        QString co = (process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0) ?
                         (process.readAllStandardOutput()) :
                         (process.readAllStandardError());

        commandOutput = co.trimmed().toUtf8().replace(10, (char)155);

        sio->port()->writeDataAck();
        sio->port()->writeComplete();
        return;
    }
        break;

    case 0x88 : // get server command
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }


        QByteArray  fdata(255, 0);

        if(commandOutput.length() < 1) commandOutput = " ";

        for(int i=0; i < 251; i++)
            fdata[i] = (commandOutput.length() > i) ? (commandOutput[i] & 0xff) : 0x00;

        if(commandOutput.length() > 250) {
            commandOutput = commandOutput.remove(0,251);
            fdata[251] = 0x00;
            fdata[252] = 0x01;
        }

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(fdata);

        return;
    }
        break;




    case 0x89 : // set list filter
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }

        QByteArray ddata = sio->port()->readDataFrame(32);
        if (ddata.isEmpty()) {
            qCritical() << "!e" << tr("[%1] Read data frame failed")
                           .arg(deviceName());
            sio->port()->writeDataNak();
            sio->port()->writeError();
            fFilter = "*";
            return;
        }
        sio->port()->writeDataAck();
        sio->port()->writeComplete();
        fFilter = ddata;
        qCritical() << "!i" << tr("[%1] List filter set: [%2]")
                       .arg(deviceName())
                       .arg(fFilter);
        return;

    }
        break;

    case 0x90 : // get list option
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }

        quint8 cmdpPrm  = (aux  % 256);
        QByteArray  ddata(255, 0);

        if(!files.contains(cmdpPrm))
        {
            ddata[0] = '@';
        }
        else
        {
            imageFileName = files.value(cmdpPrm);
            if(imageFileName.startsWith("+"))
            {
                imageFileName = imageFileName.mid(2, imageFileName.length()-3).trimmed();
                if(imageFileName == "home")
                    fPath = "";
                else if(imageFileName == "up" )
                    fPath = fPath.left(fPath.lastIndexOf("/"));
                else
                    fPath = fPath + "/"+ imageFileName;

                ddata[0] = '$';
                qCritical() << "!i" << tr("[%1] Set Path: [%2]")
                               .arg(deviceName())
                               .arg(fPath);

            }
            else
            {
                ddata[0] = char(cmdpPrm);
            }
        }
        ddata[1] = (char)155;
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(ddata);
        return;
    }

        break;



    case 0x91 : // list  folder(up to 65000 files)
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }

        QByteArray  ddata(255, 0);
        QString pth = respeqtSettings->lastRclDir() + fPath;
        if(pth.trimmed().isEmpty())
        {
            QByteArray fn = QString("Home not set in Options>Emulation").toUtf8();
            for(int i=0; i < 253; i++)
                ddata[i] = (fn.length() > i) ? (fn[i] & 0xff) : 0x00;
            ddata[252] =  0x41;
            ddata[253] =  (1) / 256;
            ddata[254] =  (1) % 256;
            qCritical() << "!e" << tr("** AspeQT home folder not set - Goto Tools>Options>Emulation");
            sio->port()->writeComplete();
            sio->port()->writeDataFrame(ddata);
            return;
        }


        QDir dir(pth);
        QStringList filters;
        if(fFilter == "*" || fFilter == "")
        {
            filters << "*.*";
            filters << "*";
        }
        else
        {
            filters <<fFilter+"*.*";
            filters <<fFilter+"*";
        }

        dir.setNameFilters(filters);
        QFileInfoList list = dir.entryInfoList();
        quint8 index  = 0;
        ddata[index++] = (char)155;
        ddata[252] =  0;
        ddata[253] =  0;
        ddata[254] =  0;
        files.clear();


        for (quint16 i = aux; i < list.size() && i < 0xFFFA && i-aux < 0x10;  ++i) {
            QFileInfo fileInfo = list.at(i);
            QString dosfilname;

            fileInfo.isDir()? dosfilname = "+ " +fileInfo.fileName().trimmed(): dosfilname = fileInfo.fileName().trimmed();

            if(fileInfo.fileName().trimmed() == "." )  dosfilname = "+[home]";
            else if(fileInfo.fileName().trimmed() == ".." )  dosfilname = "+[up]";
            else if(fileInfo.isDir())   dosfilname = "+[" +fileInfo.fileName().trimmed()+"]";
            else dosfilname = fileInfo.fileName().trimmed();

            quint8 fileNum = i-aux+0x41;
            files.insert(fileNum, dosfilname);

            QString atariFilenum = QString(QChar::fromLatin1(fileNum));
            QString atariFileDsc = dosfilname.left(33);

            QByteArray fn  = (" "+atariFilenum+" "+atariFileDsc).toUtf8();
            if(index + fn.length() < 250) {
                for(int n = 0; n < fn.length(); n++)
                    ddata[index++] = fn[n] & 0xff;

                ddata[index++] = (char)155;

            } else  {
                break;
            }


            if(index > 0 ) ddata[252] =  0x41 + (i - aux);

            ddata[253] =  (i + 1) / 256;
            ddata[254] =  (i + 1) % 256;

        }

        for(int n = index; n < 252 ; n++) ddata[index++] = 0x00;
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(ddata);
        return;


    }
      break;

    case 0x92 :   // get slots filename
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }

        QByteArray  fdata(31, 0);
        qint8 deviceNo;
        deviceNo =  (aux  % 256);
        deviceNo  = (deviceNo > 9) ? (deviceNo -16) :deviceNo;

        if (deviceNo >= 0x0 && deviceNo <= 15 ) {
            SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(deviceNo -1 +  DISK_BASE_CDEVIC));
            QString  filename = "";
            if (img) {
                int i = -1;
                i = img->originalFileName().lastIndexOf("/");
                if ((i != -1) || (img->originalFileName().mid(0, 14) == "Untitled image")) {
                    QString dosfilname =  img->originalFileName().right(img->originalFileName().size() - ++i);
                    filename = dosfilname.left(35);
                  }
              }

            QByteArray fn = filename.toUtf8();
            for(int i=0; i < 32; i++)
                fdata[i] = (fn.length() > i) ? (fn[i] & 0xff) : 0x00;


            sio->port()->writeComplete();
            sio->port()->writeDataFrame(fdata);
            return;
        }
        sio->port()->writeDataNak();
    }


    case 0x93 :   // Send Date/Time
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }

        QDateTime   dateTime = QDateTime::currentDateTime();
        QByteArray  data(5, 0);

        data[0] = dateTime.date().day();
        data[1] = dateTime.date().month();
        data[2] = dateTime.date().year() % 100;
        data[3] = dateTime.time().hour();
        data[4] = dateTime.time().minute();
        data[5] = dateTime.time().second();
        qDebug() << "!n" << tr("[%1] Date/time sent to client (%2).")
                    .arg(deviceName())
                    .arg(dateTime.toString(Qt::SystemLocaleShortDate));

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(data);
    }
        break;

    case 0x94 :   // Swap Disks
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }
        qint8 swapDisk1, swapDisk2;
        swapDisk1 = aux /256 - 1;
        swapDisk2 = aux % 256 - 1;
        if (swapDisk1 > 9) swapDisk1 -= 16;
        if (swapDisk2 > 9) swapDisk2 -= 16;
        if (swapDisk1 >= 0 and swapDisk1 < 15 and swapDisk2 >=0 and swapDisk2 < 15 and swapDisk1 != swapDisk2) {
            sio->swapDevices(swapDisk1 + DISK_BASE_CDEVIC, swapDisk2 + DISK_BASE_CDEVIC);
            respeqtSettings->swapImages(swapDisk1, swapDisk2);
            qDebug() << "!n" << tr("[%1] Swapped disk %2 with disk %3.")
                        .arg(deviceName())
                        .arg(swapDisk2 + 1)
                        .arg(swapDisk1 + 1);
        } else {
            sio->port()->writeCommandNak();
            qDebug() << "!e" << tr("[%1] Invalid swap request for drives: (%2)-(%3).")
                        .arg(deviceName())
                        .arg(swapDisk2 + 1)
                        .arg(swapDisk1 + 1);
        }
        sio->port()->writeComplete();
    }
        break;


    case 0x95 :   // Unmount Disk(s)
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }
        qint8 unmountDisk;
        unmountDisk = aux /256;
        if (unmountDisk == -6) unmountDisk = 0;        // All drives
        if (unmountDisk > 9)   unmountDisk -= 16;        // Drive 10-15
        if (unmountDisk >= 0 and unmountDisk <= 15) {
            if (unmountDisk == 0) {
                // Eject All disks

                for (int i = 0; i <= 14; i++) {    //
                    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
                    if (img && img->isModified() && !img->isUnnamed()) {
                        img->save();
                    }
                }
                for (int i = 14; i >= 0; i--) {
                    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
                    sio->uninstallDevice(i + DISK_BASE_CDEVIC);
                    delete img;
                    respeqtSettings->unmountImage(i);
                    qDebug() << "!n" << tr("[%1] Unmounted disk %2")
                                .arg(deviceName())
                                .arg(i + 1);
                }
                qDebug() << "!n" << tr("[%1] ALL images were remotely unmounted")
                            .arg(deviceName());

            } else {
                // Single Disk Eject
                SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(unmountDisk - 1 + DISK_BASE_CDEVIC));
                if (img && img->isModified() && !img->isUnnamed()) {
                    img->save();
                }
                sio->uninstallDevice(unmountDisk - 1 + DISK_BASE_CDEVIC);
                delete img;
                respeqtSettings->unmountImage(unmountDisk - 1);
                qDebug() << "!n" << tr("[%1] Remotely unmounted disk %2")
                            .arg(deviceName())
                            .arg(unmountDisk);

            }
        } else {
            sio->port()->writeCommandNak();
            qDebug() << "!e" << tr("[%1] Invalid drive number: %2 for remote unmount")
                        .arg(deviceName())
                        .arg(unmountDisk);
        }
        sio->port()->writeComplete();
    }
        break;

    case 0x96 :   // Mount Disk Image
    case 0x97 :   // Create and Mount a new Disk Image
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }
        // If no Folder Image has ever been mounted abort the command as we won't
        // know which folder to use to remotely create/mount an image file.
        if(respeqtSettings->lastRclDir() == "") {
            qCritical() << "!e" << tr("[%1] AspeQt can't determine the folder where the image file must be created/mounted!")
                           .arg(deviceName());
            qCritical() << "!e" << tr("[%1] Mount a Folder Image at least once before issuing a remote mount command.")
                           .arg(deviceName());
            sio->port()->writeDataNak();
            sio->port()->writeError();
            return;
        }
        // Get the name of the image file
        int len;
        if (command == 0x96) {
            len = 12;
        } else {
            len = 14;
        }
            QByteArray data(len, 0);
            data = sio->port()->readDataFrame(len);
            if (data.isEmpty()) {
                qCritical() << "!e" << tr("[%1] Read data frame failed")
                               .arg(deviceName());
                sio->port()->writeDataNak();
                sio->port()->writeError();
                return;
            }

            imageFileName = data;

            if (command == 0x97) {     // Create new image file first
                int i, type;
                bool ok;
                i = imageFileName.lastIndexOf(".");
                type = imageFileName.mid(i+1).toInt(&ok, 10);
                if (ok && (type < 1 || type > 6)) ok = false;
                if(!ok) {
                    qCritical() << "!e" << tr("[%1] Invalid image file attribute: %2")
                                   .arg(deviceName())
                                   .arg(type);
                    sio->port()->writeDataNak();
                    sio->port()->writeError();
                    return;
                }
                imageFileName = imageFileName.left(i);
                QFile file(respeqtSettings->lastRclDir() + "/" + imageFileName);
                if (!file.open(QIODevice::WriteOnly)) {
                    qCritical() << "!e" << tr("[%1] Can not create PC File: %2")
                                   .arg(deviceName())
                                   .arg(imageFileName);
                    sio->port()->writeDataNak();
                    sio->port()->writeError();
                    return;
                }
                sio->port()->writeDataAck();

                int fileSize;
                QByteArray fileData;
                switch (type){
                case 1 :        // Single Density
                {
                    fileSize = 92160;
                    fileData.resize(fileSize+16);
                    fileData.fill(0);
                    fileData[2] = 0x80;
                    fileData[3] = 0x16;
                    fileData[4] = 0x80;
                }
                    break;
                case 2 :        // Enhanced Density
                {
                    fileSize = 133120;
                    fileData.resize(fileSize+16);
                    fileData.fill(0);
                    fileData[2] = 0x80;
                    fileData[3] = 0x20;
                    fileData[4] = 0x80;
                }
                    break;
                case 3 :        // Double Density
                {
                    fileSize = 183936;
                    fileData.resize(fileSize+16);
                    fileData.fill(0);
                    fileData[2] = 0xE8;
                    fileData[3] = 0x2C;
                    fileData[4] = 0x00;
                    fileData[5] = 0x01;
                }
                    break;
                case 4 :        // Double Sided, Double Density
                {
                    fileSize = 368256;
                    fileData.resize(fileSize+16);
                    fileData.fill(0);
                    fileData[2] = 0xE8;
                    fileData[3] = 0x59;
                    fileData[4] = 0x00;
                    fileData[5] = 0x01;
                }
                    break;
                case 5 :        // Double Density Hard Disk
                {
                    fileSize = 16776576;
                    fileData.resize(fileSize+16);
                    fileData.fill(0);
                    fileData[2] = 0xD8;
                    fileData[3] = 0xFF;
                    fileData[4] = 0x00;
                    fileData[5] = 0x01;
                    fileData[6] = 0x0F;
                }
                    break;
                case 6 :        // Quad Density Hard Disk
                {
                    fileSize = 33553576;
                    fileData.resize(fileSize+16);
                    fileData.fill(0);
                    fileData[2] = 0xE0;
                    fileData[3] = 0xFF;
                    fileData[4] = 0x00;
                    fileData[5] = 0x02;
                    fileData[6] = 0x1F;
                }
                    break;
                }
                fileData[0] = 0x96;
                fileData[1] = 0x02;
                file.write(fileData);
                fileData.clear();
                file.close();

            } // Cmd 0x97 -- Create new image file first

            qint8 mountDisk;
            mountDisk = aux % 256 - 1;
            if (mountDisk > 9) mountDisk -= 16;
            if (mountDisk != -7 && (mountDisk <0 || mountDisk > 14)) {
                sio->port()->writeCommandNak();
                return;
            }

            sio->port()->writeDataAck();
            sio->port()->writeComplete();
            imageFileName = "*" + imageFileName;
            emit mountFile(mountDisk,imageFileName);


    }
        break;

    case 0x98 :   // Auto-Commit toggle
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }
        qint8 commitDisk;
        bool commitOnOff;
        commitDisk = aux % 256 - 1;
        commitOnOff = (aux/256)?false:true;

        if (commitDisk > 9) commitDisk -= 16;
        if (commitDisk != -7 && (commitDisk <0 || commitDisk > 14)) {
            sio->port()->writeCommandNak();
            return;
        }

        // All disks or a given disk
        if (commitDisk == -7) {
            for (int i = 0; i < 15; i++) {
                emit toggleAutoCommit(i, commitOnOff);
            }
        } else {
            emit toggleAutoCommit(commitDisk, commitOnOff);
        }

        sio->port()->writeComplete();
    }
        break;

    case 0x99 :   // save disks
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }

        int diskSaved = 0;
        qint8 deviceNo;
        deviceNo = aux /256;

        if (deviceNo == -6) deviceNo = 0;        // All drives
        if (deviceNo > 9) deviceNo -= 16;        // Drive 10-15
        if (deviceNo >= 0 and deviceNo <= 15) {
            if (deviceNo == 0) {
                // Eject All disks
                for (int i = 0; i < 15; i++) {    //
                    SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(i + DISK_BASE_CDEVIC));
                    if (img && img->isModified() && !img->isUnnamed()) {
                        img->save();
                        qDebug() << "!n" << tr("[%1] Saved disk %2")
                                    .arg(deviceName())
                                    .arg(i + 1);
                        diskSaved++;
                    }
                }

            } else {
                // Single Disk save
                SimpleDiskImage *img = qobject_cast <SimpleDiskImage*> (sio->getDevice(deviceNo - 1 + DISK_BASE_CDEVIC));

                if (img && img->isModified() && !img->isUnnamed()) {
                    img->save();
                    qDebug() << "!n" << tr("[%1] Saved disk %2")
                                .arg(deviceName())
                                .arg(deviceNo);
                    diskSaved++;
                }
            }
            if (!diskSaved)
                sio->port()->writeCommandNak();

        } else {
            sio->port()->writeCommandNak();
            qDebug() << "!e" << tr("[%1] Invalid drive number: %2 for remote unmount")
                        .arg(deviceName())
                        .arg(deviceNo);
        }
        sio->port()->writeComplete();
    }
        break;



    case 0x9A :   // Mount slot and boot
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }

        qint8 mountDisk;
        mountDisk = aux % 256 - 1;
        if (mountDisk > 9) mountDisk -= 16;
        if (mountDisk != -7 && (mountDisk <0 || mountDisk > 14)) {
           mountDisk = 0;
        }

        // If no Folder Image has ever been mounted abort the command as we won't
        // know which folder to use to remotely create/mount an image file.
        if(respeqtSettings->lastRclDir() == "") {
            qCritical() << "!e" << tr("[%1] AspeQt can't determine the folder where the image file must be created/mounted!")
                           .arg(deviceName());
            qCritical() << "!e" << tr("[%1] Mount a Folder Image at least once before issuing a remote mount command.")
                           .arg(deviceName());
            sio->port()->writeDataNak();
            sio->port()->writeError();
            return;
        }
        // Get the name of the image file


        QByteArray data(12, 0);
        data = sio->port()->readDataFrame(12);
        if (data.isEmpty()) {
            qCritical() << "!e" << tr("[%1] Read data frame failed")
                           .arg(deviceName());
            sio->port()->writeDataNak();
            sio->port()->writeError();
            return;
        }

        sio->port()->writeDataAck();
        sio->port()->writeComplete();

        quint8 fileNum;
        fileNum = (aux / 256);
        if(fileNum > 40 && files.contains(fileNum))
            imageFileName = fPath +"/"+ files.value(fileNum);
        else
            imageFileName = data;

        imageFileName = imageFileName.trimmed();

        bool isCasTmage = (imageFileName.toLower().endsWith("cas")) ? true: false;

        bool isExeTmage = (imageFileName.toLower().endsWith("xex")
                       || imageFileName.toLower().endsWith("exe")
                       || imageFileName.toLower().endsWith("com")) ? true: false;


        if (isCasTmage)
        {
            emit bootCas(imageFileName);
        }
        else if(isExeTmage)
        {
            emit bootExe(imageFileName);
        }
        else
        {
            imageFileName = "*" + imageFileName;
            emit mountFile(mountDisk,imageFileName);

        }

    }

        break;

    case 0x9B :   // toggle printer
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }
        bool enable = (aux/256)?true:false;
        sio->port()->writeComplete();
        emit togglePrinterServer(enable);
    }
      break;



    case 0x9C : // get RCL path
    {
        if (!sio->port()->writeCommandAck()) {
            return;
        }

        QByteArray  fdata(255, 0);
        QString pth = respeqtSettings->lastRclDir() + fPath;
        QByteArray fn = pth.toUtf8();

        if(fn.length() < 5)
        {
            qCritical() << "!e" << tr("** AspeQT home folder not set - Goto Tools>Options>Emulation");
            fn = QString("Home not set in Options >Emulation").toUtf8();
        }

        for(int i=0; i < 253; i++)
            fdata[i] = (fn.length() > i) ? (fn[i] & 0xff) : 0x00;

        qCritical() << "!i" << tr(" Get Path: [%1]") .arg(pth);

        sio->port()->writeComplete();
        sio->port()->writeDataFrame(fdata);
        return;
    }

     break;

    default :
        // Invalid Command
        sio->port()->writeCommandNak();
        qWarning() << "!e" << tr("[%1] command: $%2, aux: $%3 NAKed.")
                      .arg(deviceName())
                      .arg(command, 2, 16, QChar('0'))
                      .arg(aux, 4, 16, QChar('0'));
    }
}


// Get the next slot number available for mounting a disk image
void Mnu::gotNewSlot(int slot)
{
    g_rclSlotNo = slot;

    // Ask the MainWindow to mount the file
    emit mountFile(slot, imageFileName);
}

void Mnu::fileMounted(bool mounted)
{
    if (mounted) {
        sio->port()->writeComplete();
        qDebug() << "!n" << tr("[%1] Image %2 mounted")
                    .arg(deviceName())
                    .arg(imageFileName.mid(1,imageFileName.size()-1));
    } else {
        sio->port()->writeDataNak();
    }
}

