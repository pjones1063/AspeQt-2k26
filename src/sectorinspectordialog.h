#ifndef SECTORINSPECTORDIALOG_H
#define SECTORINSPECTORDIALOG_H

#include <QDialog>
#include "diskimage.h"

namespace Ui {
class SectorInspectorDialog;
}

class SectorInspectorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SectorInspectorDialog(SimpleDiskImage *img, QWidget *parent = nullptr);
    ~SectorInspectorDialog();

private slots:
    void on_sectorSpinBox_valueChanged(int sector);
    void refreshSector();
    void on_btnSearch_clicked();

private:
    Ui::SectorInspectorDialog *ui;
    SimpleDiskImage *m_img;
    void formatSector(const QByteArray &data);
};

#endif // SECTORINSPECTORDIALOG_H
