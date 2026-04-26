#ifndef PHONEDIRECTORY_H
#define PHONEDIRECTORY_H

#include <QDialog>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCloseEvent>
#include "bbsdata.h"

class PhoneDirectory : public QDialog {
    Q_OBJECT
public:
    explicit PhoneDirectory(QWidget *parent = nullptr);
    void loadFromFile(const QString &path);
    BbsEntry getSelectedEntry();

protected:
    // Intercept window close events (X button)
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onSearch(const QString &text);
    void onDialClicked();
    void onEditClicked();
    void onSaveClicked();
    void onAddClicked();
    void onDeleteClicked();
    void onCloseClicked(); // Custom slot for the Close button

private:
    QLineEdit *m_searchEdit;
    QTreeWidget *m_tree;
    QPushButton *m_dialBtn;
    QPushButton *m_editBtn;
    QPushButton *m_saveBtn;

    QString m_filePath;
    QList<BbsEntry> m_entries;
    bool m_isDirty; // Tracks unsaved changes

    void parseXml();
    void saveToFile();
    void refreshList(const QString &filter = "");
    bool runEditDialog(BbsEntry &entry);

    // Helper to handle the "Save Changes?" prompt
    bool checkUnsavedChanges(const QString &actionName);
};

#endif // PHONEDIRECTORY_H
