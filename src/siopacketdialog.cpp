#include "siopacketdialog.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QColor>
#include <QItemSelectionModel>

// --- Authentic Atari Checksum Algorithm ---
quint8 calculateAtariChecksum(const QByteArray &data, int length) {
    quint16 sum = 0;
    for (int i = 0; i < length; ++i) {
        sum += static_cast<quint8>(data[i]);
        if (sum > 255) sum -= 255;
    }
    return static_cast<quint8>(sum);
}

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

    // --- SMART COLOR CODING ---
    if (role == Qt::BackgroundRole) {
        if (pkt.checksumStatus.startsWith("FAIL")) return QColor("#5A1515"); // Bright Red for Checksum Failures
        if (pkt.payloadHex.startsWith("ACK") || pkt.payloadHex.startsWith("CMP")) return QColor("#153A5A"); // Blue for good control bytes
        if (pkt.payloadHex.startsWith("NAK") || pkt.payloadHex.startsWith("ERR")) return QColor("#5A3A15"); // Orange/Yellow for NAKs
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
    setWindowTitle(tr("SIO Packet Sniffer & Inspector"));
    resize(900, 600);

    model = new SioPacketModel(this);
    tableView = new QTableView(this);
    tableView->setModel(model);
    tableView->verticalHeader()->setVisible(false);
    tableView->setAlternatingRowColors(true);
    tableView->setStyleSheet("QTableView { background-color: #1E1E1E; gridline-color: #333333; }");

    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    tableView->setColumnWidth(0, 80);   // Time
    tableView->setColumnWidth(1, 100);  // Dir
    tableView->setColumnWidth(2, 180);  // Cmd
    tableView->setColumnWidth(3, 60);   // Aux1
    tableView->setColumnWidth(4, 60);   // Aux2
    tableView->setColumnWidth(5, 50);   // Len
    tableView->setColumnWidth(7, 130);  // Chk (Wide enough for "FAIL")
    tableView->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch); // Payload stretches

    // --- Collapsible Packet Inspector Pane ---
    inspectorContainer = new QWidget(this);
    QVBoxLayout *inspectorLayout = new QVBoxLayout(inspectorContainer);
    inspectorLayout->setContentsMargins(0, 0, 0, 0);

    // Header Bar (Native OS styling)
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *lblInspector = new QLabel(tr("Packet Inspector Details"), this);
    lblInspector->setStyleSheet("font-weight: bold; font-size: 11px; text-transform: uppercase;");

    btnToggleInspector = new QToolButton(this);
    btnToggleInspector->setText("▲"); // Points UP indicating it will maximize over the table
    btnToggleInspector->setStyleSheet("border: none; background: transparent; font-weight: bold; font-size: 14px;");
    btnToggleInspector->setCursor(Qt::PointingHandCursor);
    connect(btnToggleInspector, &QToolButton::clicked, this, &SioPacketDialog::toggleInspector);

    headerLayout->addWidget(lblInspector);
    headerLayout->addStretch();
    headerLayout->addWidget(btnToggleInspector);

    // Text Area
    txtDetails = new QTextEdit(this);
    txtDetails->setReadOnly(true);
    txtDetails->setStyleSheet("background-color: #121212; color: #d4d4d4; font-family: 'Courier New', monospace;");
    txtDetails->setHtml("<span style='color:#888;'>Click a packet row to view details...</span>");

    inspectorLayout->addLayout(headerLayout);
    inspectorLayout->addWidget(txtDetails);

    // Splitter Wrapper
    splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(tableView);
    splitter->addWidget(inspectorContainer);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    connect(tableView->selectionModel(), &QItemSelectionModel::currentChanged, this, &SioPacketDialog::onRowSelected);

    // Buttons
    btnClear = new QPushButton(tr("Clear"), this);
    btnSave = new QPushButton(tr("Save CSV..."), this);
    btnInject = new QPushButton(tr("Inject Selected (Step)"), this);
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
    mainLayout->addWidget(splitter);
    mainLayout->addLayout(btnLayout);
}


// --- Inspector Toggle Logic ---
void SioPacketDialog::toggleInspector() {
    if (tableView->isVisible()) {
        // Hide the table so the text area takes up 100% of the screen
        savedSplitterSizes = splitter->sizes();
        tableView->hide();
        btnToggleInspector->setText("▼"); // Point down to say "Shrink me back"
    } else {
        // Show the table again
        tableView->show();
        btnToggleInspector->setText("▲"); // Point up to say "Maximize me"

        if (!savedSplitterSizes.isEmpty()) {
            splitter->setSizes(savedSplitterSizes); // Snap back to how it was dragged
        }
    }
}



