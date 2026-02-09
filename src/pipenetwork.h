/*
 * pipenetwork.h
 * Network Streaming Device (N:) for AspeQt
 */

#ifndef PIPENETWORK_H
#define PIPENETWORK_H

#include "sioworker.h" // <--- User requested change
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>

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

    // Circular-ish buffer for incoming stream data
    QByteArray m_rxBuffer;

    // Accumulator for outgoing (Write) data
    QByteArray m_txAccumulator;

    bool m_netFinished;
    bool m_isWriteMode;
    QString m_lastUrl;

    void reset();
};

#endif // PIPENETWORK_H
