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
    case 0x53: // GET STATUS
    {
        if (!sio->port()->writeCommandAck()) return;
        QByteArray status(4, 0);
        status[0] = 0; status[1] = m_lastOperation; status[2] = 3; status[3] = 0;
        sio->port()->writeComplete();
        sio->port()->writeDataFrame(status);
        qDebug() << "!n" << tr("[%1] Get status.").arg(deviceName());
        break;
    }

    case 0x57: // WRITE DATA
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

        parsePrintJob(data);

        sio->port()->writeComplete();
        break;
    }

    case 0x43: // CLOSE CONNECTION
    {
        if (!sio->port()->writeCommandAck()) return;
        qDebug() << "!n" << tr("[%1] Print Job Closed by Atari.").arg(deviceName());

        if (!m_currentTextLine.isEmpty()) {
            drawTextString(m_currentTextLine);
            m_currentTextLine.clear();
        }

        emit paperUpdated(m_paper);

        sio->port()->writeComplete();
        break;
    }

    default: // UNKNOWN COMMAND
        sio->port()->writeCommandNak();
        qWarning() << "!w" << tr("[%1] Unknown Command: $%2").arg(deviceName()).arg(command, 2, 16, QChar('0'));
        break;
    }
}


void EpsonPrinter::parsePrintJob(const QByteArray &data)
{
    if(m_paper.isNull()) initializePaper();

    int marginLeft = aspeqtSettings->printerMarginLeft();
    int marginTop = aspeqtSettings->printerMarginTop();
    int pageLen = aspeqtSettings->printerMarginLength();

    for (int i = 0; i < data.size(); i++) {
        unsigned char b = static_cast<unsigned char>(data[i]);

        switch (m_state) {
        case State_Text:
            if (b == 27) { // ESC
                if (!m_currentTextLine.isEmpty()) { drawTextString(m_currentTextLine); m_currentTextLine.clear(); }
                m_state = State_Escape;
            } else if (b == 15) { // SI (Condensed Mode ON)
                if (!m_currentTextLine.isEmpty()) { drawTextString(m_currentTextLine); m_currentTextLine.clear(); }
                m_isCondensed = true;
            } else if (b == 18) { // DC2 (Condensed Mode OFF)
                if (!m_currentTextLine.isEmpty()) { drawTextString(m_currentTextLine); m_currentTextLine.clear(); }
                m_isCondensed = false;
            } else if (b == 12) { // Form Feed (Page Break)
                if (!m_currentTextLine.isEmpty()) { drawTextString(m_currentTextLine); m_currentTextLine.clear(); }

                m_cursorX = marginLeft;
                m_cursorY = ((m_cursorY / pageLen) + 1) * pageLen + marginTop;

                if (m_cursorY > m_paper.height() - marginTop) {
                    QImage longerPaper(m_paper.width(), m_paper.height() + pageLen, QImage::Format_RGB32);
                    fillPaperBackground(longerPaper);
                    QPainter p(&longerPaper);
                    p.drawImage(0, 0, m_paper);
                    m_paper = longerPaper;
                }
            } else if (b == 13) { // CR
                if (!m_currentTextLine.isEmpty()) { drawTextString(m_currentTextLine); m_currentTextLine.clear(); }
                m_cursorX = marginLeft;
            } else if (b == 10) { // LF
                if (!m_currentTextLine.isEmpty()) { drawTextString(m_currentTextLine); m_currentTextLine.clear(); }
                lineFeed();
            } else if (b == 155) { // Atari EOL
                if (!m_currentTextLine.isEmpty()) { drawTextString(m_currentTextLine); m_currentTextLine.clear(); }
                m_cursorX = marginLeft;
                lineFeed();
                i = data.size();
                break;
            } else {

                if (b > 127) {
                    // Epson used the high-bit to trigger Italics in some modes!
                    // If we aren't handling high-bit international sets, just strip it
                    // and force Italics so it looks right.
                    m_isItalic = true;
                    b = b & 0x7F;
                }

                char c = translateAtascii(b);
                if (c != 0) m_currentTextLine.append(c);

                int maxCols = m_isCondensed ? 132 : (m_isElite ? 96 : 80);
                if (m_currentTextLine.length() >= maxCols) {
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
            } else if (b == 'A') { m_state = State_Spacing_A;
            } else if (b == '3') { m_state = State_Spacing_3;
            } else if (b == 'J') { m_state = State_Feed_J;

                // --- FONT STYLE TOGGLES ---
            } else if (b == 'E') { m_isBold = true; m_state = State_Text;
            } else if (b == 'F') { m_isBold = false; m_state = State_Text;
            } else if (b == '4') { m_isItalic = true; m_state = State_Text;
            } else if (b == '5') { m_isItalic = false; m_state = State_Text;
            } else if (b == '-') { m_state = State_Underline;
            } else if (b == 'W') { m_state = State_Expanded_W;

                // --- NEW ENHANCED TOGGLES ---
            } else if (b == 'M') { m_isElite = true; m_state = State_Text;
            } else if (b == 'P') { m_isElite = false; m_state = State_Text;
            } else if (b == 'p') { m_state = State_Proportional_p;
            } else if (b == 'S') { m_state = State_SuperSub_S;
            } else if (b == 'T') { m_scriptMode = 0; m_state = State_Text;

                // --- ALIASES ---
            } else if (b == 'G') { m_isBold = true; m_state = State_Text; // Double-Strike (Alias to Bold)
            } else if (b == 'H') { m_isBold = false; m_state = State_Text;
            } else if (b == '!') { m_state = State_MasterPrint;

            } else if (b == '0') { m_lineHeight = 27; m_state = State_Text;
            } else if (b == '1') { m_lineHeight = 21; m_state = State_Text;
            } else if (b == '2') { m_lineHeight = 36; m_state = State_Text;
            } else { m_state = State_Text; }
            break;

        case State_Underline:
            m_isUnderlined = (b == '1' || b == 1);
            m_state = State_Text;
            break;

        case State_Expanded_W:
            m_isExpanded = (b == '1' || b == 1);
            m_state = State_Text;
            break;

            // --- NEW ENHANCED STATES ---
        case State_Proportional_p:
            m_isProportional = (b == '1' || b == 1);
            m_state = State_Text;
            break;

        case State_SuperSub_S:
            if (b == '0' || b == 0) m_scriptMode = 1; // 0 = Superscript
            else m_scriptMode = 2;                    // 1 = Subscript
            m_state = State_Text;
            break;

        case State_MasterPrint:
            // The Master Print byte is a bitmask mapping multiple toggles at once
            m_isElite        = (b & 1);
            m_isProportional = (b & 2);
            m_isCondensed    = (b & 4);
            m_isBold         = (b & 8) || (b & 16);
            m_isExpanded     = (b & 32);
            m_isItalic       = (b & 64);
            m_isUnderlined   = (b & 128);
            m_state = State_Text;
            break;

        case State_Feed_J:
            m_cursorY += b;
            if (m_cursorY > m_paper.height() - marginTop) {
                QImage longerPaper(m_paper.width(), m_paper.height() + pageLen, QImage::Format_RGB32);
                fillPaperBackground(longerPaper);
                QPainter p(&longerPaper);
                p.drawImage(0, 0, m_paper);
                m_paper = longerPaper;
            }
            m_state = State_Text;
            break;

        case State_Spacing_A: m_lineHeight = b * 3; m_state = State_Text; break;
        case State_Spacing_3: m_lineHeight = b; m_state = State_Text; break;

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
    b = b & 0x7F;
    if (b == 0) return 0;
    if (b < 32 || b > 126) return ' ';
    return b;
}

void EpsonPrinter::initializePaper()
{
    int marginLeft = aspeqtSettings->printerMarginLeft();
    int marginTop = aspeqtSettings->printerMarginTop();
    int pageLen = aspeqtSettings->printerMarginLength();

    m_paper = QImage(2040, pageLen, QImage::Format_RGB32);
    fillPaperBackground(m_paper);

    m_cursorX = marginLeft;
    m_cursorY = marginTop;
    m_lineHeight = 36;

    // --- RESET HARDWARE SWITCHES ---
    m_isBold = false;
    m_isUnderlined = false;
    m_isItalic = false;
    m_isCondensed = false;
    m_isExpanded = false;

    m_isElite = false;
    m_isProportional = false;
    m_scriptMode = 0;
}


void EpsonPrinter::drawGraphics(const QByteArray &payload)
{
    if(m_paper.isNull()) initializePaper();

    QPainter painter(&m_paper);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);

    int dotWidth = 1;
    if (m_currentGraphicMode == 'K') dotWidth = 4;
    else if (m_currentGraphicMode == 'L' || m_currentGraphicMode == 'Y') dotWidth = 2;

    for(int i = 0; i < payload.size(); i++) {
        unsigned char col = static_cast<unsigned char>(payload[i]);
        for(int pin = 0; pin < 8; pin++) {
            if(col & (128 >> pin)) {
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

    // Swap to a proportional font if requested, otherwise stick to monospace
    QString fontFamily = m_isProportional ? "Helvetica" : "Courier New";
    QFont font(fontFamily);

    int yOffset = 0;

    // Handle Super/Subscript sizing and Y-axis offsets
    if (m_scriptMode != 0) {
        font.setPixelSize(18); // Squish the font
        if (m_scriptMode == 1) yOffset = -8; // Superscript moves up
        else if (m_scriptMode == 2) yOffset = 8; // Subscript moves down
    } else {
        font.setPixelSize(30);
    }

    font.setWeight(m_isBold ? QFont::Black : QFont::Medium);
    font.setUnderline(m_isUnderlined);
    font.setItalic(m_isItalic);

    // Calculate Pitch Stretching
    int stretch = 100;
    if (m_isCondensed) stretch = 60;         // ~17 CPI
    else if (m_isElite) stretch = 83;        // 12 CPI vs standard 10 CPI

    if (m_isExpanded) stretch *= 2;          // Double the calculated width

    font.setStretch(stretch);
    painter.setFont(font);

    QFontMetrics metrics(font);

    painter.drawText(m_cursorX, m_cursorY + metrics.ascent() + yOffset, text);
    m_cursorX += metrics.horizontalAdvance(text);
}

void EpsonPrinter::lineFeed()
{
    int marginLeft = aspeqtSettings->printerMarginLeft();
    int marginTop = aspeqtSettings->printerMarginTop();
    int pageLen = aspeqtSettings->printerMarginLength();

    m_cursorX = marginLeft;
    m_cursorY += m_lineHeight;

    if (m_cursorY > m_paper.height() - marginTop) {
        QImage longerPaper(m_paper.width(), m_paper.height() + pageLen, QImage::Format_RGB32);
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
        img.fill(QColor(255, 248, 220));
    } else {
        img.fill(Qt::white);
        if (style == 1) {
            QPainter p(&img);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(225, 255, 225));

            for (int y = 0; y < img.height(); y += 216) {
                // CHANGED: 60 to 108 (Exactly 1/2 inch)
                p.drawRect(0, y, img.width(), 108);
            }
        }
    }
}
