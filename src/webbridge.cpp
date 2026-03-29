#include <QMetaObject>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QDomDocument>
#include <QFile>
#include <QSettings>

#include "webbridge.h"
#include "mainwindow.h"
#include "aspeqtsettings.h"

// Grab AspeQt's global variables so we can find the phonebook
extern AspeQtSettings *aspeqtSettings;
extern QString g_aspeQtAppPath;

WebBridge::WebBridge(MainWindow *mainWin, QObject *parent)
    : QObject(parent), mainWindow(mainWin)
{
    // Setup TNFS Background Streamer
    m_tnfsClient = new TnfsClient(this);
    m_tnfsTimer = new QTimer(this);
    connect(m_tnfsTimer, &QTimer::timeout, this, &WebBridge::fetchNextTnfsBatch);
}

void WebBridge::mountDiskUi(int slot) { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "on_actionMountDisk_triggered", Q_ARG(int, slot)); }
void WebBridge::mountFolderUi(int slot) { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "on_actionMountFolder_triggered", Q_ARG(int, slot)); }
void WebBridge::ejectDiskUi(int slot) { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "ejectHeadless", Q_ARG(int, slot)); }
void WebBridge::saveDiskUi(int slot) { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "on_actionSave_triggered", Q_ARG(int, slot)); }
void WebBridge::setHappyModeUi(int slot, bool enabled) { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "on_actionHappyMode_triggered", Q_ARG(int, slot), Q_ARG(bool, enabled)); }
void WebBridge::toggleAutoSaveUi(int slot) { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "toggleAutoSaveHeadless", Q_ARG(int, slot)); }

void WebBridge::hangupUi() { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "hangupModem"); }
void WebBridge::macroUserUi() { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "sendMacroUser"); }
void WebBridge::macroPassUi() { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "sendMacroPass"); }

void WebBridge::toggleEmulationUi() { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "toggleEmulationHeadless"); }
void WebBridge::togglePrinterUi() { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "togglePrinterHeadless"); }

void WebBridge::requestFullStatus() {
    if (mainWindow) QMetaObject::invokeMethod(mainWindow, "refreshWebUi");

    QSettings settings("AspeQt", "TNFS");
    QStringList savedHosts = settings.value("hostHistory").toStringList();
    emit tnfsHostHistoryReceived(savedHosts);

}

// -----------------------------------------------------------------
// WEB FILE BROWSER LOGIC
// -----------------------------------------------------------------
void WebBridge::requestDirectoryList(int slot, const QString &path) {
    QString targetPath = path;
    if (targetPath.isEmpty()) targetPath = QDir::homePath();

    QDir dir(targetPath);
    QJsonArray list;

    if (!dir.exists() || !dir.isReadable()) {
        emit notificationReceived(tr("Cannot access directory: %1").arg(targetPath), true);
        emit directoryListReceived(slot, targetPath, list); // Send empty list to clear UI
        return;
    }

    if (!dir.isRoot()) {
        QJsonObject up;
        up["name"] = "..";
        up["path"] = dir.absoluteFilePath("..");
        up["isDir"] = true;
        list.append(up);
    }

    QStringList filters;
    filters << "*.atr" << "*.xfd" << "*.pro" << "*.atx" << "*.xex" << "*.com" << "*.cas";

    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Dirs | QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name);

    for (const QFileInfo &info : fileList) {
        QJsonObject item;
        item["name"] = info.fileName();
        item["path"] = info.absoluteFilePath();
        item["isDir"] = info.isDir();
        list.append(item);
    }
    emit directoryListReceived(slot, dir.absolutePath(), list);
}

void WebBridge::mountFileSilentUi(int slot, const QString &filePath) {
    if (mainWindow) {
        QMetaObject::invokeMethod(mainWindow, "mountFileHeadless", Qt::QueuedConnection, Q_ARG(int, slot), Q_ARG(QString, filePath));
    }
}

// -----------------------------------------------------------------
// WEB PHONEBOOK LOGIC
// -----------------------------------------------------------------
void WebBridge::requestPhonebookList() {
    // 1. Find the phonebook path
    QString pbPath = aspeqtSettings->modemBridgePhonebookPath();
    if (pbPath.isEmpty()) pbPath = g_aspeQtAppPath + "/phonebook.xml";

    QJsonArray list;
    QFile file(pbPath);

    // 2. Parse the XML and build the JSON Array
    if (file.open(QIODevice::ReadOnly)) {
        QDomDocument doc;

        // --- NEW: Catch XML parsing errors! ---
        if (doc.setContent(&file)) {
            QDomNodeList bbsNodes = doc.elementsByTagName("BBS");
            for (int i = 0; i < bbsNodes.size(); i++) {
                QDomElement e = bbsNodes.at(i).toElement();
                QJsonObject item;
                item["name"] = e.attribute("name");
                item["ip"] = e.attribute("ip");
                item["port"] = e.attribute("port").toInt();
                item["protocol"] = e.attribute("protocol");
                item["login"] = e.attribute("login");
                item["password"] = e.attribute("password");
                list.append(item);
            }
        } else {
            emit notificationReceived(tr("Phonebook XML is corrupted or invalid!"), true);
        }
        file.close();
    } else {
        // --- NEW: Catch missing file errors! ---
        emit notificationReceived(tr("No phonebook found at: %1").arg(pbPath), true);
    }

    // 3. Send it to the Web UI!
    emit phonebookListReceived(list);
}


