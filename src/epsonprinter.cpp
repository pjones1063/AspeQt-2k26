/*
 * epsonprinter.cpp
 */

#include "epsonprinter.h"
#include "aspeqtsettings.h"
#include <QtDebug>
#include <QFontMetrics>

EpsonPrinter::EpsonPrinter(SioWorker *worker) : SioDevice(worker)
{
    m_lastOperation = 1;
    m_state = State_Text;
    m_graphicBytesExpected = 0;
    m_currentGraphicMode = 0;
}


void EpsonPrinter::handleCommand(quint8 command, quint16 aux)
{
    if(!aspeqtSettings->printerEmulation()) {
        qDebug() << "!u" << tr("[%1] ignored").arg(deviceName());
        return;
    }

    // --- Catch the UI Clear Signal to wipe memory ---
    if (aspeqtSettings->isPrinterClearRequested()) {
        m_currentTextLine.clear();
        m_currentGraphicPayload.clear();
        m_state = State_Text;
        initializePaper();
        aspeqtSettings->setPrinterClearRequested(false);
    }

    switch(command) {
    // =============================================================
    // COMMAND: GET STATUS (0x53)
    // =============================================================
    case 0x53:
    {
        if (!sio->port()->writeCommandAck()) return;
        QByteArray status(4, 0);
        status[0] = 0; status[1] = m_lastOperation; status[2] = 3; status[3] = 0;
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(status);
        qDebug() << "!n" << tr("[%1] Get status.").arg(deviceName());
        break;
    }

        // =============================================================
        // COMMAND: WRITE DATA (0x57)
        // =============================================================
    case 0x57:
    {
        int aux2 = aux % 256;
        int len = 256;

        if (aux2 == 0x4E) len = 40;
        else if (aux2 == 0x53) len = 29;
        else if (aux2 == 0x44) len = 21;

        if (!sio->port()->writeCommandAck()) return;

        QByteArray data = sio->port()->readDataFrame(len);
        if (data.isEmpty()) {
            sio->port()->writeDataNak();
            return;
        }
        sio->port()->writeDataAck();

        qDebug() << "!n" << tr("[%1] Received %2 bytes. Parsing instantly.").arg(deviceName()).arg(len);

        // Feed the data to the parser instantly!
        parsePrintJob(data);

        sio->port()->writeComplete();
        break;
    }

        // =============================================================
        // COMMAND: CLOSE CONNECTION (0x43)
        // =============================================================
    case 0x43:
    {
        if (!sio->port()->writeCommandAck()) return;
        qDebug() << "!n" << tr("[%1] Print Job Closed by Atari.").arg(deviceName());

        // Print any leftover text that didn't have an EOL character
        if (!m_currentTextLine.isEmpty()) {
            drawTextString(m_currentTextLine);
            m_currentTextLine.clear();
        }

        emit paperUpdated(m_paper);

        sio->port()->writeComplete();
        break;
    }

        // =============================================================
        // UNKNOWN COMMAND
        // =============================================================
    default:
        sio->port()->writeCommandNak();
        qWarning() << "!w" << tr("[%1] Unknown Command: $%2").arg(deviceName()).arg(command, 2, 16, QChar('0'));
        break;
    }
}


