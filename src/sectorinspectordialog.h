#ifndef SECTORINSPECTORDIALOG_H
#define SECTORINSPECTORDIALOG_H

#include <QDialog>
#include <QString>
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

    QString m_lastSearchTerm;
    int m_lastSearchType = -1;
    int m_lastMatchSector = -1;
    int m_lastMatchOffset = -1;
};

#endif // SECTORINSPECTORDIALOG_H
