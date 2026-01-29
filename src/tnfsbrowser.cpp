#include "tnfsbrowser.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QRegularExpression>
#include <QTimer>
#include <QDebug>
#include <QSettings> // <--- Added missing header

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
    hostCombo->setInsertPolicy(QComboBox::InsertAtTop);

    // Load History
    QSettings settings("AspeQt", "TNFS");
    QStringList savedHosts = settings.value("hostHistory").toStringList();
    if (!savedHosts.isEmpty()) {
        hostCombo->addItems(savedHosts);
    } else {
        hostCombo->addItem("13leader.net");
    }

    QPushButton *btnConnect = new QPushButton(tr("Connect"), this);

    btnClear = new QPushButton(tr("Clear"), this);
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

    setCursor(Qt::WaitCursor); // Show busy
    statusLabel->setText(tr("Connecting to %1...").arg(host));
    QApplication::processEvents(); // Force UI update

    // 1. Establish UDP "Connection"
    if (client->connectToHost(host)) {

        // 2. Perform Initial Mount of Root
        if (client->mount("/")) {
            statusLabel->setText(tr("Connected: /"));
            currentPath = "/";

            // Save history
            if (hostCombo->findText(host) == -1) {
                hostCombo->addItem(host);
            }
            QSettings settings("AspeQt", "TNFS");
            QStringList history;
            for (int i = 0; i < hostCombo->count(); ++i) {
                history << hostCombo->itemText(i);
            }
            settings.setValue("hostHistory", history);

            unsetCursor();
            refreshList(); // Call refresh immediately

        } else {
            unsetCursor();
            statusLabel->setText(tr("Error: Could not mount root."));
            QMessageBox::critical(this, tr("Connection Failed"),
                                  tr("Server rejected mount request.\nCheck 'tnfsd' logs if possible."));
        }
    } else {
        unsetCursor();
        statusLabel->setText(tr("Error: Host not found."));
        QMessageBox::critical(this, tr("Connection Failed"), tr("Could not resolve hostname."));
    }
}

void TnfsBrowser::refreshList()
{
    setCursor(Qt::WaitCursor);
    fileList->setEnabled(false);
    statusLabel->setText(tr("Fetching %1...").arg(currentPath));
    QApplication::processEvents(); // Keep UI responsive-ish

    // Run Synchronously in Main Thread to avoid Thread Affinity issues
    QList<TnfsClient::DirectoryEntry> entries = client->listDirectory(currentPath);

    fileList->clear();

    for (const auto &entry : entries) {
        if (entry.name == "." || entry.name == "..") continue;

        QListWidgetItem *item = new QListWidgetItem(entry.name);

        if (entry.isDirectory) {
            item->setIcon(QIcon::fromTheme("folder"));
            item->setData(Qt::UserRole, true);
        } else {
            item->setIcon(QIcon::fromTheme("media-floppy"));
            item->setData(Qt::UserRole, false);
        }

        fileList->addItem(item);
    }

    statusLabel->setText(tr("Browsing: %1").arg(currentPath));
    fileList->setEnabled(true);
    unsetCursor();
}


void TnfsBrowser::onItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;

    QString name = item->text();

    // Build the target path
    QString targetPath = currentPath;
    if (!targetPath.endsWith("/")) targetPath += "/";
    targetPath += name;

    // FILE SELECTION
    bool isDir = item->data(Qt::UserRole).toBool();

    // Fallback if data not set (e.g. extension check)
    if (!isDir && (name.contains(".") || targetPath.contains("."))) {
        QUrl url;
        url.setScheme("tnfs");
        url.setHost(hostCombo->currentText());
        url.setPath(targetPath);

        selectedUrl = url.toString();
        accept();
        return;
    }

    // FOLDER NAVIGATION
    currentPath = targetPath;
    statusLabel->setText(tr("Browsing: %1").arg(currentPath));
    refreshList();
}

void TnfsBrowser::onBackClicked()
{
    if (currentPath == "/" || currentPath.isEmpty()) return;

    int lastSlash = currentPath.lastIndexOf('/', currentPath.length() - 2);
    QString parentPath = (lastSlash != -1) ? currentPath.left(lastSlash + 1) : "/";

    currentPath = parentPath;
    refreshList();
}

void TnfsBrowser::onClearHistory()
{
    auto reply = QMessageBox::question(this, tr("Clear History"),
                                       tr("Are you sure you want to clear all saved hosts?"),
                                       QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        hostCombo->clear();
        hostCombo->addItem("13leader.net");

        QSettings settings("AspeQt", "TNFS");
        settings.remove("hostHistory");

        statusLabel->setText(tr("History cleared."));
    }
}
