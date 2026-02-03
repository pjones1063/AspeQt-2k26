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
#include <QtConcurrent> // Required for background threads
#include <algorithm>

TnfsBrowser::TnfsBrowser(QWidget *parent, const QString &initialUrl)
    : QDialog(parent), client(new TnfsClient(this))
{
    setWindowTitle(tr("TNFS Network Browser"));
    resize(600, 450);

    connectWatcher = new QFutureWatcher<bool>(this);
    connect(connectWatcher, &QFutureWatcherBase::finished, this, &TnfsBrowser::onConnectionFinished);

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

    btnConnect = new QPushButton(tr("Connect"), this);

    btnClear = new QPushButton(tr("Clear"), this);
    btnClear->setToolTip(tr("Clear saved host history"));

    topLayout->addWidget(new QLabel(tr("TNFS Host:")));
    topLayout->addWidget(hostCombo, 1);
    topLayout->addWidget(btnConnect);
    topLayout->addWidget(btnClear);

    // 2. Navigation Bar
    QHBoxLayout *navLayout = new QHBoxLayout();

    QPushButton *btnBack = new QPushButton(tr(".. (Up)"), this);

    btnSort = new QToolButton(this);
    btnSort->setIcon(getIcon("view-sort-ascending"));
    if (btnSort->icon().isNull()) btnSort->setText("A-Z");
    btnSort->setToolTip(tr("Sort A-Z / Z-A"));
    m_sortAscending = true;

    statusLabel = new QLabel(tr("Not Connected"), this);

    // Activity Bar (Hidden by default)
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 0); // Marquee mode (Infinite loop)
    progressBar->setTextVisible(false);
    progressBar->setFixedHeight(15);
    progressBar->setVisible(false);

    navLayout->addWidget(btnBack);
    navLayout->addWidget(btnSort);
    navLayout->addWidget(statusLabel);
    navLayout->addWidget(progressBar);
    navLayout->addStretch();

    // 3. File List
    fileList = new QListWidget(this);

    // 4. Bottom Button Bar
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    btnMore = new QPushButton(tr("More..."), this);
    btnMore->setVisible(false);
    btnCancel = new QPushButton(tr("Cancel"), this);

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
    connect(btnSort, &QToolButton::clicked, this, &TnfsBrowser::onSortClicked);

    currentPath = "/";

    // --- Auto-Navigate Logic ---
    if (!initialUrl.isEmpty()) {
        QUrl qurl(initialUrl);
        QString host = qurl.host();
        QString path = qurl.path();

        if (!path.endsWith("/")) {
            int lastSlash = path.lastIndexOf('/');
            if (lastSlash != -1) path = path.left(lastSlash + 1);
            else path = "/";
        }

        if (!host.isEmpty()) {
            hostCombo->setEditText(host);
            // Trigger connection immediately
            onConnect();
        }
    }
}



TnfsBrowser::~TnfsBrowser()
{
    // Clean up watcher if running
    if (connectWatcher->isRunning()) {
        connectWatcher->waitForFinished();
    }
}

QString TnfsBrowser::getSelectedUrl() const
{
    return selectedUrl;
}

QIcon TnfsBrowser::getIcon(const QString &name)
{
    // 1. Try embedded resources first
    if (name == "folder") return QIcon(":/icons/silk-icons/icons/folder.png");
    if (name == "disk")   return QIcon(":/icons/silk-icons/icons/drive_disk.png");
    if (name == "file")   return QIcon(":/icons/silk-icons/icons/page_white.png");

    if (name == "view-sort-ascending") {
        if (QIcon::hasThemeIcon("view-sort-ascending")) return QIcon::fromTheme("view-sort-ascending");
        // Fallback if system theme missing
        return QIcon(":/icons/silk-icons/icons/arrow_up.png");
    }
    if (name == "view-sort-descending") {
        if (QIcon::hasThemeIcon("view-sort-descending")) return QIcon::fromTheme("view-sort-descending");
        return QIcon(":/icons/silk-icons/icons/arrow_down.png");
    }

    // 2. Try System Theme (Linux Fallback)
    if (QIcon::hasThemeIcon(name)) {
        return QIcon::fromTheme(name);
    }

    // 3. Fallback to Qt Standard Icons
    if (name == "folder") return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
    if (name == "file")   return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
    if (name == "disk")   return QApplication::style()->standardIcon(QStyle::SP_DriveFDIcon);

    return QIcon();
}



void TnfsBrowser::onConnect()
{
    QString host = hostCombo->currentText();

    // 1. Disable UI
    hostCombo->setEnabled(false);
    btnConnect->setEnabled(false);
    fileList->setEnabled(false);

    // 2. Show Infinite Progress Bar
    progressBar->setVisible(true);
    statusLabel->setText(tr("Connecting to %1...").arg(host));

    // 3. Run Connection in Background Thread
    QFuture<bool> future = QtConcurrent::run([this, host]() {
        // This runs in a worker thread
        if (client->connectToHost(host)) {
            return client->mount("/");
        }
        return false;
    });

    connectWatcher->setFuture(future);
}