void SioPacketDialog::onRowSelected(const QModelIndex &current, const QModelIndex &previous) {
    Q_UNUSED(previous);
    if (!current.isValid()) return;
    int row = current.row();
    const SioPacket &pkt = model->getPackets().at(row);

    QString html = QString("<div style='margin-bottom: 10px;'>"
                           "<b style='color:#8be9fd;'>Direction:</b> %1 &nbsp;&nbsp;|&nbsp;&nbsp; "
                           "<b style='color:#8be9fd;'>Command:</b> %2<br>"
                           "<b style='color:#8be9fd;'>Aux1:</b> %3 &nbsp;&nbsp;|&nbsp;&nbsp; "
                           "<b style='color:#8be9fd;'>Aux2:</b> %4 &nbsp;&nbsp;|&nbsp;&nbsp; "
                           "<b style='color:#8be9fd;'>Checksum:</b> %5"
                           "</div>")
                       .arg(pkt.direction).arg(pkt.command)
                       .arg(pkt.aux1).arg(pkt.aux2).arg(pkt.checksumStatus);

    if (!pkt.rawData.isEmpty()) {
        html += "<b style='color:#ff79c6;'>Payload Hex & ATASCII:</b><br><pre style='margin-top: 5px; color: #d4d4d4;'>";

        for (int i = 0; i < pkt.rawData.size(); i += 16) {
            QByteArray chunk = pkt.rawData.mid(i, 16);
            QString offset = QString("%1").arg(i, 4, 16, QChar('0')).toUpper();

            QString hex;
            QString ascii;
            for (int j = 0; j < 16; ++j) {
                if (j < chunk.size()) {
                    quint8 byte = static_cast<quint8>(chunk[j]);
                    hex += QString("%1 ").arg(byte, 2, 16, QChar('0')).toUpper();

                    // Unified ATASCII & Inverse Video Mappings
                    if (byte >= 32 && byte <= 126) {
                        ascii += QString(QChar(byte)).toHtmlEscaped();
                    } else if (byte >= 160 && byte <= 254) {
                        ascii += QString(QChar(byte - 128)).toHtmlEscaped();
                    } else if (byte == 155) {
                        ascii += "<font color='#ffb86c'>&para;</font>";
                    } else {
                        ascii += "<font color='#6272a4'>.</font>";
                    }
                } else {
                    hex += "   ";
                }
            }
            html += QString("<font color='#6272a4'>%1:</font>  %2  <font color='#6272a4'>|</font>  %3\n").arg(offset).arg(hex).arg(ascii);
        }
        html += "</pre>";
    }
    txtDetails->setHtml(html);
}


void SioPacketDialog::onInjectClicked() {
    QModelIndexList selection = tableView->selectionModel()->selectedRows();

    if (selection.isEmpty()) {
        QMessageBox::warning(this, tr("No Selection"), tr("Please select a packet row to inject."));
        return;
    }

    int row = selection.first().row();
    const SioPacket &pkt = model->getPackets().at(row);

    if (!pkt.direction.startsWith("RX")) {
        QMessageBox::warning(this, tr("Invalid Direction"), tr("You can only inject RX packets (traffic originating from the Atari)."));
        return;
    }

    emit injectPacketRequested(pkt.rawData);
}

