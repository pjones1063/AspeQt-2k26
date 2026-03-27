#ifndef WEBSOCKETCLIENTWRAPPER_H
#define WEBSOCKETCLIENTWRAPPER_H

#include <QObject>
#include <QWebChannelAbstractTransport>

class QWebSocketServer;
class QWebSocket;

// --- 1. The Transport Layer ---
// This translates WebSocket text messages into JSON objects for QWebChannel
class WebSocketTransport : public QWebChannelAbstractTransport
{
    Q_OBJECT
public:
    explicit WebSocketTransport(QWebSocket *socket);
    virtual ~WebSocketTransport();
    void sendMessage(const QJsonObject &message) override;

private slots:
    void textMessageReceived(const QString &message);

private:
    QWebSocket *m_socket;
};

// --- 2. The Server Wrapper ---
// This listens for new connections and wraps them in our Transport class
class WebSocketClientWrapper : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClientWrapper(QWebSocketServer *server, QObject *parent = nullptr);

signals:
    void clientConnected(WebSocketTransport *client);

private slots:
    void handleNewConnection();

private:
    QWebSocketServer *m_server;
};

#endif // WEBSOCKETCLIENTWRAPPER_H
