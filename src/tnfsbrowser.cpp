#include "tnfsbrowser.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QRegularExpression>
#include <QTimer>
#include <QDebug>
#include <QSettings>
#include <QStyle>

TnfsBrowser::TnfsBrowser(QWidget *parent, const QString &initialUrl) : QDialog(parent), client(new TnfsClient(this))
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

    // Bottom Button Bar (New)
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    btnCancel = new QPushButton(tr("Cancel"), this);
    bottomLayout->addStretch();
    bottomLayout->addWidget(btnCancel);

    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(navLayout);
    mainLayout->addWidget(fileList);
    mainLayout->addWidget(new QLabel(tr("Double-click a file (.ATR/.XEX) to Mount it."), this));
    mainLayout->addLayout(bottomLayout); // Add the bottom layout

    // --- Connections ---
    connect(btnConnect, &QPushButton::clicked, this, &TnfsBrowser::onConnect);
    connect(btnBack, &QPushButton::clicked, this, &TnfsBrowser::onBackClicked);
    connect(fileList, &QListWidget::itemDoubleClicked, this, &TnfsBrowser::onItemDoubleClicked);
    connect(btnClear, &QPushButton::clicked, this, &TnfsBrowser::onClearHistory);
    connect(btnCancel, &QPushButton::clicked, this, &TnfsBrowser::onCancelClicked); // Connect Cancel

    currentPath = "/";

    // --- Auto-Navigate Logic ---
    if (!initialUrl.isEmpty()) {
        QUrl qurl(initialUrl);
        QString host = qurl.host();
        QString path = qurl.path();

        // Strip filename to get directory: /folder/game.atr -> /folder/
        if (!path.endsWith("/")) {
            int lastSlash = path.lastIndexOf('/');
            if (lastSlash != -1) path = path.left(lastSlash + 1);
            else path = "/";
        }

        if (!host.isEmpty()) {
            hostCombo->setEditText(host);

            // Manual Connect Sequence
            // Note: Main Window handles the *Initial* WaitCursor, but we leave this
            // label update here for visual clarity.
            statusLabel->setText(tr("Connecting to %1...").arg(host));
            QApplication::processEvents();

            if (client->connectToHost(host) && client->mount("/")) {
                // Update Path to the saved one
                currentPath = path;

                // Update History
                if (hostCombo->findText(host) == -1) hostCombo->addItem(host);
                QStringList history;
                for (int i = 0; i < hostCombo->count(); ++i) history << hostCombo->itemText(i);
                settings.setValue("hostHistory", history);

                refreshList();
            } else {
                statusLabel->setText(tr("Connection failed."));
                // Ensure cursor is unset if we fail (just in case)
                unsetCursor();
            }
        }
    }
}


TnfsBrowser::~TnfsBrowser()
{
}

QString TnfsBrowser::getSelectedUrl() const
{
    return selectedUrl;
}

void TnfsBrowser::onConnect()
{
    QString host = hostCombo->currentText();

    setCursor(Qt::WaitCursor); // Show busy for manual clicks
    statusLabel->setText(tr("Connecting to %1...").arg(host));
    QApplication::processEvents();

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
            refreshList();

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
    // Set cursor for internal navigation events
    setCursor(Qt::WaitCursor);
    fileList->setEnabled(false);
    statusLabel->setText(tr("Fetching %1...").arg(currentPath));

    // [NEW] Log Message
    // MainWindow will handle the [xNN] aggregation automatically
    qDebug() << "!n" << "Directory loading...";

    QApplication::processEvents();

    // Run Synchronously
    QList<TnfsClient::DirectoryEntry> entries = client->listDirectory(currentPath);

    fileList->clear();

    for (const auto &entry : entries) {
        if (entry.name == "." || entry.name == "..") continue;

        QListWidgetItem *item = new QListWidgetItem(entry.name);

        bool isDir = entry.isDirectory;
        if (!isDir && !entry.name.contains('.')) {
            isDir = true;
        }

        if (isDir) {
            QIcon icon = QIcon::fromTheme("folder");
            if (icon.isNull()) icon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
            item->setIcon(icon);
            item->setData(Qt::UserRole, true);
        } else {
            QIcon icon = QIcon::fromTheme("media-floppy");
            if (icon.isNull()) icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
            item->setIcon(icon);
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

    QString targetPath = currentPath;
    if (!targetPath.endsWith("/")) targetPath += "/";
    targetPath += name;

    bool isDir = item->data(Qt::UserRole).toBool();

    if (!isDir && (name.contains(".") || targetPath.contains("."))) {
        QUrl url;
        url.setScheme("tnfs");
        url.setHost(hostCombo->currentText());
        url.setPath(targetPath);

        selectedUrl = url.toString();
        accept();
        return;
    }

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

// [NEW] Cancel Slot
void TnfsBrowser::onCancelClicked()
{
    reject();
}
