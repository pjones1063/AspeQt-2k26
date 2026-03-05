/*
 * sectorinspectordialog.cpp
 */

#include "sectorinspectordialog.h"
#include "ui_sectorinspectordialog.h"
#include "sectoreditdialog.h"
#include <QList>

SectorInspectorDialog::SectorInspectorDialog(SimpleDiskImage *img, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SectorInspectorDialog),
    m_img(img)
{
    ui->setupUi(this);

    // 1. Force a strict Monospace font at a readable size
    QFont monoFont("Courier New");
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setFixedPitch(true);
    monoFont.setPointSize(10); // Slightly larger than default
    ui->textEdit->setFont(monoFont);

    // 2. Prevent line-wrapping so the 16-byte columns never break
    ui->textEdit->setLineWrapMode(QTextEdit::NoWrap);

    // 3. Make the window wider by default so it fits the hex dump beautifully
    resize(640, 380);

    if (m_img) {
        setWindowTitle(tr("Sector Inspector - %1").arg(m_img->originalFileName()));

        // Configure SpinBox based on disk geometry
        ui->sectorSpinBox->setRange(1, m_img->geometry().sectorCount());
        ui->sectorSpinBox->setValue(1);

        refreshSector();
    }

    // Wire Core Buttons
    connect(ui->btnReload, &QPushButton::clicked, this, &SectorInspectorDialog::refreshSector);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);

    // Wire Search Functionality
    connect(ui->btnSearch, &QPushButton::clicked, this, &SectorInspectorDialog::on_btnSearch_clicked);
    connect(ui->searchLineEdit, &QLineEdit::returnPressed, ui->btnSearch, &QPushButton::click);

    connect(ui->btnEdit, &QPushButton::clicked, this, [this]() {
        if (!m_img) return;

        SectorEditDialog dlg(m_img, ui->sectorSpinBox->value(), this);

        // If the user clicks "Save" in the popup, refresh our HTML view!
        connect(&dlg, &SectorEditDialog::sectorSaved, this, &SectorInspectorDialog::refreshSector);

        dlg.exec(); // Open as a modal blocking popup
    });

}

SectorInspectorDialog::~SectorInspectorDialog() {
    delete ui;
}

void SectorInspectorDialog::on_sectorSpinBox_valueChanged(int /*sector*/) {
    refreshSector();
}

void SectorInspectorDialog::refreshSector() {
    if (!m_img) return;

    int sector = ui->sectorSpinBox->value();
    QByteArray buffer;

    if (m_img->readSector(sector, buffer)) {
        formatSector(buffer);
    } else {
        ui->textEdit->setHtml("<b style='color:red;'>Error reading sector!</b>");
    }
}

void SectorInspectorDialog::on_btnSearch_clicked() {
    if (!m_img) return;

    QString term = ui->searchLineEdit->text();
    if (term.isEmpty()) return;

    QByteArray searchBytes;

    // Index 0 is "Text", Index 1 is "Hex"
    if (ui->searchTypeCombo->currentIndex() == 0) {
        searchBytes = term.toLatin1();
    } else {
        searchBytes = QByteArray::fromHex(term.toLatin1());
    }

    if (searchBytes.isEmpty()) {
        ui->searchStatusLabel->setText("<font color='red'>Invalid hex</font>");
        return;
    }

    ui->searchStatusLabel->setText("Searching...");
    QCoreApplication::processEvents(); // Force UI update

    int currentSector = ui->sectorSpinBox->value();
    int maxSector = m_img->geometry().sectorCount();

    int startSector = currentSector + 1;
    if (startSector > maxSector) startSector = 1;

    bool found = false;
    QByteArray buffer;
    QByteArray overlap; // Holds the tail end of the previous sector

    // We need to keep exactly (SearchLength - 1) bytes to catch splits
    int overlapSize = searchBytes.size() - 1;

    // Helper lambda to do the overlapping search
    auto doSearch = [&](int start, int end) {
        overlap.clear();
        for (int i = start; i <= end; ++i) {
            if (m_img->readSector(i, buffer)) {

                // Glue the end of the last sector to the front of this one
                QByteArray combined = overlap + buffer;

                int matchPos = combined.indexOf(searchBytes);
                if (matchPos != -1) {
                    // Did it start in the overlap (previous sector) or this sector?
                    // Note: If i is the very first sector of the search and it matches
                    // instantly, overlap.size() is 0, so matchPos (>= 0) is not < 0.
                    int foundInSector = (matchPos < overlap.size()) ? (i - 1) : i;

                    // Safety check just in case it points to sector 0
                    if (foundInSector < 1) foundInSector = 1;

                    ui->sectorSpinBox->setValue(foundInSector);
                    QString hexSector = QString("%1").arg(foundInSector, 4, 16, QChar('0')).toUpper();
                    ui->searchStatusLabel->setText(QString("<font color='green'>Found in %1 ($%2)</font>").arg(foundInSector).arg(hexSector));
                    found = true;
                    return true;
                }

                // Save the tail end of this sector for the next loop
                if (overlapSize > 0 && buffer.size() >= overlapSize) {
                    overlap = buffer.right(overlapSize);
                }
            }
        }
        return false;
    };

    // Pass 1: Search from Next Sector to End of Disk
    if (!doSearch(startSector, maxSector)) {
        // Pass 2: Wrap around and search from Sector 1 to Current Sector
        doSearch(1, currentSector);
    }

    if (!found) {
        ui->searchStatusLabel->setText("<font color='red'>Not found</font>");
    }
}

