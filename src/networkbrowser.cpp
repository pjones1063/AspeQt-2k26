#include "networkbrowser.h"
#include "tnfsclient.h"
#include "ftpclient.h"
#include "sftpclient.h"
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
    radioSftp = new QRadioButton(tr("SFTP"), this);
    radioTnfs->setChecked(true); // Default

    protocolLayout->addWidget(new QLabel(tr("Protocol:")));
    protocolLayout->addWidget(radioTnfs);
    protocolLayout->addWidget(radioFtp);
    protocolLayout->addWidget(radioSftp);
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
    connect(radioSftp, &QRadioButton::toggled, this, &NetworkBrowser::onProtocolChanged);

    currentPath = "/";
    m_activeHost = "";
    m_activePort = 0;
    m_activeProtocol = "tnfs";

    // --- Sanitize UI and set initial padlock state BEFORE auto-navigating ---
    onProtocolChanged();

    // --- Auto-Navigate Logic ---
    if (!initialUrl.isEmpty()) {
        QUrl qurl(initialUrl);
        QString host = qurl.host();
        QString path = qurl.path();
        QString scheme = qurl.scheme().toLower();

        if (!path.endsWith("/")) {
            int lastSlash = path.lastIndexOf('/');
            if (lastSlash != -1) path = path.left(lastSlash + 1);
            else path = "/";
        }

        currentPath = path;

        // Sync UI toggles with incoming URL scheme including SFTP
        if (scheme == "ftp" || scheme == "sftp") {
            if (scheme == "ftp") radioFtp->setChecked(true);
            else radioSftp->setChecked(true);

            // If AspeQt feeds us a URL with a password, override memory temporarily
            if (!qurl.userName().isEmpty()) {
                m_savedUser = qurl.userName(QUrl::FullyDecoded);
                m_savedPass = qurl.password(QUrl::FullyDecoded);
            }
        } else {
            radioTnfs->setChecked(true);
        }

        if (!host.isEmpty()) {
            hostCombo->setEditText(host);
            onConnect();
        }
    }

    // Set padlock tooltip on boot if credentials are known
    if (!m_savedUser.isEmpty() && m_savedUser.toLower() != "anonymous") {
        btnLogin->setToolTip(tr("Credentials Set: ") + m_savedUser);
    }
}

NetworkBrowser::~NetworkBrowser()
{
    if (m_client) {
        delete m_client;
        m_client = nullptr;
    }

    if (connectWatcher->isRunning()) {
        connectWatcher->waitForFinished();
    }
    if (fetchWatcher->isRunning()) {
        fetchWatcher->waitForFinished();
    }
}

// --- Dynamic Padlock & State Sanitizer ---
void NetworkBrowser::onProtocolChanged()
{
    // 1. Enable Padlock for BOTH FTP and SFTP
    if (radioFtp->isChecked() || radioSftp->isChecked()) {
        btnLogin->setEnabled(true);
    } else {
        btnLogin->setEnabled(false);
    }

    // 2. Kill any active connection immediately
    if (m_client) {
        delete m_client;
        m_client = nullptr;
    }

    // 3. Sanitize the UI to prevent interacting with stale data
    fileList->clear();
    statusLabel->setText(tr("Not Connected"));
    btnMore->setVisible(false);

    // 4. Reset internal state so the next "Connect" click starts fresh
    currentPath = "/";
    m_activeHost = "";
    m_activePort = 0;
}

