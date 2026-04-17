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

    // 1. Create a temporary copy so we don't permanently scar the saved PNG!
    QImage displayImage = m_baseImage.copy();

    // 2. If the user is in "Single Sheet" mode, draw the page breaks
    if (aspeqtSettings->printerFeedMode() == 1) {
        int pageLength = aspeqtSettings->printerMarginLength();

        if (pageLength > 0) {
            QPainter painter(&displayImage);

            // Set up a nice, visible red dashed line
            QPen breakPen(QColor(255, 0, 0, 180)); // Soft red with slight transparency
            breakPen.setStyle(Qt::DashLine);
            breakPen.setWidth(4); // Make it thick enough to see easily
            painter.setPen(breakPen);

            // Draw a horizontal line at every page break interval
            for (int y = pageLength; y < displayImage.height(); y += pageLength) {
                painter.drawLine(0, y, displayImage.width(), y);
            }
            painter.end();
        }
    }

    // 3. Calculate the new scaled size
    QSize newSize = displayImage.size() * m_zoomFactor;

    // 4. Scale the decorated image smoothly and apply it to the label
    m_paperLabel->setPixmap(QPixmap::fromImage(displayImage).scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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

    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Atari_Printout";

    QString selectedFilter;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Printout"), defaultPath,
                                                    tr("PDF Document (*.pdf);;PNG Image (*.png)"),
                                                    &selectedFilter);

    if (fileName.isEmpty()) return;

    if (selectedFilter.contains(".pdf")) {
        if (fileName.endsWith(".png", Qt::CaseInsensitive)) fileName.chop(4);
        if (!fileName.endsWith(".pdf", Qt::CaseInsensitive)) fileName += ".pdf";
    } else {
        if (fileName.endsWith(".pdf", Qt::CaseInsensitive)) fileName.chop(4);
        if (!fileName.endsWith(".png", Qt::CaseInsensitive)) fileName += ".png";
    }

    if (fileName.endsWith(".pdf", Qt::CaseInsensitive)) {
        QPdfWriter pdfWriter(fileName);

        if (aspeqtSettings->printerFeedMode() == 0) {
            // --- TRACTOR FEED ---
            QSizeF sizeInMm = QSizeF((m_baseImage.width() / 240.0) * 25.4, (m_baseImage.height() / 216.0) * 25.4);
            pdfWriter.setPageSize(QPageSize(sizeInMm, QPageSize::Millimeter));
            pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));
            pdfWriter.setResolution(300); // Standardize PDF to 300 DPI

            QPainter painter(&pdfWriter);

            // Explicitly correct the non-square aspect ratio
            painter.scale(300.0 / 240.0, 300.0 / 216.0);

            painter.drawImage(0, 0, m_baseImage);
            painter.end();
        } else {
            // --- SINGLE SHEET ---
            pdfWriter.setPageSize(QPageSize(QPageSize::Letter));
            pdfWriter.setPageMargins(QMarginsF(0, 0, 0, 0));
            pdfWriter.setResolution(300); // Standardize PDF to 300 DPI

            QPainter painter(&pdfWriter);

            // Explicitly correct the non-square aspect ratio
            painter.scale(300.0 / 240.0, 300.0 / 216.0);

            int pageHeightPx = aspeqtSettings->printerMarginLength();
            int totalPages = std::ceil((double)m_baseImage.height() / pageHeightPx);

            for (int i = 0; i < totalPages; i++) {
                if (i > 0) pdfWriter.newPage();
                QRect sourceRect(0, i * pageHeightPx, m_baseImage.width(), pageHeightPx);
                QImage pageImg = m_baseImage.copy(sourceRect);
                painter.drawImage(0, 0, pageImg);
            }
            painter.end();
        }
    } else {
        m_baseImage.save(fileName);
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

        // Dynamically correct the non-square aspect ratio based on the OS Printer DPI
        painter.scale(printer.logicalDpiX() / 240.0, printer.logicalDpiY() / 216.0);

        if (aspeqtSettings->printerFeedMode() == 0) {
            // Tractor Feed Print
            painter.drawImage(0, 0, m_baseImage);
        } else {
            // Single Sheet Print
            int pageHeightPx = aspeqtSettings->printerMarginLength();
            int totalPages = std::ceil((double)m_baseImage.height() / pageHeightPx);

            for (int i = 0; i < totalPages; i++) {
                if (i > 0) printer.newPage();

                QRect sourceRect(0, i * pageHeightPx, m_baseImage.width(), pageHeightPx);
                QImage pageImg = m_baseImage.copy(sourceRect);
                painter.drawImage(0, 0, pageImg);
            }
        }
        painter.end();
    }
}