void SectorInspectorDialog::formatSector(const QByteArray &data) {
    // 1. Check if we need to highlight anything based on the current search box
    QByteArray searchBytes;
    QString term = ui->searchLineEdit->text();
    if (!term.isEmpty()) {
        if (ui->searchTypeCombo->currentIndex() == 0) searchBytes = term.toLatin1();
        else searchBytes = QByteArray::fromHex(term.toLatin1());
    }

    // 2. Find absolute byte indices of all matches in this sector
    QList<int> highlightIndices;
    if (!searchBytes.isEmpty()) {
        int pos = 0;
        while ((pos = data.indexOf(searchBytes, pos)) != -1) {
            for (int k = 0; k < searchBytes.size(); ++k) {
                highlightIndices.append(pos + k);
            }
            pos += searchBytes.size();
        }
    }

    QString dump = QString("<pre style='margin: 0; line-height: 1.2; font-size: 10pt; font-family: \"Courier New\", Courier, monospace;'>");

    for (int i = 0; i < data.size(); i += 16) {
        QByteArray chunk = data.mid(i, 16);
        QString offset = QString("%1").arg(i, 4, 16, QChar('0')).toUpper();

        QString hex;
        QString ascii;

        for (int j = 0; j < 16; ++j) {
            if (j == 8) hex += " "; // Middle Gutter

            int byteIndex = i + j;
            bool isHighlighted = highlightIndices.contains(byteIndex);

            if (j < chunk.size()) {
                quint8 byte = static_cast<quint8>(chunk[j]);
                QString bHex = QString("%1").arg(byte, 2, 16, QChar('0')).toUpper();

                // HEX FORMATTING
                if (isHighlighted) {
                    hex += QString("<span style='background-color: #FFFF00; color: #000000;'>%1</span> ").arg(bHex);
                } else if (byte == 0x00 || byte == 0x20) {
                    hex += QString("<font color='#444444'>%1</font> ").arg(bHex);
                } else {
                    hex += bHex + " ";
                }

                // ASCII FORMATTING
                char mappedChar = '.';
                if (byte >= 32 && byte <= 126) mappedChar = static_cast<char>(byte);
                else if (byte >= 160 && byte <= 254) mappedChar = static_cast<char>(byte - 128);

                // We must escape characters BEFORE wrapping them in HTML spans
                QString escapedChar = QString(mappedChar).toHtmlEscaped();

                if (isHighlighted) {
                    ascii += QString("<span style='background-color: #FFFF00; color: #000000;'>%1</span>").arg(escapedChar);
                } else {
                    ascii += escapedChar;
                }

            } else {
                hex += "   ";
            }
        }

        // We pass the raw 'ascii' string here because it already contains HTML tags and is pre-escaped
        dump += QString("<font color='#888888'>%1:</font>  %2  <font color='#888888'>|</font>  %3\n")
                    .arg(offset).arg(hex).arg(ascii);
    }

    dump += "</pre>";
    ui->textEdit->setHtml(dump);
}
