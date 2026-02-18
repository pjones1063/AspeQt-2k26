#ifndef PHONEDIRECTORY_H
#define PHONEDIRECTORY_H

#include <QDialog>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include "bbsdata.h"

class PhoneDirectory : public QDialog {
    Q_OBJECT
public:
    explicit PhoneDirectory(QWidget *parent = nullptr);
    void loadFromFile(const QString &path);
    BbsEntry getSelectedEntry();

private slots:
    void onSearch(const QString &text);
    void onDialClicked();
    void onEditClicked();
    void onSaveClicked();
    void onAddClicked();
    void onDeleteClicked();

private:
    QLineEdit *m_searchEdit;
    QTreeWidget *m_tree;
    QPushButton *m_dialBtn;
    QPushButton *m_editBtn;
    QPushButton *m_saveBtn;

    QString m_filePath;
    QList<BbsEntry> m_entries;

    void parseXml();
    void saveToFile(); // New helper
    void refreshList(const QString &filter = "");
};

#endif // PHONEDIRECTORY_H
