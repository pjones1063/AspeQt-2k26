/*
 * optionsdialog.cpp
 */

#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "aspeqtsettings.h"
#include <QtSerialPort/QtSerialPort>
#include <QTranslator>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QCheckBox>

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::OptionsDialog)
{
    Qt::WindowFlags flags = windowFlags();
    flags = flags & (~Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);

    m_ui->setupUi(this);

    // --- SIGNAL/SLOT ADJUSTMENT FOR VALIDATION ---
    disconnect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(OptionsDialog_accepted()));
    // ---------------------------------------------

    m_ui->treeWidget->expandAll();
    itemStandard = m_ui->treeWidget->topLevelItem(0)->child(0);
    itemAtariSio = m_ui->treeWidget->topLevelItem(0)->child(1);
    itemEmulation = m_ui->treeWidget->topLevelItem(1);
    itemModemBridge = m_ui->treeWidget->topLevelItem(2);
    itemPrinter = m_ui->treeWidget->topLevelItem(3);
    itemI18n = m_ui->treeWidget->topLevelItem(4);
    itemWebUi = m_ui->treeWidget->topLevelItem(5);

#ifndef Q_OS_LINUX
    m_ui->treeWidget->topLevelItem(0)->removeChild(itemAtariSio);
#endif

    // --- VIRTUAL PRINTER SETTINGS ---
    m_ui->printerAutoPopBox->setChecked(aspeqtSettings->printerAutoPop());
    m_ui->printerFeedCombo->setCurrentIndex(aspeqtSettings->printerFeedMode());
    m_ui->printerStyleCombo->setCurrentIndex(aspeqtSettings->printerStyle());

    // MUTE SIGNALS: Prevent the UI from crashing while we load data
    m_ui->serialPortComboBox->blockSignals(true);
    m_ui->modemPortComboBox->blockSignals(true);

    // --- Standard Serial Port Combo Setup ---
    m_ui->serialPortComboBox->clear();
    const QList<QSerialPortInfo>& infos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : infos) {
        m_ui->serialPortComboBox->addItem(info.portName(), info.systemLocation());
    }

    m_ui->serialPortComboBox->setCurrentText(aspeqtSettings->serialPortName());
    if(0 != m_ui->serialPortComboBox->currentText().compare(aspeqtSettings->serialPortName(), Qt::CaseInsensitive)) {
        m_ui->serialPortComboBox->setEditable(true);
        m_ui->serialPortComboBox->addItem(aspeqtSettings->serialPortName());
        m_ui->serialPortComboBox->setCurrentText(aspeqtSettings->serialPortName());
    } else {
        m_ui->serialPortComboBox->addItem(tr("Custom"));
    }

    // --- Modem Bridge Port Combo Setup ---
    m_ui->modemPortComboBox->clear();
    for (const QSerialPortInfo &info : infos) {
        m_ui->modemPortComboBox->addItem(info.portName(), info.systemLocation());
    }

    m_ui->modemPortComboBox->setCurrentText(aspeqtSettings->modemBridgePortName());
    if(0 != m_ui->modemPortComboBox->currentText().compare(aspeqtSettings->modemBridgePortName(), Qt::CaseInsensitive)) {
        m_ui->modemPortComboBox->setEditable(true);
        m_ui->modemPortComboBox->addItem(aspeqtSettings->modemBridgePortName());
        m_ui->modemPortComboBox->setCurrentText(aspeqtSettings->modemBridgePortName());
    } else {
        m_ui->modemPortComboBox->addItem(tr("Custom"));
    }

    // UNMUTE SIGNALS
    m_ui->serialPortComboBox->blockSignals(false);
    m_ui->modemPortComboBox->blockSignals(false);

    // ==========================================
    // 1. General & UI Settings
    // ==========================================
    m_ui->minimizeToTrayBox->setChecked(aspeqtSettings->minimizeToTray());
    m_ui->saveWinPosBox->setChecked(aspeqtSettings->saveWindowsPos());
    m_ui->saveDiskVisBox->setChecked(aspeqtSettings->saveDiskVis());
    m_ui->filterUscore->setChecked(aspeqtSettings->filterUnderscore());
    m_ui->capitalLettersPCLINK->setChecked(aspeqtSettings->capitalLettersInPCLINK());
    m_ui->useLargerFont->setChecked(aspeqtSettings->useLargeFont());
    m_ui->enableShade->setChecked(aspeqtSettings->enableShade());

    // ==========================================
    // 2. Standard Serial Port Backend
    // ==========================================
    m_ui->serialPortHandshakeCombo->setCurrentIndex(aspeqtSettings->serialPortHandshakingMethod());
    m_ui->mDirectUart->setChecked(aspeqtSettings->serialPortHardwareUart());
    on_mDirectUart_toggled(aspeqtSettings->serialPortHardwareUart()); // Trigger gray-outs
    m_ui->serialPortWriteDelayCombo->setCurrentIndex(aspeqtSettings->serialPortWriteDelay());
    m_ui->serialPortBaudCombo->setCurrentIndex(aspeqtSettings->serialPortMaximumSpeed());
    m_ui->serialPortUseDivisorsBox->setChecked(aspeqtSettings->serialPortUsePokeyDivisors());
    m_ui->serialPortDivisorEdit->setValue(aspeqtSettings->serialPortPokeyDivisor());
    m_ui->serialPortCompErrDelayBox->setValue(aspeqtSettings->serialPortCompErrDelay());

    // ==========================================
    // 3. Emulation & Virtual Devices
    // ==========================================
    m_ui->atariSioDriverNameEdit->setText(aspeqtSettings->atariSioDriverName());
    m_ui->atariSioHandshakingMethodCombo->setCurrentIndex(aspeqtSettings->atariSioHandshakingMethod());
    m_ui->emulationHighSpeedExeLoaderBox->setChecked(aspeqtSettings->useHighSpeedExeLoader());
    m_ui->emulationUseCustomCasBaudBox->setChecked(aspeqtSettings->useCustomCasBaud());
    m_ui->emulationCustomCasBaudSpin->setValue(aspeqtSettings->customCasBaud());

    // ==========================================
    // 4. Modem Bridge, RDevice & BBS Listener
    // ==========================================
    m_ui->modemEnableBox->setChecked(aspeqtSettings->isModemBridgeEnabled());
    m_ui->modemPortComboBox->setCurrentText(aspeqtSettings->modemBridgePortName());
    m_ui->modemBaudComboBox->setCurrentText(QString::number(aspeqtSettings->modemBridgeBaudRate()));
    m_ui->modemFlowControlBox->setChecked(aspeqtSettings->modemBridgeFlowControl());
    m_ui->modemSshBox->setChecked(aspeqtSettings->modemBridgeSshEnabled());
    m_ui->modemLocalEchoBox->setChecked(aspeqtSettings->modemBridgeLocalEcho());
    m_ui->modemPhonebookPathEdit->setText(aspeqtSettings->modemBridgePhonebookPath());
    m_ui->modemInvertCtsBox->setChecked(aspeqtSettings->invertCtsLogic());
    m_ui->sbStreamGuardDelay->setValue(aspeqtSettings->streamGuardDelay());

    m_ui->modemRBox->setChecked(aspeqtSettings->isRDeviceEnabled());
    on_modemEnableBox_toggled(aspeqtSettings->isModemBridgeEnabled()); // Trigger initial states

    // [NEW] BBS Listener Setup
    m_ui->enableBbsPort->setChecked(aspeqtSettings->bbsListenerEnabled());
    m_ui->bbsPortBox->setMinimum(1024);
    m_ui->bbsPortBox->setMaximum(65535);
    m_ui->bbsPortBox->setValue(aspeqtSettings->modemListenPort());
    m_ui->bbsPortBox->setEnabled(aspeqtSettings->bbsListenerEnabled());
    connect(m_ui->enableBbsPort, &QCheckBox::toggled, m_ui->bbsPortBox, &QSpinBox::setEnabled);

    // ==========================================
    // 5. TNFS & Web UI
    // ==========================================
    m_ui->tnfsAutoConnectBox->setChecked(aspeqtSettings->restoreTnfsLocation());
    m_ui->eolPostCheckBox->setChecked(aspeqtSettings->translateEolOnPost());
    m_ui->eolGetCheckBox->setChecked(aspeqtSettings->translateEolOnGet());
    m_ui->cbEnableWebUi->setChecked(aspeqtSettings->isWebUiEnabled());
    m_ui->sbHttpPort->setValue(aspeqtSettings->webUiPort());
    m_ui->sbWsPort->setValue(aspeqtSettings->webUiWsPort());

    // --- Backend Tree Mapping ---
    switch (aspeqtSettings->backend()) {
    case SERIAL_BACKEND_STANDARD:
        itemStandard->setCheckState(0, Qt::Checked);
        itemAtariSio->setCheckState(0, Qt::Unchecked);
        m_ui->treeWidget->setCurrentItem(itemStandard);
        break;
    case SERIAL_BACKEND_SIO_DRIVER:
        itemStandard->setCheckState(0, Qt::Unchecked);
        itemAtariSio->setCheckState(0, Qt::Checked);
        m_ui->treeWidget->setCurrentItem(itemAtariSio);
        break;
    }
    m_ui->serialPortBox->setCheckState(itemStandard->checkState(0));
    m_ui->atariSioBox->setCheckState(itemAtariSio->checkState(0));

    // --- Translations ---
    QTranslator local_translator;
    m_ui->i18nLanguageCombo->clear();
    m_ui->i18nLanguageCombo->addItem(tr("Automatic"), "auto");
    if (aspeqtSettings->i18nLanguage().compare("auto") == 0)
        m_ui->i18nLanguageCombo->setCurrentIndex(0);
    m_ui->i18nLanguageCombo->addItem(QT_TR_NOOP("English"), "en");
    if (aspeqtSettings->i18nLanguage().compare("en") == 0)
        m_ui->i18nLanguageCombo->setCurrentIndex(1);
    QDir dir(":/translations/i18n/");
    QStringList filters;
    filters << "aspeqt_*.qm";
    dir.setNameFilters(filters);
    for (int i = 0; i < dir.entryList().size(); ++i) {
        (void)local_translator.load(":/translations/i18n/" + dir.entryList()[i]);
        m_ui->i18nLanguageCombo->addItem(local_translator.translate("OptionsDialog", "English"), dir.entryList()[i].replace("aspeqt_", "").replace(".qm", ""));
        if (dir.entryList()[i].replace("aspeqt_", "").replace(".qm", "").compare(aspeqtSettings->i18nLanguage()) == 0) {
            m_ui->i18nLanguageCombo->setCurrentIndex(i+2);
        }
    }

    bool software_handshake = (aspeqtSettings->serialPortHandshakingMethod()==HANDSHAKE_SOFTWARE);
    m_ui->serialPortWriteDelayLabel->setVisible(software_handshake);
    m_ui->serialPortWriteDelayCombo->setVisible(software_handshake);
    m_ui->serialPortBaudLabel->setVisible(!software_handshake);
    m_ui->serialPortBaudCombo->setVisible(!software_handshake);
    m_ui->serialPortUseDivisorsBox->setVisible(!software_handshake);
    m_ui->serialPortDivisorLabel->setVisible(!software_handshake);
    m_ui->serialPortDivisorEdit->setVisible(!software_handshake);
    m_ui->serialPortCompErrDelayLabel->setVisible(!software_handshake);
    m_ui->serialPortCompErrDelayBox->setVisible(!software_handshake);

    if((SERIAL_BACKEND_STANDARD == aspeqtSettings->backend()) && software_handshake) {
        m_ui->emulationHighSpeedExeLoaderBox->setVisible(false);
    } else {
        m_ui->emulationHighSpeedExeLoaderBox->setVisible(true);
    }

    if (aspeqtSettings->isRDeviceEnabled()) {
        on_modemRBox_toggled(true);
    }
}

