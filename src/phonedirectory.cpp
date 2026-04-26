#include "phonedirectory.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFile>
#include <QDomDocument>
#include <QTextStream>
#include <QFormLayout>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QDebug>
#include <QToolButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QCloseEvent>
#include <QFileDialog> // <-- NEW: Required for the file browser

PhoneDirectory::PhoneDirectory(QWidget *parent) : QDialog(parent), m_isDirty(false) {
    setWindowTitle(tr("BBS Phonebook"));
    resize(700, 450);

    // Layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Search Bar
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search BBS Name..."));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PhoneDirectory::onSearch);
    mainLayout->addWidget(m_searchEdit);

    // List
    m_tree = new QTreeWidget(this);
    // Columns: Name, IP, Port, Protocol, User
    m_tree->setHeaderLabels({tr("BBS Name"), tr("Address"), tr("Port"), tr("Protocol"), tr("User ID")});

    // Column Resizing Logic
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setAlternatingRowColors(true);
    m_tree->setRootIsDecorated(false);

    mainLayout->addWidget(m_tree);

    // Bottom Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();

    QPushButton *addBtn = new QPushButton(tr("Add"), this);
    m_editBtn = new QPushButton(tr("Edit"), this);
    QPushButton *delBtn = new QPushButton(tr("Delete"), this);

    m_saveBtn = new QPushButton(tr("Save Changes"), this);
    m_saveBtn->setEnabled(false); // Disabled until a change is made
    m_saveBtn->setStyleSheet("background-color: #2D5A2D; color: white; font-weight: bold; border-radius: 4px; padding: 4px;");

    m_dialBtn = new QPushButton(tr("Dial Selected"), this);
    QPushButton *closeBtn = new QPushButton(tr("Close"), this);

    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(m_editBtn);
    btnLayout->addWidget(delBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_saveBtn);
    btnLayout->addWidget(m_dialBtn);
    btnLayout->addWidget(closeBtn);

    mainLayout->addLayout(btnLayout);

    // Connections
    connect(addBtn, &QPushButton::clicked, this, &PhoneDirectory::onAddClicked);
    connect(m_editBtn, &QPushButton::clicked, this, &PhoneDirectory::onEditClicked);
    connect(delBtn, &QPushButton::clicked, this, &PhoneDirectory::onDeleteClicked);
    connect(m_saveBtn, &QPushButton::clicked, this, &PhoneDirectory::onSaveClicked);
    connect(m_dialBtn, &QPushButton::clicked, this, &PhoneDirectory::onDialClicked);
    connect(closeBtn, &QPushButton::clicked, this, &PhoneDirectory::onCloseClicked);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &PhoneDirectory::onDialClicked);

    // Initial state
    m_dialBtn->setEnabled(false);
    m_editBtn->setEnabled(false);
    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, [this]() {
        bool hasSelection = m_tree->selectedItems().count() > 0;
        m_dialBtn->setEnabled(hasSelection);
        m_editBtn->setEnabled(hasSelection);
    });
}

void PhoneDirectory::loadFromFile(const QString &path) {
    m_filePath = path;
    parseXml();
    refreshList();
    m_isDirty = false;
    m_saveBtn->setEnabled(false);
}

BbsEntry PhoneDirectory::getSelectedEntry() {
    QTreeWidgetItem *item = m_tree->currentItem();
    if (!item) return BbsEntry();

    int index = item->data(0, Qt::UserRole).toInt();
    if (index >= 0 && index < m_entries.size()) {
        return m_entries[index];
    }
    return BbsEntry();
}

void PhoneDirectory::parseXml() {
    m_entries.clear();
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QDomDocument doc;
    if (!doc.setContent(&file)) {
        file.close();
        return;
    }
    file.close();

    QDomNodeList nodes = doc.elementsByTagName("BBS");
    for (int i = 0; i < nodes.size(); ++i) {
        QDomElement e = nodes.at(i).toElement();
        BbsEntry entry;
        entry.name = e.attribute("name");
        entry.ip = e.attribute("ip");
        entry.port = e.attribute("port", "23").toInt();
        entry.protocol = e.attribute("protocol", "TELNET");
        entry.login = e.attribute("login");
        entry.password = e.attribute("password");

        // --- NEW: Read Key File Attribute ---
        entry.privateKey = e.attribute("keyfile");

        m_entries.append(entry);
    }
}

