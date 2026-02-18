#ifndef BBSDATA_H
#define BBSDATA_H

#include <QString>

struct BbsEntry {
    QString name;
    QString ip;
    int port;
    QString protocol; // "TELNET", "SSH"
    QString login;
    QString password;
    QString font;
    QString keyMap;

    // Helper to get "ip:port"
    QString address() const { return ip + ":" + QString::number(port); }
};

#endif // BBSDATA_H
