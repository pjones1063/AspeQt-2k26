#include "backendlibrarydialog.h"
#include "backendeditdialog.h"
#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QMessageBox>
#include <QDesktopServices>
#include <QFormLayout>
#include <QUrl>
#include <QDir>

#include <QRegularExpression>


BackendLibraryDialog::BackendLibraryDialog(BackendManager* manager, QWidget *parent)
    : QDialog(parent), m_manager(manager)
{
    setWindowTitle(tr("Backend W: Apps Library"));
    setMinimumSize(600, 400);

    // --- 1. The Table ---
    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({tr("Status"), tr("Name"), tr("Command"), tr("Auto-Start")});
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // --- 2. The Toolbar Buttons (with Silk/Oxygen Icons) ---
    QPushButton* btnAdd = new QPushButton(QIcon(":/icons/silk-icons/icons/add.png"), tr("Add..."), this);
    QPushButton* btnEdit = new QPushButton(QIcon(":/icons/silk-icons/icons/wrench.png"), tr("Edit..."), this);
    QPushButton* btnCode = new QPushButton(QIcon(":/icons/silk-icons/icons/folder_edit.png"), tr("Open Code"), this);
    QPushButton* btnDelete = new QPushButton(QIcon(":/icons/silk-icons/icons/delete.png"), tr("Delete"), this);

    QPushButton* btnStart = new QPushButton(QIcon(":/icons/silk-icons/icons/connect.png"), tr("Start"), this);
    QPushButton* btnStop = new QPushButton(QIcon(":/icons/silk-icons/icons/disconnect.png"), tr("Stop"), this);
    QPushButton* btnViewLog = new QPushButton(QIcon(":/icons/silk-icons/icons/page_white_text.png"), tr("View Log"), this);
    QPushButton* btnMountDrivers = new QPushButton(QIcon(":/icons/oxygen-icons/16x16/devices/media_floppy.png"), tr("Mount W: Drivers (D1:)"), this);

    // --- 3. Button Connections ---
    connect(btnAdd, &QPushButton::clicked, this, &BackendLibraryDialog::onAddClicked);
    connect(btnEdit, &QPushButton::clicked, this, &BackendLibraryDialog::onEditClicked);
    connect(btnCode, &QPushButton::clicked, this, &BackendLibraryDialog::onEditSourceClicked);
    connect(btnDelete, &QPushButton::clicked, this, &BackendLibraryDialog::onDeleteClicked);

    connect(btnStart, &QPushButton::clicked, this, &BackendLibraryDialog::onStartClicked);
    connect(btnStop, &QPushButton::clicked, this, &BackendLibraryDialog::onStopClicked);
    connect(btnViewLog, &QPushButton::clicked, this, &BackendLibraryDialog::onViewLogClicked);
    connect(btnMountDrivers, &QPushButton::clicked, this, &BackendLibraryDialog::onMountDriversClicked);

    // Status updater connection
    connect(m_manager, &BackendManager::backendStateChanged, this, &BackendLibraryDialog::updateProcessStatus);

    // --- 4. Top Layout ---
    QHBoxLayout* topLayout = new QHBoxLayout();
    topLayout->addWidget(btnStart);
    topLayout->addWidget(btnStop);
    topLayout->addWidget(btnViewLog);
    topLayout->addWidget(btnMountDrivers);
    topLayout->addStretch();
    topLayout->addWidget(btnAdd);
    topLayout->addWidget(btnEdit);
    topLayout->addWidget(btnCode);
    topLayout->addWidget(btnDelete);

    // --- 5. The Live Console & Splitter ---
    m_console = new QTextEdit(this);
    m_console->setReadOnly(true);
    // Style it like a developer terminal
    m_console->setStyleSheet("background-color: #1e1e1e; color: #cccccc; font-family: monospace; font-size: 10pt;");

    m_splitter = new QSplitter(Qt::Vertical, this);
    m_splitter->addWidget(m_table);
    m_splitter->addWidget(m_console);
    m_splitter->setSizes({250, 150}); // Default to a 60/40 height split

    // --- 6. Main Layout ---
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_splitter); // Add the splitter instead of just the table

    // --- 7. Console Connections ---
    connect(m_table, &QTableWidget::itemSelectionChanged, this, &BackendLibraryDialog::onSelectionChanged);
    connect(m_manager, &BackendManager::backendOutputLine, this, &BackendLibraryDialog::onBackendOutputLine);

    refreshTable();
}



