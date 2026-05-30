/*
 * networkbrowser.h
 * Universal Network Browser for AspeQt-2k26
 */
#ifndef NETWORKBROWSER_H
#define NETWORKBROWSER_H

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
#include <QRadioButton>
#include "inetworkclient.h"

class NetworkBrowser : public QDialog
{
    Q_OBJECT

public:
    explicit NetworkBrowser(QWidget *parent = nullptr, const QString &initialUrl = "");
    ~NetworkBrowser();
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
    void onFetchFinished();
    void onLoginClicked();
    void onProtocolChanged(); // <--- NEW: Listens for radio button toggles

private:
    INetworkClient *m_client;
    QComboBox *hostCombo;
    QListWidget *fileList;
    QLabel *statusLabel;

    // UI Elements
    QRadioButton *radioTnfs;
    QRadioButton *radioFtp;
    QPushButton *btnConnect;
    QPushButton *btnClear;
    QPushButton *btnLogin;
    QPushButton *btnCancel;
    QPushButton *btnMore;
    QToolButton *btnSort;
    QProgressBar *progressBar;

    // Async Connection Handling
    QFutureWatcher<bool> *connectWatcher;

    // Async Directory Fetching
    QFutureWatcher<QList<INetworkClient::DirectoryEntry>> *fetchWatcher;

    bool m_sortAscending;
    QString currentPath;
    QString selectedUrl;
    QString m_activeHost;
    QString m_activeProtocol;
    bool m_isFirstBatch;

    // Credential Storage
    QString m_savedUser;
    QString m_savedPass;

    void refreshList();
    void loadNextBatch();
    QIcon getIcon(const QString &name);
};

#endif // NETWORKBROWSER_H
