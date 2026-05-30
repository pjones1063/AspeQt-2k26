/*
 * inetworkclient.h
 * Universal Network Interface for AspeQt-2k26
 */

#ifndef INETWORKCLIENT_H
#define INETWORKCLIENT_H

#include <QObject>
#include <QString>
#include <QList>
#include <QByteArray>

class INetworkClient : public QObject
{
    Q_OBJECT
public:
    explicit INetworkClient(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~INetworkClient() = default;

    // Extracted from TnfsClient so both TNFS and FTP can share the same struct
    struct DirectoryEntry {
        QString name;
        bool isDirectory;
    };

    // --- CONNECTION ---
    // Port has a default value of 0 here so the specific client (TNFS=16384, FTP=21)
    // can handle its own fallback logic if the user doesn't provide one in the URL.
    virtual bool connectToHost(const QString &host, quint16 port = 0) = 0;

    // --- DIRECTORY BROWSING ---
    virtual bool beginListing(const QString &path) = 0;
    virtual QList<DirectoryEntry> fetchNextBatch(int count) = 0;
    virtual void endListing() = 0;
    virtual bool isListingFinished() const = 0;

    // --- FILE I/O (Designed for SIO Emulation) ---
    virtual quint32 getFileSize(const QString &path) = 0;

    // Note: TNFS natively uses a quint8 handle. For FTP, we will just return a dummy
    // handle (like 0x01) on success, and keep track of the active download state internally,
    // so we don't have to rewrite the way TnfsImage.cpp tracks open files.
    virtual quint8 openFile(const QString &path) = 0;
    virtual quint32 getFileSize(quint8 handle) = 0;
    virtual QByteArray readFile(quint8 handle, quint32 offset, quint32 size) = 0;
    virtual void closeFile(quint8 handle) = 0;
};

#endif // INETWORKCLIENT_H
