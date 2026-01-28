#include "tnfsimage.h"
#include <QUrl>
#include <QMetaObject>
#include <QDebug> // Ensure this is included

TnfsImage::TnfsImage(SioWorker *worker) : SimpleDiskImage(worker)
{
    m_fileHandle = 0xFF;
    m_worker = worker;
    m_headerSkip = 0;
    m_client = new TnfsClient(nullptr); // Heap allocation, No Parent
}

TnfsImage::~TnfsImage()
{
    if (m_client) {
        if (m_fileHandle != 0xFF) {
            QMetaObject::invokeMethod(m_client, "closeFile", Qt::QueuedConnection, Q_ARG(quint8, m_fileHandle));
        }
        m_client->deleteLater();
    }
}

bool TnfsImage::openUrl(const QString &url)
{
    QUrl qurl(url);
    QString fullPath = qurl.path(QUrl::ComponentFormattingOption::FullyDecoded);

    m_host = qurl.host();

    // 1. Setup (Main Thread)
    if (!m_client->connectToHost(m_host)) return false;

    if (!m_client->mount("/")) {
        qWarning() << "TNFS: Mount Root Failed";
        return false;
    }

    if (fullPath.startsWith("/")) fullPath.remove(0, 1);

    m_fileHandle = m_client->openFile(fullPath);

    if (m_fileHandle == 0xFF) {
        qWarning() << "TNFS: Open Failed for:" << fullPath;
        return false;
    }

    this->m_originalFileName = url;

    // --- ATR DETECTION ---
    m_headerSkip = 0;
    if (fullPath.endsWith(".atr", Qt::CaseInsensitive)) {
        m_headerSkip = 16;
        qWarning() << "TNFS: ATR Detected. Offset set to 16.";
    }

    // --- SANITY CHECK (The new test) ---
    // We try to read the first 16 bytes RIGHT NOW in the Main Thread.
    // This proves if the handle works before we even try the Thread Bridge.
    qWarning() << "TNFS: Performing sanity check read on handle" << m_fileHandle;
    QByteArray header = m_client->readFile(m_fileHandle, 0, 16);

    if (header.isEmpty()) {
        qWarning() << "TNFS: FATAL - Sanity check read FAILED. Handle is bad or network is down.";
        return false; // Fail early
    } else {
        qWarning() << "TNFS: Sanity check PASSED. First 4 bytes:" << header.left(4).toHex();
        // If this prints '9602...', it is definitely an ATR.
    }

    return true;
}

bool TnfsImage::readSector(quint16 sector, QByteArray &data)
{
    if (!m_client || m_fileHandle == 0xFF) {
        qWarning() << "TNFS: readSector failed - Invalid Handle!";
        return false;
    }

    // Apply Offset
    quint32 offset = ((sector - 1) * 128) + m_headerSkip;

    QByteArray result;

    // We explicitly cast the arguments to ensure Q_ARG matches the signature perfectly
    bool success = QMetaObject::invokeMethod(m_client, "readFile",
                                             Qt::BlockingQueuedConnection,
                                             Q_RETURN_ARG(QByteArray, result),
                                             Q_ARG(quint8, m_fileHandle),
                                             Q_ARG(quint32, offset),
                                             Q_ARG(quint16, (quint16)128));

    if (success && result.size() == 128) {
        data = result;
        // Success log only for sector 1 to avoid spam
        if (sector == 1) {
            qWarning() << "TNFS: Sector 1 Read SUCCESS. Data[0-3]:" << result.left(4).toHex();
        }
        return true;
    }

    // FAILURE LOGGING (Visible via qWarning)
    if (!success) {
        qWarning() << "TNFS: BRIDGE FAILED for Sector" << sector << "- invokeMethod returned false.";
    } else {
        qWarning() << "TNFS: SIZE MISMATCH for Sector" << sector << "- Got" << result.size() << "bytes.";
    }

    return false;
}
