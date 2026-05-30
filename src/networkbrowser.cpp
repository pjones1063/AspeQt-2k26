#include "networkbrowser.h"
#include "tnfsclient.h"
#include "ftpclient.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QApplication>
#include <QRegularExpression>
#include <QTimer>
#include <QDebug>
#include <QSettings>
#include <QStyle>
#include <QtConcurrent>
#include <algorithm>

/* networkbrowser.cpp - Constructor */

NetworkBrowser::NetworkBrowser(QWidget *parent, const QString &initialUrl)
    : QDialog(parent), m_client(nullptr)
{
    setWindowTitle(tr("Universal Network Browser"));
    resize(600, 480);

    // --- Load saved credentials from disk immediately ---
    QSettings settings("AspeQt", "TNFS");
    m_savedUser = settings.value("ftpUser", "").toString();
    m_savedPass = settings.value("ftpPass", "").toString();
    // ----------------------------------------------------

    connectWatcher = new QFutureWatcher<bool>(this);
    connect(connectWatcher, &QFutureWatcherBase::finished, this, &NetworkBrowser::onConnectionFinished);

    fetchWatcher = new QFutureWatcher<QList<INetworkClient::DirectoryEntry>>(this);
    connect(fetchWatcher, &QFutureWatcherBase::finished, this, &NetworkBrowser::onFetchFinished);

    // --- UI Layout ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 1. Top Bar: Protocol Selection
    QHBoxLayout *protocolLayout = new QHBoxLayout();
    radioTnfs = new QRadioButton(tr("TNFS"), this);
    radioFtp = new QRadioButton(tr("FTP"), this);
    radioTnfs->setChecked(true); // Default

    protocolLayout->addWidget(new QLabel(tr("Protocol:")));
    protocolLayout->addWidget(radioTnfs);
    protocolLayout->addWidget(radioFtp);
    protocolLayout->addStretch();

    // 2. Top Bar: Host Input
    QHBoxLayout *topLayout = new QHBoxLayout();
    hostCombo = new QComboBox(this);
    hostCombo->setEditable(true);
    hostCombo->setInsertPolicy(QComboBox::InsertAtTop);

    // Load History
    QStringList savedHosts = settings.value("hostHistory").toStringList();
    if (!savedHosts.isEmpty()) {
        hostCombo->addItems(savedHosts);
    }

    btnConnect = new QPushButton(tr("Connect"), this);
    btnClear = new QPushButton(tr("Clear"), this);
    btnClear->setToolTip(tr("Clear saved host history"));

    // Login Button
    btnLogin = new QPushButton(this);
    btnLogin->setIcon(QIcon(":/icons/silk-icons/icons/lock.png"));
    btnLogin->setToolTip(tr("Set Server Credentials"));
    connect(btnLogin, &QPushButton::clicked, this, &NetworkBrowser::onLoginClicked);

    topLayout->addWidget(new QLabel(tr("Host Address:")));
    topLayout->addWidget(hostCombo, 1);
    topLayout->addWidget(btnLogin);
    topLayout->addWidget(btnConnect);
    topLayout->addWidget(btnClear);

    // 3. Navigation Bar
    QHBoxLayout *navLayout = new QHBoxLayout();
    QPushButton *btnBack = new QPushButton(tr(".. (Up)"), this);
    btnSort = new QToolButton(this);
    btnSort->setIcon(getIcon("view-sort-ascending"));
    if (btnSort->icon().isNull()) btnSort->setText("A-Z");
    btnSort->setToolTip(tr("Sort A-Z / Z-A"));
    m_sortAscending = true;

    statusLabel = new QLabel(tr("Not Connected"), this);
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 0); // Marquee mode
    progressBar->setTextVisible(false);
    progressBar->setFixedHeight(15);
    progressBar->setVisible(false);

    navLayout->addWidget(btnBack);
    navLayout->addWidget(btnSort);
    navLayout->addWidget(statusLabel);
    navLayout->addWidget(progressBar);
    navLayout->addStretch();

    // 4. File List
    fileList = new QListWidget(this);

    // 5. Bottom Button Bar
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    btnMore = new QPushButton(tr("More..."), this);
    btnMore->setVisible(false);
    btnCancel = new QPushButton(tr("Cancel"), this);

    bottomLayout->addWidget(btnMore);
    bottomLayout->addStretch();
    bottomLayout->addWidget(btnCancel);

    // Assemble Main Layout
    mainLayout->addLayout(protocolLayout);
    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(navLayout);
    mainLayout->addWidget(fileList);
    mainLayout->addWidget(new QLabel(tr("Double-click a file (.ATR/.XEX) to Mount it."), this));
    mainLayout->addLayout(bottomLayout);

    // --- Connections ---
    connect(btnConnect, &QPushButton::clicked, this, &NetworkBrowser::onConnect);
    connect(btnBack, &QPushButton::clicked, this, &NetworkBrowser::onBackClicked);
    connect(fileList, &QListWidget::itemDoubleClicked, this, &NetworkBrowser::onItemDoubleClicked);
    connect(btnClear, &QPushButton::clicked, this, &NetworkBrowser::onClearHistory);
    connect(btnMore, &QPushButton::clicked, this, &NetworkBrowser::onMoreClicked);
    connect(btnCancel, &QPushButton::clicked, this, &NetworkBrowser::onCancelClicked);
    connect(btnSort, &QToolButton::clicked, this, &NetworkBrowser::onSortClicked);

    // --- Wire up the radio buttons to the UI toggle ---
    connect(radioTnfs, &QRadioButton::toggled, this, &NetworkBrowser::onProtocolChanged);
    connect(radioFtp, &QRadioButton::toggled, this, &NetworkBrowser::onProtocolChanged);

    currentPath = "/";
    m_activeHost = "";
    m_activeProtocol = "tnfs";

    // --- Auto-Navigate Logic ---
    if (!initialUrl.isEmpty()) {
        QUrl qurl(initialUrl);
        QString host = qurl.host();
        QString path = qurl.path();
        QString scheme = qurl.scheme();

        if (!path.endsWith("/")) {
            int lastSlash = path.lastIndexOf('/');
            if (lastSlash != -1) path = path.left(lastSlash + 1);
            else path = "/";
        }

        currentPath = path;

        // Sync UI toggles with incoming URL scheme
        if (scheme.toLower() == "ftp") {
            radioFtp->setChecked(true);

            // If AspeQt feeds us a URL with a password, override memory temporarily
            if (!qurl.userName().isEmpty()) {
                m_savedUser = qurl.userName();
                m_savedPass = qurl.password();
            }
        } else {
            radioTnfs->setChecked(true);
        }

        if (!host.isEmpty()) {
            hostCombo->setEditText(host);
            onConnect();
        }
    }

    // Ensure the padlock is in the correct state right when the window opens
    onProtocolChanged();

    // Set padlock tooltip on boot if credentials are known
    if (!m_savedUser.isEmpty() && m_savedUser.toLower() != "anonymous") {
        btnLogin->setToolTip(tr("Credentials Set: ") + m_savedUser);
    }
}

