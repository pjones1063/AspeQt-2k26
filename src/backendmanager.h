#ifndef BACKENDMANAGER_H
#define BACKENDMANAGER_H

#include <QObject>
#include <QProcess>
#include <QMap>
#include <QString>
#include <QList>
#include <QFile>
#include <QRegularExpression>
#include <QDateTime>
#include <QDir>
#include <QFileSystemWatcher>
#include <QMultiMap>


#include "backendconfigmanager.h" // Assume the config manager and BackendConfig struct are here

// Wrapper to hold the runtime state of a configured backend
struct BackendInstance {
    BackendConfig config;
    QProcess* process = nullptr;
    QFile* logFile = nullptr;
};

class BackendManager : public QObject
{
    Q_OBJECT

public:
    explicit BackendManager(QObject *parent = nullptr);
    ~BackendManager();

    void loadLibrary();
    void startAllAutoStart();

    void startBackend(const QString& id);
    void stopBackend(const QString& id);
    void shutdownAll();

    bool isRunning(const QString& id) const;
    QList<BackendConfig> getConfigurations() const;

signals:
    // Emitted when a process changes state (useful for UI buttons)
    void backendStateChanged(const QString& id, bool isRunning);
    void backendOutputLine(const QString& id, const QString& line, bool isError);

    // Connect this to MainWindow::doLogMessage
    void logMessage(int type, const QString& msg);

private slots:
    void onProcessReadyReadStandardOutput();
    void onProcessReadyReadStandardError();
    void onProcessStateChanged(QProcess::ProcessState state);
    void onProcessErrorOccurred(QProcess::ProcessError error);
    void onDirectoryChanged(const QString& path);

private:
    BackendConfigManager m_configManager;
    QMap<QString, BackendInstance> m_instances;
    QFileSystemWatcher* m_watcher;
    QMultiMap<QString, QString> m_watchMap;
    QString getIdByProcess(QProcess* process) const;
};

#endif // BACKENDMANAGER_H
