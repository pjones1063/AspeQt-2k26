/*
 * epsonprinter.h
 */

#ifndef EPSONPRINTER_H
#define EPSONPRINTER_H

#include "sioworker.h"
#include <QByteArray>
#include <QImage>
#include <QPainter>
#include <QFont>

class EpsonPrinter : public SioDevice
{
    Q_OBJECT
public:
    EpsonPrinter(SioWorker *worker);
    void handleCommand(quint8 command, quint16 aux) override;

signals:
    void paperUpdated(const QImage &image);

private:
    int m_lastOperation;

    // --- Persistent State Machine Variables ---
    enum ParserState {
        State_Text,
        State_Escape,
        State_Graphic_N1,
        State_Graphic_N2,
        State_Graphic_Data,
        State_Spacing_A,
        State_Spacing_3,
        State_Feed_J,
        State_Underline,
        State_Expanded_W,
        State_MasterPrint,
        State_Proportional_p,
        State_SuperSub_S
    };

    ParserState m_state;
    int m_graphicBytesExpected;
    char m_currentGraphicMode;
    QByteArray m_currentGraphicPayload;
    QString m_currentTextLine;

    // --- Virtual Print Head ---
    QImage m_paper;
    int m_cursorX;
    int m_cursorY;
    int m_lineHeight;

    // --- Hardware Font Flags ---
    bool m_isBold;
    bool m_isUnderlined;
    bool m_isItalic;
    bool m_isCondensed;
    bool m_isExpanded;

    // --- ENHANCED FONT FLAGS ---
    bool m_isElite;
    bool m_isProportional;
    int m_scriptMode; // 0 = Normal, 1 = Superscript, 2 = Subscript

    void initializePaper();
    void drawTextString(const QString &text);
    void drawGraphics(const QByteArray &payload);
    void lineFeed();
    void parsePrintJob(const QByteArray &data);
    char translateAtascii(unsigned char b);
    void fillPaperBackground(QImage &img);

};

#endif // EPSONPRINTER_H