void BackendLibraryDialog::refreshTable()
{
    m_table->setRowCount(0);
    QList<BackendConfig> configs = m_manager->getConfigurations();

    for (int i = 0; i < configs.size(); ++i) {
        m_table->insertRow(i);

        bool isRunning = m_manager->isRunning(configs[i].id);
        QTableWidgetItem* statusItem = new QTableWidgetItem(isRunning ? tr("Running") : tr("Stopped"));
        statusItem->setForeground(isRunning ? Qt::darkGreen : Qt::darkRed);

        QTableWidgetItem* nameItem = new QTableWidgetItem(configs[i].name);
        nameItem->setData(Qt::UserRole, configs[i].id); // Secretly store the ID in the name item

        m_table->setItem(i, 0, statusItem);
        m_table->setItem(i, 1, nameItem);
        m_table->setItem(i, 2, new QTableWidgetItem(configs[i].command));
        m_table->setItem(i, 3, new QTableWidgetItem(configs[i].autoStart ? tr("Yes") : tr("No")));
    }
}

QString BackendLibraryDialog::getSelectedId() const
{
    int row = m_table->currentRow();
    if (row < 0) return QString();
    return m_table->item(row, 1)->data(Qt::UserRole).toString();
}

void BackendLibraryDialog::onAddClicked()
{
    BackendEditDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        BackendConfig newConfig = dlg.getConfig();
        // Since BackendManager doesn't have an "add" function yet, we extract the list, append, and save
        BackendConfigManager store;
        QList<BackendConfig> current = store.loadConfigurations();
        current.append(newConfig);
        store.saveConfigurations(current);

        m_manager->loadLibrary(); // Tell manager to reload
        refreshTable();
    }
}

void BackendLibraryDialog::onEditClicked()
{
    QString id = getSelectedId();
    if (id.isEmpty()) return;

    // Find the config
    QList<BackendConfig> configs = m_manager->getConfigurations();
    BackendConfig target;
    for (const auto& c : configs) { if (c.id == id) target = c; }

    BackendEditDialog dlg(this);
    dlg.setConfig(target);

    if (dlg.exec() == QDialog::Accepted) {
        BackendConfig updatedConfig = dlg.getConfig();

        // 1. Check if it is currently running, and stop it BEFORE applying new settings
        bool wasRunning = m_manager->isRunning(id);
        if (wasRunning) {
            m_manager->stopBackend(id);
        }

        // 2. Save the new settings to disk
        BackendConfigManager store;
        QList<BackendConfig> current = store.loadConfigurations();
        for (int i = 0; i < current.size(); ++i) {
            if (current[i].id == id) current[i] = updatedConfig;
        }
        store.saveConfigurations(current);

        // 3. Tell the manager to intelligently refresh its memory map
        m_manager->loadLibrary();

        // 4. If it was running, automatically restart it so the new settings take effect
        if (wasRunning) {
            m_manager->startBackend(id);
        }

        refreshTable();
    }
}

void BackendLibraryDialog::onEditSourceClicked()
{
    QString id = getSelectedId();
    if (id.isEmpty()) return;

    QList<BackendConfig> configs = m_manager->getConfigurations();
    for (const auto& c : configs) {
        if (c.id == id) {
            if (!c.workingDirectory.isEmpty()) {

                // 1. Try to launch a specific IDE explicitly.
                // You can change "code" to "eclipse", "subl", or "gedit" depending on your primary Python editor.
                bool launched = QProcess::startDetached("code", QStringList() << c.workingDirectory);

                // 2. Fallback: If the IDE command isn't in the system PATH, default back to the file manager
                if (!launched) {
                    QDesktopServices::openUrl(QUrl::fromLocalFile(c.workingDirectory));
                }

            } else {
                QMessageBox::information(this, tr("No Directory"), tr("Please set a Working Directory in the Edit menu first."));
            }
            break;
        }
    }
}


void BackendLibraryDialog::onDeleteClicked()
{
    QString id = getSelectedId();
    if (id.isEmpty()) return;

    if (QMessageBox::question(this, tr("Delete"), tr("Are you sure you want to delete this app?")) == QMessageBox::Yes) {
        m_manager->stopBackend(id); // Stop it if it's running

        BackendConfigManager store;
        QList<BackendConfig> current = store.loadConfigurations();
        current.erase(std::remove_if(current.begin(), current.end(),
                                     [&](const BackendConfig& c) { return c.id == id; }), current.end());
        store.saveConfigurations(current);

        m_manager->loadLibrary();
        refreshTable();
    }
}

