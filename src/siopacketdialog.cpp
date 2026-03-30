#include "siopacketdialog.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QColor>

// --- Model Implementation ---
SioPacketModel::SioPacketModel(QObject *parent) : QAbstractTableModel(parent) {}

int SioPacketModel::rowCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return m_packets.size();
}

int SioPacketModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent);
    return 8; // Time, Dir, Cmd, Aux1, Aux2, Len, Payload, Checksum
}

QVariant SioPacketModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_packets.size())
        return QVariant();

    const SioPacket &pkt = m_packets.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: return QString("%1 ms").arg(pkt.timestamp);
        case 1: return pkt.direction;
        case 2: return pkt.command;
        case 3: return pkt.aux1;
        case 4: return pkt.aux2;
        case 5: return pkt.dataLength;
        case 6: return pkt.payloadHex;
        case 7: return pkt.checksumStatus;
        }
    }

    // Color code rows based on TX vs RX
    if (role == Qt::BackgroundRole) {
        if (pkt.direction.startsWith("TX")) return QColor("#2A1B14"); // Dark red tint
        if (pkt.direction.startsWith("RX")) return QColor("#142A1B"); // Dark green tint
    }
    if (role == Qt::ForegroundRole) {
        return QColor(Qt::white);
    }

    return QVariant();
}

QVariant SioPacketModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) return QVariant();
    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0: return "Time";
        case 1: return "Dir";
        case 2: return "Cmd";
        case 3: return "Aux1";
        case 4: return "Aux2";
        case 5: return "Len";
        case 6: return "Payload";
        case 7: return "Chk";
        }
    }
    return QVariant();
}

void SioPacketModel::addPacket(const SioPacket &packet) {
    beginInsertRows(QModelIndex(), m_packets.size(), m_packets.size());
    m_packets.append(packet);
    endInsertRows();
}

void SioPacketModel::clear() {
    beginResetModel();
    m_packets.clear();
    endResetModel();
}


// --- Dialog Implementation ---
SioPacketDialog::SioPacketDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle(tr("SIO Packet Sniffer"));
    resize(850, 450);

    model = new SioPacketModel(this);
    tableView = new QTableView(this);
    tableView->setModel(model);
    tableView->verticalHeader()->setVisible(false);
    tableView->setAlternatingRowColors(true);
    tableView->setStyleSheet("QTableView { background-color: #1E1E1E; gridline-color: #333333; }");

    // ==========================================
    // UI FIX 1: Full Row Selection
    // ==========================================
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection); // Only let them select one row to inject

    // ==========================================
    // UI FIX 2: Stop Column Jumping
    // ==========================================
    // Set to Interactive so the user can manually drag borders if they want
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    // Lock in the initial pixel widths before data arrives
    tableView->setColumnWidth(0, 80);   // Time
    tableView->setColumnWidth(1, 100);  // Dir (e.g., "RX (CMD)*")
    tableView->setColumnWidth(2, 180);  // Cmd (e.g., "D1: STATUS ($53)")
    tableView->setColumnWidth(3, 60);   // Aux1
    tableView->setColumnWidth(4, 60);   // Aux2
    tableView->setColumnWidth(5, 50);   // Len
    tableView->setColumnWidth(7, 50);   // Chk (Checksum)

    // Force the "Payload" column (index 6) to stretch and eat up all remaining blank space
    tableView->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);

    // Buttons
    btnClear = new QPushButton(tr("Clear"), this);
    btnSave = new QPushButton(tr("Save CSV..."), this);
    btnInject = new QPushButton(tr("Inject Selected (Step)"), this);

    // Make the inject button pop a bit so it feels like a debugger action
    btnInject->setStyleSheet("background-color: #2D5A2D; color: white; font-weight: bold;");

    connect(btnClear, &QPushButton::clicked, model, &SioPacketModel::clear);
    connect(btnSave, &QPushButton::clicked, this, &SioPacketDialog::onSaveClicked);
    connect(btnInject, &QPushButton::clicked, this, &SioPacketDialog::onInjectClicked);

    // Layouts
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(btnInject);
    btnLayout->addStretch();
    btnLayout->addWidget(btnSave);
    btnLayout->addWidget(btnClear);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tableView);
    mainLayout->addLayout(btnLayout);
}


void SioPacketDialog::onInjectClicked() {
    QModelIndexList selection = tableView->selectionModel()->selectedRows();

    if (selection.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a packet row to inject."));
        return;
    }

    // Get the row index
    int row = selection.first().row();

    // Grab the original raw bytes
    const SioPacket &pkt = model->getPackets().at(row);

    // Safety check: Only inject packets that originally came from the Atari (RX)
    if (!pkt.direction.startsWith("RX")) {
        QMessageBox::warning(this, tr("Invalid Direction"),
                             tr("You can only inject RX packets (traffic originating from the Atari)."));
        return;
    }

    // Fire the bytes out to MainWindow!
    emit injectPacketRequested(pkt.rawData);
}


