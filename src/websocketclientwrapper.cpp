#include "websocketclientwrapper.h"
#include <QWebSocketServer>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>

// --- Transport Implementation ---
WebSocketTransport::WebSocketTransport(QWebSocket *socket)
    : QWebChannelAbstractTransport(socket), m_socket(socket)
{
    connect(socket, &QWebSocket::textMessageReceived, this, &WebSocketTransport::textMessageReceived);
    connect(socket, &QWebSocket::disconnected, this, &WebSocketTransport::deleteLater);
}

WebSocketTransport::~WebSocketTransport()
{
    m_socket->deleteLater();
}

void WebSocketTransport::sendMessage(const QJsonObject &message)
{
    QJsonDocument doc(message);
    m_socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void WebSocketTransport::textMessageReceived(const QString &message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;
    emit messageReceived(doc.object(), this);
}

// --- Wrapper Implementation ---
WebSocketClientWrapper::WebSocketClientWrapper(QWebSocketServer *server, QObject *parent)
    : QObject(parent), m_server(server)
{
    connect(server, &QWebSocketServer::newConnection, this, &WebSocketClientWrapper::handleNewConnection);
}

void WebSocketClientWrapper::handleNewConnection()
{
    QWebSocket *socket = m_server->nextPendingConnection();
    if (socket) {
        WebSocketTransport *transport = new WebSocketTransport(socket);
        emit clientConnected(transport);
    }
}
