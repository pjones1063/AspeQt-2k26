#ifndef SIOPACKETDIALOG_H
#define SIOPACKETDIALOG_H

#include <QDialog>
#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QList>
#include <QString>
#include <QByteArray>
#include <QTextEdit>
#include <QSplitter>
#include <QLabel>
#include <QTimer>
#include <QSpinBox>
#include <QLineEdit>
#include <QComboBox>

// The Core Data Structure
struct SioPacket {
    qint64 timestamp;
    QString direction;
    QString command;
    QString aux1;
    QString aux2;
    int dataLength;
    QString payloadHex;
    QString checksumStatus;
    QByteArray rawData;
};

// The Table Model
class SioPacketModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit SioPacketModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void addPacket(const SioPacket &packet);
    void clear();
    const QList<SioPacket>& getPackets() const { return m_packets; }

private:
    QList<SioPacket> m_packets;
    const int MAX_PACKETS = 10000; // Ring Buffer Limit
};

// The UI Dialog
class SioPacketDialog : public QDialog {
    Q_OBJECT
public:
    explicit SioPacketDialog(QWidget *parent = nullptr);
    void appendPacket(const QString &dir, const QByteArray &data, qint64 elapsedMs);

signals:
    void dialogClosed();
    void injectPacketRequested(const QByteArray &data);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onSaveClicked();
    void onInjectClicked();
    void onRowSelected(const QModelIndex &current, const QModelIndex &previous);
    void toggleInspector();
    void onRecordToggled(bool checked);
    void onPlayToggled(bool checked);
    void onPlaybackStep();
    void onFilterChanged(); // New Filter Slot

private:
    QTableView *tableView;
    SioPacketModel *model;
    QSortFilterProxyModel *proxyModel; // Wireshark Filter Engine

    // Filter UI
    QLineEdit *txtFilter;
    QComboBox *cmbFilterColumn;

    // Playback & Record Deck
    QPushButton *btnRecord;
    QPushButton *btnPlay;
    QSpinBox *spinPlayDelay;
    bool m_isRecording = true;
    bool m_isPlaying = false;
    QTimer *m_playbackTimer;

    // UI Controls
    QPushButton *btnClear;
    QPushButton *btnSave;
    QPushButton *btnInject;
    QTextEdit *txtDetails;

    // Collapsible Pane Variables
    QSplitter *splitter;
    QWidget *inspectorContainer;
    QPushButton *btnToggleInspector;
    QList<int> savedSplitterSizes;
};

#endif // SIOPACKETDIALOG_H
