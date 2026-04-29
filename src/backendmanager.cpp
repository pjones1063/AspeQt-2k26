#include "backendmanager.h"
#include <QDir>

BackendManager::BackendManager(QObject *parent) : QObject(parent)
{
    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &BackendManager::onDirectoryChanged);
    loadLibrary();
}

BackendManager::~BackendManager()
{
    // Ensure no zombie Python processes are left running when AspeQt closes
    shutdownAll();
}

void BackendManager::loadLibrary()
{
    QList<BackendConfig> configs = m_configManager.loadConfigurations();

    // 1. Find apps that were deleted from disk and stop them cleanly
    QStringList newIds;
    for (const BackendConfig& c : configs) newIds.append(c.id);

    QStringList currentIds = m_instances.keys();
    for (const QString& id : currentIds) {
        if (!newIds.contains(id)) {
            stopBackend(id); // Safely spin down the process
            m_instances.remove(id);
        }
    }

    // 2. Add new apps or update existing ones
    for (const BackendConfig& config : configs) {
        if (m_instances.contains(config.id)) {
            // Safely update config in place without destroying a running QProcess
            m_instances[config.id].config = config;
        } else {
            // It's a brand new app
            BackendInstance instance;
            instance.config = config;
            instance.process = nullptr;
            instance.logFile = nullptr;
            m_instances.insert(config.id, instance);
        }
    }
}


void BackendManager::startAllAutoStart()
{
    for (const BackendInstance& instance : std::as_const(m_instances)) {
        if (instance.config.autoStart) {
            startBackend(instance.config.id);
        }
    }
}

QList<BackendConfig> BackendManager::getConfigurations() const
{
    QList<BackendConfig> list;
    for (const BackendInstance& instance : std::as_const(m_instances)) {
        list.append(instance.config);
    }
    return list;
}

bool BackendManager::isRunning(const QString& id) const
{
    if (!m_instances.contains(id)) return false;
    QProcess* p = m_instances.value(id).process;
    return p && p->state() == QProcess::Running;
}

void BackendManager::startBackend(const QString& id)
{
    if (!m_instances.contains(id)) return;

    BackendInstance& instance = m_instances[id];

    // 1. Prevent double-starting
    if (instance.process && instance.process->state() != QProcess::NotRunning) {
        return;
    }

    // 2. Clean up old QProcess and log file objects if they exist
    if (instance.process) {
        instance.process->deleteLater();
    }
    if (instance.logFile) {
        if (instance.logFile->isOpen()) instance.logFile->close();
        delete instance.logFile;
        instance.logFile = nullptr;
    }

    instance.process = new QProcess(this);

    // 3. Configure Working Directory
    if (!instance.config.workingDirectory.isEmpty() && QDir(instance.config.workingDirectory).exists()) {
        instance.process->setWorkingDirectory(instance.config.workingDirectory);
    }

    m_watcher->addPath(instance.config.workingDirectory);
    m_watchMap.insert(instance.config.workingDirectory, id);

    // 4. Inject Custom Environment Variables
    if (!instance.config.environment.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QMapIterator<QString, QString> iter(instance.config.environment);
        while (iter.hasNext()) {
            iter.next();
            env.insert(iter.key(), iter.value());
        }
        instance.process->setProcessEnvironment(env);
    }

    // 5. Setup Dedicated Log File
    QString safeName = instance.config.name;
    safeName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
    QString logPath = QDir::temp().absoluteFilePath(QString("aspeqt_backend_%1.log").arg(safeName));

    instance.logFile = new QFile(logPath, instance.process);
    if (instance.logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QString startHeader = QString("\n=== Started %1 at %2 ===\n")
        .arg(instance.config.name)
            .arg(QDateTime::currentDateTime().toString());
        instance.logFile->write(startHeader.toUtf8());
        instance.logFile->flush();
    }

    // 6. Map Asynchronous Signals
    connect(instance.process, &QProcess::readyReadStandardOutput, this, &BackendManager::onProcessReadyReadStandardOutput);
    connect(instance.process, &QProcess::readyReadStandardError, this, &BackendManager::onProcessReadyReadStandardError);
    connect(instance.process, &QProcess::stateChanged, this, &BackendManager::onProcessStateChanged);
    connect(instance.process, &QProcess::errorOccurred, this, &BackendManager::onProcessErrorOccurred);

    // 7. Format Arguments and Execute
    QStringList args = instance.config.arguments.split(" ", Qt::SkipEmptyParts);
    QString execCommand = instance.config.command;

    // --- NEW: Virtual Environment Override ---
    if (!instance.config.virtualEnvPath.isEmpty()) {
        QDir venvDir(instance.config.virtualEnvPath);

#ifdef Q_OS_WIN
        // Windows venv structure
        QString binPath = venvDir.absoluteFilePath("Scripts/" + execCommand);
        if (!binPath.endsWith(".exe")) binPath += ".exe";
#else
        QString binPath = venvDir.absoluteFilePath("bin/" + execCommand);
#endif

        if (QFile::exists(binPath)) {
            execCommand = binPath; // Override "python3" with "/path/to/venv/bin/python3"
        } else {
            emit logMessage('w', QString("[W: Backend] Warning: Could not find %1 in virtual environment. Falling back to system command.").arg(instance.config.command));
        }
    }
    emit logMessage('i', QString("[W: Backend] Starting '%1'").arg(instance.config.name));
    instance.process->start(execCommand, args); // Use the overridden command
}


