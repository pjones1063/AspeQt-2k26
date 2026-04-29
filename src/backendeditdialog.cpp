#include "backendeditdialog.h"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QHeaderView>
#include <QUuid>

BackendEditDialog::BackendEditDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Edit Backend W: App"));
    setMinimumWidth(500);
    resize(600, 400); // Opens wider by default

    editName = new QLineEdit(this);
    editCommand = new QLineEdit(this);
    editCommand->setPlaceholderText("e.g., python3, node, or full path");

    editArguments = new QLineEdit(this);
    editArguments->setPlaceholderText("e.g., script.py --port 8080");

    editWorkingDir = new QLineEdit(this);
    QPushButton* btnBrowse = new QPushButton(tr("Browse..."), this);
    connect(btnBrowse, &QPushButton::clicked, this, &BackendEditDialog::browseDirectory);

    QHBoxLayout* dirLayout = new QHBoxLayout();
    dirLayout->addWidget(editWorkingDir);
    dirLayout->addWidget(btnBrowse);
    dirLayout->setContentsMargins(0, 0, 0, 0);

    chkAutoStart = new QCheckBox(tr("Start automatically with AspeQt"), this);

    // --- Environment Variables Table ---
    tableEnv = new QTableWidget(0, 2, this);
    tableEnv->setHorizontalHeaderLabels({tr("Key (e.g. PORT)"), tr("Value")});
    tableEnv->horizontalHeader()->setStretchLastSection(true);
    tableEnv->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableEnv->setSelectionMode(QAbstractItemView::SingleSelection);
    tableEnv->setFixedHeight(100); // Keep it compact

    btnAddEnv = new QPushButton("+", this);
    btnAddEnv->setFixedWidth(30);
    btnRemoveEnv = new QPushButton("-", this);
    btnRemoveEnv->setFixedWidth(30);

    connect(btnAddEnv, &QPushButton::clicked, this, &BackendEditDialog::addEnvRow);
    connect(btnRemoveEnv, &QPushButton::clicked, this, &BackendEditDialog::removeEnvRow);

    editVenvPath = new QLineEdit(this);
    editVenvPath->setPlaceholderText("Optional: Path to venv directory");
    QPushButton* btnBrowseVenv = new QPushButton(tr("Browse..."), this);
    connect(btnBrowseVenv, &QPushButton::clicked, this, &BackendEditDialog::browseVenv);

    QHBoxLayout* venvLayout = new QHBoxLayout();
    venvLayout->addWidget(editVenvPath);
    venvLayout->addWidget(btnBrowseVenv);
    venvLayout->setContentsMargins(0, 0, 0, 0);

    // Stack the buttons horizontally and push them to the right
    QHBoxLayout* envBtnLayout = new QHBoxLayout();
    envBtnLayout->addStretch(); // This forces the buttons to the right edge
    envBtnLayout->addWidget(btnAddEnv);
    envBtnLayout->addWidget(btnRemoveEnv);
    envBtnLayout->setContentsMargins(0, 0, 0, 0);

    // Stack the table and the buttons vertically
    QVBoxLayout* envMainLayout = new QVBoxLayout();
    envMainLayout->addWidget(tableEnv);
    envMainLayout->addLayout(envBtnLayout);
    envMainLayout->setContentsMargins(0, 0, 0, 0);

    // --- Main Form Assembly ---
    QFormLayout* formLayout = new QFormLayout();
    formLayout->addRow(tr("Name:"), editName);
    formLayout->addRow(tr("Command:"), editCommand);
    formLayout->addRow(tr("Arguments:"), editArguments);
    formLayout->addRow(tr("Working Dir:"), dirLayout);
    formLayout->addRow(tr("Environment:"), envMainLayout);
    formLayout->addRow(tr("Virtual Env:"), venvLayout);
    formLayout->addRow("", chkAutoStart);

    QPushButton* btnSave = new QPushButton(tr("Save"), this);
    QPushButton* btnCancel = new QPushButton(tr("Cancel"), this);
    connect(btnSave, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnSave);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(btnLayout);

    // Generate a new ID by default
    currentId = QUuid::createUuid().toString(QUuid::WithoutBraces);
}



void BackendEditDialog::browseDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Working Directory"), editWorkingDir->text());
    if (!dir.isEmpty()) {
        editWorkingDir->setText(dir);
    }
}

void BackendEditDialog::setConfig(const BackendConfig& config)
{
    currentId = config.id;
    editName->setText(config.name);
    editCommand->setText(config.command);
    editArguments->setText(config.arguments);
    editWorkingDir->setText(config.workingDirectory);
    chkAutoStart->setChecked(config.autoStart);
    tableEnv->setRowCount(0);
    for (auto it = config.environment.constBegin(); it != config.environment.constEnd(); ++it) {
        int row = tableEnv->rowCount();
        tableEnv->insertRow(row);
        tableEnv->setItem(row, 0, new QTableWidgetItem(it.key()));
        tableEnv->setItem(row, 1, new QTableWidgetItem(it.value()));
    }
}


BackendConfig BackendEditDialog::getConfig() const
{
    BackendConfig config;
    config.id = currentId;
    config.name = editName->text();
    config.command = editCommand->text();
    config.arguments = editArguments->text();
    config.workingDirectory = editWorkingDir->text();
    config.autoStart = chkAutoStart->isChecked();
    config.environment.clear();
    for (int i = 0; i < tableEnv->rowCount(); ++i) {
        QString key = tableEnv->item(i, 0) ? tableEnv->item(i, 0)->text().trimmed() : "";
        QString val = tableEnv->item(i, 1) ? tableEnv->item(i, 1)->text().trimmed() : "";

        if (!key.isEmpty()) {
            config.environment.insert(key, val);
        }
    }
    return config;
}

void BackendEditDialog::addEnvRow()
{
    int row = tableEnv->rowCount();
    tableEnv->insertRow(row);
    tableEnv->setItem(row, 0, new QTableWidgetItem(""));
    tableEnv->setItem(row, 1, new QTableWidgetItem(""));
    tableEnv->editItem(tableEnv->item(row, 0)); // Auto-focus the new row
}

void BackendEditDialog::removeEnvRow()
{
    int row = tableEnv->currentRow();
    if (row >= 0) {
        tableEnv->removeRow(row);
    }
}

void BackendEditDialog::browseVenv()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Virtual Environment Directory"), editVenvPath->text());
    if (!dir.isEmpty()) {
        editVenvPath->setText(dir);
    }
}
