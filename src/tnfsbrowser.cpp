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
#include <algorithm>

TnfsBrowser::TnfsBrowser(QWidget *parent, const QString &initialUrl) : QDialog(parent), client(new TnfsClient(this))
{
    setWindowTitle(tr("TNFS Network Browser"));
    resize(600, 450);

    // --- UI Layout ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 1. Top Bar: Host Input
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
        hostCombo->addItem("13leader.net"); // Default
    }

    QPushButton *btnConnect = new QPushButton(tr("Connect"), this);

    btnClear = new QPushButton(tr("Clear"), this);
    btnClear->setToolTip(tr("Clear saved host history"));

    topLayout->addWidget(new QLabel(tr("TNFS Host:")));
    topLayout->addWidget(hostCombo, 1);
    topLayout->addWidget(btnConnect);
    topLayout->addWidget(btnClear);

    // 2. Navigation Bar (Back, Sort, Status)
    QHBoxLayout *navLayout = new QHBoxLayout();

    QPushButton *btnBack = new QPushButton(tr(".. (Up)"), this);

    // --- NEW: Sort Button ---
    btnSort = new QToolButton(this);
    btnSort->setIcon(QIcon::fromTheme("view-sort-ascending"));
    if (btnSort->icon().isNull()) btnSort->setText("A-Z"); // Fallback text
    btnSort->setToolTip(tr("Sort A-Z / Z-A"));
    m_sortAscending = true; // Default to A-Z

    statusLabel = new QLabel(tr("Not Connected"), this);

    navLayout->addWidget(btnBack);
    navLayout->addWidget(btnSort); // Add sort button next to Back
    navLayout->addWidget(statusLabel);
    navLayout->addStretch();

    // 3. File List
    fileList = new QListWidget(this);

    // 4. Bottom Button Bar
    QHBoxLayout *bottomLayout = new QHBoxLayout();

    // --- FIX: Create Buttons FIRST (Prevents Crash) ---
    btnMore = new QPushButton(tr("More..."), this);
    btnMore->setVisible(false); // Hidden by default

    btnCancel = new QPushButton(tr("Cancel"), this);

    // --- FIX: Add to Layout SECOND ---
    bottomLayout->addWidget(btnMore);
    bottomLayout->addStretch();
    bottomLayout->addWidget(btnCancel);

    // Assemble Main Layout
    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(navLayout);
    mainLayout->addWidget(fileList);
    mainLayout->addWidget(new QLabel(tr("Double-click a file (.ATR/.XEX) to Mount it."), this));
    mainLayout->addLayout(bottomLayout);

    // --- Connections ---
    connect(btnConnect, &QPushButton::clicked, this, &TnfsBrowser::onConnect);
    connect(btnBack, &QPushButton::clicked, this, &TnfsBrowser::onBackClicked);
    connect(fileList, &QListWidget::itemDoubleClicked, this, &TnfsBrowser::onItemDoubleClicked);
    connect(btnClear, &QPushButton::clicked, this, &TnfsBrowser::onClearHistory);
    connect(btnMore, &QPushButton::clicked, this, &TnfsBrowser::onMoreClicked);
    connect(btnCancel, &QPushButton::clicked, this, &TnfsBrowser::onCancelClicked);

    // --- NEW: Sort Connection ---
    connect(btnSort, &QToolButton::clicked, this, &TnfsBrowser::onSortClicked);

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

            // Visual feedback
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

    setCursor(Qt::WaitCursor);
    statusLabel->setText(tr("Connecting to %1...").arg(host));
    QApplication::processEvents();

    if (client->connectToHost(host)) {
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
    fileList->clear();
    btnMore->setVisible(false);
    btnMore->setEnabled(true);
    btnMore->setText(tr("More..."));

    statusLabel->setText(tr("Fetching %1...").arg(currentPath));

    // --- NEW: Reset Sort State on folder change ---
    m_sortAscending = true;
    QIcon icon = QIcon::fromTheme("view-sort-ascending");
    if (icon.isNull()) btnSort->setText("A-Z");
    else btnSort->setIcon(icon);

    QApplication::processEvents();

    // Start the listing process
    if (client->beginListing(currentPath)) {
        m_isFirstBatch = true; // Mark start
        loadNextBatch();       // Load first page immediately
    } else {
        statusLabel->setText(tr("Error opening directory."));
    }
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

void TnfsBrowser::onCancelClicked()
{
    reject();
}

void TnfsBrowser::onMoreClicked() {
    loadNextBatch();
}

void TnfsBrowser::loadNextBatch()
{
    setCursor(Qt::WaitCursor);

    // Fetch 20 items (Small enough for low latency)
    auto newItems = client->fetchNextBatch(20);
    bool finished = client->isListingFinished();

    // LOGIC: If this is the FIRST batch AND it finished, we sort!
    if (m_isFirstBatch && finished) {
        std::sort(newItems.begin(), newItems.end(), [](const TnfsClient::DirectoryEntry &a, const TnfsClient::DirectoryEntry &b) {
            // Folders first, then files
            if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
            return a.name.toLower() < b.name.toLower();
        });
    }

    // Add items to UI
    for (const auto &entry : newItems) {
        QListWidgetItem *item = new QListWidgetItem(entry.name);

        if (entry.isDirectory) {
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

    // UPDATE BUTTON STATE
    if (finished) {
        btnMore->setEnabled(false); // Grey it out

        // Optional: Hide it completely if it's a small folder to look cleaner
        if (m_isFirstBatch) btnMore->setVisible(false);
        else btnMore->setText(tr("No more items"));

    } else {
        btnMore->setVisible(true);
        btnMore->setEnabled(true);
        btnMore->setText(tr("More..."));
    }

    if (!m_isFirstBatch) {
        fileList->scrollToBottom(); // Only scroll if appending
    }

    m_isFirstBatch = false; // Next time, do not sort
    statusLabel->setText(tr("Browsing: %1").arg(currentPath));
    unsetCursor();
}

// --- NEW: Sorting Logic ---
void TnfsBrowser::onSortClicked()
{
    // 1. Toggle State
    m_sortAscending = !m_sortAscending;

    // 2. Update Icon
    if (m_sortAscending) {
        QIcon icon = QIcon::fromTheme("view-sort-ascending");
        if (icon.isNull()) btnSort->setText("A-Z");
        else btnSort->setIcon(icon);
    } else {
        QIcon icon = QIcon::fromTheme("view-sort-descending");
        if (icon.isNull()) btnSort->setText("Z-A");
        else btnSort->setIcon(icon);
    }

    // 3. Extract all items from the list
    QList<QListWidgetItem*> items;
    while (fileList->count() > 0) {
        items.append(fileList->takeItem(0));
    }

    // 4. Sort the list
    std::sort(items.begin(), items.end(), [this](QListWidgetItem *a, QListWidgetItem *b) {
        bool dirA = a->data(Qt::UserRole).toBool();
        bool dirB = b->data(Qt::UserRole).toBool();

        // RULE 1: Directories ALWAYS on top, regardless of sort direction
        if (dirA != dirB) return dirA > dirB;

        // RULE 2: Sort Names based on direction
        if (m_sortAscending) {
            return a->text().toLower() < b->text().toLower();
        } else {
            return a->text().toLower() > b->text().toLower();
        }
    });

    // 5. Re-insert sorted items
    for (auto *item : items) {
        fileList->addItem(item);
    }
}
