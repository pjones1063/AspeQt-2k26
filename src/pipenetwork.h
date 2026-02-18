/*
 * pipenetwork.h
 * Network Streaming Device (W:) for AspeQt
 */

#ifndef PIPENETWORK_H
#define PIPENETWORK_H

#include "sioworker.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QProcess> // Added for system curl support (FTP)

class PipeNetwork : public SioDevice
{
    Q_OBJECT
public:
    explicit PipeNetwork(SioWorker *worker);
    ~PipeNetwork();

    void handleCommand(quint8 command, quint16 aux) override;

private:
    QNetworkAccessManager *m_manager;
    QNetworkReply *m_reply;
    QProcess *m_process; // Helper process for protocols Qt6 dropped (FTP)

    // Circular-ish buffer for incoming stream data
    QByteArray m_rxBuffer;

    // Accumulator for outgoing (Write) data
    QByteArray m_txAccumulator;

    bool m_netFinished;
    bool m_isWriteMode;
    bool m_sessionTranslate;
    QString m_lastUrl;

    void reset();
    QString cleanUrl(QString raw);
    bool shouldTranslate(quint16 aux, bool globalSetting);

signals:
    void sendFireAndForget(QString url, QByteArray data);

};

#endif // PIPENETWORK_H
