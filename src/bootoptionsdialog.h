/*
 * bootoptionsdialog.h
 */

#ifndef BOOTOPTIONSDIALOG_H
#define BOOTOPTIONSDIALOG_H

#include <QtWidgets/QDialog>
#include <QDebug>
#include <QAbstractButton>

namespace Ui {
    class BootOptionsDialog;
}

class BootOptionsDialog : public QDialog {
    Q_OBJECT

public:
    BootOptionsDialog(const QString& bootFolderPath, QWidget *parent = 0);
    ~BootOptionsDialog();

protected:
    void changeEvent(QEvent *e);

private:
    const QString& bootFolderPath_;
    Ui::BootOptionsDialog *m_ui;

public slots:
    virtual void accept();

private slots:
    void picoDOSToggled();
};
#endif // BOOTOPTIONSDIALOG_H
