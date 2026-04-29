#ifndef BACKENDLIBRARYDIALOG_H
#define BACKENDLIBRARYDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QSplitter>
#include <QTextEdit>
#include "backendmanager.h"

class BackendLibraryDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BackendLibraryDialog(BackendManager* manager, QWidget *parent = nullptr);

private slots:
    void onAddClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onStartClicked();
    void onViewLogClicked();
    void onStopClicked();
    void refreshTable();
    void onMountDriversClicked();
    void updateProcessStatus(const QString& id, bool isRunning);
    void onBackendOutputLine(const QString& id, const QString& line, bool isError);
    void onEditSourceClicked();
    void onSelectionChanged();

private:
    BackendManager* m_manager;
    QTableWidget* m_table;
    QSplitter* m_splitter;
    QTextEdit* m_console;

    // Helper methods
    void loadLogHistory(const QString& id);
    void saveToManager();
    QString getSelectedId() const;
};

#endif // BACKENDLIBRARYDIALOG_H
