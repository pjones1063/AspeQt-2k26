#include "siopacketdialog.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QColor>
#include <QItemSelectionModel>
#include <QRegularExpression>
#include <QFontDatabase>

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

    // --- SMART COLOR CODING (Light UI / Wireshark Style) ---
    if (role == Qt::BackgroundRole) {
        if (pkt.checksumStatus.startsWith("FAIL")) return QColor("#FFCCCC"); // Light Red
        if (pkt.payloadHex.startsWith("ACK") || pkt.payloadHex.startsWith("CMP")) return QColor("#CCE5FF"); // Light Blue
        if (pkt.payloadHex.startsWith("NAK") || pkt.payloadHex.startsWith("ERR")) return QColor("#FFF2CC"); // Light Yellow
        if (pkt.direction.startsWith("TX")) return QColor("#F8F9FA"); // Light gray for TX
        if (pkt.direction.startsWith("RX")) return QColor("#E8F5E9"); // Pale green for RX
    }

    if (role == Qt::ForegroundRole) {
        if (pkt.checksumStatus.startsWith("FAIL")) return QColor("#990000"); // Dark red text
        return QVariant();
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
    // --- FEATURE 4: THE RING BUFFER ---
    if (m_packets.size() >= MAX_PACKETS) {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_packets.removeFirst();
        endRemoveRows();
    }

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
    resize(950, 650);

    model = new SioPacketModel(this);

    // --- SNIFFER DE-FRAGMENTATION TIMER ---
    m_snifferTimer = new QTimer(this);
    m_snifferTimer->setSingleShot(true);
    connect(m_snifferTimer, &QTimer::timeout, this, &SioPacketDialog::processPendingPacket);

    m_pendingTimestamp = 0;

    // --- FEATURE 1: WIRESHARK FILTER PROXY ---
    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(-1); // Search all columns by default

    tableView = new QTableView(this);
    tableView->setModel(proxyModel); // Bind table to the proxy
    tableView->verticalHeader()->setVisible(false);
    tableView->setAlternatingRowColors(true);

    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    tableView->setColumnWidth(0, 80);   // Time
    tableView->setColumnWidth(1, 100);  // Dir
    tableView->setColumnWidth(2, 180);  // Cmd
    tableView->setColumnWidth(3, 60);   // Aux1
    tableView->setColumnWidth(4, 60);   // Aux2
    tableView->setColumnWidth(5, 50);   // Len
    tableView->setColumnWidth(7, 130);  // Chk
    tableView->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch); // Payload stretches

    // --- Filter UI ---
    cmbFilterColumn = new QComboBox(this);
    cmbFilterColumn->addItems({"All Columns", "Direction", "Command (Device)", "Payload"});
    txtFilter = new QLineEdit(this);
    txtFilter->setPlaceholderText(tr("e.g. R1: or NAK..."));
    txtFilter->setClearButtonEnabled(true);

    connect(cmbFilterColumn, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SioPacketDialog::onFilterChanged);
    connect(txtFilter, &QLineEdit::textChanged, this, &SioPacketDialog::onFilterChanged);

    QHBoxLayout *filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel(tr("🔍 Filter:"), this));
    filterLayout->addWidget(cmbFilterColumn);
    filterLayout->addWidget(txtFilter);

    // --- Collapsible Packet Inspector Pane ---
    inspectorContainer = new QWidget(this);
    QVBoxLayout *inspectorLayout = new QVBoxLayout(inspectorContainer);
    inspectorLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *lblInspector = new QLabel(tr("Packet Inspector Details"), this);
    lblInspector->setStyleSheet("font-weight: bold; font-size: 11px; text-transform: uppercase;");

    btnToggleInspector = new QPushButton(tr("Collapse Details ▼"), this);
    btnToggleInspector->setCursor(Qt::PointingHandCursor);
    connect(btnToggleInspector, &QPushButton::clicked, this, &SioPacketDialog::toggleInspector);

    headerLayout->addWidget(lblInspector);
    headerLayout->addStretch();
    headerLayout->addWidget(btnToggleInspector);

    txtDetails = new QTextEdit(this);
    txtDetails->setReadOnly(true);

    // --- THE FIX: Clean OS-Native Monospace Font ---
// --- THE FIX: Clean OS-Native Monospace Font ---
#ifdef Q_OS_MAC
    QFont monoFont("Menlo", 10);       // Apple's crisp Retina-ready monospace