void NetworkBrowser::onLoginClicked()
{
    QDialog authDialog(this);
    authDialog.setWindowTitle(tr("Server Authentication"));
    authDialog.setMinimumWidth(300);

    QFormLayout form(&authDialog);

    QLineEdit userEdit;
    userEdit.setText(m_savedUser);

    QLineEdit passEdit;
    passEdit.setText(m_savedPass);
    passEdit.setEchoMode(QLineEdit::Password);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &authDialog);

    // Add the Anonymous reset button
    QPushButton *btnAnon = buttonBox.addButton(tr("Anonymous"), QDialogButtonBox::ActionRole);

    // Dynamically adjust UI for SFTP strict auth
    if (radioSftp->isChecked()) {
        btnAnon->setEnabled(false);
        btnAnon->setToolTip(tr("SFTP requires an explicit SSH username and password."));
        userEdit.setPlaceholderText(tr("Required for SFTP"));
        passEdit.setPlaceholderText(tr("Required for SFTP"));
    } else {
        userEdit.setPlaceholderText("anonymous");
        passEdit.setPlaceholderText(tr("Leave blank for default"));
    }

    form.addRow(tr("Username:"), &userEdit);
    form.addRow(tr("Password:"), &passEdit);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &authDialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &authDialog, &QDialog::reject);

    // Clear fields and trigger accept when Anonymous is clicked
    connect(btnAnon, &QPushButton::clicked, [&]() {
        userEdit.clear();
        passEdit.clear();
        authDialog.accept();
    });

    if (authDialog.exec() == QDialog::Accepted) {
        m_savedUser = userEdit.text().trimmed();
        m_savedPass = passEdit.text(); // Passwords should NEVER be trimmed!

        // Save to disk!
        QSettings settings("AspeQt", "TNFS");
        settings.setValue("ftpUser", m_savedUser);
        settings.setValue("ftpPass", m_savedPass);

        if (!m_savedUser.isEmpty() && m_savedUser.toLower() != "anonymous") {
            btnLogin->setToolTip(tr("Credentials Set: ") + m_savedUser);
        } else {
            btnLogin->setToolTip(tr("Set Server Credentials"));
        }

        // Fire the connection immediately if a host is ready!
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
    quint16 port = 0; // Default port initialization

    if (m_client) {
        delete m_client;
        m_client = nullptr;
    }

    // Track the old protocol
    QString oldProtocol = m_activeProtocol;

    if (radioFtp->isChecked()) {
        m_activeProtocol = "ftp";
        m_client = new FtpClient(this);
    } else if (radioSftp->isChecked()) {
        m_activeProtocol = "sftp";
        m_client = new SftpClient(this);
    } else {
        m_activeProtocol = "tnfs";
        m_client = new TnfsClient(this);
    }

    QString urlToParse = host;
    if (!urlToParse.contains("://")) {
        urlToParse = m_activeProtocol + "://" + urlToParse;
    }

    // Create local credentials that can be overridden by the address bar
    QString activeUser = m_savedUser;
    QString activePass = m_savedPass;

    // Let QUrl handle port and credential extraction intelligently
    QUrl parsed(urlToParse);
    if (!parsed.host().isEmpty()) {
        host = parsed.host();
        port = parsed.port(0);

        // If the user typed user:pass@host in the combo box, USE IT!
        if (!parsed.userName().isEmpty()) {
            activeUser = parsed.userName(QUrl::FullyDecoded);
            activePass = parsed.password(QUrl::FullyDecoded);
        }
    }

    int slashIdx = host.indexOf('/');
    if (slashIdx != -1) {
        host = host.left(slashIdx);
    }

    // Reset the path if EITHER the host OR protocol changed!
    if (!m_activeHost.isEmpty() && (host != m_activeHost || m_activeProtocol != oldProtocol)) {
        currentPath = "/";
    }
    m_activeHost = host;
    m_activePort = port; // Save the port for later file transfers!

    hostCombo->setEnabled(false);
    btnConnect->setEnabled(false);
    btnLogin->setEnabled(false);
    radioTnfs->setEnabled(false);
    radioFtp->setEnabled(false);
    radioSftp->setEnabled(false);
    fileList->setEnabled(false);

    fileList->clear();

    hostCombo->setEditText(rawInput); // Keep what the user actually typed in the box

    progressBar->setVisible(true);

    // UI Feedback for custom ports
    if (port != 0) {
        statusLabel->setText(tr("Connecting to %1 on port %2...").arg(host).arg(port));
    } else {
        statusLabel->setText(tr("Connecting to %1...").arg(host));
    }

    QApplication::processEvents();

    bool success = false;

    // Inject credentials and connect based on Protocol, using the dynamic activeUser/activePass
    if (m_activeProtocol == "ftp") {
        FtpClient* ftp = static_cast<FtpClient*>(m_client);
        ftp->setCredentials(activeUser, activePass);
        success = ftp->connectToHost(host, port);
    } else if (m_activeProtocol == "sftp") {
        SftpClient* sftp = static_cast<SftpClient*>(m_client);
        sftp->setCredentials(activeUser, activePass);
        success = sftp->connectToHost(host, port);
    } else {
        success = m_client->connectToHost(host, port);
        if (success) {
            success = static_cast<TnfsClient*>(m_client)->mount("/");
        }
    }

    hostCombo->setEnabled(true);
    btnConnect->setEnabled(true);
    fileList->setEnabled(true);
    progressBar->setVisible(false);
    radioTnfs->setEnabled(true);
    radioFtp->setEnabled(true);
    radioSftp->setEnabled(true);
    btnLogin->setEnabled(radioFtp->isChecked() || radioSftp->isChecked());
    progressBar->setVisible(false);

    if (success) {
        statusLabel->setText(tr("Connected: %1").arg(currentPath));

        if (hostCombo->findText(rawInput) == -1) {
            hostCombo->addItem(rawInput);
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
                              tr("Could not reach host '%1'.\nCheck credentials, hostname, or port.").arg(host));
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

        // Safely map credentials into the final URL string for TnfsImage
        if ((m_activeProtocol == "ftp" || m_activeProtocol == "sftp") && !m_savedUser.isEmpty() && m_savedUser.toLower() != "anonymous") {
            url.setUserName(m_savedUser);
            url.setPassword(m_savedPass);
        }

        url.setHost(m_activeHost);

        // Inject the custom port if one was used
        if (m_activePort != 0) {
            url.setPort(m_activePort);
        }

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
