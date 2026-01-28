#include "tnfsimage.h"
#include <QUrl>
#include <QThread> // Needed for moveToThread

TnfsImage::TnfsImage(SioWorker *worker) : SimpleDiskImage(worker)
{
    m_fileHandle = 0xFF;
    m_worker = worker; // <--- 1. SAVE THE WORKER POINTER
}

TnfsImage::~TnfsImage()
{
    if (m_fileHandle != 0xFF) {
        m_client.closeFile(m_fileHandle);
    }
}

bool TnfsImage::openUrl(const QString &url)
{
    QUrl qurl(url);
    QString fullPath = qurl.path(QUrl::ComponentFormattingOption::FullyDecoded);

    m_host = qurl.host();
    if (!m_client.connectToHost(m_host)) return false;

    // --- SETUP PHASE (Runs in Main Thread) ---

    // 1. Mount Root
    if (!m_client.mount("/")) {
        qDebug() << "!e TNFS Mount Root Failed";
        return false;
    }

    // 2. Open File
    if (fullPath.startsWith("/")) fullPath.remove(0, 1);

    m_fileHandle = m_client.openFile(fullPath);

    if (m_fileHandle == 0xFF) {
        qDebug() << "!e TNFS Open Failed for:" << fullPath;
        return false;
    }

    this->m_originalFileName = url;

    // --- THE HAND-OFF (The Fix) ---
    // Now that setup is done, we move the network client to the Worker Thread.
    // This allows readSector (which runs in the worker) to use the socket without blocking.
    if (m_worker && m_worker->thread()) {
        m_client.moveToThread(m_worker->thread());
        qDebug() << "TNFS: Network client moved to Worker Thread.";
    }

    return true;
}

bool TnfsImage::readSector(quint16 sector, QByteArray &data)
{
    if (m_fileHandle == 0xFF) return false;

    // This now runs in the Worker Thread, and since we moved m_client here,
    // the socket calls will work!
    quint32 offset = (sector - 1) * 128;
    data = m_client.readFile(m_fileHandle, offset, 128);

    return (data.size() == 128);
}