#elif defined(Q_OS_WIN)
    QFont monoFont("Consolas", 10);    // Microsoft's modern programming font
#else
    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setPointSize(10);
#endif

    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setFixedPitch(true);
    txtDetails->document()->setDefaultFont(monoFont);
    txtDetails->setFont(monoFont);

    txtDetails->setLineWrapMode(QTextEdit::NoWrap);
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

    // --- Manual Injector & Record Deck ---
    btnRecord = new QPushButton(tr("🔴 Recording"), this);
    btnRecord->setCheckable(true);
    btnRecord->setChecked(true);
    btnRecord->setStyleSheet("QPushButton:checked { background-color: #CC0000; color: white; font-weight: bold; border-radius: 4px; padding: 4px; }");
    connect(btnRecord, &QPushButton::toggled, this, &SioPacketDialog::onRecordToggled);

    btnClear = new QPushButton(tr("Clear"), this);
    btnSave = new QPushButton(tr("Save CSV..."), this);

    btnInject = new QPushButton(tr("Inject Selected"), this);
    btnInject->setStyleSheet("background-color: #2D5A2D; color: white; font-weight: bold; border-radius: 4px; padding: 4px;");

    chkSafeMode = new QCheckBox(tr("Safe Mode (TX Only)"), this);
    chkSafeMode->setChecked(true);
    chkSafeMode->setToolTip(tr("Prevents injecting RX packets to avoid hardware collisions with a physical Atari."));

    connect(btnClear, &QPushButton::clicked, model, &SioPacketModel::clear);
    connect(btnSave, &QPushButton::clicked, this, &SioPacketDialog::onSaveClicked);
    connect(btnInject, &QPushButton::clicked, this, &SioPacketDialog::onInjectClicked);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(btnRecord);
    btnLayout->addWidget(btnClear);
    btnLayout->addStretch();
    btnLayout->addWidget(chkSafeMode);
    btnLayout->addWidget(btnInject);
    btnLayout->addWidget(btnSave);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(filterLayout);
    mainLayout->addWidget(splitter);
    mainLayout->addLayout(btnLayout);
}

// --- Filter Logic ---
void SioPacketDialog::onFilterChanged() {
    int col = cmbFilterColumn->currentIndex();
    if (col == 0) proxyModel->setFilterKeyColumn(-1); // All
    else if (col == 1) proxyModel->setFilterKeyColumn(1); // Dir
    else if (col == 2) proxyModel->setFilterKeyColumn(2); // Cmd
    else if (col == 3) proxyModel->setFilterKeyColumn(6); // Payload

    proxyModel->setFilterRegularExpression(QRegularExpression(txtFilter->text(), QRegularExpression::CaseInsensitiveOption));
}

void SioPacketDialog::onRecordToggled(bool checked) {
    m_isRecording = checked;
    btnRecord->setText(checked ? tr("🔴 Recording") : tr("⏸ Paused"));
}

void SioPacketDialog::toggleInspector() {
    if (tableView->isVisible()) {
        savedSplitterSizes = splitter->sizes();
        tableView->hide();
        btnToggleInspector->setText(tr("Expand Details ▲"));
    } else {
        tableView->show();
        btnToggleInspector->setText(tr("Collapse Details ▼"));
        if (!savedSplitterSizes.isEmpty()) splitter->setSizes(savedSplitterSizes);
    }
}