void SioPacketDialog::appendPacket(const QString &dir, const QByteArray &data, qint64 elapsedMs) {
    SioPacket pkt;
    pkt.timestamp = elapsedMs;
    pkt.direction = dir;
    pkt.dataLength = data.size();
    pkt.rawData = data;

    // --- SMART DECODER & HEURISTIC FIX ---
    // Hardware handshakes often emit commands as "RX (Data)".
    // We catch them by checking if it's an RX packet exactly 4 bytes long.
    bool isCmdFrame = dir.contains("CMD") || (dir.contains("RX") && data.size() == 4);

    if (isCmdFrame) {

        // Correct the label for the UI if it was mislabeled by the backend
        if (pkt.direction.contains("Data")) {
            pkt.direction = "RX (CMD)*";
        }

        quint8 devByte = static_cast<quint8>(data[0]);
        quint8 cmdByte = static_cast<quint8>(data[1]);

        // 1. Decode Device ID
        QString devStr;
        if (devByte >= 0x31 && devByte <= 0x3F) devStr = QString("D%1:").arg(devByte - 0x30);
        else if (devByte == 0x40) devStr = "P1: (Printer)";
        else if (devByte == 0x50) devStr = "R1: (Modem)";
        else if (devByte == 0x54) devStr = "W4: (Net)";
        else if (devByte == 0x55) devStr = "W3: (Net)";
        else if (devByte == 0x56) devStr = "W2: (Net)";
        else if (devByte == 0x57) devStr = "W1: (Net)";
        else if (devByte == 0x59) devStr = "Y1: (Clip)";
        else if (devByte == 0x45) devStr = "PC-LINK";
        else if (devByte == 0x46) devStr = "ASPEQT";
        else devStr = QString("DEV($%1)").arg(devByte, 2, 16, QChar('0')).toUpper();

        // 2. Decode Command Opcode
        QString cmdStr;
        if (cmdByte == 0x52 || cmdByte == 'R') cmdStr = "READ";
        else if (cmdByte == 0x57 || cmdByte == 'W') cmdStr = "WRITE";
        else if (cmdByte == 0x53 || cmdByte == 'S') cmdStr = "STATUS";
        else if (cmdByte == 0x50 || cmdByte == 'P') cmdStr = "PUT";
        else if (cmdByte == 0x21) cmdStr = "FORMAT";
        else if (cmdByte == 0x22) cmdStr = "FMT_DUAL";
        else if (cmdByte == 0x3F) cmdStr = "CAPACITY";
        else if (cmdByte == 0xFE) cmdStr = "MOUNT";
        else cmdStr = "UNK";

        // 3. Format output
        pkt.command = QString("%1 %2 ($%3)")
                          .arg(devStr)
                          .arg(cmdStr)
                          .arg(cmdByte, 2, 16, QChar('0')).toUpper();

        pkt.aux1 = QString("$%1").arg(static_cast<quint8>(data[2]), 2, 16, QChar('0')).toUpper();
        pkt.aux2 = QString("$%1").arg(static_cast<quint8>(data[3]), 2, 16, QChar('0')).toUpper();
        pkt.payloadHex = "--";

    } else {
        // It's a Data Payload or ACK/NAK
        pkt.command = "--";
        pkt.aux1 = "--";
        pkt.aux2 = "--";

        // Convert up to 16 bytes for preview
        QByteArray preview = data.mid(0, 16);
        for (char c : preview) {
            pkt.payloadHex += QString("%1 ").arg(static_cast<quint8>(c), 2, 16, QChar('0')).toUpper();
        }
        if (data.size() > 16) pkt.payloadHex += "...";
    }

    pkt.checksumStatus = "OK";

    model->addPacket(pkt);
    tableView->scrollToBottom();
}

void SioPacketDialog::onSaveClicked() {
    if (model->rowCount() == 0) {
        QMessageBox::information(this, tr("Empty"), tr("No packets to save."));
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save SIO Trace"), "", tr("CSV Files (*.csv);;All Files (*)"));
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not open file for writing."));
        return;
    }

    QTextStream out(&file);

    // Write CSV Header
    out << "Time(ms),Direction,Command,Aux1,Aux2,Length,Payload,Checksum\n";

    // Write Data Rows
    const QList<SioPacket> &packets = model->getPackets();
    for (const SioPacket &pkt : packets) {
        // Enclose strings in quotes to prevent issues with commas inside the data
        out << pkt.timestamp << ","
            << "\"" << pkt.direction << "\","
            << "\"" << pkt.command << "\","
            << "\"" << pkt.aux1 << "\","
            << "\"" << pkt.aux2 << "\","
            << pkt.dataLength << ","
            << "\"" << pkt.payloadHex << "\","
            << "\"" << pkt.checksumStatus << "\"\n";
    }

    file.close();
    QMessageBox::information(this, tr("Saved"), tr("SIO Trace saved successfully."));
}

void SioPacketDialog::closeEvent(QCloseEvent *event) {
    emit dialogClosed();
    QDialog::closeEvent(event);
}
