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

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::OptionsDialog)
{
    Qt::WindowFlags flags = windowFlags();
    flags = flags & (~Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);

    m_ui->setupUi(this);

    // --- SIGNAL/SLOT ADJUSTMENT FOR VALIDATION ---
    // We disconnect the default "accepted" signal so the window doesn't close immediately.
    // Instead, we route the OK button to our save/validate slot first.
    disconnect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(OptionsDialog_accepted()));
    // ---------------------------------------------

    m_ui->treeWidget->expandAll();
    itemStandard = m_ui->treeWidget->topLevelItem(0)->child(0);
    itemAtariSio = m_ui->treeWidget->topLevelItem(0)->child(1);
    itemEmulation = m_ui->treeWidget->topLevelItem(1);
    itemModemBridge = m_ui->treeWidget->topLevelItem(2); // New Modem Item
    itemI18n = m_ui->treeWidget->topLevelItem(3);

#ifndef Q_OS_LINUX
    m_ui->treeWidget->topLevelItem(0)->removeChild(itemAtariSio);
#endif

    /* Retrieve application settings */

    // --- Standard Serial Port Combo Setup ---
    m_ui->serialPortComboBox->clear();
    const QList<QSerialPortInfo>& infos = QSerialPortInfo::availablePorts();

    m_ui->serialPortComboBox->addItem("ttyAspeQT","ttyAspeQT");

    for (const QSerialPortInfo &info : infos)
    {
        m_ui->serialPortComboBox->addItem(info.portName(), info.systemLocation());
    }

    // --- Modem Bridge Port Combo Setup ---
    m_ui->modemPortComboBox->clear();
    for (const QSerialPortInfo &info : infos) {
        m_ui->modemPortComboBox->addItem(info.portName(), info.systemLocation());
    }

    // Set current SIO Port
    m_ui->serialPortComboBox->setCurrentText(aspeqtSettings->serialPortName());
    if(0 != m_ui->serialPortComboBox->currentText().compare(aspeqtSettings->serialPortName(),Qt::CaseInsensitive))
    {
        m_ui->serialPortComboBox->setEditable(true);
        m_ui->serialPortComboBox->addItem(aspeqtSettings->serialPortName());
        m_ui->serialPortComboBox->setCurrentText(aspeqtSettings->serialPortName());
    }
    else
    {
        m_ui->serialPortComboBox->addItem(tr("Custom"));
    }

    // Set Modem Bridge Defaults
    m_ui->modemEnableBox->setChecked(aspeqtSettings->isModemBridgeEnabled());
    m_ui->modemPortComboBox->setCurrentText(aspeqtSettings->modemBridgePortName());
    m_ui->modemBaudComboBox->setCurrentText(QString::number(aspeqtSettings->modemBridgeBaudRate()));
    m_ui->modemFlowControlBox->setChecked(aspeqtSettings->modemBridgeFlowControl());
    m_ui->modemSshBox->setChecked(aspeqtSettings->modemBridgeSshEnabled());
    m_ui->modemLocalEchoBox->setChecked(aspeqtSettings->modemBridgeLocalEcho());
    m_ui->modemRBox->setChecked(aspeqtSettings->enableRDevice());

    // Trigger initial state for Modem UI
    on_modemEnableBox_toggled(aspeqtSettings->isModemBridgeEnabled());


    m_ui->serialPortHandshakeCombo->setCurrentIndex(aspeqtSettings->serialPortHandshakingMethod());
    m_ui->serialPortFallingEdge->setChecked(aspeqtSettings->serialPortTriggerOnFallingEdge());
    m_ui->serialPortWriteDelayCombo->setCurrentIndex(aspeqtSettings->serialPortWriteDelay());
    m_ui->serialPortBaudCombo->setCurrentIndex(aspeqtSettings->serialPortMaximumSpeed());
    m_ui->serialPortUseDivisorsBox->setChecked(aspeqtSettings->serialPortUsePokeyDivisors());
    m_ui->serialPortDivisorEdit->setValue(aspeqtSettings->serialPortPokeyDivisor());
    m_ui->serialPortCompErrDelayBox->setValue(aspeqtSettings->serialPortCompErrDelay());
    m_ui->atariSioDriverNameEdit->setText(aspeqtSettings->atariSioDriverName());
    m_ui->atariSioHandshakingMethodCombo->setCurrentIndex(aspeqtSettings->atariSioHandshakingMethod());
    m_ui->emulationHighSpeedExeLoaderBox->setChecked(aspeqtSettings->useHighSpeedExeLoader());
    m_ui->emulationUseCustomCasBaudBox->setChecked(aspeqtSettings->useCustomCasBaud());
    m_ui->emulationCustomCasBaudSpin->setValue(aspeqtSettings->customCasBaud());
    m_ui->minimizeToTrayBox->setChecked(aspeqtSettings->minimizeToTray());
    m_ui->saveWinPosBox->setChecked(aspeqtSettings->saveWindowsPos());
    m_ui->saveDiskVisBox->setChecked(aspeqtSettings->saveDiskVis());
    m_ui->filterUscore->setChecked(aspeqtSettings->filterUnderscore());
    m_ui->capitalLettersPCLINK->setChecked(aspeqtSettings->capitalLettersInPCLINK());
    m_ui->useLargerFont->setChecked(aspeqtSettings->useLargeFont());
    m_ui->enableShade->setChecked(aspeqtSettings->enableShade());
    m_ui->tnfsAutoConnectBox->setChecked(aspeqtSettings->restoreTnfsLocation());
    m_ui->modemPhonebookPathEdit->setText(aspeqtSettings->modemBridgePhonebookPath());

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

    /* list available translations */
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
        local_translator.load(":/translations/i18n/" + dir.entryList()[i]);
        m_ui->i18nLanguageCombo->addItem(local_translator.translate("OptionsDialog", "English"), dir.entryList()[i].replace("aspeqt_", "").replace(".qm", ""));
        if (dir.entryList()[i].replace("aspeqt_", "").replace(".qm", "").compare(aspeqtSettings->i18nLanguage()) == 0) {
            m_ui->i18nLanguageCombo->setCurrentIndex(i+2);
        }
    }

    bool no_handshake = (aspeqtSettings->serialPortHandshakingMethod()==HANDSHAKE_NO_HANDSHAKE);
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
    m_ui->eolPostCheckBox->setChecked(aspeqtSettings->translateEolOnPost());
    m_ui->eolGetCheckBox->setChecked(aspeqtSettings->translateEolOnGet());