OptionsDialog::~OptionsDialog()
{
    delete m_ui;
}

void OptionsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void OptionsDialog::on_serialPortComboBox_currentIndexChanged(int index)
{
    bool isCustomPath = !m_ui->serialPortComboBox->itemData(index).isValid();
    m_ui->serialPortComboBox->setEditable(isCustomPath);
}

void OptionsDialog::on_serialPortHandshakeCombo_currentIndexChanged(int index)
{
    bool software_handshake = (index==HANDSHAKE_SOFTWARE);
    bool hwUart = m_ui->mDirectUart->isChecked();

    m_ui->serialPortWriteDelayLabel->setVisible(software_handshake);
    m_ui->serialPortWriteDelayCombo->setVisible(software_handshake);
    m_ui->serialPortBaudLabel->setVisible(!software_handshake);
    m_ui->serialPortBaudCombo->setVisible(!software_handshake);
    m_ui->serialPortUseDivisorsBox->setVisible(!software_handshake);
    m_ui->serialPortDivisorLabel->setVisible(!software_handshake);
    m_ui->serialPortDivisorEdit->setVisible(!software_handshake);
    m_ui->serialPortCompErrDelayLabel->setVisible(!software_handshake);
    m_ui->serialPortCompErrDelayBox->setVisible(!software_handshake);

    m_ui->serialPortCompErrDelayBox->setEnabled(!hwUart);
    m_ui->serialPortCompErrDelayLabel->setEnabled(!hwUart);
    m_ui->serialPortWriteDelayCombo->setEnabled(!hwUart);
    m_ui->serialPortWriteDelayLabel->setEnabled(!hwUart);

    if(itemStandard->checkState((0)) == Qt::Checked) {
        m_ui->emulationHighSpeedExeLoaderBox->setVisible(!software_handshake);
    }
}