void TnfsBrowser::onConnectionFinished()
{
    // 1. Re-enable UI
    hostCombo->setEnabled(true);
    btnConnect->setEnabled(true);
    fileList->setEnabled(true);
    progressBar->setVisible(false);

    bool success = connectWatcher->result();
    QString host = hostCombo->currentText();

    if (success) {
        statusLabel->setText(tr("Connected: /"));
        currentPath = "/";

        // Save History
        if (hostCombo->findText(host) == -1) {
            hostCombo->addItem(host);
        }
        QSettings settings("AspeQt", "TNFS");
        QStringList history;
        for (int i = 0; i < hostCombo->count(); ++i) {
            history << hostCombo->itemText(i);
        }
        settings.setValue("hostHistory", history);

        refreshList(); // Load files
    } else {
        statusLabel->setText(tr("Connection Failed."));
        QMessageBox::critical(this, tr("Connection Error"),
                              tr("Could not reach host '%1'.\nCheck internet or hostname.").arg(host));
    }
}

void TnfsBrowser::refreshList()
{
    fileList->clear();
    btnMore->setVisible(false);
    btnMore->setEnabled(true);
    btnMore->setText(tr("More..."));

    statusLabel->setText(tr("Fetching %1...").arg(currentPath));

    // Reset Sort
    m_sortAscending = true;

    // Use helper or theme for sort icon
    QIcon icon = QIcon::fromTheme("view-sort-ascending");
    if (icon.isNull()) btnSort->setText("A-Z");
    else btnSort->setIcon(icon);

    QApplication::processEvents();

    if (client->beginListing(currentPath)) {
        m_isFirstBatch = true;
        loadNextBatch();
    } else {
        statusLabel->setText(tr("Error opening directory."));
    }
}


void TnfsBrowser::loadNextBatch()
{
    setCursor(Qt::WaitCursor);

    auto newItems = client->fetchNextBatch(20);
    bool finished = client->isListingFinished();

    // Sort first batch
    if (m_isFirstBatch && finished) {
        std::sort(newItems.begin(), newItems.end(), [](const TnfsClient::DirectoryEntry &a, const TnfsClient::DirectoryEntry &b) {
            if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
            return a.name.toLower() < b.name.toLower();
        });
    }

    for (const auto &entry : newItems) {
        QListWidgetItem *item = new QListWidgetItem(entry.name);

        if (entry.isDirectory) {
            // FIX 3: Use the robust getIcon helper
            item->setIcon(getIcon("folder"));
            item->setData(Qt::UserRole, true);
        } else {
            // FIX 4: Check for ALL common Atari/Retro extensions
            QString name = entry.name.toLower();
            if (name.endsWith(".atr") || name.endsWith(".xex") ||
                name.endsWith(".exe") || name.endsWith(".com") ||
                name.endsWith(".dsk") || name.endsWith(".pro") ||
                name.endsWith(".atx") || name.endsWith(".xfd") ||
                name.endsWith(".cas") || name.endsWith(".bin") ||
                name.endsWith(".car") || name.endsWith(".rom")) {

                item->setIcon(getIcon("disk"));
            } else {
                item->setIcon(getIcon("file"));
            }
            item->setData(Qt::UserRole, false);
        }
        fileList->addItem(item);
    }

    if (finished) {
        btnMore->setEnabled(false);
        if (m_isFirstBatch) btnMore->setVisible(false);
        else btnMore->setText(tr("No more items"));
    } else {
        btnMore->setVisible(true);
        btnMore->setEnabled(true);
        btnMore->setText(tr("More..."));
    }

    if (!m_isFirstBatch) fileList->scrollToBottom();

    m_isFirstBatch = false;
    statusLabel->setText(tr("Browsing: %1").arg(currentPath));
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
    if (QMessageBox::Yes == QMessageBox::question(this, tr("Clear History"),
                                                  tr("Clear all saved hosts?"), QMessageBox::Yes | QMessageBox::No)) {
        hostCombo->clear();
        hostCombo->addItem("13leader.net");
        QSettings settings("AspeQt", "TNFS");
        settings.remove("hostHistory");
        statusLabel->setText(tr("History cleared."));
    }
}

void TnfsBrowser::onCancelClicked() { reject(); }
void TnfsBrowser::onMoreClicked() { loadNextBatch(); }

void TnfsBrowser::onSortClicked()
{
    m_sortAscending = !m_sortAscending;

    // Update Icon
    QIcon icon = m_sortAscending ? getIcon("view-sort-ascending")
                                 : getIcon("view-sort-descending");

    if (!icon.isNull()) btnSort->setIcon(icon);

    QList<QListWidgetItem*> items;
    while (fileList->count() > 0) items.append(fileList->takeItem(0));

    std::sort(items.begin(), items.end(), [this](QListWidgetItem *a, QListWidgetItem *b) {
        bool dirA = a->data(Qt::UserRole).toBool();
        bool dirB = b->data(Qt::UserRole).toBool();
        if (dirA != dirB) return dirA > dirB; // Directories first
        return m_sortAscending ? a->text().toLower() < b->text().toLower()
                               : a->text().toLower() > b->text().toLower();
    });

    for (auto *item : items) fileList->addItem(item);
}