void SioPacketDialog::appendPacket(const QString &dir, const QByteArray &data, qint64 elapsedMs) {
    SioPacket pkt;
    pkt.timestamp = elapsedMs;
    pkt.direction = dir;
    pkt.dataLength = data.size();
    pkt.rawData = data;

    // Allow both 4-byte (header only) and 5-byte (header + checksum) command frames
    bool isCmdFrame = dir.contains("CMD") || (dir.contains("RX") && (data.size() == 4 || data.size() == 5));

    // 1. Intercept 1-Byte Protocol Control Characters
    if (data.size() == 1) {
        quint8 byte = static_cast<quint8>(data[0]);
        pkt.command = "--";
        pkt.aux1 = "--";
        pkt.aux2 = "--";
        pkt.checksumStatus = "--";

        if (byte == 'A') pkt.payloadHex = "ACK (0x41)";
        else if (byte == 'N') pkt.payloadHex = "NAK (0x4E)";
        else if (byte == 'C') pkt.payloadHex = "CMP (0x43)";
        else if (byte == 'E') pkt.payloadHex = "ERR (0x45)";
        else pkt.payloadHex = QString("UNK: %1").arg(byte, 2, 16, QChar('0')).toUpper();

        model->addPacket(pkt);
        tableView->scrollToBottom();
        return;
    }

    if (isCmdFrame && data.size() >= 4) {
        if (pkt.direction.contains("Data")) pkt.direction = "RX (CMD)*";

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
        else if (devByte == 0x4F) devStr = "O1: (FujiNet)";
        else if (devByte == 0x70) devStr = "T1: (APE/Smart)";
        else devStr = QString("DEV($%1)").arg(devByte, 2, 16, QChar('0')).toUpper();

        // 2. Decode Command Opcode
        QString cmdStr;
        if (cmdByte == 0x52 || cmdByte == 'R') cmdStr = "READ";
        else if (cmdByte == 0x57 || cmdByte == 'W') cmdStr = "WRITE";
        else if (cmdByte == 0x53 || cmdByte == 'S') cmdStr = "STATUS";
        else if (cmdByte == 0x50 || cmdByte == 'P') cmdStr = "PUT/POST";
        else if (cmdByte == 0x4F || cmdByte == 'O') cmdStr = "OPEN";
        else if (cmdByte == 0x43 || cmdByte == 'C') cmdStr = "CLOSE";
        else if (cmdByte == 0x21) cmdStr = "FORMAT";
        else if (cmdByte == 0x22) cmdStr = "FMT_DUAL";
        else if (cmdByte == 0x3F) {
            // $3F is CAPACITY for Disk Drives, but SMART POLL for the T1: Device
            if (devByte == 0x70) cmdStr = "SMART_POLL";
            else cmdStr = "CAPACITY";
        }
        else if (cmdByte == 0x93 && devByte == 0x70) cmdStr = "APE_TIME"; // NEW: Smart Device Date/Time
        else if (cmdByte == 0xFE) cmdStr = "MOUNT";
        else if (cmdByte == 0x40 && devByte == 0x4F) cmdStr = "PING";
        else cmdStr = "UNK";


        pkt.command = QString("%1 %2 ($%3)").arg(devStr).arg(cmdStr).arg(cmdByte, 2, 16, QChar('0')).toUpper();
        pkt.aux1 = QString("$%1").arg(static_cast<quint8>(data[2]), 2, 16, QChar('0')).toUpper();
        pkt.aux2 = QString("$%1").arg(static_cast<quint8>(data[3]), 2, 16, QChar('0')).toUpper();
        pkt.payloadHex = "--";

        if (data.size() == 5) {
            quint8 calcSum = calculateAtariChecksum(data, 4);
            quint8 actualSum = static_cast<quint8>(data[4]);
            pkt.checksumStatus = (calcSum == actualSum) ? "OK" : QString("FAIL (Expected $%1)").arg(calcSum, 2, 16, QChar('0')).toUpper();
        } else {
            pkt.checksumStatus = "N/A (Header)";
        }

    } else {
        pkt.command = "--";
        pkt.aux1 = "--";
        pkt.aux2 = "--";

        int payloadEnd = data.size() > 1 ? data.size() - 1 : data.size();
        int previewLen = std::min(payloadEnd, 16);

        QByteArray preview = data.mid(0, previewLen);
        for (char c : preview) {
            pkt.payloadHex += QString("%1 ").arg(static_cast<quint8>(c), 2, 16, QChar('0')).toUpper();
        }
        if (payloadEnd > 16) pkt.payloadHex += "...";

        if (data.size() > 1) {
            quint8 calcSum = calculateAtariChecksum(data, data.size() - 1);
            quint8 actualSum = static_cast<quint8>(data[data.size() - 1]);
            pkt.checksumStatus = (calcSum == actualSum) ? "OK" : QString("FAIL (Expected $%1)").arg(calcSum, 2, 16, QChar('0')).toUpper();
        } else {
            pkt.checksumStatus = "--";
        }
    }

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
    out << "Time(ms),Direction,Command,Aux1,Aux2,Length,Payload,Checksum\n";

    const QList<SioPacket> &packets = model->getPackets();
    for (const SioPacket &pkt : packets) {
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