NetworkBrowser::~NetworkBrowser()
{
    if (connectWatcher->isRunning()) {
        connectWatcher->waitForFinished();
    }
    if (fetchWatcher->isRunning()) {
        fetchWatcher->waitForFinished();
    }
}

// --- Dynamic Padlock Toggler ---
void NetworkBrowser::onProtocolChanged()
{
    if (radioFtp->isChecked()) {
        btnLogin->setEnabled(true);
    } else {
        btnLogin->setEnabled(false);
    }
}

void NetworkBrowser::onLoginClicked()
{
    QDialog authDialog(this);
    authDialog.setWindowTitle(tr("Server Authentication"));
    authDialog.setMinimumWidth(300);

    QFormLayout form(&authDialog);

    QLineEdit userEdit;
    userEdit.setText(m_savedUser);
    userEdit.setPlaceholderText("anonymous");

    QLineEdit passEdit;
    passEdit.setText(m_savedPass);
    passEdit.setEchoMode(QLineEdit::Password);
    passEdit.setPlaceholderText(tr("Leave blank for default"));

    form.addRow(tr("Username:"), &userEdit);
    form.addRow(tr("Password:"), &passEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &authDialog);

    // --- NEW: Add the Anonymous reset button ---
    QPushButton *btnAnon = buttonBox.addButton(tr("Anonymous"), QDialogButtonBox::ActionRole);

    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &authDialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &authDialog, &QDialog::reject);

    // --- NEW: Clear fields and trigger accept when Anonymous is clicked ---
    connect(btnAnon, &QPushButton::clicked, [&]() {
        userEdit.clear();
        passEdit.clear();
        authDialog.accept();
    });

    if (authDialog.exec() == QDialog::Accepted) {
        m_savedUser = userEdit.text().trimmed();
        m_savedPass = passEdit.text().trimmed();

        // --- Save to disk! ---
        QSettings settings("AspeQt", "TNFS");
        settings.setValue("ftpUser", m_savedUser);
        settings.setValue("ftpPass", m_savedPass);

        if (!m_savedUser.isEmpty() && m_savedUser.toLower() != "anonymous") {
            btnLogin->setToolTip(tr("Credentials Set: ") + m_savedUser);
        } else {
            btnLogin->setToolTip(tr("Set Server Credentials"));
        }

        // --- Fire the connection immediately if a host is ready! ---
        if (!hostCombo->currentText().trimmed().isEmpty()) {
            onConnect();
        }
    }
}

