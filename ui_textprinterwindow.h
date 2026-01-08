/********************************************************************************
** Form generated from reading UI file 'textprinterwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TEXTPRINTERWINDOW_H
#define UI_TEXTPRINTERWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFontComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TextPrinterWindow
{
public:
    QAction *actionSave;
    QAction *actionClear;
    QAction *actionWord_wrap;
    QAction *actionPrint;
    QAction *actionAtasciiFont;
    QAction *actionFont_Size;
    QAction *actionHideShow_Ascii;
    QAction *actionHideShow_Atascii;
    QAction *actionStrip_Line_Numbers;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QPlainTextEdit *printerTextEdit;
    QPlainTextEdit *printerTextEditASCII;
    QLabel *atasciiFontName;
    QFontComboBox *asciiFontName;
    QStatusBar *statusbar;
    QToolBar *toolBar;

    void setupUi(QMainWindow *TextPrinterWindow)
    {
        if (TextPrinterWindow->objectName().isEmpty())
            TextPrinterWindow->setObjectName(QString::fromUtf8("TextPrinterWindow"));
        TextPrinterWindow->resize(666, 437);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/silk-icons/icons/printer.png"), QSize(), QIcon::Normal, QIcon::Off);
        TextPrinterWindow->setWindowIcon(icon);
        actionSave = new QAction(TextPrinterWindow);
        actionSave->setObjectName(QString::fromUtf8("actionSave"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/silk-icons/icons/disk.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionSave->setIcon(icon1);
        actionClear = new QAction(TextPrinterWindow);
        actionClear->setObjectName(QString::fromUtf8("actionClear"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/silk-icons/icons/page_white.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionClear->setIcon(icon2);
        actionWord_wrap = new QAction(TextPrinterWindow);
        actionWord_wrap->setObjectName(QString::fromUtf8("actionWord_wrap"));
        actionWord_wrap->setCheckable(true);
        actionWord_wrap->setChecked(false);
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/silk-icons/icons/text_align_left.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionWord_wrap->setIcon(icon3);
        actionPrint = new QAction(TextPrinterWindow);
        actionPrint->setObjectName(QString::fromUtf8("actionPrint"));
        actionPrint->setIcon(icon);
        actionAtasciiFont = new QAction(TextPrinterWindow);
        actionAtasciiFont->setObjectName(QString::fromUtf8("actionAtasciiFont"));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/icons/silk-icons/icons/font_go.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon4.addFile(QString::fromUtf8(":/icons/silk-icons/icons/font_go.png"), QSize(), QIcon::Normal, QIcon::On);
        actionAtasciiFont->setIcon(icon4);
        actionAtasciiFont->setVisible(true);
        actionAtasciiFont->setIconVisibleInMenu(true);
        actionFont_Size = new QAction(TextPrinterWindow);
        actionFont_Size->setObjectName(QString::fromUtf8("actionFont_Size"));
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/format_font_size_more.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon5.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/format_font_size_more.png"), QSize(), QIcon::Normal, QIcon::On);
        actionFont_Size->setIcon(icon5);
        actionFont_Size->setVisible(true);
        actionHideShow_Ascii = new QAction(TextPrinterWindow);
        actionHideShow_Ascii->setObjectName(QString::fromUtf8("actionHideShow_Ascii"));
        actionHideShow_Ascii->setCheckable(true);
        actionHideShow_Ascii->setChecked(false);
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/view_right_close.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon6.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/view_right_close.png"), QSize(), QIcon::Normal, QIcon::On);
        actionHideShow_Ascii->setIcon(icon6);
        actionHideShow_Ascii->setVisible(true);
        actionHideShow_Atascii = new QAction(TextPrinterWindow);
        actionHideShow_Atascii->setObjectName(QString::fromUtf8("actionHideShow_Atascii"));
        actionHideShow_Atascii->setCheckable(true);
        actionHideShow_Atascii->setChecked(false);
        actionHideShow_Atascii->setEnabled(true);
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/view_left_close.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon7.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/view_left_close.png"), QSize(), QIcon::Normal, QIcon::On);
        actionHideShow_Atascii->setIcon(icon7);
        actionHideShow_Atascii->setVisible(true);
        actionStrip_Line_Numbers = new QAction(TextPrinterWindow);
        actionStrip_Line_Numbers->setObjectName(QString::fromUtf8("actionStrip_Line_Numbers"));
        actionStrip_Line_Numbers->setEnabled(true);
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/icons/silk-icons/icons/cut_red.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionStrip_Line_Numbers->setIcon(icon8);
        centralwidget = new QWidget(TextPrinterWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(centralwidget->sizePolicy().hasHeightForWidth());
        centralwidget->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        printerTextEdit = new QPlainTextEdit(centralwidget);
        printerTextEdit->setObjectName(QString::fromUtf8("printerTextEdit"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(printerTextEdit->sizePolicy().hasHeightForWidth());
        printerTextEdit->setSizePolicy(sizePolicy1);
        QFont font;
        font.setFamily(QString::fromUtf8("MS Shell Dlg 2"));
        font.setBold(true);
        font.setWeight(75);
        printerTextEdit->setFont(font);
        printerTextEdit->setMouseTracking(false);
        printerTextEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
        printerTextEdit->setReadOnly(true);

        gridLayout->addWidget(printerTextEdit, 0, 0, 1, 1);

        printerTextEditASCII = new QPlainTextEdit(centralwidget);
        printerTextEditASCII->setObjectName(QString::fromUtf8("printerTextEditASCII"));
        printerTextEditASCII->setEnabled(true);
        printerTextEditASCII->setReadOnly(true);

        gridLayout->addWidget(printerTextEditASCII, 0, 1, 1, 1);

        atasciiFontName = new QLabel(centralwidget);
        atasciiFontName->setObjectName(QString::fromUtf8("atasciiFontName"));
        sizePolicy.setHeightForWidth(atasciiFontName->sizePolicy().hasHeightForWidth());
        atasciiFontName->setSizePolicy(sizePolicy);
        atasciiFontName->setMaximumSize(QSize(16777215, 16777215));

        gridLayout->addWidget(atasciiFontName, 1, 0, 1, 1);

        asciiFontName = new QFontComboBox(centralwidget);
        asciiFontName->setObjectName(QString::fromUtf8("asciiFontName"));
        QFont font1;
        font1.setFamily(QString::fromUtf8("MS Shell Dlg 2"));
        font1.setPointSize(8);
        asciiFontName->setFont(font1);
        QFont font2;
        font2.setFamily(QString::fromUtf8("Arial"));
        asciiFontName->setCurrentFont(font2);

        gridLayout->addWidget(asciiFontName, 1, 1, 1, 1);

        TextPrinterWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(TextPrinterWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        TextPrinterWindow->setStatusBar(statusbar);
        toolBar = new QToolBar(TextPrinterWindow);
        toolBar->setObjectName(QString::fromUtf8("toolBar"));
        QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(toolBar->sizePolicy().hasHeightForWidth());
        toolBar->setSizePolicy(sizePolicy2);
        toolBar->setMovable(false);
        toolBar->setIconSize(QSize(16, 16));
        toolBar->setFloatable(false);
        TextPrinterWindow->addToolBar(Qt::TopToolBarArea, toolBar);

        toolBar->addAction(actionSave);
        toolBar->addAction(actionPrint);
        toolBar->addAction(actionAtasciiFont);
        toolBar->addAction(actionFont_Size);
        toolBar->addAction(actionHideShow_Atascii);
        toolBar->addAction(actionHideShow_Ascii);
        toolBar->addAction(actionStrip_Line_Numbers);
        toolBar->addAction(actionWord_wrap);
        toolBar->addAction(actionClear);

        retranslateUi(TextPrinterWindow);

        QMetaObject::connectSlotsByName(TextPrinterWindow);
    } // setupUi

    void retranslateUi(QMainWindow *TextPrinterWindow)
    {
        TextPrinterWindow->setWindowTitle(QCoreApplication::translate("TextPrinterWindow", "AspeQt - Printer text output", nullptr));
        actionSave->setText(QCoreApplication::translate("TextPrinterWindow", "Save to a file...", nullptr));
#if QT_CONFIG(tooltip)
        actionSave->setToolTip(QCoreApplication::translate("TextPrinterWindow", "Save contents to a file (Ctrl+S)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionSave->setStatusTip(QCoreApplication::translate("TextPrinterWindow", "Save contents to a file", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionSave->setShortcut(QCoreApplication::translate("TextPrinterWindow", "Ctrl+S", nullptr));
#endif // QT_CONFIG(shortcut)
        actionClear->setText(QCoreApplication::translate("TextPrinterWindow", "Clear", nullptr));
#if QT_CONFIG(tooltip)
        actionClear->setToolTip(QCoreApplication::translate("TextPrinterWindow", "Clear contents (Ctrl+C)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionClear->setStatusTip(QCoreApplication::translate("TextPrinterWindow", "Clear contents", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionClear->setShortcut(QCoreApplication::translate("TextPrinterWindow", "Ctrl+C", nullptr));
#endif // QT_CONFIG(shortcut)
        actionWord_wrap->setText(QCoreApplication::translate("TextPrinterWindow", "Word wrap", nullptr));
#if QT_CONFIG(tooltip)
        actionWord_wrap->setToolTip(QCoreApplication::translate("TextPrinterWindow", "Toggle word wrapping (Ctrl+W)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionWord_wrap->setStatusTip(QCoreApplication::translate("TextPrinterWindow", "Toggle word wrapping", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionWord_wrap->setShortcut(QCoreApplication::translate("TextPrinterWindow", "Ctrl+W", nullptr));
#endif // QT_CONFIG(shortcut)
        actionPrint->setText(QCoreApplication::translate("TextPrinterWindow", "Print", nullptr));
#if QT_CONFIG(tooltip)
        actionPrint->setToolTip(QCoreApplication::translate("TextPrinterWindow", "Send contents to printer (Ctrl+P)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionPrint->setStatusTip(QCoreApplication::translate("TextPrinterWindow", "Send contents to printer", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionPrint->setShortcut(QCoreApplication::translate("TextPrinterWindow", "Ctrl+P", nullptr));
#endif // QT_CONFIG(shortcut)
        actionAtasciiFont->setText(QCoreApplication::translate("TextPrinterWindow", "Atascii Font", nullptr));
#if QT_CONFIG(tooltip)
        actionAtasciiFont->setToolTip(QCoreApplication::translate("TextPrinterWindow", "Toggle ATASCII fonts (Alt+F)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionAtasciiFont->setStatusTip(QCoreApplication::translate("TextPrinterWindow", "Toggle ATASCII fonts", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionAtasciiFont->setShortcut(QCoreApplication::translate("TextPrinterWindow", "Alt+F", nullptr));
#endif // QT_CONFIG(shortcut)
        actionFont_Size->setText(QCoreApplication::translate("TextPrinterWindow", "Font Size", nullptr));
#if QT_CONFIG(tooltip)
        actionFont_Size->setToolTip(QCoreApplication::translate("TextPrinterWindow", "Toggle Font Size (6, 9, 12 pt) (Alt+Shift+F)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionFont_Size->setStatusTip(QCoreApplication::translate("TextPrinterWindow", "Toggle Font Size (6, 9, 12 pt)", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionFont_Size->setShortcut(QCoreApplication::translate("TextPrinterWindow", "Alt+Shift+F", nullptr));
#endif // QT_CONFIG(shortcut)
        actionHideShow_Ascii->setText(QCoreApplication::translate("TextPrinterWindow", "Hide/Show Ascii", nullptr));
#if QT_CONFIG(tooltip)
        actionHideShow_Ascii->setToolTip(QCoreApplication::translate("TextPrinterWindow", "Hide/Show Ascii Printer Output (Alt+Shift+H)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionHideShow_Ascii->setStatusTip(QCoreApplication::translate("TextPrinterWindow", "Hide/Show Ascii Printer Output", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionHideShow_Ascii->setShortcut(QCoreApplication::translate("TextPrinterWindow", "Alt+Shift+H", nullptr));
#endif // QT_CONFIG(shortcut)
        actionHideShow_Atascii->setText(QCoreApplication::translate("TextPrinterWindow", "HideShow_Atascii", nullptr));
#if QT_CONFIG(tooltip)
        actionHideShow_Atascii->setToolTip(QCoreApplication::translate("TextPrinterWindow", " Hide/Show Atascii Printer Output (Alt+H)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionHideShow_Atascii->setStatusTip(QCoreApplication::translate("TextPrinterWindow", "Hide/Show Atascii Printer Output", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionHideShow_Atascii->setShortcut(QCoreApplication::translate("TextPrinterWindow", "Alt+H", nullptr));
#endif // QT_CONFIG(shortcut)
        actionStrip_Line_Numbers->setText(QCoreApplication::translate("TextPrinterWindow", "Strip Line Numbers", nullptr));
#if QT_CONFIG(tooltip)
        actionStrip_Line_Numbers->setToolTip(QCoreApplication::translate("TextPrinterWindow", "Strip Line numbers from the text output (Ctrl-S)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionStrip_Line_Numbers->setStatusTip(QCoreApplication::translate("TextPrinterWindow", "Strip Line numbers from the text output", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionStrip_Line_Numbers->setShortcut(QCoreApplication::translate("TextPrinterWindow", "Ctrl+S", nullptr));
#endif // QT_CONFIG(shortcut)
#if QT_CONFIG(tooltip)
        printerTextEdit->setToolTip(QCoreApplication::translate("TextPrinterWindow", "Atari Output (Atascii)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        printerTextEdit->setStatusTip(QCoreApplication::translate("TextPrinterWindow", "Atari Output (Atascii)", nullptr));
#endif // QT_CONFIG(statustip)
        printerTextEdit->setPlainText(QString());
#if QT_CONFIG(tooltip)
        printerTextEditASCII->setToolTip(QCoreApplication::translate("TextPrinterWindow", "Atari Output (Ascii)", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        printerTextEditASCII->setStatusTip(QCoreApplication::translate("TextPrinterWindow", "Atari Output (Ascii)", nullptr));
#endif // QT_CONFIG(statustip)
        atasciiFontName->setText(QString());
        toolBar->setWindowTitle(QCoreApplication::translate("TextPrinterWindow", "toolBar", nullptr));
    } // retranslateUi

};

namespace Ui {
    class TextPrinterWindow: public Ui_TextPrinterWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TEXTPRINTERWINDOW_H
