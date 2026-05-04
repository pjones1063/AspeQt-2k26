#include "backendconfigmanager.h"

BackendConfigManager::BackendConfigManager() {}

QList<BackendConfig> BackendConfigManager::loadConfigurations() const {
    QList<BackendConfig> configs;
    QSettings settings; // Uses the default Org/App names set in main()

    int size = settings.beginReadArray(BACKEND_GROUP_NAME);
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        BackendConfig config;
        config.id = settings.value("id").toString();
        config.name = settings.value("name").toString();
        config.command = settings.value("command").toString();
        config.arguments = settings.value("arguments").toString();
        config.workingDirectory = settings.value("workingDirectory").toString();
        config.virtualEnvPath = settings.value("virtualEnvPath").toString();
        config.autoStart = settings.value("autoStart", false).toBool();
        settings.beginGroup("Environment");
        for (const QString& key : settings.childKeys()) {
            config.environment.insert(key, settings.value(key).toString());
        }
        settings.endGroup();
        configs.append(config);
    }
    settings.endArray();
    return configs;
}

void BackendConfigManager::saveConfigurations(const QList<BackendConfig>& configs) {
    QSettings settings;
    settings.beginWriteArray(BACKEND_GROUP_NAME);
    for (int i = 0; i < configs.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("id", configs[i].id);
        settings.setValue("name", configs[i].name);
        settings.setValue("command", configs[i].command);
        settings.setValue("arguments", configs[i].arguments);
        settings.setValue("workingDirectory", configs[i].workingDirectory);
        settings.setValue("virtualEnvPath", configs[i].virtualEnvPath);
        settings.setValue("autoStart", configs[i].autoStart);
        settings.beginGroup("Environment");
        settings.remove(""); // Clear old keys first just in case they deleted some
        QMapIterator<QString, QString> iter(configs[i].environment);
        while (iter.hasNext()) {
            iter.next();
            settings.setValue(iter.key(), iter.value());
        }
        settings.endGroup();

    }
    settings.endArray();
}