void OptionsDialog::on_serialPortUseDivisorsBox_toggled(bool checked)
{
    m_ui->serialPortBaudLabel->setEnabled(!checked);
    m_ui->serialPortBaudCombo->setEnabled(!checked);
    m_ui->serialPortDivisorLabel->setEnabled(checked);
    m_ui->serialPortDivisorEdit->setEnabled(checked);
}

void OptionsDialog::on_treeWidget_itemClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (item->checkState(0) == Qt::Checked) {
        if (item == itemStandard) {
            m_ui->emulationHighSpeedExeLoaderBox->setVisible(HANDSHAKE_SOFTWARE != m_ui->serialPortHandshakeCombo->currentIndex());
        } else {
            itemStandard->setCheckState(0, Qt::Unchecked);
        }
        if (item == itemAtariSio) {
            m_ui->emulationHighSpeedExeLoaderBox->setVisible(true);
        } else {
            itemAtariSio->setCheckState(0, Qt::Unchecked);
        }
    } else if ((itemStandard->checkState(0) == Qt::Unchecked) && (itemAtariSio->checkState(0) == Qt::Unchecked)) {
        item->setCheckState(0, Qt::Checked);
    }
    m_ui->serialPortBox->setCheckState(itemStandard->checkState(0));
    m_ui->atariSioBox->setCheckState(itemAtariSio->checkState(0));
}

