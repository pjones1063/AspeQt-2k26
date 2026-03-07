#ifndef ASPEQTCLIENTDEVICE_H
#define ASPEQTCLIENTDEVICE_H

#include "sioworker.h"
#include <QString>
#include <QByteArray>
#include <QHash>

// Forward declarations
class AspeQtSettings;
extern AspeQtSettings* aspeqtSettings;

class AspeqtClientDevice : public SioDevice
{
    Q_OBJECT

public:
    AspeqtClientDevice(SioWorker *worker);
    ~AspeqtClientDevice() override = default;

    void handleCommand(quint8 command, quint16 aux) override;
    QString deviceName() const { return "AspeQt Client"; }

public slots:
    void gotNewSlot(int slot);
    void fileMounted(bool mounted);

signals:
    void findNewSlot(int startFrom, bool createOne);
    void mountFile(int no, const QString &fileName);
    void toggleAutoCommit(int no, bool st);
    void bootExe(const QString &fileName);
    void bootCas(const QString &fileName);
    void togglePrinterServer(bool enable);

private:
    // Encapsulated SIO State Variables
    QString m_imageFileName;
    QHash<quint8, QString> m_files;
    QString m_fFilter;
    QString m_fPath;
    int m_lastSlotNo;
};

#endif // ASPEQTCLIENTDEVICE_H
