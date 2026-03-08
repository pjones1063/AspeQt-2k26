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
    mainLayout->addWidget(m_tree);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();

    m_editBtn = new QPushButton(tr("Edit Entry"), this);
    connect(m_editBtn, &QPushButton::clicked, this, &PhoneDirectory::onEditClicked);

    m_saveBtn = new QPushButton(tr("Save to XML"), this);
    connect(m_saveBtn, &QPushButton::clicked, this, &PhoneDirectory::onSaveClicked);

    m_dialBtn = new QPushButton(tr("Dial Selected"), this);
    connect(m_dialBtn, &QPushButton::clicked, this, &PhoneDirectory::onDialClicked);

    QPushButton *addBtn = new QPushButton(tr("Add"), this);
    connect(addBtn, &QPushButton::clicked, this, &PhoneDirectory::onAddClicked);

    QPushButton *delBtn = new QPushButton(tr("Delete"), this);
    connect(delBtn, &QPushButton::clicked, this, &PhoneDirectory::onDeleteClicked);

    QPushButton *closeBtn = new QPushButton(tr("Close"), this);
    // Connect to custom slot to check for changes
    connect(closeBtn, &QPushButton::clicked, this, &PhoneDirectory::onCloseClicked);

    btnLayout->insertWidget(0, addBtn);
    btnLayout->insertWidget(1, delBtn);
    btnLayout->addWidget(m_editBtn);
    btnLayout->addWidget(m_saveBtn);

    btnLayout->addStretch();
    btnLayout->addWidget(m_dialBtn);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
    m_dialBtn->setDefault(true);
    m_dialBtn->setFocus();

    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &PhoneDirectory::onDialClicked);
}

void PhoneDirectory::loadFromFile(const QString &path) {
    if (path.isEmpty()) return;
    m_filePath = path;
    parseXml();
    refreshList();
    m_isDirty = false; // Reset dirty flag after load
}

void PhoneDirectory::parseXml() {
    m_entries.clear();
    QFile file(m_filePath);
    if (!file.open(QIODevice::ReadOnly)) return;

    QDomDocument doc;
    if (!doc.setContent(&file)) { file.close(); return; }

    QDomNodeList list = doc.elementsByTagName("BBS");
    for (int i = 0; i < list.size(); i++) {
        QDomElement e = list.at(i).toElement();
        BbsEntry bbs;
        bbs.name = e.attribute("name");
        bbs.ip = e.attribute("ip");
        bbs.port = e.attribute("port").toInt();
        bbs.protocol = e.attribute("protocol");
        bbs.login = e.attribute("login");
        bbs.password = e.attribute("password");
        m_entries.append(bbs);
    }
    file.close();
}

void PhoneDirectory::saveToFile() {
    if (m_filePath.isEmpty()) return;

    QDomDocument doc;
    QDomElement root = doc.createElement("EtherTerm");
    doc.appendChild(root);

    QDomElement pb = doc.createElement("Phonebook");
    pb.setAttribute("version", "1.0");
    root.appendChild(pb);

    for (const BbsEntry &entry : m_entries) {
        QDomElement tag = doc.createElement("BBS");
        tag.setAttribute("name", entry.name);
        tag.setAttribute("ip", entry.ip);
        tag.setAttribute("port", entry.port);
        tag.setAttribute("protocol", entry.protocol);
        tag.setAttribute("login", entry.login);
        tag.setAttribute("password", entry.password);
        pb.appendChild(tag);
    }

    QFile file(m_filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << doc.toString();
        file.close();

        m_isDirty = false; // Changes are now saved
        QMessageBox::information(this, tr("Saved"), tr("Phonebook saved successfully!"));
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Could not write to file."));
    }
}

// Central logic for checking unsaved changes
bool PhoneDirectory::checkUnsavedChanges(const QString &actionName) {
    if (!m_isDirty) return true;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("Unsaved Changes"),
                                  tr("You have unsaved changes. Save before %1?").arg(actionName),
                                  QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel);

    if (reply == QMessageBox::StandardButton::Yes) {
        saveToFile();
        return true; // Saved and ready to go
    } else if (reply == QMessageBox::StandardButton::No) {
        return true; // Proceed without saving
    } else {
        return false; // Cancel action
    }
}

void PhoneDirectory::refreshList(const QString &filter) {
    m_tree->clear();
    for (int i = 0; i < m_entries.size(); ++i) {
        const BbsEntry &e = m_entries[i];

        if (filter.isEmpty() || e.name.contains(filter, Qt::CaseInsensitive)) {
            QTreeWidgetItem *item = new QTreeWidgetItem(m_tree);
            item->setText(0, e.name);
            item->setText(1, e.ip);
            item->setText(2, QString::number(e.port));
            item->setText(3, e.protocol.isEmpty() ? "TELNET" : e.protocol);
            item->setText(4, e.login);
            item->setData(0, Qt::UserRole, i);
        }
    }
}

