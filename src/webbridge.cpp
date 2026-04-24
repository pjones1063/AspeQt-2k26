#include <QMetaObject>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QDomDocument>
#include <QFile>
#include <QSettings>
#include <QtConcurrent>

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

    // --- [NEW] Setup the Async Watcher instead of a QTimer ---
    m_tnfsWatcher = new QFutureWatcher<QList<TnfsClient::DirectoryEntry>>(this);
    connect(m_tnfsWatcher, &QFutureWatcherBase::finished, this, &WebBridge::onTnfsBatchFetched);
}

// -----------------------------------------------------------------
// UI ROUTING LOGIC (RESTORED NAMES + QUEUED CONNECTION FIX)
// -----------------------------------------------------------------
void WebBridge::mountDiskUi(int slot) { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "on_actionMountDisk_triggered", Qt::QueuedConnection, Q_ARG(int, slot)); }
void WebBridge::mountFolderUi(int slot) { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "on_actionMountFolder_triggered", Qt::QueuedConnection, Q_ARG(int, slot)); }
void WebBridge::ejectDiskUi(int slot) { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "ejectHeadless", Qt::QueuedConnection, Q_ARG(int, slot)); }
void WebBridge::saveDiskUi(int slot) { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "on_actionSave_triggered", Qt::QueuedConnection, Q_ARG(int, slot)); }
void WebBridge::setHappyModeUi(int slot, bool enabled) { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "handle_actionHappyMode_triggered", Qt::QueuedConnection, Q_ARG(int, slot), Q_ARG(bool, enabled)); }
void WebBridge::toggleAutoSaveUi(int slot) { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "toggleAutoSaveHeadless", Qt::QueuedConnection, Q_ARG(int, slot)); }
void WebBridge::swapDiskUi(int slot) { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "handle_actionSwap_triggered", Qt::QueuedConnection, Q_ARG(int, slot)); }

void WebBridge::hangupUi() { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "hangupModem", Qt::QueuedConnection); }
void WebBridge::macroUserUi() { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "sendMacroUser", Qt::QueuedConnection); }
void WebBridge::macroPassUi() { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "sendMacroPass", Qt::QueuedConnection); }

void WebBridge::toggleEmulationUi() { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "toggleEmulationHeadless", Qt::QueuedConnection); }
void WebBridge::togglePrinterUi() { if (mainWindow) QMetaObject::invokeMethod(mainWindow, "togglePrinterHeadless", Qt::QueuedConnection); }


void WebBridge::requestFullStatus() {
    if (mainWindow) QMetaObject::invokeMethod(mainWindow, "refreshWebUi", Qt::QueuedConnection);

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
        emit directoryListReceived(slot, targetPath, list);
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
    QString pbPath = aspeqtSettings->modemBridgePhonebookPath();
    if (pbPath.isEmpty()) pbPath = g_aspeQtAppPath + "/phonebook.xml";

    QJsonArray list;
    QFile file(pbPath);

    if (file.open(QIODevice::ReadOnly)) {
        QDomDocument doc;

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
        emit notificationReceived(tr("No phonebook found at: %1").arg(pbPath), true);
    }

    emit phonebookListReceived(list);
}


void WebBridge::dialBbsUi(const QString &name, const QString &ip, int port, const QString &protocol, const QString &login, const QString &password) {
    if (mainWindow) {
        QMetaObject::invokeMethod(mainWindow, "dialBbsSilent", Qt::QueuedConnection,
                                  Q_ARG(QString, name), Q_ARG(QString, ip), Q_ARG(int, port),
                                  Q_ARG(QString, protocol), Q_ARG(QString, login), Q_ARG(QString, password));
    }
}

void WebBridge::requestLogTextUi() {
    if (mainWindow) {
        // Since we are waiting on a return value, we don't queue this specific invoke
        QString logData = mainWindow->getLogText();
        emit logTextReceived(logData);
    }
}

// -----------------------------------------------------------------
// WEB TNFS BROWSER LOGIC (Async Background Streamer)
// -----------------------------------------------------------------