void OptionsDialog::on_treeWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/)
{
    if (current == itemStandard) {
        m_ui->stackedWidget->setCurrentIndex(0);
    } else if (current == itemAtariSio) {
        m_ui->stackedWidget->setCurrentIndex(1);
    } else if (current == itemEmulation) {
        m_ui->stackedWidget->setCurrentIndex(2);
    } else if (current == itemModemBridge) {
        m_ui->stackedWidget->setCurrentIndex(3);
    } else if (current == itemPrinter) {          // <--- NEW
        m_ui->stackedWidget->setCurrentIndex(4);  // <--- NEW (Assuming it is the 5th page you created)
    } else if (current == itemI18n) {
        m_ui->stackedWidget->setCurrentIndex(5);  // Shifted
    } else if (current == itemWebUi) {
        m_ui->stackedWidget->setCurrentIndex(6);  // Shifted
    }
}


void OptionsDialog::on_modemEnableBox_toggled(bool checked)
{
    if (checked) {
        m_ui->modemRBox->setChecked(false);
    }
    m_ui->modemPortComboBox->setEnabled(checked);
    m_ui->modemBaudComboBox->setEnabled(checked);
    m_ui->modemFlowControlBox->setEnabled(checked);
    m_ui->modemSshBox->setEnabled(checked);
    m_ui->modemLocalEchoBox->setEnabled(checked);

    bool rDeviceEnabled = m_ui->modemRBox->isChecked();
    bool phonebookEnabled = checked || rDeviceEnabled;

    m_ui->modemPhonebookPathEdit->setEnabled(phonebookEnabled);
    m_ui->modemPhonebookBrowseBtn->setEnabled(phonebookEnabled);
    m_ui->modemPhonebookNewBtn->setEnabled(phonebookEnabled);
}