void PhoneDirectory::onEditClicked() {
    QTreeWidgetItem *item = m_tree->currentItem();
    if (!item) return;

    int index = item->data(0, Qt::UserRole).toInt();
    if (index < 0 || index >= m_entries.size()) return;

    BbsEntry &entry = m_entries[index];

    if (runEditDialog(entry)) {
        item->setText(0, entry.name);
        item->setText(1, entry.ip);
        item->setText(2, QString::number(entry.port));
        item->setText(3, entry.protocol);
        item->setText(4, entry.login);

        m_isDirty = true; // Flag as dirty
    }
}

void PhoneDirectory::onSaveClicked() {
    saveToFile();
}

void PhoneDirectory::onSearch(const QString &text) {
    refreshList(text);
}

void PhoneDirectory::onDialClicked() {
    if (m_tree->currentItem()) {
        // Check changes before dialing
        if (checkUnsavedChanges("dialing")) {
            accept();
        }
    }
}

// Handle the "Close" button click
void PhoneDirectory::onCloseClicked() {
    if (checkUnsavedChanges("closing")) {
        reject();
    }
}

// Handle the "X" window button or ESC key
void PhoneDirectory::closeEvent(QCloseEvent *event) {
    if (checkUnsavedChanges("closing")) {
        event->accept();
    } else {
        event->ignore();
    }
}

BbsEntry PhoneDirectory::getSelectedEntry() {
    BbsEntry empty;
    QTreeWidgetItem *item = m_tree->currentItem();
    if (!item) return empty;

    int index = item->data(0, Qt::UserRole).toInt();
    if (index >= 0 && index < m_entries.size()) {
        return m_entries[index];
    }
    return empty;
}

bool PhoneDirectory::runEditDialog(BbsEntry &entry) {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Edit BBS Entry"));
    dlg.resize(400, 250);
    QFormLayout layout(&dlg);

    QLineEdit *nameEdit = new QLineEdit(entry.name);
    QLineEdit *ipEdit = new QLineEdit(entry.ip);
    QLineEdit *portEdit = new QLineEdit(QString::number(entry.port));
    QLineEdit *userEdit = new QLineEdit(entry.login);

    // --- NEW: 3 Protocol Radio Buttons ---
    QRadioButton *rbTelnet = new QRadioButton(tr("Telnet"), &dlg);
    QRadioButton *rbSsh = new QRadioButton(tr("SSH (BBS)"), &dlg);
    QRadioButton *rbSshAuth = new QRadioButton(tr("SSH (Auth)"), &dlg);

    QHBoxLayout *protoLayout = new QHBoxLayout();
    protoLayout->addWidget(rbTelnet);
    protoLayout->addWidget(rbSsh);
    protoLayout->addWidget(rbSshAuth);
    protoLayout->addStretch();

    // Set initial checked state based on XML string
    if (entry.protocol.compare("SSH-AUTH", Qt::CaseInsensitive) == 0) rbSshAuth->setChecked(true);
    else if (entry.protocol.compare("SSH", Qt::CaseInsensitive) == 0) rbSsh->setChecked(true);
    else rbTelnet->setChecked(true);

    // Auto-switch Ports for convenience
    QObject::connect(rbSsh, &QRadioButton::toggled, [portEdit](bool checked){
        if(checked && portEdit->text() == "23") portEdit->setText("22");
    });
    QObject::connect(rbSshAuth, &QRadioButton::toggled, [portEdit](bool checked){
        if(checked && portEdit->text() == "23") portEdit->setText("22");
    });
    QObject::connect(rbTelnet, &QRadioButton::toggled, [portEdit](bool checked){
        if(checked && portEdit->text() == "22") portEdit->setText("23");
    });

    QHBoxLayout *passLayout = new QHBoxLayout();
    QLineEdit *passEdit = new QLineEdit(entry.password);
    passEdit->setEchoMode(QLineEdit::Password);
    QToolButton *showPassBtn = new QToolButton(&dlg);
    showPassBtn->setText(tr("Show"));
    showPassBtn->setCheckable(true);
    connect(showPassBtn, &QToolButton::toggled, [passEdit](bool c){ passEdit->setEchoMode(c?QLineEdit::Normal:QLineEdit::Password); });
    passLayout->addWidget(passEdit);
    passLayout->addWidget(showPassBtn);

    layout.addRow(tr("Name:"), nameEdit);
    layout.addRow(tr("Address:"), ipEdit);
    layout.addRow(tr("Protocol:"), protoLayout);
    layout.addRow(tr("Port:"), portEdit);
    layout.addRow(tr("User ID:"), userEdit);
    layout.addRow(tr("Password:"), passLayout);

    QDialogButtonBox btns(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(&btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(&btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout.addRow(&btns);

    if (dlg.exec() == QDialog::Accepted) {
        entry.name = nameEdit->text();
        entry.ip = ipEdit->text();
        entry.port = portEdit->text().toInt();
        entry.login = userEdit->text();
        entry.password = passEdit->text();

        // --- NEW: Save the exact protocol string ---
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
        m_isDirty = true; // Flag as dirty

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

    if (QMessageBox::question(this, tr("Confirm"), tr("Delete this entry?"), QMessageBox::StandardButton::Yes|QMessageBox::StandardButton::No) == QMessageBox::StandardButton::Yes) {
        m_entries.removeAt(index);
        refreshList();
        m_isDirty = true; // Flag as dirty
    }
}

