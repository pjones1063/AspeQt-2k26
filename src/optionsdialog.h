/*
 * optionsdialog.h
 */

#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QtWidgets/QDialog>
#include <QTreeWidget>
#include <QtDebug>

#include "serialport.h"

namespace Ui {
    class OptionsDialog;
}

class OptionsDialog : public QDialog {
    Q_OBJECT

public:
    OptionsDialog(QWidget *parent = 0);
    ~OptionsDialog();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::OptionsDialog *m_ui;
    QTreeWidgetItem *itemStandard, *itemAtariSio, *itemEmulation, *itemI18n;

private slots:
    void on_serialPortComboBox_currentIndexChanged(int index);
    void on_serialPortHandshakeCombo_currentIndexChanged(int index);
    void on_serialPortUseDivisorsBox_toggled(bool checked);
    void on_treeWidget_itemClicked(QTreeWidgetItem* item, int column);
    void on_treeWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void OptionsDialog_accepted();
    void on_useEmulationCustomCasBaudBox_toggled(bool checked);
    void on_buttonRclFolder_clicked();
};

#endif // OPTIONSDIALOG_H