void OptionsDialog::on_modemRBox_toggled(bool checked)
{
    if (checked) {
        if (m_ui->modemEnableBox->isChecked()) {
            m_ui->modemEnableBox->blockSignals(true);
            m_ui->modemInvertCtsBox->setEnabled(true);
            m_ui->modemEnableBox->setChecked(false);
            m_ui->modemEnableBox->blockSignals(false);
            on_modemEnableBox_toggled(false);
        }
        m_ui->modemPortComboBox->setEnabled(false);
        m_ui->modemBaudComboBox->setEnabled(false);
        m_ui->modemFlowControlBox->setEnabled(false);
        m_ui->modemSshBox->setEnabled(false);
        m_ui->modemLocalEchoBox->setEnabled(false);
        m_ui->modemPhonebookPathEdit->setEnabled(true);
        m_ui->modemPhonebookBrowseBtn->setEnabled(true);
        m_ui->modemPhonebookNewBtn->setEnabled(true);
        m_ui->modemInvertCtsBox->setEnabled(true);
    } else {
        bool bridgeEnabled = m_ui->modemEnableBox->isChecked();
        m_ui->modemPhonebookPathEdit->setEnabled(bridgeEnabled);
        m_ui->modemPhonebookBrowseBtn->setEnabled(bridgeEnabled);
        m_ui->modemPhonebookNewBtn->setEnabled(bridgeEnabled);
        m_ui->modemInvertCtsBox->setEnabled(false);
    }

    bool hwUart = m_ui->mDirectUart->isChecked();
    m_ui->sbStreamGuardDelay->setEnabled(checked && !hwUart);
    m_ui->label_streamGuardDelay->setEnabled(checked && !hwUart);
}

void OptionsDialog::on_modemPortComboBox_currentIndexChanged(int index)
{
    bool isCustomPath = !m_ui->modemPortComboBox->itemData(index).isValid();
    m_ui->modemPortComboBox->setEditable(isCustomPath);
}

void OptionsDialog::on_mDirectUart_toggled(bool checked)
{
    m_ui->serialPortCompErrDelayBox->setEnabled(!checked);
    m_ui->serialPortCompErrDelayLabel->setEnabled(!checked);
    m_ui->serialPortWriteDelayCombo->setEnabled(!checked);
    m_ui->serialPortWriteDelayLabel->setEnabled(!checked);

    bool rDevice = m_ui->modemRBox->isChecked();
    m_ui->sbStreamGuardDelay->setEnabled(rDevice && !checked);
    m_ui->label_streamGuardDelay->setEnabled(rDevice && !checked);

    if (checked && m_ui->serialPortHandshakeCombo->currentIndex() == HANDSHAKE_SOFTWARE) {
        m_ui->serialPortHandshakeCombo->setCurrentIndex(HANDSHAKE_CTS);
    }
}

