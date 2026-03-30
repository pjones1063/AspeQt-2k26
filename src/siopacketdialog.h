#ifndef SIOPACKETDIALOG_H
#define SIOPACKETDIALOG_H

#include <QDialog>
#include <QAbstractTableModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QList>
#include <QString>
#include <QByteArray>

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

private:
    QTableView *tableView;
    SioPacketModel *model;
    QPushButton *btnClear;
    QPushButton *btnSave;
    QPushButton *btnInject;

};

#endif // SIOPACKETDIALOG_H