void BackendManager::stopBackend(const QString& id)
{
    if (!m_instances.contains(id)) return;

    BackendInstance& instance = m_instances[id];

    if (!instance.config.workingDirectory.isEmpty()) {
        m_watchMap.remove(instance.config.workingDirectory, id);
        if (!m_watchMap.contains(instance.config.workingDirectory)) {
            m_watcher->removePath(instance.config.workingDirectory);
        }
    }

    if (instance.process && instance.process->state() != QProcess::NotRunning) {
        emit logMessage('w', QString("[W: Backend] Stopping '%1'").arg(instance.config.name));

        instance.process->terminate(); // Ask nicely first

        // Give it 2 seconds to clean up its sockets/files
        if (!instance.process->waitForFinished(2000)) {
            instance.process->kill(); // Force kill if hung
        }
    }

    if (instance.logFile) {
        if (instance.logFile->isOpen()) {
            QString stopHeader = QString("=== Stopped at %1 ===\n")
            .arg(QDateTime::currentDateTime().toString());
            instance.logFile->write(stopHeader.toUtf8());
            instance.logFile->close();
        }
        instance.logFile = nullptr; // QProcess parent handles actual deletion
    }
}

void BackendManager::shutdownAll()
{
    for (const QString& id : m_instances.keys()) {
        stopBackend(id);
    }
}

QString BackendManager::getIdByProcess(QProcess* process) const
{
    for (auto it = m_instances.constBegin(); it != m_instances.constEnd(); ++it) {
        if (it.value().process == process) {
            return it.key();
        }
    }
    return QString();
}

// --- Signal Handlers ---

void BackendManager::onProcessReadyReadStandardOutput()
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    QString id = getIdByProcess(process);
    if (id.isEmpty()) return;

    BackendInstance& instance = m_instances[id]; // Get the instance to access the log file

    while (process->canReadLine()) {
        QString line = QString::fromLocal8Bit(process->readLine()).trimmed();
        if (!line.isEmpty()) {

            // 1. Write to the dedicated backend log file
            if (instance.logFile && instance.logFile->isOpen()) {
                instance.logFile->write((line + "\n").toUtf8());
                instance.logFile->flush(); // Force write to disk immediately
            }

            // 2. Send to the main AspeQt logger
            emit logMessage('n', QString("[%1] %2").arg(instance.config.name, line));

            // 3. Send to Live Console
            emit backendOutputLine(id, line, false);
        }
    }
}


void BackendManager::onProcessReadyReadStandardError()
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    QString id = getIdByProcess(process);
    if (id.isEmpty()) return;

    BackendInstance& instance = m_instances[id];

    while (process->canReadLine()) {
        QString line = QString::fromLocal8Bit(process->readLine()).trimmed();
        if (!line.isEmpty()) {

            // 1. Write to the dedicated backend log file (Prefix with ERROR)
            if (instance.logFile && instance.logFile->isOpen()) {
                instance.logFile->write(("[ERROR] " + line + "\n").toUtf8());
                instance.logFile->flush();
            }

            // 2. Send to the main AspeQt logger in Red ('e')
            emit logMessage('e', QString("[%1] %2").arg(instance.config.name, line));

            // 3. Send to Live Console
            emit backendOutputLine(id, line, true);
        }
    }
}


void BackendManager::onProcessStateChanged(QProcess::ProcessState state)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    QString id = getIdByProcess(process);
    if (!id.isEmpty()) {
        emit backendStateChanged(id, state == QProcess::Running);
    }
}

void BackendManager::onProcessErrorOccurred(QProcess::ProcessError error)
{
    QProcess* process = qobject_cast<QProcess*>(sender());
    if (!process) return;

    QString id = getIdByProcess(process);
    if (!id.isEmpty()) {
        QString name = m_instances[id].config.name;
        emit logMessage('e', QString("[W: Backend] Process '%1' encountered an error: %2")
                                 .arg(name, process->errorString()));
    }
}

void BackendManager::onDirectoryChanged(const QString& path)
{
    // A file in the working directory was saved/modified!
    QList<QString> affectedIds = m_watchMap.values(path);

    for (const QString& id : affectedIds) {
        if (isRunning(id)) {
            emit logMessage('w', QString("[W: Backend] File change detected in '%1'. Hot-reloading...").arg(m_instances[id].config.name));

            // Cleanly restart it
            stopBackend(id);
            startBackend(id);
        }
    }
}