void WebBridge::requestTnfsDirectoryList(int slot, const QString &host, const QString &path) {
    m_tnfsSlot = slot;
    m_tnfsHost = host;
    m_tnfsPath = path;

    if (m_tnfsClient->connectToHost(host)) {

        QSettings settings("AspeQt", "TNFS");
        QStringList savedHosts = settings.value("hostHistory").toStringList();
        if (!savedHosts.contains(host)) {
            savedHosts.prepend(host);
            settings.setValue("hostHistory", savedHosts);
            emit tnfsHostHistoryReceived(savedHosts);
        }

        if (m_tnfsClient->mount("/")) {
            if (m_tnfsClient->beginListing(path)) {
                triggerNextTnfsBatch();
                return;
            } else {
                emit notificationReceived(tr("TNFS path not found: %1").arg(path), true);
                emit tnfsDirectoryListReceived(slot, host, path, QJsonArray(), true);
                return;
            }
        } else {
            emit notificationReceived(tr("Failed to mount TNFS host: %1").arg(host), true);
            emit tnfsDirectoryListReceived(slot, host, path, QJsonArray(), true);
            return;
        }
    }

    emit notificationReceived(tr("Failed to connect to TNFS host: %1").arg(host), true);
    emit tnfsDirectoryListReceived(slot, host, path, QJsonArray(), true);
}

void WebBridge::triggerNextTnfsBatch() {
    QFuture<QList<TnfsClient::DirectoryEntry>> future = QtConcurrent::run([this]() {
        return m_tnfsClient->fetchNextBatch(20);
    });
    m_tnfsWatcher->setFuture(future);
}

void WebBridge::onTnfsBatchFetched() {
    auto batch = m_tnfsWatcher->result();

    QJsonArray list;
    for (const auto &entry : batch) {
        QJsonObject item;
        item["name"] = entry.name;
        item["isDir"] = entry.isDirectory;
        list.append(item);
    }

    bool finished = m_tnfsClient->isListingFinished();

    emit tnfsDirectoryListReceived(m_tnfsSlot, m_tnfsHost, m_tnfsPath, list, finished);

    if (!finished) {
        triggerNextTnfsBatch();
    }
}

// -----------------------------------------------------------------
// CAS, PRINTER, AND DISK OPERATIONS
// -----------------------------------------------------------------

void WebBridge::mountTnfsSilentUi(int slot, const QString &url) {
    if (mainWindow) {
        QMetaObject::invokeMethod(mainWindow, "mountTnfsHeadless", Qt::QueuedConnection, Q_ARG(int, slot), Q_ARG(QString, url));
    }
}

void WebBridge::mountCasSilentUi(const QString &filePath) {
    if (mainWindow) QMetaObject::invokeMethod(mainWindow, "mountCasHeadless", Qt::QueuedConnection, Q_ARG(QString, filePath));
}

void WebBridge::playCasUi() {
    if (mainWindow) QMetaObject::invokeMethod(mainWindow, "playCasHeadless", Qt::QueuedConnection);
}

void WebBridge::rewindCasUi() {
    if (mainWindow) QMetaObject::invokeMethod(mainWindow, "rewindCasHeadless", Qt::QueuedConnection);
}

void WebBridge::ejectCasUi() {
    if (mainWindow) QMetaObject::invokeMethod(mainWindow, "ejectCasHeadless", Qt::QueuedConnection);
}

void WebBridge::toggleWriteProtectUi(int slot, bool enabled) {
    if (mainWindow)  QMetaObject::invokeMethod(mainWindow, "toggleWriteProtectHeadless", Qt::QueuedConnection, Q_ARG(int, slot), Q_ARG(bool, enabled));
}

void WebBridge::requestPrinterImageUi() {
    if (mainWindow) {
        QString base64 = mainWindow->getPrinterImageBase64();
        emit printerImageReceived(base64);
    }
}

void WebBridge::createBlankDiskUi(int slot, const QString &folder, const QString &filename, int type) {
    if (mainWindow) {
        QMetaObject::invokeMethod(mainWindow, "createBlankDiskHeadless", Qt::QueuedConnection, Q_ARG(int, slot), Q_ARG(QString, folder), Q_ARG(QString, filename), Q_ARG(int, type));
    }
}

void WebBridge::requestCurrentSavePathUi() {
    QString path = aspeqtSettings->lastDiskImageDir();
    if (path.isEmpty()) path = QDir::homePath();

    emit currentSavePathReceived(path);
}

void WebBridge::uploadAndMountUi(int slot, const QString &filename, const QString &base64Data) {
    if (mainWindow) {
        QMetaObject::invokeMethod(mainWindow, "uploadAndMountHeadless", Qt::QueuedConnection,
                                  Q_ARG(int, slot),
                                  Q_ARG(QString, filename),
                                  Q_ARG(QString, base64Data));
    }
}
