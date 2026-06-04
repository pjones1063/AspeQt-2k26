#include "ftpclient.h"
#include <QRegularExpression>
#include <QDebug>
#include <QThread>

FtpClient::FtpClient(QObject *parent) : INetworkClient(parent)
{
    m_controlSocket = new QTcpSocket(this);
    m_dataSocket = nullptr;
    m_listingFinished = true;
}

FtpClient::~FtpClient()
{
    if (m_controlSocket->state() == QAbstractSocket::ConnectedState) {
        QString res;
        sendCommand("QUIT", res);
        m_controlSocket->close();
    }
    cleanupDataSocket();
}

void FtpClient::cleanupDataSocket()
{
    if (m_dataSocket) {
        if (m_dataSocket->state() == QAbstractSocket::ConnectedState) {
            m_dataSocket->close();
        }
        m_dataSocket->deleteLater();
        m_dataSocket = nullptr;
    }
}

void FtpClient::setCredentials(const QString &user, const QString &pass) {
    m_user = user;
    m_pass = pass;
}

int FtpClient::sendCommand(const QString &cmd, QString &response)
{
    if (!cmd.isEmpty()) {
        m_controlSocket->write((cmd + "\r\n").toUtf8());
        m_controlSocket->waitForBytesWritten(3000);
    }

    response.clear();

    while (true) {
        if (!m_controlSocket->waitForReadyRead(5000)) break;
        response += m_controlSocket->readAll();

        QStringList lines = response.split("\r\n", Qt::SkipEmptyParts);
        if (!lines.isEmpty()) {
            QString lastLine = lines.last();
            if (lastLine.length() >= 4 && lastLine.at(3) == ' ') {
                break;
            }
        }
    }

    if (response.length() >= 3) {
        return response.left(3).toInt();
    }
    return 0;
}

bool FtpClient::enterPassiveMode(QString &ip, quint16 &port)
{
    QString res;
    int code = sendCommand("PASV", res);
    if (code != 227) return false;

    QRegularExpression rx("\\((\\d+),(\\d+),(\\d+),(\\d+),(\\d+),(\\d+)\\)");
    QRegularExpressionMatch match = rx.match(res);

    if (match.hasMatch()) {
        ip = QString("%1.%2.%3.%4").arg(match.captured(1), match.captured(2), match.captured(3), match.captured(4));
        port = (match.captured(5).toUInt() << 8) + match.captured(6).toUInt();
        return true;
    }
    return false;
}

bool FtpClient::connectToHost(const QString &host, quint16 port)
{
    if (port == 0) port = 21;

    m_controlSocket->connectToHost(host, port);
    if (!m_controlSocket->waitForConnected(5000)) {
        qCritical() << "!e" << "FTP: Could not connect to control port.";
        return false;
    }

    QString res;
    int code = sendCommand("", res);
    if (code != 220 ) return false;

    QString actualUser = m_user.isEmpty() ? "anonymous" : m_user;
    QString actualPass = m_pass.isEmpty() ? "aspeqt@retro.net" : m_pass;

    code = sendCommand("USER " + actualUser, res);
    if (code == 331) {
        code = sendCommand("PASS " + actualPass, res);
    }

    if (code != 230 && code != 202) {
        qCritical() << "!e" << "FTP: Login failed. Server replied:" << res.trimmed();
        return false;
    }

    sendCommand("TYPE I", res);
    return true;
}

bool FtpClient::beginListing(const QString &path)
{
    m_directoryCache.clear();
    m_listingFinished = false;

    QString target = path.isEmpty() ? "/" : path;
    QString res;

    if (sendCommand("CWD " + target, res) >= 400) return false;

    QString ip;
    quint16 port;
    if (!enterPassiveMode(ip, port)) return false;

    cleanupDataSocket();
    m_dataSocket = new QTcpSocket(this);
    m_dataSocket->connectToHost(ip, port);
    if (!m_dataSocket->waitForConnected(3000)) return false;

    int code = sendCommand("LIST", res);
    if (code >= 400) return false;

    QByteArray rawList;
    while (m_dataSocket->waitForReadyRead(3000)) {
        rawList.append(m_dataSocket->readAll());
    }

    cleanupDataSocket();
    sendCommand("", res);

    QString listStr = QString::fromUtf8(rawList);
    m_directoryCache = listStr.split("\n", Qt::SkipEmptyParts);

    return true;
}

QList<INetworkClient::DirectoryEntry> FtpClient::fetchNextBatch(int count)
{
    QList<DirectoryEntry> entries;

    int processed = 0;
    while (!m_directoryCache.isEmpty() && processed < count) {
        QString line = m_directoryCache.takeFirst().trimmed();
        if (line.isEmpty()) continue;

        QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (tokens.size() < 4) continue;

        DirectoryEntry entry;
        bool isDir = false;

        if (tokens[0].startsWith("d") || tokens.contains("<DIR>")) {
            isDir = true;
        }

        int nameIndex = (tokens[0].startsWith("-") || tokens[0].startsWith("d")) ? 8 : 3;

        if (tokens.size() > nameIndex) {
            QStringList nameTokens = tokens.mid(nameIndex);
            entry.name = nameTokens.join(" ");

            if (entry.name != "." && entry.name != "..") {
                entry.isDirectory = isDir;
                entries.append(entry);
                processed++;
            }
        }
    }

    if (m_directoryCache.isEmpty()) {
        m_listingFinished = true;
    }

    return entries;
}

void FtpClient::endListing() {
    m_directoryCache.clear();
    m_listingFinished = true;
}

quint32 FtpClient::getFileSize(const QString &path)
{
    QString res;
    int code = sendCommand("SIZE " + path, res);
    if (code == 213) {
        return res.mid(4).trimmed().toUInt();
    }
    return 0;
}

quint32 FtpClient::getFileSize(quint8 /*handle*/) {
    return 0;
}

quint8 FtpClient::openFile(const QString &path)
{
    QString ip;
    quint16 port;
    if (!enterPassiveMode(ip, port)) return 0xFF;

    cleanupDataSocket();
    m_dataSocket = new QTcpSocket(this);
    m_dataSocket->connectToHost(ip, port);
    if (!m_dataSocket->waitForConnected(3000)) return 0xFF;

    QString res;
    int code = sendCommand("RETR " + path, res);
    if (code >= 400) {
        cleanupDataSocket();
        return 0xFF;
    }

    return 0x01;
}

QByteArray FtpClient::readFile(quint8 handle, quint32 offset, quint32 size)
{
    Q_UNUSED(handle);
    Q_UNUSED(offset);

    if (!m_dataSocket) return QByteArray();

    while (m_dataSocket->bytesAvailable() < size && m_dataSocket->state() == QAbstractSocket::ConnectedState) {
        m_dataSocket->waitForReadyRead(100);
    }

    return m_dataSocket->read(size);
}

void FtpClient::closeFile(quint8 handle)
{
    Q_UNUSED(handle);
    cleanupDataSocket();

    QString res;
    sendCommand("", res);
}
