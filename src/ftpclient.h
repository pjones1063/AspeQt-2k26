/*
 * ftpclient.h
 * Lightweight FTP Client for AspeQt-2k26
 */
#ifndef FTPCLIENT_H
#define FTPCLIENT_H

#include "inetworkclient.h"
#include <QTcpSocket>
#include <QStringList>

class FtpClient : public INetworkClient
{
    Q_OBJECT
public:
    explicit FtpClient(QObject *parent = nullptr);
    ~FtpClient() override;

    bool connectToHost(const QString &host, quint16 port = 0) override;

    void setCredentials(const QString &user, const QString &pass);

    bool beginListing(const QString &path) override;
    QList<DirectoryEntry> fetchNextBatch(int count) override;
    void endListing() override;
    bool isListingFinished() const override { return m_listingFinished; }

    quint32 getFileSize(const QString &path) override;
    quint32 getFileSize(quint8 handle) override;
    quint8 openFile(const QString &path) override;

    Q_INVOKABLE QByteArray readFile(quint8 handle, quint32 offset, quint32 size) override;
    Q_INVOKABLE void closeFile(quint8 handle) override;

private:
    QTcpSocket *m_controlSocket;
    QTcpSocket *m_dataSocket;

    bool m_listingFinished;
    QString m_currentPath;

    QString m_user;
    QString m_pass;

    QStringList m_directoryCache;

    int sendCommand(const QString &cmd, QString &response);
    bool enterPassiveMode(QString &ip, quint16 &port);
    void cleanupDataSocket();
};

#endif // FTPCLIENT_H