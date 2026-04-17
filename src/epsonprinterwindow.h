/*
 * textprinterwindow.h
 */

#ifndef EPSONPRINTERWINDOW_H
#define EPSONPRINTERWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QLabel>
#include <QScrollArea>
#include <QTimer>
#include <QScrollBar>


namespace Ui {
class EpsonPrinterWindow;
}

class EpsonPrinterWindow : public QMainWindow {
    Q_OBJECT
public:
    EpsonPrinterWindow(QWidget *parent = 0);
    ~EpsonPrinterWindow();
    QImage getPaperImage() const { return m_baseImage; }

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *e);

public slots:
    void updatePaper(const QImage &image);
    void renderPaper();

private:
    Ui::EpsonPrinterWindow *ui;

    // The new Virtual Paper Viewer
    QScrollArea *m_scrollArea;
    QLabel *m_paperLabel;
    QImage m_baseImage;
    double m_zoomFactor;
    QTimer *m_renderTimer;
    bool m_dirtyPaper;
    void applyZoom();

private slots:
    void on_actionSave_triggered();
    void on_actionClear_triggered();
    void on_actionPrint_triggered();
    void on_actionZoom_In_triggered();
    void on_actionZoom_Out_triggered();

signals:
    void closed();
};

#endif // EPSONPRINTERWINDOW_H
