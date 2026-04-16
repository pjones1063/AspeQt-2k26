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
        State_Feed_J
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

    void initializePaper();
   void drawTextString(const QString &text);
    void drawGraphics(const QByteArray &payload);
    void lineFeed();
    void parsePrintJob(const QByteArray &data);
    char translateAtascii(unsigned char b);
    void fillPaperBackground(QImage &img);

};

#endif // EPSONPRINTER_H