void SioPacketDialog::onRowSelected(const QModelIndex &current, const QModelIndex &previous) {
    Q_UNUSED(previous);
    if (!current.isValid()) return;

    // Proxy Translation!
    QModelIndex sourceIndex = proxyModel->mapToSource(current);
    if (!sourceIndex.isValid()) return;
    const SioPacket &pkt = model->getPackets().at(sourceIndex.row());

    QString html = QString("<div style='margin-bottom: 10px;'>"
                           "<b style='color:#003366;'>Direction:</b> %1 &nbsp;&nbsp;|&nbsp;&nbsp; "
                           "<b style='color:#003366;'>Command:</b> %2<br>"
                           "<b style='color:#003366;'>Aux1:</b> %3 &nbsp;&nbsp;|&nbsp;&nbsp; "
                           "<b style='color:#003366;'>Aux2:</b> %4 &nbsp;&nbsp;|&nbsp;&nbsp; "
                           "<b style='color:#003366;'>Checksum:</b> %5"
                           "</div>")
                       .arg(pkt.direction).arg(pkt.command)
                       .arg(pkt.aux1).arg(pkt.aux2).arg(pkt.checksumStatus);

    // --- FEATURE 2: DEEP PAYLOAD PARSING ---
    if (!pkt.command.contains("--")) {
        html += "<div style='background-color:#E8F0FE; padding: 5px; border-left: 3px solid #1A73E8; margin-bottom: 10px;'>";
        html += "<b style='color:#1A73E8;'>Protocol Decoder:</b><br>";

        if (pkt.command.contains("D") && pkt.command.contains("READ")) {
            html += "Standard Disk Sector Read. ";
            if (pkt.dataLength == 133) html += "(128 Byte Payload + SIO Header/Chk)";
            else if (pkt.dataLength == 261) html += "(256 Byte Payload + SIO Header/Chk)";
        }
        else if (pkt.command.contains("D") && pkt.command.contains("WRITE")) {
            html += "Standard Disk Sector Write. ";
            if (pkt.dataLength == 133) html += "(128 Byte Payload + SIO Header/Chk)";
            else if (pkt.dataLength == 261) html += "(256 Byte Payload + SIO Header/Chk)";
        }
        else if (pkt.command.contains("P") && pkt.dataLength > 1) {
            html += "<b>Epson/Atari Printer Text Stream:</b><br><span style='color:#000000;'>";
            // Strip SIO frame and decode plain text
            int start = (pkt.direction.contains("CMD")) ? 4 : 0;
            int end = pkt.dataLength - 1; // Drop checksum
            for (int i = start; i < end; i++) {
                quint8 b = static_cast<quint8>(pkt.rawData[i]);
                if (b >= 32 && b <= 126) html += QString(QChar(b)).toHtmlEscaped();
                else if (b == 0x9B || b == 0x0A || b == 0x0D) html += "<br>"; // Handle EOL
                else if (b == 0x1B) html += "<b>[ESC]</b>"; // Highlight Escapes
            }
            html += "</span>";
        }
        else {
            html += "No deep decoder available for this device type.";
        }
        html += "</div>";
    }

    if (!pkt.rawData.isEmpty()) {
        // --- THE FIX: Standard CSS Monospace ensures perfect alignment ---
        html += "<b style='color:#660000;'>Payload Hex & ATASCII:</b><br><pre style='font-family: monospace; margin-top: 5px;'>";

        for (int i = 0; i < pkt.rawData.size(); i += 16) {
            QByteArray chunk = pkt.rawData.mid(i, 16);
            QString offset = QString("%1").arg(i, 4, 16, QChar('0')).toUpper();

            QString hex;
            QString ascii;
            for (int j = 0; j < 16; ++j) {
                if (j < chunk.size()) {
                    quint8 byte = static_cast<quint8>(chunk[j]);
                    hex += QString("%1 ").arg(byte, 2, 16, QChar('0')).toUpper();

                    if (byte >= 32 && byte <= 126) {
                        ascii += QString(QChar(byte)).toHtmlEscaped();
                    } else if (byte >= 160 && byte <= 254) {
                        ascii += QString(QChar(byte - 128)).toHtmlEscaped();
                    } else if (byte == 155) {
                        ascii += "<font color='#CC6600'>&para;</font>";
                    } else {
                        ascii += "<font color='#888888'>.</font>";
                    }
                } else {
                    hex += "   ";
                }
            }
            html += QString("<font color='#888888'>%1:</font>  %2  <font color='#888888'>|</font>  %3\n").arg(offset).arg(hex).arg(ascii);
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

    // Proxy Translation
    QModelIndex sourceIndex = proxyModel->mapToSource(selection.first());
    const SioPacket &pkt = model->getPackets().at(sourceIndex.row());

    // --- ENFORCE SAFE MODE ---
    if (chkSafeMode->isChecked() && pkt.direction.contains("RX")) {
        QMessageBox::warning(this, tr("Safe Mode Block"),
                             tr("You are trying to inject an RX (Atari Command) packet while Safe Mode is enabled.\n\n"
                                "This is blocked to prevent TX/RX collisions on the bus. Uncheck Safe Mode if you are purely testing virtual components without a physical Atari connected."));
        return;
    }

    emit injectPacketRequested(pkt.rawData);
}

void SioPacketDialog::appendPacket(const QString &dir, const QByteArray &data, qint64 elapsedMs) {
    if (!m_isRecording) return;

    // If the direction switches (e.g., RX to TX), flush the buffer immediately
    if (!m_pendingBuffer.isEmpty() && m_pendingDirection != dir) {
        processPendingPacket();
    }

    // If this is the start of a new packet, record the initial timestamp and direction
    if (m_pendingBuffer.isEmpty()) {
        m_pendingDirection = dir;
        m_pendingTimestamp = elapsedMs;
    }

    // Append incoming fragmented data to the buffer
    m_pendingBuffer.append(data);

    // Restart the idle timer. 15ms is plenty of time for an FTDI chip to flush the checksum byte
    m_snifferTimer->start(15);
}

void SioPacketDialog::processPendingPacket() {
    if (m_pendingBuffer.isEmpty()) return;

    // Grab the complete packet
    QByteArray data = m_pendingBuffer;
    QString dir = m_pendingDirection;
    qint64 elapsedMs = m_pendingTimestamp;

    // Reset the pending buffer for the next incoming stream
    m_pendingBuffer.clear();
    m_pendingDirection.clear();
    m_pendingTimestamp = 0;

    // --- Core Parsing Logic ---
    SioPacket pkt;
    pkt.timestamp = elapsedMs;
    pkt.direction = dir;
    pkt.dataLength = data.size();
    pkt.rawData = data;

    bool isCmdFrame = dir.contains("CMD") || (dir.contains("RX") && (data.size() == 4 || data.size() == 5));

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

        if (txtFilter->text().isEmpty()) tableView->scrollToBottom();
        return;
    }

    if (isCmdFrame && data.size() >= 4) {
        if (pkt.direction.contains("Data")) pkt.direction = "RX (CMD)*";

        quint8 devByte = static_cast<quint8>(data[0]);
        quint8 cmdByte = static_cast<quint8>(data[1]);

        QString devStr;
        if (devByte >= 0x31 && devByte <= 0x3F) devStr = QString("D%1:").arg(devByte - 0x30);
        else if (devByte >= 0x40 && devByte <= 0x43) devStr = QString("P%1: (Printer)").arg(devByte - 0x3F);
        else if (devByte >= 0x50 && devByte <= 0x53) devStr = QString("R%1: (Modem)").arg(devByte - 0x4F);
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
            if (devByte == 0x70) cmdStr = "SMART_POLL";
            else cmdStr = "CAPACITY";
        }
        else if (cmdByte == 0x93 && devByte == 0x70) cmdStr = "APE_TIME";
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

    // Only auto-scroll to bottom if the user isn't actively filtering
    if (txtFilter->text().isEmpty()) {
        tableView->scrollToBottom();
    }
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
    // --- Reordered Headers: Checksum moved before Payload ---
    out << "Time(ms),Direction,Command,Aux1,Aux2,Length,Checksum,Payload,ATASCII\n";

    const QList<SioPacket> &packets = model->getPackets();
    for (const SioPacket &pkt : packets) {

        // --- Safely translate raw payload to ATASCII for CSV ---
        QString safeAtascii = "--";
        if (!pkt.rawData.isEmpty()) {
            safeAtascii = "";
            for (int i = 0; i < pkt.rawData.size(); ++i) {
                quint8 byte = static_cast<quint8>(pkt.rawData[i]);

                if (byte >= 32 && byte <= 126) {
                    if (byte == 34) safeAtascii += "\"\""; // Escape double quotes
                    else safeAtascii += QChar(byte);
                }
                else if (byte >= 160 && byte <= 254) {
                    quint8 invByte = byte - 128;
                    if (invByte == 34) safeAtascii += "\"\""; // Escape double quotes
                    else safeAtascii += QChar(invByte);
                }
                else {
                    safeAtascii += "."; // Mask unprintable characters
                }
            }
        }

        // --- Reordered Output ---
        out << pkt.timestamp << ","
            << "\"" << pkt.direction << "\","
            << "\"" << pkt.command << "\","
            << "\"" << pkt.aux1 << "\","
            << "\"" << pkt.aux2 << "\","
            << pkt.dataLength << ","
            << "\"" << pkt.checksumStatus << "\"," // Moved up
            << "\"" << pkt.payloadHex << "\","     // Second to last
            << "\"" << safeAtascii << "\"\n";      // Last
    }

    file.close();
    QMessageBox::information(this, tr("Saved"), tr("SIO Trace saved successfully."));
}


void SioPacketDialog::closeEvent(QCloseEvent *event) {
    if (!m_pendingBuffer.isEmpty()) {
        processPendingPacket(); // Flush any remaining bits
    }
    emit dialogClosed();
    QDialog::closeEvent(event);
}