QString NetworkBrowser::getSelectedUrl() const { return selectedUrl; }

QIcon NetworkBrowser::getIcon(const QString &name)
{
    if (name == "folder") return QIcon(":/icons/silk-icons/icons/folder.png");
    if (name == "disk")   return QIcon(":/icons/silk-icons/icons/drive_disk.png");
    if (name == "file")   return QIcon(":/icons/silk-icons/icons/page_white.png");

    if (name == "view-sort-ascending") {
        if (QIcon::hasThemeIcon("view-sort-ascending")) return QIcon::fromTheme("view-sort-ascending");
        return QIcon(":/icons/silk-icons/icons/arrow_up.png");
    }
    if (name == "view-sort-descending") {
        if (QIcon::hasThemeIcon("view-sort-descending")) return QIcon::fromTheme("view-sort-descending");
        return QIcon(":/icons/silk-icons/icons/arrow_down.png");
    }

    if (QIcon::hasThemeIcon(name)) return QIcon::fromTheme(name);

    if (name == "folder") return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
    if (name == "file")   return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
    if (name == "disk")   return QApplication::style()->standardIcon(QStyle::SP_DriveFDIcon);

    return QIcon();
}

void NetworkBrowser::onConnect()
{
    QString rawInput = hostCombo->currentText().trimmed();
    QString host = rawInput;

    if (m_client) {
        delete m_client;
        m_client = nullptr;
    }

    // --- Track the old protocol ---
    QString oldProtocol = m_activeProtocol;

    if (radioFtp->isChecked()) {
        m_activeProtocol = "ftp";
        m_client = new FtpClient(this);
    } else {
        m_activeProtocol = "tnfs";
        m_client = new TnfsClient(this);
    }

    if (host.contains("://")) {
        QUrl parsed(host);
        if (!parsed.host().isEmpty()) {
            host = parsed.host();
        }
    }

    int slashIdx = host.indexOf('/');
    if (slashIdx != -1) {
        host = host.left(slashIdx);
    }

    // --- Reset the path if EITHER the host OR protocol changed! ---
    if (!m_activeHost.isEmpty() && (host != m_activeHost || m_activeProtocol != oldProtocol)) {
        currentPath = "/";
    }
    m_activeHost = host;

    hostCombo->setEnabled(false);
    btnConnect->setEnabled(false);
    fileList->setEnabled(false);
    fileList->clear();

    hostCombo->setEditText(host);

    progressBar->setVisible(true);
    statusLabel->setText(tr("Connecting to %1...").arg(host));

    QApplication::processEvents();

    bool success = false;

    if (m_activeProtocol == "ftp") {
        FtpClient* ftp = static_cast<FtpClient*>(m_client);
        ftp->setCredentials(m_savedUser, m_savedPass);
        success = ftp->connectToHost(host);
    } else {
        success = m_client->connectToHost(host);
        if (success) {
            success = static_cast<TnfsClient*>(m_client)->mount("/");
        }
    }

    hostCombo->setEnabled(true);
    btnConnect->setEnabled(true);
    fileList->setEnabled(true);
    progressBar->setVisible(false);

    if (success) {
        statusLabel->setText(tr("Connected: %1").arg(currentPath));

        if (hostCombo->findText(host) == -1) {
            hostCombo->addItem(host);
        }

        QSettings settings("AspeQt", "TNFS");
        QStringList history;
        for (int i = 0; i < hostCombo->count(); ++i) {
            history << hostCombo->itemText(i);
        }
        settings.setValue("hostHistory", history);

        refreshList();
    } else {
        statusLabel->setText(tr("Connection Failed."));
        QMessageBox::critical(this, tr("Connection Error"),
                              tr("Could not reach host '%1'.\nCheck credentials or hostname.").arg(host));
    }
}

void NetworkBrowser::onConnectionFinished()
{
    hostCombo->setEnabled(true);
    btnConnect->setEnabled(true);
    fileList->setEnabled(true);
    progressBar->setVisible(false);

    bool success = connectWatcher->result();

    if (success) {
        statusLabel->setText(tr("Connected: %1").arg(currentPath));

        if (hostCombo->findText(m_activeHost) == -1) {
            hostCombo->addItem(m_activeHost);
        }
        hostCombo->setEditText(m_activeHost);

        QSettings settings("AspeQt", "TNFS");
        QStringList history;
        for (int i = 0; i < hostCombo->count(); ++i) {
            history << hostCombo->itemText(i);
        }
        settings.setValue("hostHistory", history);

        refreshList();
    } else {
        statusLabel->setText(tr("Connection Failed."));
        QMessageBox::critical(this, tr("Connection Error"),
                              tr("Could not reach host '%1'.\nCheck credentials or hostname.").arg(m_activeHost));
    }
}

