/*
 * sectoreditdialog.cpp
 */

#include "sectoreditdialog.h"
#include "ui_sectoreditdialog.h"
#include <QMessageBox>
#include <QInputDialog>

SectorEditDialog::SectorEditDialog(SimpleDiskImage *img, int sector, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SectorEditDialog),
    m_img(img),
    m_sector(sector)
{
    ui->setupUi(this);
    setWindowTitle(QString("Edit Sector %1").arg(sector));

    // Make the window wide enough to fit all 16 columns perfectly
    resize(720, 420);

    // Setup the Hex Grid Columns
    ui->tableWidget->setColumnCount(16);
    QStringList headers;
    for(int i=0; i<16; i++) headers << QString("%1").arg(i, 1, 16).toUpper();
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    // Squeeze the columns tight so it looks like a real hex editor
    for(int i=0; i<16; i++) ui->tableWidget->setColumnWidth(i, 35);

    loadData();

    // Wire the Live Status Bar
    connect(ui->tableWidget, &QTableWidget::currentCellChanged, this, &SectorEditDialog::onCellSelected);

    // Wire the Inject Text Button
    connect(ui->btnInjectText, &QPushButton::clicked, this, &SectorEditDialog::on_btnInjectText_clicked);
}

SectorEditDialog::~SectorEditDialog() {
    delete ui;
}

void SectorEditDialog::loadData() {
    QByteArray buffer;
    if (m_img->readSector(m_sector, buffer)) {
        m_sectorSize = buffer.size();
        int rows = m_sectorSize / 16;
        ui->tableWidget->setRowCount(rows);

        // Setup Row Headers (0000, 0010, 0020...)
        QStringList vHeaders;
        for(int r=0; r<rows; r++) vHeaders << QString("%1").arg(r*16, 4, 16, QChar('0')).toUpper();
        ui->tableWidget->setVerticalHeaderLabels(vHeaders);

        // Populate the grid
        for (int i = 0; i < buffer.size(); ++i) {
            int row = i / 16;
            int col = i % 16;
            quint8 byte = static_cast<quint8>(buffer[i]);
            QTableWidgetItem *item = new QTableWidgetItem(QString("%1").arg(byte, 2, 16, QChar('0')).toUpper());
            item->setTextAlignment(Qt::AlignCenter);

            // Use a monospace font for the cells
            QFont mono("Courier New");
            item->setFont(mono);

            ui->tableWidget->setItem(row, col, item);
        }
    }
}

void SectorEditDialog::on_buttonBox_accepted() {
    QByteArray newData;
    newData.reserve(m_sectorSize);

    // Read the grid back into a byte array
    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        for (int c = 0; c < 16; ++c) {
            QTableWidgetItem *item = ui->tableWidget->item(r, c);
            if (!item) continue;

            bool ok;
            int val = item->text().toInt(&ok, 16); // Convert text to hex

            // Validation: Ensure they didn't type "G9" or "1FF"
            if (!ok || val < 0 || val > 255) {
                QMessageBox::warning(this, tr("Invalid Data"),
                                     tr("Invalid hex value '%1' at row %2, col %3. Please use 00-FF.").arg(item->text()).arg(r).arg(c));
                return; // Abort the save process
            }
            newData.append(static_cast<char>(val));
        }
    }

    // Write it to the disk image!
    if (m_img->writeSector(m_sector, newData)) {
        emit sectorSaved(); // Tell the main inspector to refresh
        accept();           // Close the window
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to write sector to disk image! Is it write-protected?"));
    }
}

void SectorEditDialog::on_buttonBox_rejected() {
    reject();
}

// =========================================================================
// ATASCII QUALITY OF LIFE FEATURES
// =========================================================================

void SectorEditDialog::onCellSelected(int row, int col) {
    QTableWidgetItem *item = ui->tableWidget->item(row, col);
    if (!item) return;

    bool ok;
    quint8 byte = item->text().toInt(&ok, 16);
    if (!ok) return;

    // Standard ASCII mapping
    char stdChar = '.';
    if (byte >= 32 && byte <= 126) stdChar = static_cast<char>(byte);

    // Atari Inverse Video mapping
    char invChar = '.';
    if (byte >= 160 && byte <= 254) invChar = static_cast<char>(byte - 128);

    // Update the label with a formatted string
    ui->lblStatus->setText(QString("<b>Hex:</b> $%1 &nbsp;|&nbsp; <b>Norm:</b> '%2' &nbsp;|&nbsp; <b>Inv:</b> '%3'")
                               .arg(item->text())
                               .arg(QString(stdChar).toHtmlEscaped())
                               .arg(QString(invChar).toHtmlEscaped()));
}

void SectorEditDialog::on_btnInjectText_clicked() {
    bool ok;
    QString text = QInputDialog::getText(this, tr("Inject ATASCII Text"),
                                         tr("Enter text to write into the sector:"), QLineEdit::Normal,
                                         "", &ok);
    if (!ok || text.isEmpty()) return;

    // Get current cursor position to start injecting
    int row = ui->tableWidget->currentRow();
    int col = ui->tableWidget->currentColumn();

    // Default to the very beginning if no cell is selected
    if (row < 0 || col < 0) { row = 0; col = 0; }

    int absoluteIndex = (row * 16) + col;

    // Loop through the typed string and overwrite cells
    for (int i = 0; i < text.length(); ++i) {
        if (absoluteIndex >= m_sectorSize) break; // Stop if we hit the end of the sector

        // Standard ASCII maps directly to standard ATASCII hex values
        quint8 byte = static_cast<quint8>(text.at(i).toLatin1());

        int targetRow = absoluteIndex / 16;
        int targetCol = absoluteIndex % 16;

        QTableWidgetItem *item = ui->tableWidget->item(targetRow, targetCol);
        if (item) {
            item->setText(QString("%1").arg(byte, 2, 16, QChar('0')).toUpper());

            // Highlight it lightly so the user sees what was changed
            item->setBackground(QBrush(QColor("#E0FFE0")));
        }

        absoluteIndex++;
    }

    // Manually trigger the status bar update for the last edited cell
    onCellSelected(ui->tableWidget->currentRow(), ui->tableWidget->currentColumn());
}
