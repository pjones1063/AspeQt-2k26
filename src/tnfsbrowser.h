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
    void onConnectionFinished(); // New slot for async result
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

    // UI Elements
    QPushButton *btnConnect; // Made member to disable during connect
    QPushButton *btnClear;
    QPushButton *btnCancel;
    QPushButton *btnMore;
    QToolButton *btnSort;
    QProgressBar *progressBar; // The "Activity Bar"

    // Async Connection Handling
    QFutureWatcher<bool> *connectWatcher;

    bool m_sortAscending;
    QString currentPath;
    QString selectedUrl;
    QString m_activeHost;
    bool m_isFirstBatch;

    void refreshList();
    void loadNextBatch();

    // New Helper for Cross-Platform Icons
    QIcon getIcon(const QString &name);
};

#endif // TNFSBROWSER_H
