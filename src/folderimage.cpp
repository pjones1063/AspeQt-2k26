/*
 * folderimage.cpp
 */

#include "folderimage.h"
#include "aspeqtsettings.h"

#include <QFileInfoList>
#include <QtDebug>
#include <QRegularExpression>


// CIRCULAR SECTORS USED FOR SERVING FILES FROM FOLDER IMAGES
// ==========================================================
// Circular sectors per file logic utilizes all sectors from 433 to 1023 for a total of 591 sectors.
// Sector number will cycle back to 433 once it hits 1023, and this cycle will repeat until the entire file is read.
// The same pool of sectors are used for every file in the Folder Image.
// First sector number of each file however is selected from a different pool of (369-432) so that they are unique for each file.
// This allows for dynamic calculation of the Atari file number within the code.
// Sector numbers (5, 6, 32-134) are reserved for SpartaDos boot process.

extern QString g_aspeQtAppPath;
extern bool g_disablePicoHiSpeed;

FolderImage::~FolderImage()
{
    close();
}

void FolderImage::close()
{
    for (int i = 0; i < 64; i++) {
        atariFiles[i].exists = false;
    }

    return;
}

bool FolderImage::format(const DiskGeometry&)
{
    return false;
}

// Return the long file name of a short Atari file name from a given (last mounted) Folder Image

QString FolderImage::longName(QString &lastMountedFolder, QString &atariFileName)
{
    if (FolderImage::open(lastMountedFolder, FileTypes::Dir)) {
        for (int i = 0; i < 64; i++) {
            if(atariFiles[i].atariName + "." + atariFiles[i].atariExt == atariFileName)
                return atariFiles[i].longName;
        }
     }
     return NULL;
}
void FolderImage::buildDirectory()
{
    QFileInfoList infos = dir.entryInfoList(QDir::Files,  QDir::Name);
    QFileInfo info;
    QString name, longName;
    QString ext;

    int j = -1, k, i;
    for (i = 0; i < 64; i++) {
        do {
            j++;
            if (j >= infos.count()) {
                atariFiles[i].exists = false;
                break;
            }
            info = infos.at(j);
            longName = info.completeBaseName();
            name = longName.toUpper();
            if(aspeqtSettings->filterUnderscore()) {
                name.remove(QRegularExpression("[^A-Z0-9]"));
            } else {
                name.remove(QRegularExpression("[^A-Z0-9_]"));
            }
            name = name.left(8);
            if (name.isEmpty()) {
                name = "BADNAME";
            }
            longName += "." + info.suffix();
            ext = info.suffix().toUpper();
            if(aspeqtSettings->filterUnderscore()) {
                ext.remove(QRegularExpression("[^A-Z0-9]"));
            } else {
                ext.remove(QRegularExpression("[^A-Z0-9_]"));
            }
            ext = ext.left(3);
            QString baseName = name.left(7);

            int l = 2;
            do {
                for (k = 0; k < i; k++) {
                    if (atariFiles[k].atariName == name && atariFiles[k].atariExt == ext) {
                        break;
                    }
                }
                if (k < i) {
                    name = QString("%1%2").arg(baseName).arg(l);
                    l++;
                }
            } while (k < i && l < 10000000);
            if (l == 10) {baseName = name.left(6);}
            if (l == 100) {baseName = name.left(5);}
            if (l == 1000) {baseName = name.left(4);}
            if (l == 10000) {baseName = name.left(3);}
            if (l == 100000) {baseName = name.left(2);}
            if (l == 1000000) {baseName = name.left(1);}
            if (l == 10000000) {baseName = "";}
            if (l == 100000000) {
                qWarning() << "!w" << tr("Cannot mirror '%1' in '%2': No suitable Atari name can be found.")
                               .arg(info.fileName())
                               .arg(dir.path());
            }
        } while (k < i);

        if (j >= infos.count()) {
            break;
        }

        atariFiles[i].exists = true;
        atariFiles[i].original = info;
        atariFiles[i].atariName = name;
        atariFiles[i].longName = longName;
        atariFiles[i].atariExt = ext;
        atariFiles[i].lastSector = 0;
        atariFiles[i].pos = 0;
        atariFiles[i].sectPass = 0;
    }

    // --- VIRTUAL FIRMWARE INJECTION ---
    bool foundDos = false;
    bool foundDup = false;
    bool foundAutorun = false;

    // 1. Scan what we just built
    for (int k = 0; k < i; k++) {
        if (atariFiles[k].atariName == "DOS" && atariFiles[k].atariExt == "SYS") foundDos = true;
        if (atariFiles[k].atariName == "DUP" && atariFiles[k].atariExt == "SYS") foundDup = true;
        if (atariFiles[k].atariName == "AUTORUN" && atariFiles[k].atariExt == "SYS") foundAutorun = true;
    }

    // 2. Inject DOS.SYS if missing
    if (!foundDos && i < 64) {
        atariFiles[i].exists = true;
        atariFiles[i].atariName = "DOS";
        atariFiles[i].atariExt = "SYS";
        atariFiles[i].longName = "DOS.SYS";
        atariFiles[i].original = QFileInfo(":/boot_templates/$bootmyd/dos.sys");
        atariFiles[i].lastSector = 0;
        atariFiles[i].pos = 0;
        atariFiles[i].sectPass = 0;
        i++;
    }

    // 3. Inject DUP.SYS if missing
    if (!foundDup && i < 64) {
        atariFiles[i].exists = true;
        atariFiles[i].atariName = "DUP";
        atariFiles[i].atariExt = "SYS";
        atariFiles[i].longName = "DUP.SYS";
        atariFiles[i].original = QFileInfo(":/boot_templates/$bootmyd/dup.sys");
        atariFiles[i].lastSector = 0;
        atariFiles[i].pos = 0;
        atariFiles[i].sectPass = 0;
        i++;
    }

    if (!foundAutorun && i < 64) {
        atariFiles[i].exists = true;
        atariFiles[i].atariName = "AUTORUN";
        atariFiles[i].atariExt = "SYS";
        atariFiles[i].longName = "AUTORUN.SYS";
        atariFiles[i].original = QFileInfo(":/boot_templates/$bootmyd/autorun.sys");
        atariFiles[i].lastSector = 0;
        atariFiles[i].pos = 0;
        atariFiles[i].sectPass = 0;
        i++;
    }

    // ---------------------------------

    if (i < infos.count()) {
        qWarning() << "!w" << tr("Cannot mirror %1 of %2 files in '%3': Atari directory is full.")
                       .arg(infos.count() - i)
                       .arg(infos.count())
                       .arg(dir.path());
    }
}

