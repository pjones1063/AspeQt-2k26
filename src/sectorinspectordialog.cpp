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
    int searchType = ui->searchTypeCombo->currentIndex();

    if (searchType == 0) {
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
    int overlapSize = searchBytes.size() - 1;

    // Reset search state if the user changed the term, type, or manually scrolled the sector
    if (term != m_lastSearchTerm || searchType != m_lastSearchType || currentSector != m_lastMatchSector) {
        m_lastMatchSector = currentSector;
        m_lastMatchOffset = -1;
        m_lastSearchTerm = term;
        m_lastSearchType = searchType;
    }

    QByteArray buffer;
    QByteArray overlap;
    bool found = false;
    int previousSectorSize = 0;

    // Loop maxSector + 1 times.
    // count = 0: Search the REST of the current sector
    // count > 0: Search all other sectors and wrap around
    for (int count = 0; count <= maxSector; ++count) {

        int i = currentSector + count;
        if (i > maxSector) i -= maxSector;

        if (m_img->readSector(i, buffer)) {
            QByteArray combined;
            int startSearchPos = 0;

            if (count == 0) {
                // Pass 0: Resume searching exactly where we left off in the current sector
                combined = buffer;
                startSearchPos = m_lastMatchOffset + 1;
            } else {
                // Subsequent passes: append new sector to the tail of the last one
                combined = overlap + buffer;

                if (count == 1) {
                    // Prevent infinite loop on cross-sector matches located in the overlap region.
                    // Calculate if our resume point (m_lastMatchOffset + 1) falls inside the overlap
                    int overlapStartPos = (m_lastMatchOffset + 1) - (previousSectorSize - overlap.size());
                    if (overlapStartPos > 0) {
                        startSearchPos = overlapStartPos;
                    }
                }
            }

            int matchPos = -1;
            if (startSearchPos < combined.size()) {
                matchPos = combined.indexOf(searchBytes, startSearchPos);
            }

            if (matchPos != -1) {
                int foundInSector = i;
                int offsetInSector = matchPos;

                if (count > 0 && matchPos < overlap.size()) {
                    // Match spanned the boundary, starting in the tail of the previous sector
                    foundInSector = i - 1;
                    if (foundInSector < 1) foundInSector = maxSector;
                    offsetInSector = previousSectorSize - overlap.size() + matchPos;
                } else if (count > 0) {
                    // Adjust index to strip the overlap padding
                    offsetInSector = matchPos - overlap.size();
                }

                // Save state so the next click resumes exactly after this byte
                m_lastMatchSector = foundInSector;
                m_lastMatchOffset = offsetInSector;

                ui->sectorSpinBox->setValue(foundInSector);
                QString hexSector = QString("%1").arg(foundInSector, 4, 16, QChar('0')).toUpper();
                ui->searchStatusLabel->setText(QString("<font color='green'>Found in %1 ($%2)</font>").arg(foundInSector).arg(hexSector));

                // Force a re-render to update the active green highlight
                refreshSector();
                found = true;
                break;
            }

            // Save the tail for the next loop's overlap
            previousSectorSize = buffer.size();
            if (overlapSize > 0) {
                overlap = combined.right(qMin(overlapSize, (int)combined.size()));
            }
        }
    }

    if (!found) {
        ui->searchStatusLabel->setText("<font color='red'>Not found</font>");
        // Reset offset so clicking "Find Next" again wraps back to the top of the disk
        m_lastMatchOffset = -1;
    }
}


void SectorInspectorDialog::formatSector(const QByteArray &data) {
    QByteArray searchBytes;
    QString term = ui->searchLineEdit->text();
    if (!term.isEmpty()) {
        if (ui->searchTypeCombo->currentIndex() == 0) searchBytes = term.toLatin1();
        else searchBytes = QByteArray::fromHex(term.toLatin1());
    }

    QList<int> activeHighlightIndices;
    QList<int> passiveHighlightIndices;

    // Distinguish between the "Active" match and all other "Passive" matches in this sector
    if (!searchBytes.isEmpty() && m_img) {
        QByteArray searchSpace = data;

        // Bring in the start of the next sector so cross-sector matches can be visualized
        int nextSector = ui->sectorSpinBox->value() + 1;
        if (nextSector > m_img->geometry().sectorCount()) nextSector = 1;

        QByteArray nextBuffer;
        if (m_img->readSector(nextSector, nextBuffer)) {
            int neededBytes = searchBytes.size() - 1;
            if (neededBytes > 0) {
                searchSpace.append(nextBuffer.left(qMin(neededBytes, (int)nextBuffer.size())));
            }
        }

        int pos = 0;
        while ((pos = searchSpace.indexOf(searchBytes, pos)) != -1) {
            for (int k = 0; k < searchBytes.size(); ++k) {
                int byteIndex = pos + k;

                // Only highlight bytes that actually belong to the current sector's UI page
                if (byteIndex < data.size()) {
                    if (ui->sectorSpinBox->value() == m_lastMatchSector && pos == m_lastMatchOffset) {
                        activeHighlightIndices.append(byteIndex);
                    } else {
                        passiveHighlightIndices.append(byteIndex);
                    }
                }
            }
            pos += 1;
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
            bool isActive = activeHighlightIndices.contains(byteIndex);
            bool isPassive = passiveHighlightIndices.contains(byteIndex);

            if (j < chunk.size()) {
                quint8 byte = static_cast<quint8>(chunk[j]);
                QString bHex = QString("%1").arg(byte, 2, 16, QChar('0')).toUpper();

                // HEX FORMATTING
                if (isActive) {
                    hex += QString("<span style='background-color: #00FF00; color: #000000;'>%1</span> ").arg(bHex);
                } else if (isPassive) {
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

                QString escapedChar = QString(mappedChar).toHtmlEscaped();

                if (isActive) {
                    ascii += QString("<span style='background-color: #00FF00; color: #000000;'>%1</span>").arg(escapedChar);
                } else if (isPassive) {
                    ascii += QString("<span style='background-color: #FFFF00; color: #000000;'>%1</span>").arg(escapedChar);
                } else {
                    ascii += escapedChar;
                }

            } else {
                hex += "   ";
            }
        }

        dump += QString("<font color='#888888'>%1:</font>  %2  <font color='#888888'>|</font>  %3\n")
                    .arg(offset).arg(hex).arg(ascii);
    }

    dump += "</pre>";
    ui->textEdit->setHtml(dump);
}
