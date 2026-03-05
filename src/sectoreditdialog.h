#ifndef SECTOREDITDIALOG_H
#define SECTOREDITDIALOG_H

#include <QDialog>
#include "diskimage.h"

namespace Ui {
class SectorEditDialog;
}

class SectorEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SectorEditDialog(SimpleDiskImage *img, int sector, QWidget *parent = nullptr);
    ~SectorEditDialog();

signals:
    void sectorSaved();

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();

    // --> NEW SLOTS <--
    void onCellSelected(int row, int col);
    void on_btnInjectText_clicked();

private:
    Ui::SectorEditDialog *ui;
    SimpleDiskImage *m_img;
    int m_sector;
    int m_sectorSize;

    void loadData();
};

#endif // SECTOREDITDIALOG_H
