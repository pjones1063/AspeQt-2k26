/*
 * tnfsbrowser.h
 */
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
#include <QProgressBar>
#include <QFutureWatcher>
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
    void onConnectionFinished();
    void onItemDoubleClicked(QListWidgetItem *item);
    void onBackClicked();
    void onClearHistory();
    void onCancelClicked();
    void onSortClicked();
    void onMoreClicked();
    void onFetchFinished(); // --- [NEW] Slot for background fetching ---

private:
    TnfsClient *client;
    QComboBox *hostCombo;
    QListWidget *fileList;
    QLabel *statusLabel;

    // UI Elements
    QPushButton *btnConnect;
    QPushButton *btnClear;
    QPushButton *btnCancel;
    QPushButton *btnMore;
    QToolButton *btnSort;
    QProgressBar *progressBar;

    // Async Connection Handling
    QFutureWatcher<bool> *connectWatcher;

    // --- [NEW] Async Directory Fetching ---
    QFutureWatcher<QList<TnfsClient::DirectoryEntry>> *fetchWatcher;

    bool m_sortAscending;
    QString currentPath;
    QString selectedUrl;
    QString m_activeHost;
    bool m_isFirstBatch;

    void refreshList();
    void loadNextBatch();
    QIcon getIcon(const QString &name);
};

#endif // TNFSBROWSER_H