void PhoneDirectory::saveToFile() {
    QDomDocument doc;
    QDomElement root = doc.createElement("Phonebook");
    doc.appendChild(root);

    for (const BbsEntry &entry : m_entries) {
        QDomElement bbs = doc.createElement("BBS");
        bbs.setAttribute("name", entry.name);
        bbs.setAttribute("ip", entry.ip);
        bbs.setAttribute("port", entry.port);
        bbs.setAttribute("protocol", entry.protocol);
        bbs.setAttribute("login", entry.login);
        bbs.setAttribute("password", entry.password);

        // --- NEW: Write Key File Attribute ---
        bbs.setAttribute("keyfile", entry.privateKey);

        root.appendChild(bbs);
    }

    QFile file(m_filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << doc.toString(4); // 4 spaces indent
        file.close();

        m_isDirty = false;
        m_saveBtn->setEnabled(false);
        QMessageBox::information(this, tr("Saved"), tr("Phonebook saved successfully."));
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save phonebook to:\n%1").arg(m_filePath));
    }
}

void PhoneDirectory::refreshList(const QString &filter) {
    m_tree->clear();
    for (int i = 0; i < m_entries.size(); ++i) {
        const BbsEntry &entry = m_entries[i];

        if (!filter.isEmpty() && !entry.name.contains(filter, Qt::CaseInsensitive)) {
            continue;
        }

        QTreeWidgetItem *item = new QTreeWidgetItem(m_tree);
        item->setText(0, entry.name);
        item->setText(1, entry.ip);
        item->setText(2, QString::number(entry.port));
        item->setText(3, entry.protocol);
        item->setText(4, entry.login);

        // Store the original index so we can retrieve it even when filtered
        item->setData(0, Qt::UserRole, i);
    }
}

void PhoneDirectory::onSearch(const QString &text) {
    refreshList(text);
}

void PhoneDirectory::onDialClicked() {
    if (m_isDirty) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Unsaved Changes"),
                                                                  tr("You have unsaved changes. Do you want to save the phonebook before dialing?"),
                                                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Cancel) return;
        if (reply == QMessageBox::Yes) saveToFile();
    }
    accept(); // Closes the dialog and returns Accepted
}

void PhoneDirectory::onSaveClicked() {
    saveToFile();
}

bool PhoneDirectory::runEditDialog(BbsEntry &entry) {
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle(tr("Edit BBS"));
    dlg->resize(400, 300);

    QFormLayout *formLayout = new QFormLayout(dlg);

    QLineEdit *nameEdit = new QLineEdit(dlg);
    nameEdit->setText(entry.name);
    QLineEdit *ipEdit = new QLineEdit(dlg);
    ipEdit->setText(entry.ip);
    QLineEdit *portEdit = new QLineEdit(dlg);
    portEdit->setText(QString::number(entry.port));

    // Protocol Selection
    QHBoxLayout *protoLayout = new QHBoxLayout();
    QRadioButton *rbTelnet = new QRadioButton("Telnet", dlg);
    QRadioButton *rbSsh = new QRadioButton("SSH", dlg);
    QRadioButton *rbSshAuth = new QRadioButton("SSH-Auth", dlg);

    QButtonGroup *bg = new QButtonGroup(dlg);
    bg->addButton(rbTelnet);
    bg->addButton(rbSsh);
    bg->addButton(rbSshAuth);

    protoLayout->addWidget(rbTelnet);
    protoLayout->addWidget(rbSsh);
    protoLayout->addWidget(rbSshAuth);

    if (entry.protocol == "SSH") rbSsh->setChecked(true);
    else if (entry.protocol == "SSH-AUTH") rbSshAuth->setChecked(true);
    else rbTelnet->setChecked(true);

    QLineEdit *userEdit = new QLineEdit(dlg);
    userEdit->setText(entry.login);
    QLineEdit *passEdit = new QLineEdit(dlg);
    passEdit->setText(entry.password);
    passEdit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    passEdit->setToolTip(tr("Used for standard auth, or as the Passphrase for an encrypted Key."));

    // --- NEW: Private Key File Picker ---
    QLineEdit *keyEdit = new QLineEdit(dlg);
    keyEdit->setText(entry.privateKey);
    keyEdit->setPlaceholderText(tr("Optional: Path to private key file"));

    QPushButton *btnBrowseKey = new QPushButton("...", dlg);
    btnBrowseKey->setFixedWidth(30);

    QHBoxLayout *keyLayout = new QHBoxLayout();
    keyLayout->addWidget(keyEdit);
    keyLayout->addWidget(btnBrowseKey);

    connect(btnBrowseKey, &QPushButton::clicked, [dlg, keyEdit]() {
        QString file = QFileDialog::getOpenFileName(dlg, tr("Select Private Key File"), "", tr("All Files (*)"));
        if (!file.isEmpty()) keyEdit->setText(file);
    });

    formLayout->addRow(tr("BBS Name:"), nameEdit);
    formLayout->addRow(tr("Address (IP/DNS):"), ipEdit);
    formLayout->addRow(tr("Port:"), portEdit);
    formLayout->addRow(tr("Protocol:"), protoLayout);
    formLayout->addRow(tr("User ID:"), userEdit);
    formLayout->addRow(tr("Password/Passphrase:"), passEdit);
    formLayout->addRow(tr("Private Key:"), keyLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dlg);
    connect(buttonBox, &QDialogButtonBox::accepted, dlg, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dlg, &QDialog::reject);
    formLayout->addWidget(buttonBox);

    if (dlg->exec() == QDialog::Accepted) {
        entry.name = nameEdit->text();
        entry.ip = ipEdit->text();
        entry.port = portEdit->text().toInt();
        entry.login = userEdit->text();
        entry.password = passEdit->text();

        // --- NEW: Save Key Path ---
        entry.privateKey = keyEdit->text();

        if (rbSshAuth->isChecked()) entry.protocol = "SSH-AUTH";
        else if (rbSsh->isChecked()) entry.protocol = "SSH";
        else entry.protocol = "TELNET";

        return true;
    }

    return false;
}

