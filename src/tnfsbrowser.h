#ifndef TNFSBROWSER_H
#define TNFSBROWSER_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QToolButton>
#include <QIcon>
#include "tnfsclient.h"

class TnfsBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit TnfsBrowser(QWidget *parent = nullptr, const QString &initialUrl = "");
    ~TnfsBrowser();
    QString getSelectedUrl() const;

private slots:
    void onConnect();
    void onItemDoubleClicked(QListWidgetItem *item);
    void onBackClicked();
    void onClearHistory();
    void onCancelClicked();
    void onSortClicked();
    void onMoreClicked();

private:
    TnfsClient *client;
    QComboBox *hostCombo;
    QListWidget *fileList;
    QLabel *statusLabel;
    QPushButton *btnClear;
    QPushButton *btnCancel;
    QPushButton *btnMore;
    QToolButton *btnSort;

    bool m_sortAscending; // TRACK STA
    QString currentPath;
    QString selectedUrl;
    bool m_isFirstBatch;

    void refreshList();     // Starts the listing (Resets state)
    void loadNextBatch();   // Fetches the actual data
};

#endif // TNFSBROWSER_H
