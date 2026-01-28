#include "tnfsbrowser.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QRegularExpression>
#include <QTimer>
#include <QDebug> // Required for logs

TnfsBrowser::TnfsBrowser(QWidget *parent) : QDialog(parent), client(new TnfsClient(this))
{
    setWindowTitle(tr("TNFS Network Browser (13leader.net)"));
    resize(600, 450);

    // --- UI Layout ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Top Bar: Host Input
    QHBoxLayout *topLayout = new QHBoxLayout();
    hostCombo = new QComboBox(this);
    hostCombo->setEditable(true);
    hostCombo->setInsertPolicy(QComboBox::InsertAtTop); // New hosts go to the top
    QSettings settings("AspeQt", "TNFS");
    QStringList savedHosts = settings.value("hostHistory").toStringList();
    if (!savedHosts.isEmpty()) {
        hostCombo->addItems(savedHosts);
    } else {
        hostCombo->addItem("13leader.net");
    }



    QPushButton *btnConnect = new QPushButton(tr("Connect"), this);

    btnClear = new QPushButton(tr("Clear"), this); // The new button
    btnClear->setToolTip(tr("Clear saved host history"));

    topLayout->addWidget(new QLabel(tr("TNFS Host:")));
    topLayout->addWidget(hostCombo,1);
    topLayout->addWidget(btnConnect);
    topLayout->addWidget(btnClear);

    // Navigation Bar
    QHBoxLayout *navLayout = new QHBoxLayout();
    QPushButton *btnBack = new QPushButton(tr(".. (Up)"), this);
    statusLabel = new QLabel(tr("Not Connected"), this);

    navLayout->addWidget(btnBack);
    navLayout->addWidget(statusLabel);
    navLayout->addStretch();

    // File List
    fileList = new QListWidget(this);

    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(navLayout);
    mainLayout->addWidget(fileList);
    mainLayout->addWidget(new QLabel(tr("Double-click a file (.ATR/.XEX) to Mount it."), this));

    // --- Connections ---
    connect(btnConnect, &QPushButton::clicked, this, &TnfsBrowser::onConnect);
    connect(btnBack, &QPushButton::clicked, this, &TnfsBrowser::onBackClicked);
    connect(fileList, &QListWidget::itemDoubleClicked, this, &TnfsBrowser::onItemDoubleClicked);
    connect(btnClear, &QPushButton::clicked, this, &TnfsBrowser::onClearHistory);

    currentPath = "/";
}

TnfsBrowser::~TnfsBrowser()
{
    // Client is a child of this, so it cleans up automatically
}

QString TnfsBrowser::getSelectedUrl() const
{
    return selectedUrl;
}

void TnfsBrowser::onConnect()
{
    QString host = hostCombo->currentText();
    // Add the host to the list if it's not already there


    statusLabel->setText(tr("Connecting to %1...").arg(host));
    QApplication::processEvents();

    // 1. Establish UDP "Connection" (Resolve IP)
    // This resets the session ID to 0 internally in client
    if (client->connectToHost(host)) {

        // 2. Perform Initial Mount of Root
        // This generates the FIRST session ID from the server
        if (client->mount("/")) {
            statusLabel->setText(tr("Connected: /"));
            currentPath = "/";
            refreshList();

            if (hostCombo->findText(host) == -1) {
                hostCombo->addItem(host);
            }

            QSettings settings("AspeQt", "TNFS");
            QStringList history;
            for (int i = 0; i < hostCombo->count(); ++i) {
                history << hostCombo->itemText(i);

            }

            settings.setValue("hostHistory", history);

        } else {
            statusLabel->setText(tr("Error: Could not mount root."));
            QMessageBox::critical(this, tr("Connection Failed"),
                                  tr("Server rejected mount request.\nCheck 'tnfsd' logs if possible."));
        }
    } else {
        statusLabel->setText(tr("Error: Host not found."));
        QMessageBox::critical(this, tr("Connection Failed"), tr("Could not resolve hostname."));
    }
}

void TnfsBrowser::refreshList()
{
    setCursor(Qt::WaitCursor);
    fileList->setEnabled(false);
    statusLabel->setText(tr("Fetching %1...").arg(currentPath));

    // Qt 6 Cleaner Async Pattern
    auto *watcher = new QFutureWatcher<QList<TnfsClient::DirectoryEntry>>(this);

    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher]() {
        QList<TnfsClient::DirectoryEntry> entries = watcher->result();

        fileList->clear();

        if (entries.isEmpty()) {
            // It might be empty, OR it might be an error.
            // Since listDirectory swallows errors, we assume empty for now.
            // statusLabel->setText(tr("Directory is empty."));
        }

        for (const auto &entry : entries) {
            // Filter out "." and ".." explicitly if client didn't already
            if (entry.name == "." || entry.name == "..") continue;

            QListWidgetItem *item = new QListWidgetItem(entry.name);

            // Simple Icon Logic
            if (entry.isDirectory) {
                item->setIcon(QIcon::fromTheme("folder")); // Use system theme or resource
                item->setData(Qt::UserRole, true); // Mark as folder
            } else {
                item->setIcon(QIcon::fromTheme("media-floppy"));
                item->setData(Qt::UserRole, false); // Mark as file
            }

            fileList->addItem(item);
        }

        statusLabel->setText(tr("Browsing: %1").arg(currentPath));
        unsetCursor();
        fileList->setEnabled(true);
        watcher->deleteLater();
    });

    // Qt 6 requires explicit template arguments sometimes, but this usually works:
    QFuture<QList<TnfsClient::DirectoryEntry>> future = QtConcurrent::run(
        [this, path = currentPath]() {
            // Thread-safe call: listDirectory uses QMutex internally
            return client->listDirectory(path);
        }
        );

    watcher->setFuture(future);
}


void TnfsBrowser::onItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;

    QString name = item->text();
    // ... (Keep existing name cleanup) ...

    // Build the target path
    QString targetPath = currentPath;
    if (!targetPath.endsWith("/")) targetPath += "/";
    targetPath += name;

    // FILE SELECTION
    if (name.contains(".")) { // Or use item->data(Qt::UserRole)
        // FIX: Use QUrl to construct the URL safely.
        // This handles spaces and special chars like '#' automatically.
        QUrl url;
        url.setScheme("tnfs");
        url.setHost(hostCombo->currentText());
        url.setPath(targetPath); // setPath encodes '#' to '%23'

        selectedUrl = url.toString();
        accept();
        return;
    }

    // FOLDER NAVIGATION (Existing Fix)
    currentPath = targetPath;
    statusLabel->setText(tr("Browsing: %1").arg(currentPath));
    refreshList();
}

void TnfsBrowser::onBackClicked()
{
    if (currentPath == "/" || currentPath.isEmpty()) return;

    int lastSlash = currentPath.lastIndexOf('/', currentPath.length() - 2);
    QString parentPath = (lastSlash != -1) ? currentPath.left(lastSlash + 1) : "/";

    // Just update path and refresh. Do NOT mount.
    currentPath = parentPath;
    refreshList();
}

void TnfsBrowser::onClearHistory()
{
    // Ask for confirmation so you don't accidentally wipe it
    auto reply = QMessageBox::question(this, tr("Clear History"),
                                       tr("Are you sure you want to clear all saved hosts?"),
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        // 1. Clear the UI
        hostCombo->clear();
        hostCombo->addItem("13leader.net"); // Keep a default if you like

        // 2. Clear the QSettings
        QSettings settings("AspeQt", "TNFS");
        settings.remove("hostHistory");

        statusLabel->setText(tr("History cleared."));
    }
}