void OptionsDialog::OptionsDialog_accepted()
{
    // --- VALIDATION ---
    bool modemEnabled = m_ui->modemEnableBox->isChecked();
    QString sioPort = m_ui->serialPortComboBox->currentText();
    QString modemPort = m_ui->modemPortComboBox->currentText();
    bool standardBackend = (itemStandard->checkState(0) == Qt::Checked);

    if (modemEnabled && standardBackend && (sioPort == modemPort)) {
        QMessageBox::critical(this, tr("Port Conflict"),
                              tr("You cannot use the same Serial Port (%1) for both\nSIO Emulation and the Modem Bridge.\n\nPlease select a different port for the Modem.").arg(sioPort));
        return;
    }

    if (m_ui->cbEnableWebUi->isChecked()) {
        if (m_ui->sbHttpPort->value() == m_ui->sbWsPort->value()) {
            QMessageBox::critical(this, tr("Port Conflict"), tr("The HTTP Dashboard Port and WebSocket Bridge Port cannot be the same.\n\nPlease assign different ports."));
            return;
        }
    }

    // --- SAVING ---
    aspeqtSettings->setSerialPortName(m_ui->serialPortComboBox->currentText());
    aspeqtSettings->setSerialPortHandshakingMethod(m_ui->serialPortHandshakeCombo->currentIndex());
    aspeqtSettings->setSerialPortHardwareUart(m_ui->mDirectUart->isChecked());
    aspeqtSettings->setSerialPortWriteDelay(m_ui->serialPortWriteDelayCombo->currentIndex());
    aspeqtSettings->setSerialPortCompErrDelay(m_ui->serialPortCompErrDelayBox->value());
    aspeqtSettings->setStreamGuardDelay(m_ui->sbStreamGuardDelay->value());
    aspeqtSettings->setSerialPortMaximumSpeed(m_ui->serialPortBaudCombo->currentIndex());
    aspeqtSettings->setSerialPortUsePokeyDivisors(m_ui->serialPortUseDivisorsBox->isChecked());
    aspeqtSettings->setSerialPortPokeyDivisor(m_ui->serialPortDivisorEdit->value());
    aspeqtSettings->setAtariSioDriverName(m_ui->atariSioDriverNameEdit->text());
    aspeqtSettings->setAtariSioHandshakingMethod(m_ui->atariSioHandshakingMethodCombo->currentIndex());
    aspeqtSettings->setUseHighSpeedExeLoader(m_ui->emulationHighSpeedExeLoaderBox->isChecked());
    aspeqtSettings->setUseCustomCasBaud(m_ui->emulationUseCustomCasBaudBox->isChecked());
    aspeqtSettings->setCustomCasBaud(m_ui->emulationCustomCasBaudSpin->value());
    aspeqtSettings->setMinimizeToTray(m_ui->minimizeToTrayBox->isChecked());
    aspeqtSettings->setsaveWindowsPos(m_ui->saveWinPosBox->isChecked());
    aspeqtSettings->setsaveDiskVis(m_ui->saveDiskVisBox->isChecked());
    aspeqtSettings->setfilterUnderscore(m_ui->filterUscore->isChecked());
    aspeqtSettings->setCapitalLettersInPCLINK(m_ui->capitalLettersPCLINK->isChecked());
    aspeqtSettings->setUseLargeFont(m_ui->useLargerFont->isChecked());
    aspeqtSettings->setEnableShade(m_ui->enableShade->isChecked());
    aspeqtSettings->setRestoreTnfsLocation(m_ui->tnfsAutoConnectBox->isChecked());
    aspeqtSettings->setTranslateEolOnPost(m_ui->eolPostCheckBox->isChecked());
    aspeqtSettings->setTranslateEolOnGet(m_ui->eolGetCheckBox->isChecked());

    // Modem Bridge Settings
    aspeqtSettings->setModemBridgeEnabled(m_ui->modemEnableBox->isChecked());
    aspeqtSettings->setModemBridgePortName(m_ui->modemPortComboBox->currentText());
    aspeqtSettings->setModemBridgeBaudRate(m_ui->modemBaudComboBox->currentText().toInt());
    aspeqtSettings->setModemBridgeFlowControl(m_ui->modemFlowControlBox->isChecked());
    aspeqtSettings->setModemBridgeSshEnabled(m_ui->modemSshBox->isChecked());
    aspeqtSettings->setModemBridgeLocalEcho(m_ui->modemLocalEchoBox->isChecked());
    aspeqtSettings->setModemBridgePhonebookPath(m_ui->modemPhonebookPathEdit->text());
    aspeqtSettings->setEnableRDevice(m_ui->modemRBox->isChecked());
    aspeqtSettings->setInvertCtsLogic(m_ui->modemInvertCtsBox->isChecked());

    // [NEW] BBS Listener Settings
    aspeqtSettings->setBbsListenerEnabled(m_ui->enableBbsPort->isChecked());
    aspeqtSettings->setModemListenPort(m_ui->bbsPortBox->value());

    // --- NEW: SAVE VIRTUAL PRINTER SETTINGS ---
    aspeqtSettings->setPrinterAutoPop(m_ui->printerAutoPopBox->isChecked());
    aspeqtSettings->setPrinterFeedMode(m_ui->printerFeedCombo->currentIndex());
    aspeqtSettings->setPrinterStyle(m_ui->printerStyleCombo->currentIndex());

    // Web UI
    aspeqtSettings->setWebUiEnabled(m_ui->cbEnableWebUi->isChecked());
    aspeqtSettings->setWebUiPort(m_ui->sbHttpPort->value());
    aspeqtSettings->setWebUiWsPort(m_ui->sbWsPort->value());

    int backend = SERIAL_BACKEND_STANDARD;
    if (itemAtariSio->checkState(0) == Qt::Checked) {
        backend = SERIAL_BACKEND_SIO_DRIVER;
    }
    aspeqtSettings->setBackend(backend);
    aspeqtSettings->setI18nLanguage(m_ui->i18nLanguageCombo->itemData(m_ui->i18nLanguageCombo->currentIndex()).toString());

    accept();
}

void OptionsDialog::on_useEmulationCustomCasBaudBox_toggled(bool checked)
{
    m_ui->emulationCustomCasBaudSpin->setEnabled(checked);
}

void OptionsDialog::on_modemPhonebookBrowseBtn_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Dial Directory"), QDir::homePath(), tr("XML Files (*.xml);;All Files (*)"));
    if (!fileName.isEmpty()) {
        m_ui->modemPhonebookPathEdit->setText(fileName);
    }
}

void OptionsDialog::on_modemPhonebookNewBtn_clicked()
{
#ifdef Q_OS_MAC
    QString fileName = QFileDialog::getOpenFileName(this, tr("Create New Dial Directory"), QDir::homePath(), tr("XML Files (*.xml);;All Files (*)"));
#else
    QString fileName = QFileDialog::getSaveFileName(this, tr("Create New Dial Directory"), QDir::homePath() + "/phonebook.xml", tr("XML Files (*.xml);;All Files (*)"));
#endif

    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".xml", Qt::CaseInsensitive)) {
            fileName += ".xml";
        }
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Phonebook>\n    \n    \n</Phonebook>\n";
            file.close();
            m_ui->modemPhonebookPathEdit->setText(fileName);
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Could not create the phonebook file."));
        }
    }
}