void PhoneDirectory::onAddClicked() {
    BbsEntry newEntry;
    newEntry.name = "New BBS";
    newEntry.ip = "";
    newEntry.port = 23;
    newEntry.protocol = "TELNET";

    if (runEditDialog(newEntry)) {
        m_entries.append(newEntry);
        refreshList();
        m_isDirty = true;
        m_saveBtn->setEnabled(true);

        int lastVisualIndex = m_tree->topLevelItemCount() - 1;
        if (lastVisualIndex >= 0) {
            m_tree->setCurrentItem(m_tree->topLevelItem(lastVisualIndex));
            m_tree->scrollToBottom();
        }
    }
}

void PhoneDirectory::onDeleteClicked() {
    QTreeWidgetItem *item = m_tree->currentItem();
    if (!item) return;
    int index = item->data(0, Qt::UserRole).toInt();

    if (QMessageBox::question(this, tr("Confirm"), tr("Delete this entry?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        m_entries.removeAt(index);
        refreshList();
        m_isDirty = true;
        m_saveBtn->setEnabled(true);
    }
}

void PhoneDirectory::onEditClicked() {
    QTreeWidgetItem *item = m_tree->currentItem();
    if (!item) return;
    int index = item->data(0, Qt::UserRole).toInt();

    if (runEditDialog(m_entries[index])) {
        refreshList();
        m_isDirty = true;
        m_saveBtn->setEnabled(true);

        // Reselect the edited item
        for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
            if (m_tree->topLevelItem(i)->data(0, Qt::UserRole).toInt() == index) {
                m_tree->setCurrentItem(m_tree->topLevelItem(i));
                break;
            }
        }
    }
}

bool PhoneDirectory::checkUnsavedChanges(const QString &actionName) {
    if (!m_isDirty) return true;

    QMessageBox::StandardButton reply = QMessageBox::question(this, tr("Unsaved Changes"),
                                                              tr("You have unsaved changes. Do you want to save before %1?").arg(actionName),
                                                              QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    if (reply == QMessageBox::Cancel) return false;

    if (reply == QMessageBox::Yes) {
        saveToFile();
    }
    return true;
}

void PhoneDirectory::onCloseClicked() {
    close();
}

void PhoneDirectory::closeEvent(QCloseEvent *event) {
    if (checkUnsavedChanges("closing")) {
        event->accept();
    } else {
        event->ignore();
    }
}
