/*
 * pipenetwork.h
 * Network Streaming Device (W:) for AspeQt
 * Refactored Architecture: TCP, HTTP, and FTP Support
 */

#ifndef PIPENETWORK_H
#define PIPENETWORK_H

#include "sioworker.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QProcess>
#include <QTcpSocket>

class PipeNetwork : public SioDevice
{
    Q_OBJECT
public:
    explicit PipeNetwork(SioWorker *worker);
    ~PipeNetwork();

    void handleCommand(quint8 command, quint16 aux) override;

private:
    enum Protocol { ProtoNone, ProtoHttp, ProtoFtp, ProtoTcp };

    QNetworkAccessManager *m_manager;
    QNetworkReply         *m_reply;
    QProcess              *m_process;
    QTcpSocket            *m_tcpSocket;

    QByteArray m_rxBuffer;
    QByteArray m_txAccumulator;

    Protocol m_protocol;
    bool m_netFinished;
    bool m_isWriteMode;
    bool m_sessionTranslate;
    QString m_lastUrl;

    // Core Helpers
    void reset();
    QString cleanUrl(QString raw);
    bool shouldTranslate(quint16 aux, bool globalSetting);
    void appendRxData(QByteArray rawData);

    // Protocol Handlers
    void openTcpConnection(const QUrl &url);
    void openFtpConnection(const QString &urlStr);
    void openHttpConnection(const QUrl &url);

signals:
    void sendFireAndForget(QString url, QByteArray data);
};

#endif // PIPENETWORK_H