#ifdef Q_OS_WIN
    m_ui->serialPortFallingEdge->setVisible(!no_handshake && !software_handshake);
#else
    m_ui->serialPortFallingEdge->setVisible(false);
#endif

    if((SERIAL_BACKEND_STANDARD == aspeqtSettings->backend()) && software_handshake)
    {
        m_ui->emulationHighSpeedExeLoaderBox->setVisible(false);
    }
    else
    {
        m_ui->emulationHighSpeedExeLoaderBox->setVisible(true);
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
    bool no_handshake = (index==HANDSHAKE_NO_HANDSHAKE);
    bool software_handshake = (index==HANDSHAKE_SOFTWARE);
    m_ui->serialPortWriteDelayLabel->setVisible(software_handshake);
    m_ui->serialPortWriteDelayCombo->setVisible(software_handshake);
    m_ui->serialPortBaudLabel->setVisible(!software_handshake);
    m_ui->serialPortBaudCombo->setVisible(!software_handshake);
    m_ui->serialPortUseDivisorsBox->setVisible(!software_handshake);
    m_ui->serialPortDivisorLabel->setVisible(!software_handshake);
    m_ui->serialPortDivisorEdit->setVisible(!software_handshake);
    m_ui->serialPortCompErrDelayLabel->setVisible(!software_handshake);
    m_ui->serialPortCompErrDelayBox->setVisible(!software_handshake);
#ifdef Q_OS_WIN
    m_ui->serialPortFallingEdge->setVisible(!no_handshake && !software_handshake);
#endif
    if(itemStandard->checkState((0)) == Qt::Checked)
    {
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
    if (item->checkState(0) == Qt::Checked)
    {
        if (item == itemStandard)
        {
            m_ui->emulationHighSpeedExeLoaderBox->setVisible(HANDSHAKE_SOFTWARE != m_ui->serialPortHandshakeCombo->currentIndex());
        }
        else
        {
            itemStandard->setCheckState(0, Qt::Unchecked);
        }
        if (item == itemAtariSio)
        {
            m_ui->emulationHighSpeedExeLoaderBox->setVisible(true);
        }
        else
        {
            itemAtariSio->setCheckState(0, Qt::Unchecked);
        }
    }
    else if ((itemStandard->checkState(0) == Qt::Unchecked) &&
             (itemAtariSio->checkState(0) == Qt::Unchecked))
    {
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
        m_ui->stackedWidget->setCurrentIndex(3); // PAGE_MODEM
    } else if (current == itemI18n) {
        m_ui->stackedWidget->setCurrentIndex(4);
    }
}


void OptionsDialog::on_modemEnableBox_toggled(bool checked)
{
     if (checked) {
        m_ui->modemRBox->setChecked(false);
     }
    // 1. Manage UI State for Modem Bridge
    m_ui->modemPortComboBox->setEnabled(checked);
    m_ui->modemBaudComboBox->setEnabled(checked);
    m_ui->modemFlowControlBox->setEnabled(checked);
    m_ui->modemSshBox->setEnabled(checked);
    m_ui->modemLocalEchoBox->setEnabled(checked);
    m_ui->modemPhonebookPathEdit->setEnabled(checked);
    m_ui->modemPhonebookBrowseBtn->setEnabled(checked);    
}


void OptionsDialog::on_modemRBox_toggled(bool checked)
{
    if (checked) {
        // 1. If R: Device is ON, turn Modem Bridge OFF
        if (m_ui->modemEnableBox->isChecked()) {
            m_ui->modemEnableBox->blockSignals(true);
            m_ui->modemEnableBox->setChecked(false);
            m_ui->modemEnableBox->blockSignals(false);
        }

        // 2. Strict Visual Feedback: Grey out Modem Bridge options
        m_ui->modemPortComboBox->setEnabled(false);
        m_ui->modemBaudComboBox->setEnabled(false);
        m_ui->modemFlowControlBox->setEnabled(false);
        m_ui->modemSshBox->setEnabled(false);
        m_ui->modemLocalEchoBox->setEnabled(false);
        m_ui->modemPhonebookPathEdit->setEnabled(false);
        m_ui->modemPhonebookBrowseBtn->setEnabled(false);
    }
}

void OptionsDialog::OptionsDialog_accepted()
{
    // --- VALIDATION: Check for Port Conflict ---
    bool modemEnabled = m_ui->modemEnableBox->isChecked();
    QString sioPort = m_ui->serialPortComboBox->currentText();
    QString modemPort = m_ui->modemPortComboBox->currentText();
    bool standardBackend = (itemStandard->checkState(0) == Qt::Checked);

    if (modemEnabled && standardBackend && (sioPort == modemPort)) {
        QMessageBox::critical(this, tr("Port Conflict"),
                              tr("You cannot use the same Serial Port (%1) for both\nSIO Emulation and the Modem Bridge.\n\nPlease select a different port for the Modem.").arg(sioPort));
        return; // Do not accept(), keep dialog open
    }
    // -------------------------------------------

    aspeqtSettings->setSerialPortName(m_ui->serialPortComboBox->currentText());
    aspeqtSettings->setSerialPortHandshakingMethod(m_ui->serialPortHandshakeCombo->currentIndex());
    aspeqtSettings->setSerialPortTriggerOnFallingEdge(m_ui->serialPortFallingEdge->isChecked());
    aspeqtSettings->setSerialPortWriteDelay(m_ui->serialPortWriteDelayCombo->currentIndex());
    aspeqtSettings->setSerialPortCompErrDelay(m_ui->serialPortCompErrDelayBox->value());
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
    aspeqtSettings->setUseLargeFont(m_ui->useLargerFont->isChecked());
    aspeqtSettings->setTranslateEolOnPost(m_ui->eolPostCheckBox->isChecked());
    aspeqtSettings->setTranslateEolOnGet(m_ui->eolGetCheckBox->isChecked());

    // Save Modem Bridge Settings
    aspeqtSettings->setModemBridgeEnabled(m_ui->modemEnableBox->isChecked());
    aspeqtSettings->setModemBridgePortName(m_ui->modemPortComboBox->currentText());
    aspeqtSettings->setModemBridgeBaudRate(m_ui->modemBaudComboBox->currentText().toInt());
    aspeqtSettings->setModemBridgeFlowControl(m_ui->modemFlowControlBox->isChecked());
    aspeqtSettings->setModemBridgeSshEnabled(m_ui->modemSshBox->isChecked());
    aspeqtSettings->setModemBridgeLocalEcho(m_ui->modemLocalEchoBox->isChecked());
    aspeqtSettings->setModemBridgePhonebookPath(m_ui->modemPhonebookPathEdit->text());
    aspeqtSettings->setEnableRDevice(m_ui->modemRBox->isChecked());

    int backend = SERIAL_BACKEND_STANDARD;
    if (itemAtariSio->checkState(0) == Qt::Checked)
    {
        backend = SERIAL_BACKEND_SIO_DRIVER;
    }

    aspeqtSettings->setBackend(backend);

    aspeqtSettings->setI18nLanguage(m_ui->i18nLanguageCombo->itemData(m_ui->i18nLanguageCombo->currentIndex()).toString());

    // Only accept() if validation passed (which it did if we are here)
    accept();
}

void OptionsDialog::on_useEmulationCustomCasBaudBox_toggled(bool checked)
{
    m_ui->emulationCustomCasBaudSpin->setEnabled(checked);
}

void OptionsDialog::on_modemPhonebookBrowseBtn_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Select Dial Directory"),
                                                    QDir::homePath(),
                                                    tr("XML Files (*.xml);;All Files (*)"));

    if (!fileName.isEmpty()) {
        m_ui->modemPhonebookPathEdit->setText(fileName);
    }
}