void WebBridge::dialBbsUi(const QString &name, const QString &ip, int port, const QString &protocol, const QString &login, const QString &password) {
    if (mainWindow) {
        QMetaObject::invokeMethod(mainWindow, "dialBbsSilent",
                                  Q_ARG(QString, name), Q_ARG(QString, ip), Q_ARG(int, port),
                                  Q_ARG(QString, protocol), Q_ARG(QString, login), Q_ARG(QString, password));
    }
}

void WebBridge::requestLogTextUi() {
    if (mainWindow) {
        QString logData = mainWindow->getLogText();
        emit logTextReceived(logData);
    }
}

// -----------------------------------------------------------------
// WEB TNFS BROWSER LOGIC (Streaming JSON)
// -----------------------------------------------------------------

void WebBridge::requestTnfsDirectoryList(int slot, const QString &host, const QString &path) {
    m_tnfsSlot = slot;
    m_tnfsHost = host;
    m_tnfsPath = path;

    // Connect and prepare the listing
    if (m_tnfsClient->connectToHost(host)) {

        QSettings settings("AspeQt", "TNFS");
        QStringList savedHosts = settings.value("hostHistory").toStringList();
        if (!savedHosts.contains(host)) {
            savedHosts.prepend(host); // Add to the top!
            settings.setValue("hostHistory", savedHosts);
            emit tnfsHostHistoryReceived(savedHosts);
        }

        if (m_tnfsClient->mount("/")) {
            if (m_tnfsClient->beginListing(path)) {
                // SUCCESS! Start pulling batches every 10ms
                m_tnfsTimer->start(10);
                return;
            } else {
                // --- Connected, but the specific Folder wasn't found ---
                emit notificationReceived(tr("TNFS path not found: %1").arg(path), true);
                emit tnfsDirectoryListReceived(slot, host, path, QJsonArray(), true);
                return;
            }
        } else {
            // --- Connected to IP, but the TNFS Mount command was rejected ---
            emit notificationReceived(tr("Failed to mount TNFS host: %1").arg(host), true);
            emit tnfsDirectoryListReceived(slot, host, path, QJsonArray(), true);
            return;
        }
    }

    // --- The IP address doesn't exist or timed out ---
    emit notificationReceived(tr("Failed to connect to TNFS host: %1").arg(host), true);
    emit tnfsDirectoryListReceived(slot, host, path, QJsonArray(), true);
}

void WebBridge::fetchNextTnfsBatch() {
    // Pull the next 20 files from the UDP socket
    QList<TnfsClient::DirectoryEntry> batch = m_tnfsClient->fetchNextBatch(20);

    QJsonArray list;
    for (const auto &entry : batch) {
        QJsonObject item;
        item["name"] = entry.name;
        item["isDir"] = entry.isDirectory;
        list.append(item);
    }

    bool finished = m_tnfsClient->isListingFinished();
    if (finished) {
        m_tnfsTimer->stop(); // We hit the bottom of the folder!
    }

    // Stream this batch to the phone
    emit tnfsDirectoryListReceived(m_tnfsSlot, m_tnfsHost, m_tnfsPath, list, finished);
}

void WebBridge::mountTnfsSilentUi(int slot, const QString &url) {
    // Forward the command to the Main Window headless mounter
    if (mainWindow) {
        QMetaObject::invokeMethod(mainWindow, "mountTnfsHeadless", Qt::QueuedConnection, Q_ARG(int, slot), Q_ARG(QString, url));
    }
}

void WebBridge::mountCasSilentUi(const QString &filePath) {
    if (mainWindow) QMetaObject::invokeMethod(mainWindow, "mountCasHeadless", Q_ARG(QString, filePath));
}

void WebBridge::playCasUi() {
    if (mainWindow) QMetaObject::invokeMethod(mainWindow, "playCasHeadless");
}

void WebBridge::rewindCasUi() {
    if (mainWindow) QMetaObject::invokeMethod(mainWindow, "rewindCasHeadless");
}

void WebBridge::ejectCasUi() {
    if (mainWindow) QMetaObject::invokeMethod(mainWindow, "ejectCasHeadless");
}

void WebBridge::toggleWriteProtectUi(int slot, bool enabled) {
    if (mainWindow)  QMetaObject::invokeMethod(mainWindow, "toggleWriteProtectHeadless", Qt::QueuedConnection, Q_ARG(int, slot), Q_ARG(bool, enabled));

}

void WebBridge::requestPrinterTextUi() {
    if (mainWindow) {
        QString text = mainWindow->getPrinterText();
        emit printerTextReceived(text);
    }
}

void WebBridge::createBlankDiskUi(int slot, const QString &filename, int type) {
    if (mainWindow) {
        QMetaObject::invokeMethod(mainWindow, "createBlankDiskHeadless", Qt::QueuedConnection, Q_ARG(int, slot), Q_ARG(QString, filename), Q_ARG(int, type));
    }
}

void WebBridge::requestCurrentSavePathUi() {
    // Grab the exact folder AspeQt is currently targeting
    QString path = aspeqtSettings->lastDiskImageDir();
    if (path.isEmpty()) path = QDir::homePath(); // Fallback

    emit currentSavePathReceived(path);
}
