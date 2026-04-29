#ifndef BACKENDCONFIGMANAGER_H
#define BACKENDCONFIGMANAGER_H

#include <QSettings>
#include <QList>
#include "backendconfig.h" // Assuming the struct is here

class BackendConfigManager {
public:
    BackendConfigManager();

    // Load all saved backend apps from the QSettings array
    QList<BackendConfig> loadConfigurations() const;

    // Save the entire list back to disk
    void saveConfigurations(const QList<BackendConfig>& configs);

private:
    const QString GROUP_NAME = "BackendLibrary";
};

#endif // BACKENDCONFIGMANAGER_H