void EpsonPrinter::parsePrintJob(const QByteArray &data)
{
    if(m_paper.isNull()) initializePaper(); // Ensure we have a canvas

    for (int i = 0; i < data.size(); i++) {
        unsigned char b = static_cast<unsigned char>(data[i]);

        switch (m_state) {
        case State_Text:
            if (b == 27) {
                m_state = State_Escape;
            } else if (b == 12) { // Form Feed (Page Break)
                if (!m_currentTextLine.isEmpty()) {
                    drawTextString(m_currentTextLine);
                    m_currentTextLine.clear();
                }

                m_cursorX = 60; // Return to 0.5" left margin
                m_cursorY = ((m_cursorY / 2376) + 1) * 2376 + 108; // Jump to next page boundary

                // ALWAYS grow the canvas in memory so pages aren't lost
                if (m_cursorY > m_paper.height() - 108) {
                    QImage longerPaper(m_paper.width(), m_paper.height() + 2376, QImage::Format_RGB32);
                    fillPaperBackground(longerPaper);
                    QPainter p(&longerPaper);
                    p.drawImage(0, 0, m_paper);
                    m_paper = longerPaper;
                }
            } else if (b == 13) { // Standard CR (Carriage Return)
                if (!m_currentTextLine.isEmpty()) {
                    drawTextString(m_currentTextLine);
                    m_currentTextLine.clear();
                }
                m_cursorX = 60;
            } else if (b == 10) { // Standard LF (Line Feed)
                if (!m_currentTextLine.isEmpty()) {
                    drawTextString(m_currentTextLine);
                    m_currentTextLine.clear();
                }
                lineFeed();
            } else if (b == 155) { // Atari EOL
                if (!m_currentTextLine.isEmpty()) {
                    drawTextString(m_currentTextLine);
                    m_currentTextLine.clear();
                }
                m_cursorX = 60;
                lineFeed();
                i = data.size(); // Drop Atari padding!
                break;
            } else {
                char c = translateAtascii(b);
                if (c != 0) m_currentTextLine.append(c);
                if (m_currentTextLine.length() >= 80) {
                    drawTextString(m_currentTextLine);
                    m_currentTextLine.clear();
                    lineFeed();
                }
            }
            break;

        case State_Escape:
            if (b == 'K' || b == 'L' || b == 'Y' || b == 'Z') {
                m_currentGraphicMode = b;
                m_state = State_Graphic_N1;
            } else if (b == 'A') {
                m_state = State_Spacing_A;
            } else if (b == '3') {
                m_state = State_Spacing_3;
            } else if (b == 'J') {              // <--- CATCH ESC J
                m_state = State_Feed_J;
            } else if (b == '0') {
                m_lineHeight = 27;
                m_state = State_Text;
            } else if (b == '1') {
                m_lineHeight = 21;
                m_state = State_Text;
            } else if (b == '2') {
                m_lineHeight = 36;
                m_state = State_Text;
            } else {
                m_state = State_Text;
            }
            break;

        case State_Feed_J:                      // <--- EXECUTE ESC J
            // Epson units: 1/216 inch immediate feed. At 216 DPI, 1 unit = 1 pixel.
            m_cursorY += b;

            // ALWAYS grow the canvas in memory so pages aren't lost
            if (m_cursorY > m_paper.height() - 108) {
                QImage longerPaper(m_paper.width(), m_paper.height() + 2376, QImage::Format_RGB32);
                fillPaperBackground(longerPaper);
                QPainter p(&longerPaper);
                p.drawImage(0, 0, m_paper);
                m_paper = longerPaper;
            }
            m_state = State_Text;
            break;

        case State_Spacing_A:
            m_lineHeight = b * 3;
            m_state = State_Text;
            break;

        case State_Spacing_3:
            m_lineHeight = b;
            m_state = State_Text;
            break;

        case State_Graphic_N1:
            m_graphicBytesExpected = b;
            m_state = State_Graphic_N2;
            break;

        case State_Graphic_N2:
            m_graphicBytesExpected += (b * 256);
            m_currentGraphicPayload.clear();

            if (m_graphicBytesExpected > 0) m_state = State_Graphic_Data;
            else m_state = State_Text;
            break;

        case State_Graphic_Data:
            m_currentGraphicPayload.append(b);

            if (m_currentGraphicPayload.size() >= m_graphicBytesExpected) {
                drawGraphics(m_currentGraphicPayload);
                m_state = State_Text;
            }
            break;
        }
    }

    emit paperUpdated(m_paper);
}


char EpsonPrinter::translateAtascii(unsigned char b)
{
    b = b & 0x7F; // Strip inverse video bit
    if (b == 0) return 0; // Drop null padding completely
    if (b < 32 || b > 126) return ' '; // Turn weird control characters into blank spaces
    return b;
}