void NetworkBrowser::refreshList()
{
    fileList->clear();
    btnMore->setVisible(false);
    btnMore->setEnabled(true);
    btnMore->setText(tr("More..."));

    statusLabel->setText(tr("Fetching %1...").arg(currentPath));

    m_sortAscending = true;

    QIcon icon = QIcon::fromTheme("view-sort-ascending");
    if (icon.isNull()) btnSort->setText("A-Z");
    else btnSort->setIcon(icon);

    QApplication::processEvents();

    if (m_client->beginListing(currentPath)) {
        m_isFirstBatch = true;
        loadNextBatch();
    } else {
        statusLabel->setText(tr("Error opening directory."));
    }
}

void NetworkBrowser::loadNextBatch()
{
    if (fetchWatcher->isRunning()) return;

    setCursor(Qt::WaitCursor);
    btnMore->setEnabled(false);
    progressBar->setVisible(true);
    statusLabel->setText(tr("Fetching %1...").arg(currentPath));

    QFuture<QList<INetworkClient::DirectoryEntry>> future = QtConcurrent::run([this]() {
        return m_client->fetchNextBatch(20);
    });
    fetchWatcher->setFuture(future);
}

void NetworkBrowser::onFetchFinished()
{
    auto newItems = fetchWatcher->result();
    bool finished = m_client->isListingFinished();

    if (m_isFirstBatch && finished) {
        std::sort(newItems.begin(), newItems.end(), [](const INetworkClient::DirectoryEntry &a, const INetworkClient::DirectoryEntry &b) {
            if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
            return a.name.toLower() < b.name.toLower();
        });
    }

    for (const auto &entry : newItems) {
        QListWidgetItem *item = new QListWidgetItem(entry.name);

        if (entry.isDirectory) {
            item->setIcon(getIcon("folder"));
            item->setData(Qt::UserRole, true);
        } else {
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

    if (!m_isFirstBatch && !newItems.isEmpty()) fileList->scrollToBottom();

    m_isFirstBatch = false;

    statusLabel->setText(tr("Browsing: %1").arg(currentPath));
    progressBar->setVisible(false);
    unsetCursor();
}

void NetworkBrowser::onItemDoubleClicked(QListWidgetItem *item)
{
    if (!item) return;

    QString name = item->text();
    QString targetPath = currentPath;
    if (!targetPath.endsWith("/")) targetPath += "/";
    targetPath += name;

    bool isDir = item->data(Qt::UserRole).toBool();

    if (!isDir && (name.contains(".") || targetPath.contains("."))) {
        QUrl url;

        url.setScheme(m_activeProtocol);

        if (!m_savedUser.isEmpty() && m_savedUser.toLower() != "anonymous") {
            url.setUserName(m_savedUser);
            url.setPassword(m_savedPass);
        }

        url.setHost(m_activeHost);
        url.setPath(targetPath);

        selectedUrl = url.toString();
        accept();
        return;
    }

    currentPath = targetPath;
    statusLabel->setText(tr("Browsing: %1").arg(currentPath));
    refreshList();
}

void NetworkBrowser::onBackClicked()
{
    if (currentPath == "/" || currentPath.isEmpty()) return;
    int lastSlash = currentPath.lastIndexOf('/', currentPath.length() - 2);
    QString parentPath = (lastSlash != -1) ? currentPath.left(lastSlash + 1) : "/";
    currentPath = parentPath;
    refreshList();
}

void NetworkBrowser::onClearHistory()
{
    if (QMessageBox::StandardButton::Yes == QMessageBox::question(this, tr("Clear History"),
                                                                  tr("Clear all saved hosts?"), QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No)) {
        hostCombo->clear();
        QSettings settings("AspeQt", "TNFS");
        settings.remove("hostHistory");
        statusLabel->setText(tr("History cleared."));
    }
}

void NetworkBrowser::onCancelClicked() { reject(); }
void NetworkBrowser::onMoreClicked() { loadNextBatch(); }

void NetworkBrowser::onSortClicked()
{
    m_sortAscending = !m_sortAscending;

    QIcon icon = m_sortAscending ? getIcon("view-sort-ascending")
                                 : getIcon("view-sort-descending");

    if (!icon.isNull()) btnSort->setIcon(icon);

    QList<QListWidgetItem*> items;
    while (fileList->count() > 0) items.append(fileList->takeItem(0));

    std::sort(items.begin(), items.end(), [this](QListWidgetItem *a, QListWidgetItem *b) {
        bool dirA = a->data(Qt::UserRole).toBool();
        bool dirB = b->data(Qt::UserRole).toBool();
        if (dirA != dirB) return dirA > dirB;
        return m_sortAscending ? a->text().toLower() < b->text().toLower()
                               : a->text().toLower() > b->text().toLower();
    });

    for (auto *item : items) fileList->addItem(item);
}