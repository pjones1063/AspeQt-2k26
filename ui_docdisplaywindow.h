/********************************************************************************
** Form generated from reading UI file 'docdisplaywindow.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DOCDISPLAYWINDOW_H
#define UI_DOCDISPLAYWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DocDisplayWindow
{
public:
    QAction *actionPrint;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QTextBrowser *docDisplay;
    QStatusBar *statusbar;
    QToolBar *toolBar;

    void setupUi(QMainWindow *DocDisplayWindow)
    {
        if (DocDisplayWindow->objectName().isEmpty())
            DocDisplayWindow->setObjectName(QString::fromUtf8("DocDisplayWindow"));
        DocDisplayWindow->setWindowModality(Qt::NonModal);
        DocDisplayWindow->resize(640, 480);
        QIcon icon;
        icon.addFile(QString::fromUtf8("silk-icons/icons/report.png"), QSize(), QIcon::Normal, QIcon::Off);
        DocDisplayWindow->setWindowIcon(icon);
        actionPrint = new QAction(DocDisplayWindow);
        actionPrint->setObjectName(QString::fromUtf8("actionPrint"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/silk-icons/icons/printer.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionPrint->setIcon(icon1);
        centralwidget = new QWidget(DocDisplayWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        docDisplay = new QTextBrowser(centralwidget);
        docDisplay->setObjectName(QString::fromUtf8("docDisplay"));
        docDisplay->setSource(QUrl(QString::fromUtf8("qrc:/documentation/AspeQt User Manual-English.html")));
        docDisplay->setOpenExternalLinks(true);

        verticalLayout->addWidget(docDisplay);

        DocDisplayWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(DocDisplayWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        DocDisplayWindow->setStatusBar(statusbar);
        toolBar = new QToolBar(DocDisplayWindow);
        toolBar->setObjectName(QString::fromUtf8("toolBar"));
        toolBar->setMovable(false);
        toolBar->setIconSize(QSize(16, 16));
        toolBar->setFloatable(false);
        DocDisplayWindow->addToolBar(Qt::TopToolBarArea, toolBar);

        toolBar->addAction(actionPrint);

        retranslateUi(DocDisplayWindow);

        QMetaObject::connectSlotsByName(DocDisplayWindow);
    } // setupUi

    void retranslateUi(QMainWindow *DocDisplayWindow)
    {
        DocDisplayWindow->setWindowTitle(QCoreApplication::translate("DocDisplayWindow", "AspeQt User Manual", nullptr));
        actionPrint->setText(QCoreApplication::translate("DocDisplayWindow", "Print", nullptr));
#if QT_CONFIG(tooltip)
        actionPrint->setToolTip(QCoreApplication::translate("DocDisplayWindow", "Print User Manual", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(statustip)
        actionPrint->setStatusTip(QCoreApplication::translate("DocDisplayWindow", "Print User Manual", nullptr));
#endif // QT_CONFIG(statustip)
#if QT_CONFIG(shortcut)
        actionPrint->setShortcut(QCoreApplication::translate("DocDisplayWindow", "Ctrl+P", nullptr));
#endif // QT_CONFIG(shortcut)
        toolBar->setWindowTitle(QCoreApplication::translate("DocDisplayWindow", "toolBar", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DocDisplayWindow: public Ui_DocDisplayWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DOCDISPLAYWINDOW_H
