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

PhoneDirectory::PhoneDirectory(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("BBS Phonebook"));
    // FIX: Set wider default size per your screenshot request
    resize(650, 450);

    // Layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Search Bar
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search BBS Name..."));
    connect(m_searchEdit, &QLineEdit::textChanged, this, &PhoneDirectory::onSearch);
    mainLayout->addWidget(m_searchEdit);

    // List
    m_tree = new QTreeWidget(this);
    // Added "User ID" column
    m_tree->setHeaderLabels({tr("BBS Name"), tr("Address"), tr("Port"), tr("User ID")});

    // Column Resizing Logic
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch); // Name takes avail space
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
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
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    // Layout buttons: [Edit] [Save] <space> [Dial] [Close]
    btnLayout->insertWidget(0, addBtn); // Put at start
    btnLayout->insertWidget(1, delBtn);
    btnLayout->addWidget(m_editBtn);
    btnLayout->addWidget(m_saveBtn);

    btnLayout->addStretch();
    btnLayout->addWidget(m_dialBtn);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);
    m_dialBtn->setDefault(true); // Pressing Enter triggers Dial
    m_dialBtn->setFocus();       // Initial focus is on Dial

    // Double click to dial
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &PhoneDirectory::onDialClicked);
}

void PhoneDirectory::loadFromFile(const QString &path) {

    if (path.isEmpty()) {
        qWarning() << "PhoneDirectory: No file path provided.";
        return;
    }

    m_filePath = path;
    parseXml();
    refreshList();
}

void PhoneDirectory::parseXml() {
    m_entries.clear();
    QFile file(m_filePath);

    // [FIX] Report file open errors
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("File Error"),
                             tr("Could not open phonebook file:\n%1").arg(m_filePath));
        return;
    }

    QDomDocument doc;
    QString errorMsg;
    int errorLine;
    int errorColumn;

    // [FIX] Capture XML Parsing Errors
    if (!doc.setContent(&file, &errorMsg, &errorLine, &errorColumn)) {
        file.close();
        QMessageBox::critical(this, tr("Invalid XML"),
                              tr("The phonebook file contains invalid XML and cannot be loaded.\n\n"
                                 "File: %1\n"
                                 "Error: %2\n"
                                 "Line: %3, Column: %4")
                                  .arg(m_filePath)
                                  .arg(errorMsg)
                                  .arg(errorLine)
                                  .arg(errorColumn));
        return;
    }

    QDomNodeList list = doc.elementsByTagName("BBS");
    for (int i = 0; i < list.count(); i++) {
        QDomElement e = list.at(i).toElement();
        BbsEntry bbs;
        bbs.name = e.attribute("name");
        bbs.ip = e.attribute("ip");
        bbs.port = e.attribute("port").toInt();
        bbs.login = e.attribute("login");
        bbs.password = e.attribute("password");
        bbs.protocol = e.attribute("protocol");
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
        tag.setAttribute("font", "vga8x16.bmp"); // Default
        tag.setAttribute("keyMap", "ANSI");      // Default
        pb.appendChild(tag);
    }

    QFile file(m_filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << doc.toString();
        file.close();
        QMessageBox::information(this, tr("Saved"), tr("Phonebook saved successfully!"));
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Could not write to file."));
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
            item->setText(3, e.login);
            // Store the original index in the item so we can find it for editing
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

    // Create a dynamic dialog for editing
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Edit BBS Entry"));
    dlg.resize(400, 200); // Reasonable size for edit dialog
    QFormLayout layout(&dlg);

    QLineEdit *nameEdit = new QLineEdit(entry.name);
    QLineEdit *ipEdit = new QLineEdit(entry.ip);
    QLineEdit *portEdit = new QLineEdit(QString::number(entry.port));
    QLineEdit *userEdit = new QLineEdit(entry.login);
    QLineEdit *passEdit = new QLineEdit(entry.password);
    passEdit->setEchoMode(QLineEdit::Password);

    layout.addRow(tr("Name:"), nameEdit);
    layout.addRow(tr("Address:"), ipEdit);
    layout.addRow(tr("Port:"), portEdit);
    layout.addRow(tr("User ID:"), userEdit);
    layout.addRow(tr("Password:"), passEdit);

    QDialogButtonBox btns(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(&btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(&btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout.addRow(&btns);

    if (dlg.exec() == QDialog::Accepted) {
        // Update the internal list
        entry.name = nameEdit->text();
        entry.ip = ipEdit->text();
        entry.port = portEdit->text().toInt();
        entry.login = userEdit->text();
        entry.password = passEdit->text();

        // Update the UI immediately
        item->setText(0, entry.name);
        item->setText(1, entry.ip);
        item->setText(2, QString::number(entry.port));
        item->setText(3, entry.login);
    }
}

void PhoneDirectory::onSaveClicked() {
    if (m_filePath.isEmpty()) return;
    saveToFile();
}

void PhoneDirectory::onSearch(const QString &text) {
    refreshList(text);
}

void PhoneDirectory::onDialClicked() {
    if (m_tree->currentItem()) {
        accept();
    }
}

BbsEntry PhoneDirectory::getSelectedEntry() {
    BbsEntry empty;
    QTreeWidgetItem *item = m_tree->currentItem();
    if (!item) return empty;

    // Use the stored index for robust lookup
    int index = item->data(0, Qt::UserRole).toInt();
    if (index >= 0 && index < m_entries.size()) {
        return m_entries[index];
    }
    return empty;
}

void PhoneDirectory::onAddClicked() {
    BbsEntry newEntry;
    newEntry.name = "New BBS";
    newEntry.ip = "bbs.example.com";
    newEntry.port = 23;
    m_entries.append(newEntry);
    refreshList();

    // Optional: Auto-trigger edit
    m_tree->setCurrentItem(m_tree->topLevelItem(m_entries.size()-1));
    onEditClicked();
}

void PhoneDirectory::onDeleteClicked() {
    QTreeWidgetItem *item = m_tree->currentItem();
    if (!item) return;
    int index = item->data(0, Qt::UserRole).toInt();

    if (QMessageBox::question(this, tr("Confirm"), tr("Delete this entry?"), QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
        m_entries.removeAt(index);
        refreshList();
    }
}
