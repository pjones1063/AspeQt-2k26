/*
 * textprinterwindow.cpp
 */

#include "epsonprinterwindow.h"
#include "ui_epsonprinterwindow.h"
#include "aspeqtsettings.h"

#include <QFileDialog>
#include <QStandardPaths>
#include <QMessageBox>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QPdfWriter>
#include <QPageSize>
#include <QPixmap>

EpsonPrinterWindow::EpsonPrinterWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::EpsonPrinterWindow)
{

    ui->setupUi(this);
    m_zoomFactor = 0.5;
    // --- OVERWRITE THE OLD TEXT UI WITH AN IMAGE VIEWER ---
    m_scrollArea = new QScrollArea(this);
    m_paperLabel = new QLabel(m_scrollArea);

    // Set a nice dark background behind the paper, and center it
    m_scrollArea->setBackgroundRole(QPalette::Dark);
    m_scrollArea->setWidget(m_paperLabel);
    m_scrollArea->setAlignment(Qt::AlignCenter);

    // Replace the central widget (keeps the toolbars, drops the old text boxes)
    setCentralWidget(m_scrollArea);

    m_dirtyPaper = false;
    m_renderTimer = new QTimer(this);
    connect(m_renderTimer, &QTimer::timeout, this, &EpsonPrinterWindow::renderPaper);
    m_renderTimer->start(250);

}

EpsonPrinterWindow::~EpsonPrinterWindow()
{
    delete ui;
}

void EpsonPrinterWindow::updatePaper(const QImage &image)
{
    m_baseImage = image;
    m_dirtyPaper = true; // Flag that there is new ink to show!

    // Respect the user's Auto-Pop preference
    if (aspeqtSettings->printerAutoPop() && !this->isVisible()) {
        this->show();
        this->raise();
        this->activateWindow();
    }
}

void EpsonPrinterWindow::renderPaper()
{
    if (!m_dirtyPaper || m_baseImage.isNull()) return;
    applyZoom();
    m_dirtyPaper = false;
}


void EpsonPrinterWindow::applyZoom()
{
    if (m_baseImage.isNull()) return;

    // Calculate the new scaled size
    QSize newSize = m_baseImage.size() * m_zoomFactor;

    // Scale the image smoothly and apply it to the label
    m_paperLabel->setPixmap(QPixmap::fromImage(m_baseImage).scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_paperLabel->resize(newSize);
}

// --- ZOOM SLOTS ---
void EpsonPrinterWindow::on_actionZoom_In_triggered()
{
    m_zoomFactor += 0.25; // Zoom in by 25%
    applyZoom();
}

void EpsonPrinterWindow::on_actionZoom_Out_triggered()
{
    if (m_zoomFactor > 0.25) { // Prevent zooming into negative space!
        m_zoomFactor -= 0.25;
        applyZoom();
    }
}

void EpsonPrinterWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}

void EpsonPrinterWindow::closeEvent(QCloseEvent *e)
{
    emit closed();
    QMainWindow::closeEvent(e);
}

// --- Kept to prevent compilation errors in mainwindow.cpp ---
QString EpsonPrinterWindow::getAsciiText() const {
    return QString();
}

// =================================================================
// ACTIVE UI TOOLBAR SLOTS
// =================================================================

void EpsonPrinterWindow::on_actionClear_triggered()
{
    m_paperLabel->clear();
    m_paperLabel->resize(0, 0);
    m_baseImage = QImage(); // Wipe the local cache

    // Signal the emulator core to dump its buffer on the next pass!
    aspeqtSettings->setPrinterClearRequested(true);
}

void EpsonPrinterWindow::on_actionSave_triggered()
{
    if (m_baseImage.isNull()) {
        QMessageBox::warning(this, tr("Empty"), tr("There is no printout to save!"));
        return;
    }

    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Atari_Printout.png";
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Printout"), defaultPath, tr("PNG Image (*.png);;PDF Document (*.pdf)"));
    if (fileName.isEmpty()) return;

    if (fileName.endsWith(".pdf", Qt::CaseInsensitive)) {
        QPdfWriter pdfWriter(fileName);

        if (aspeqtSettings->printerFeedMode() == 0) {
            // --- TRACTOR FEED (One Massive Banner Page) ---
            QSizeF sizeInMm = QSizeF((m_baseImage.width() / 240.0) * 25.4, (m_baseImage.height() / 216.0) * 25.4);
            pdfWriter.setPageSize(QPageSize(sizeInMm, QPageSize::Millimeter));
            pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));
            pdfWriter.setResolution(300);

            QPainter painter(&pdfWriter);
            painter.drawImage(painter.viewport(), m_baseImage);
            painter.end();
        } else {
            // --- SINGLE SHEET (Multi-Page PDF Slicer) ---
            pdfWriter.setPageSize(QPageSize(QPageSize::Letter));
            pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));
            pdfWriter.setResolution(216); // Match our exact hardware Y-DPI

            QPainter painter(&pdfWriter);
            int pageHeightPx = 2376; // 11 inches at 216 DPI
            int totalPages = std::ceil((double)m_baseImage.height() / pageHeightPx);

            for (int i = 0; i < totalPages; i++) {
                if (i > 0) pdfWriter.newPage(); // Trigger standard PDF page break

                // Cut exactly 11 inches of graphics out of the master canvas
                QRect sourceRect(0, i * pageHeightPx, m_baseImage.width(), pageHeightPx);
                QImage pageImg = m_baseImage.copy(sourceRect);
                painter.drawImage(0, 0, pageImg);
            }
            painter.end();
        }
    } else {
        m_baseImage.save(fileName); // Standard PNG
    }
}

void EpsonPrinterWindow::on_actionPrint_triggered()
{
    if (m_baseImage.isNull()) {
        QMessageBox::warning(this, tr("Empty"), tr("There is no printout to print!"));
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog printDialog(&printer, this);

    if (printDialog.exec() == QDialog::Accepted) {
        QPainter painter(&printer);

        if (aspeqtSettings->printerFeedMode() == 0) {
            // Tractor Feed Print
            QRect rect = painter.viewport();
            QSize size = m_baseImage.size();
            size.scale(rect.size(), Qt::KeepAspectRatio);
            painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
            painter.setWindow(m_baseImage.rect());
            painter.drawImage(0, 0, m_baseImage);
        } else {
            // Single Sheet Print (Slice and send to physical printer tray)
            int pageHeightPx = 2376;
            int totalPages = std::ceil((double)m_baseImage.height() / pageHeightPx);

            for (int i = 0; i < totalPages; i++) {
                if (i > 0) printer.newPage(); // Tell physical printer to pull new paper

                QRect sourceRect(0, i * pageHeightPx, m_baseImage.width(), pageHeightPx);
                QImage pageImg = m_baseImage.copy(sourceRect);

                QRect rect = painter.viewport();
                QSize size = pageImg.size();
                size.scale(rect.width(), rect.height(), Qt::KeepAspectRatio);

                painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
                painter.setWindow(pageImg.rect());
                painter.drawImage(0, 0, pageImg);
            }
        }
        painter.end();
    }
}