void BackendLibraryDialog::onStartClicked()
{
    QString id = getSelectedId();
    if (!id.isEmpty()) m_manager->startBackend(id);
}

void BackendLibraryDialog::onStopClicked()
{
    QString id = getSelectedId();
    if (!id.isEmpty()) m_manager->stopBackend(id);
}

void BackendLibraryDialog::updateProcessStatus(const QString& id, bool isRunning)
{
    for (int i = 0; i < m_table->rowCount(); ++i) {
        if (m_table->item(i, 1)->data(Qt::UserRole).toString() == id) {
            m_table->item(i, 0)->setText(isRunning ? tr("Running") : tr("Stopped"));
            m_table->item(i, 0)->setForeground(isRunning ? Qt::darkGreen : Qt::darkRed);
            break;
        }
    }
}


void BackendLibraryDialog::onViewLogClicked()
{
    QString id = getSelectedId();
    if (id.isEmpty()) return;

    // Find the name of the app to reconstruct the log file name
    QList<BackendConfig> configs = m_manager->getConfigurations();
    QString name;
    for (const auto& c : configs) {
        if (c.id == id) {
            name = c.name;
            break;
        }
    }

    if (!name.isEmpty()) {
        QString safeName = name;
        safeName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
        QString logPath = QDir::temp().absoluteFilePath(QString("aspeqt_backend_%1.log").arg(safeName));
        QDesktopServices::openUrl(QUrl::fromLocalFile(logPath));
    }
}

void BackendLibraryDialog::onMountDriversClicked()
{
    // 1. Read the driver disk directly out of the compiled Qt resources
    QFile file(":/boot_templates/wdriver/drivers.atr");
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not load drivers.atr from firmware resources."));
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    // 2. Cast the parent widget back to MainWindow so we can trigger its slots
    MainWindow* mainWindow = qobject_cast<MainWindow*>(parentWidget());
    if (mainWindow) {

        // 3. Convert to Base64 so we can use your existing RAM-drive pipeline
        QString base64Data = QString::fromLatin1(data.toBase64());

        // 4. Force mount to Slot 0 (D1:) so the Atari can boot from it immediately
        mainWindow->uploadAndMountHeadless(0, "drivers.atr", base64Data);

        QMessageBox::information(this, tr("Success"), tr("W: Drivers mounted to D1: in RAM.\n\nReboot the Atari to load them."));
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not communicate with the main emulator window."));
    }
}

// --- LIVE CONSOLE IMPLEMENTATION ---

void BackendLibraryDialog::onSelectionChanged()
{
    QString id = getSelectedId();
    if (id.isEmpty()) {
        m_console->clear();
        return;
    }

    loadLogHistory(id);
}

void BackendLibraryDialog::loadLogHistory(const QString& id)
{
    m_console->clear();

    // 1. Find the name to reconstruct the log file path
    QList<BackendConfig> configs = m_manager->getConfigurations();
    QString name;
    for (const auto& c : configs) {
        if (c.id == id) { name = c.name; break; }
    }

    if (name.isEmpty()) return;

    QString safeName = name;
    safeName.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
    QString logPath = QDir::temp().absoluteFilePath(QString("aspeqt_backend_%1.log").arg(safeName));

    // 2. Read the historical file
    QFile file(logPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_console->setPlainText(QString::fromUtf8(file.readAll()));
        file.close();

        // 3. Scroll to the absolute bottom
        QScrollBar *sb = m_console->verticalScrollBar();
        sb->setValue(sb->maximum());
    } else {
        m_console->setHtml(QString("<i>No log history found for %1. Start the backend to begin capturing output.</i>").arg(name.toHtmlEscaped()));
    }
}

void BackendLibraryDialog::onBackendOutputLine(const QString& id, const QString& line, bool isError)
{
    // Only print if this specific process is the one currently highlighted in the table
    if (id != getSelectedId()) return;

    // Check if user is scrolled to the bottom before we append
    QScrollBar *sb = m_console->verticalScrollBar();
    bool atBottom = (sb->value() == sb->maximum());

    if (isError) {
        // Wrap errors in a bright red span
        m_console->append(QString("<span style=\"color: #ff5555;\">%1</span>").arg(line.toHtmlEscaped()));
    } else {
        // QTextEdit::append() understands basic text, but we escape it just in case of weird characters
        m_console->append(line.toHtmlEscaped());
    }

    // Auto-scroll if they were already at the bottom
    if (atBottom) {
        sb->setValue(sb->maximum());
    }
}
