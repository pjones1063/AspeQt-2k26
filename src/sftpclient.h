/*
 * sftpclient.h
 * Universal SFTP Client Wrapper for AspeQt-2k26
 */
#ifndef SFTPCLIENT_H
#define SFTPCLIENT_H

#include "inetworkclient.h"
#include "sshclient.h"
#include <QStringList>
#include <QByteArray>

class SftpClient : public INetworkClient
{
    Q_OBJECT
public:
    explicit SftpClient(QObject *parent = nullptr);
    ~SftpClient() override;

    bool connectToHost(const QString &host, quint16 port = 0) override;

    // UI Configuration
    void setCredentials(const QString &user, const QString &pass);

    // Directory Listing
    bool beginListing(const QString &path) override;
    QList<DirectoryEntry> fetchNextBatch(int count) override;
    void endListing() override;
    bool isListingFinished() const override { return m_listingFinished; }

    // File I/O
    quint32 getFileSize(const QString &path) override;
    quint32 getFileSize(quint8 handle) override;
    quint8 openFile(const QString &path) override;

    Q_INVOKABLE QByteArray readFile(quint8 handle, quint32 offset, quint32 size) override;
    Q_INVOKABLE void closeFile(quint8 handle) override;

private slots:
    void onRxData(const QByteArray &data);

private:
    SshClient *m_sshClient;

    // Auth State
    QString m_user;
    QString m_pass;

    // Directory State
    bool m_listingFinished;
    QList<DirectoryEntry> m_directoryCache;

    // Transfer Buffers
    QByteArray m_downloadBuffer;
    QByteArray m_networkBuffer; // Temporary accumulator for async signals

    void parseDirectoryData();
};

#endif // SFTPCLIENT_H
