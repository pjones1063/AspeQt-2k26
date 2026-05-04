#ifndef BACKENDCONFIG_H
#define BACKENDCONFIG_H

#include <QString>
#include <QMap>

struct BackendConfig {
    QString id; // A unique identifier (UUID or timestamp)
    QString name;
    QString command; // e.g., "python3"
    QString arguments;
    QString workingDirectory;
    bool autoStart = false;
    QMap<QString, QString> environment;
    QString virtualEnvPath;
};

#endif // BACKENDCONFIG_H
