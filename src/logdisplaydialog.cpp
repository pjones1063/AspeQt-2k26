/*
 * logdisplaydialog.cpp
 */

#include "logdisplaydialog.h"
#include "ui_logdisplaydialog.h"
#include "mainwindow.h"

#include <QTranslator>
#include <QDir>
#include <QMessageBox>
#include <QFileDialog>  // Needed for Save
#include <QTextStream>  // Needed for Save

QString g_savedLog;

LogDisplayDialog::LogDisplayDialog(QWidget *parent) :
    QDialog(parent),
    l_ui(new Ui::LogDisplayDialog)
{
    Qt::WindowFlags flags = windowFlags();
    flags = flags & (~Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);

    l_ui->setupUi(this);

    // Connect the button box to our click handler
    connect(l_ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(onClick(QAbstractButton*)));
}

LogDisplayDialog::~LogDisplayDialog()
{
    delete l_ui;
}

void LogDisplayDialog::onClick(QAbstractButton* button)
{
    // Check if the user clicked the "Save" button or the "Close" button
    if (l_ui->buttonBox->standardButton(button) == QDialogButtonBox::Save) {
        saveLog();
    } else {
        this->close();
    }
}

void LogDisplayDialog::saveLog()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Log"),
                                                    QDir::homePath() + "/aspeqt_log.txt",
                                                    tr("Text Files (*.txt);;HTML Files (*.html);;All Files (*)"));
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);

        // Export cleanly depending on what extension they chose
        if (fileName.endsWith(".html", Qt::CaseInsensitive)) {
            out << l_ui->textEdit->toHtml();
        } else {
            out << l_ui->textEdit->toPlainText();
        }
        file.close();
        QMessageBox::information(this, tr("Success"), tr("Log saved successfully!"));
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not save the log file."));
    }
}

void LogDisplayDialog::closeEvent(QCloseEvent *)
{
}

void LogDisplayDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        l_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void LogDisplayDialog::getLogText(QString logText)
{
    g_savedLog.clear();
    l_ui->textEdit->clear();
    l_ui->textEdit->ensureCursorVisible();
    if (!logText.isEmpty()){
        l_ui->textEdit->setHtml(logText);
        g_savedLog.append(logText);
    }
}

void LogDisplayDialog::getLogTextChange (QString logChange)
{
    // Filter removed, append directly
    l_ui->textEdit->append(logChange);
    g_savedLog.append(logChange);
    g_savedLog.append("<br>");
}
