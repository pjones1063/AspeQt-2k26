#ifndef TNFSBROWSER_H
#define TNFSBROWSER_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
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


private:
    TnfsClient *client;
    QComboBox *hostCombo;
    QListWidget *fileList;
    QLabel *statusLabel;
    QPushButton *btnClear;
    QPushButton *btnCancel;

    QString currentPath;
    QString selectedUrl;

    void refreshList();
};

#endif // TNFSBROWSER_H