void EpsonPrinter::initializePaper()
{
    // TRUE HARDWARE MATRIX: 8.5" x 11" at 240 DPI (X) and 216 DPI (Y)
    m_paper = QImage(2040, 2376, QImage::Format_RGB32);
    fillPaperBackground(m_paper);

    m_cursorX = 60; // 0.5 inch left margin
    m_cursorY = 108; // 0.5 inch top margin
    m_lineHeight = 36;
}


void EpsonPrinter::drawGraphics(const QByteArray &payload)
{
    if(m_paper.isNull()) initializePaper();

    QPainter painter(&m_paper);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);

    // Horizontal DPI is 240.
    int dotWidth = 1; // Quad (Z - 240 DPI) is 1 pixel wide
    if (m_currentGraphicMode == 'K') dotWidth = 4;      // Single (K - 60 DPI) is 4 pixels wide
    else if (m_currentGraphicMode == 'L' || m_currentGraphicMode == 'Y') dotWidth = 2; // Double (L/Y - 120 DPI) is 2 pixels wide

    for(int i = 0; i < payload.size(); i++) {
        unsigned char col = static_cast<unsigned char>(payload[i]);

        for(int pin = 0; pin < 8; pin++) {
            if(col & (128 >> pin)) {
                // Vertical pins are 1/72 inch apart. At 216 DPI, each dot is 3 pixels tall!
                painter.drawRect(m_cursorX, m_cursorY + (pin * 3), dotWidth, 3);
            }
        }
        m_cursorX += dotWidth;
    }
}

void EpsonPrinter::drawTextString(const QString &text)
{
    if(m_paper.isNull()) initializePaper();

    QPainter painter(&m_paper);
    painter.setPen(Qt::black);

    // --- UPGRADED FONT ---
    QFont font("Courier New"); // A slightly crisper monospaced font
    font.setPixelSize(30);     // Force exactly 30 pixels tall so it perfectly fills our 36-pixel line height!
    //font.setWeight(QFont::Bold); // Give it that heavy dot-matrix ink look
    painter.setFont(font);

    QFontMetrics metrics(font);

    // Instead of hardcoding "27", we ask Qt exactly where the baseline should sit
    // based on whatever font and size we chose above!
    painter.drawText(m_cursorX, m_cursorY + metrics.ascent(), text);

    m_cursorX += metrics.horizontalAdvance(text);
}



void EpsonPrinter::lineFeed()
{
    m_cursorX = 60; // Return to left margin
    m_cursorY += m_lineHeight;

    // Tractor Feed at 216 DPI (11 inches = 2376 pixels)
    // ALWAYS grow the canvas in memory so pages aren't lost
    if (m_cursorY > m_paper.height() - 108) {
        QImage longerPaper(m_paper.width(), m_paper.height() + 2376, QImage::Format_RGB32);
        fillPaperBackground(longerPaper);

        QPainter p(&longerPaper);
        p.drawImage(0, 0, m_paper);
        p.end();

        m_paper = longerPaper;
    }
}

void EpsonPrinter::fillPaperBackground(QImage &img)
{
    int style = aspeqtSettings->printerStyle();

    if (style == 2) {
        // Style 2: Aged Yellow Paper
        img.fill(QColor(255, 248, 220)); // Vintage Cornsilk
    } else {
        // Base white for both Pure White and Green-Bar
        img.fill(Qt::white);

        if (style == 1) {
            // Style 1: Vintage Green-Bar
            QPainter p(&img);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(225, 255, 225)); // Light printer green

            // Standard green-bar paper has alternating 1/2 inch horizontal bars.
            // At our 216 DPI vertical matrix, 1/2 inch is exactly 108 pixels.
            for (int y = 0; y < img.height(); y += 216) {
                p.drawRect(0, y, img.width(), 108);
            }
        }
    }
}
