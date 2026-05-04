#ifndef BACKENDEDITDIALOG_H
#define BACKENDEDITDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QTableWidget>
#include "backendconfig.h"

class BackendEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BackendEditDialog(QWidget *parent = nullptr);

    // Load an existing config into the form
    void setConfig(const BackendConfig& config);

    // Extract the config from the form
    BackendConfig getConfig() const;

private slots:
    void browseDirectory();
    void addEnvRow();
    void removeEnvRow();
    void browseVenv();

private:
    QLineEdit* editName;
    QLineEdit* editCommand;
    QLineEdit* editArguments;
    QLineEdit* editWorkingDir;
    QCheckBox* chkAutoStart;
    QTableWidget* tableEnv;
    QPushButton* btnAddEnv;
    QPushButton* btnRemoveEnv;
    QLineEdit* editVenvPath;

    QString currentId; // Keep track of the ID so we don't overwrite it
};

#endif // BACKENDEDITDIALOG_H