bool FolderImage::open(const QString &fileName, FileTypes::FileType /* type */)
{
    if (dir.exists(fileName)) {
        dir.setPath(fileName);

        buildDirectory();

        m_originalFileName = fileName;
        m_geometry.initialize(false, 40, 26, 128);
        m_newGeometry.initialize(m_geometry);
        m_isReadOnly = true;
        m_isModified = false;
        m_isUnmodifiable = true;
        return true;
    } else {
        return false;
    }
}

bool FolderImage::readSector(quint16 sector, QByteArray &data)
{
    /* Boot */
    QFile boot(dir.path() + "/$boot.bin");
    data = QByteArray(128, 0);
    int bootFileSector;

    if (sector == 1) {
         if (!boot.open(QFile::ReadOnly)) {
             boot.setFileName(":/boot_templates/$bootmyd/$boot.bin");
             if (!boot.open(QFile::ReadOnly)) {
                 return true; 
             }
         }

         data = boot.read(128);
         // SAFETY: Ensure data is exactly 128 bytes to prevent crashes on access
         if (data.size() < 128) data.resize(128);

         buildDirectory();

         for(int i=0; i<64; i++) {
             if(atariFiles[i].longName.toUpper() == "DOS.SYS") {
                 bootFileSector = 369 + i;
                 data[15] = bootFileSector % 256;
                 data[16] = bootFileSector / 256;
                 break;
             }
             if(atariFiles[i].longName.toUpper() == "PICODOS.SYS") {
                 bootFileSector = 369 + i;
                 if(g_disablePicoHiSpeed) {
                     data[15] = 0;
                 }
                 data[9] = bootFileSector % 256;
                 data[10] = bootFileSector / 256;
                 break;
             }
             if(atariFiles[i].longName.toUpper() == "X32.DOS") {
                 QFile x32Dos(dir.path() + "/x32.dos");
                 if(x32Dos.open(QFile::ReadOnly)) {
                     QByteArray flag;
                     flag = x32Dos.readAll();
                     if(flag[0] == '\xFF') {
                         flag[0] = '\x00';
                         data[1] = 0x01;
                         data[3] = 0x07;
                         data[4] = 0x40;
                         data[5] = 0x15;
                         data[6] = 0x4c;
                         data[7] = 0x14;
                         data[8] = 0x07;
                         data[0x14] = 0x38;
                         data[0x15] = 0x60;
                     }
                 }
               break;
             }
         }
         return true;
    }

    if (sector == 2 || sector == 3) {
        if (!QFile::exists(dir.path() + "/$boot.bin")) {
             boot.setFileName(":/boot_templates/$bootmyd/$boot.bin");
        }
        boot.open(QFile::ReadOnly);
        boot.seek((sector-1)*128);
        data = boot.read(128);
        // SAFETY: Ensure data is exactly 128 bytes
        if (data.size() < 128) data.resize(128);
        return true;
    }
    
    // SpartaDOS Boot
    if ((sector >= 32 && sector <= 134) || sector == 5 || sector == 6) {
        if (!QFile::exists(dir.path() + "/$boot.bin")) {
             boot.setFileName(":/boot_templates/$bootmyd/$boot.bin");
        }
        boot.open(QFile::ReadOnly);
        boot.seek((sector-1)*128);
        data = boot.read(128);
        if (data.size() < 128) data.resize(128);
        return true;
    }

    /* VTOC */
    if (sector == 360) {
        data = QByteArray(128, 0);
        data[0] = 2;
        data[1] = 1010 % 256;
        data[2] = 1010 / 256;
        data[10] = 0x7F;
        for (int i = 11; i < 100; i++) {
            data[i] = 0xff;
        }
        return true;
    }

    /* Directory sectors */
    if (sector >= 361 && sector <=368) {
        if (sector == 361) {
            buildDirectory();
        }
        data.resize(0);
        for (int i = (sector - 361) * 8; i < (sector - 360) * 8; i++) {
            QByteArray entry;
            if (!atariFiles[i].exists) {
                entry = QByteArray(16, 0);
            } else {
                // --- CRITICAL FIX FOR SIGABRT CRASH ---
                // Old code used entry = ""; entry[0] = 0x42; which crashes because size is 0.
                entry = QByteArray(16, 0); // Initialize with 16 bytes of zeros
                // --------------------------------------
                
                entry[0] = 0x42;
                QFileInfo info = atariFiles[i].original;;
                
                //  ---  If it is our injected resource, get size correctly ** FIX - remove hard coding **
                // if (atariFiles[i].longName == "DOS.SYS") info.setFile(":/boot_templates/$bootmyd/dos.sys");
                // if (atariFiles[i].longName == "DUP.SYS") info.setFile(":/boot_templates/$bootmyd/dup.sys");
                // if (atariFiles[i].longName == "AUTORUN.SYS") info.setFile(":/boot_templates/$bootmyd/autorun.sys");

                int size = (info.size() + 124) / 125;
                // FIX: Increased limit from 999 to 65535 to support large files (>125KB)
                if (size > 65535) {
                    size = 65535;
                }
                entry[1] = size % 256;
                entry[2] = size / 256;
                int first = 369 + i;
                entry[3] = first % 256;
                entry[4] = first / 256;
                entry.replace(5, atariFiles[i].atariName.length(), atariFiles[i].atariName.toLatin1());
                // Ensure spaces padding
                for(int x=5+atariFiles[i].atariName.length(); x<13; x++) entry[x] = 32;

                entry.replace(13, atariFiles[i].atariExt.length(), atariFiles[i].atariExt.toLatin1());
                // Ensure spaces padding
                for(int x=13+atariFiles[i].atariExt.length(); x<16; x++) entry[x] = 32;
            }
            data += entry;
        }
        return true;
    }

    /* Data sectors */

    /* First sector of the file */
        int size, next;
        if  (sector >= 369 && sector <= 432) {
            atariFileNo = sector - 369;
            if (!atariFiles[atariFileNo].exists) {
                data = QByteArray(128, 0);
                return true;
            }

            // --- INJECTION HANDLING ---
            QString loadPath = atariFiles[atariFileNo].original.absoluteFilePath();
            // if (atariFiles[atariFileNo].longName == "DOS.SYS") loadPath = ":/boot_templates/$bootmyd/dos.sys";
            // if (atariFiles[atariFileNo].longName == "DUP.SYS") loadPath = ":/boot_templates/$bootmyd/dup.sys";
            // if (atariFiles[atariFileNo].longName == "AUTORUN.SYS") loadPath = ":/boot_templates/$bootmyd/autorun.sys";
            // --------------------------

            QFile file(loadPath);
            if (!file.open(QFile::ReadOnly)) {
                 data = QByteArray(128, 0);
                 return true;
            }

            data = file.read(125);
            size = data.size();
            data.resize(128);
            if (file.atEnd()) {
                next = 0;
            }
            else {
                next = 433;
            }
            data[125] = (atariFileNo * 4) | (next / 256);
            data[126] = next % 256;
            data[127] = size;
            return true;
        }

    /* Rest of the file sectors */
        if ((sector >= 433 && sector <= 1023)) {
            QString loadPath = atariFiles[atariFileNo].original.absoluteFilePath();   // ** FIX ** - remove hard coding
            // if (atariFiles[atariFileNo].longName == "DOS.SYS") loadPath = ":/boot_templates/$bootmyd/dos.sys";
            // if (atariFiles[atariFileNo].longName == "DUP.SYS") loadPath = ":/boot_templates/$bootmyd/dup.sys";
            // if (atariFiles[atariFileNo].longName == "AUTORUN.SYS") loadPath = ":/boot_templates/$bootmyd/autorun.sys";

            QFile file(loadPath);
            if (!file.open(QFile::ReadOnly)) {
                data = QByteArray(128, 0);
                return true;
            }
            
            atariFiles[atariFileNo].pos = (125+((sector-433)*125))+(atariFiles[atariFileNo].sectPass*73875);
            file.seek(atariFiles[atariFileNo].pos);
            data = file.read(125);
            next = sector + 1;
            if (sector == 1023) {
                next = 433;
                atariFiles[atariFileNo].sectPass += 1;
            }
            size = data.size();
            data.resize(128);
            atariFiles[atariFileNo].lastSector = sector;
            if (file.atEnd()) {
                next = 0;
            }
            data[125] = (atariFileNo * 4) | (next / 256);
            data[126] = next % 256;
            data[127] = size;
            return true;
        }

    /* Any other sector */

        data = QByteArray(128, 0);
        return true;
}

bool FolderImage::writeSector(quint16, const QByteArray &)
{
    return false;
}
