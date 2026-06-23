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
    m_ui->marginTop->setValue(aspeqtSettings->printerMarginTop());
    m_ui->marginLeft->setValue(aspeqtSettings->printerMarginLeft());
    m_ui->marginLength->setValue(aspeqtSettings->printerMarginLength());

    // MUTE SIGNALS: Prevent the UI from crashing while we load data
    m_ui->serialPortComboBox->blockSignals(true);

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
    m_ui->serialPortComboBox->blockSignals(false);


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
    // 4. Modem Bridge & 4-Port Matrix
    // ==========================================

    // Map UI Elements to Arrays
    m_modemLocalEchoBox[0] = m_ui->modemLocalEchoBox_R1; m_modemLocalEchoBox[1] = m_ui->modemLocalEchoBox_R2;
    m_modemLocalEchoBox[2] = m_ui->modemLocalEchoBox_R3; m_modemLocalEchoBox[3] = m_ui->modemLocalEchoBox_R4;

    m_enableBbsPort[0] = m_ui->enableBbsPort_R1; m_enableBbsPort[1] = m_ui->enableBbsPort_R2;
    m_enableBbsPort[2] = m_ui->enableBbsPort_R3; m_enableBbsPort[3] = m_ui->enableBbsPort_R4;

    m_bbsPortBox[0] = m_ui->bbsPortBox_R1; m_bbsPortBox[1] = m_ui->bbsPortBox_R2;
    m_bbsPortBox[2] = m_ui->bbsPortBox_R3; m_bbsPortBox[3] = m_ui->bbsPortBox_R4;

    m_modemPortComboBox[0] = m_ui->modemPortComboBox_R1; m_modemPortComboBox[1] = m_ui->modemPortComboBox_R2;
    m_modemPortComboBox[2] = m_ui->modemPortComboBox_R3; m_modemPortComboBox[3] = m_ui->modemPortComboBox_R4;

    m_modemBaudComboBox[0] = m_ui->modemBaudComboBox_R1; m_modemBaudComboBox[1] = m_ui->modemBaudComboBox_R2;
    m_modemBaudComboBox[2] = m_ui->modemBaudComboBox_R3; m_modemBaudComboBox[3] = m_ui->modemBaudComboBox_R4;

    m_modemFlowControlBox[0] = m_ui->modemFlowControlBox_R1; m_modemFlowControlBox[1] = m_ui->modemFlowControlBox_R2;
    m_modemFlowControlBox[2] = m_ui->modemFlowControlBox_R3; m_modemFlowControlBox[3] = m_ui->modemFlowControlBox_R4;

    m_modemInvertCtsBox[0] = m_ui->modemInvertCtsBox_R1; m_modemInvertCtsBox[1] = m_ui->modemInvertCtsBox_R2;
    m_modemInvertCtsBox[2] = m_ui->modemInvertCtsBox_R3; m_modemInvertCtsBox[3] = m_ui->modemInvertCtsBox_R4;

    m_sbStreamGuardDelay[0] = m_ui->sbStreamGuardDelay_R1; m_sbStreamGuardDelay[1] = m_ui->sbStreamGuardDelay_R2;
    m_sbStreamGuardDelay[2] = m_ui->sbStreamGuardDelay_R3; m_sbStreamGuardDelay[3] = m_ui->sbStreamGuardDelay_R4;

    // Load Global Settings
    m_ui->radioVirtual850->setChecked(aspeqtSettings->modemTransportMode() == 0);
    m_ui->radioHardwareBridge->setChecked(aspeqtSettings->modemTransportMode() == 1);
    m_ui->modemPhonebookPathEdit->setText(aspeqtSettings->modemBridgePhonebookPath());

    for(int i = 0; i < 4; i++) {
        m_modemLocalEchoBox[i]->setChecked(aspeqtSettings->modemBridgeLocalEcho(i));
        m_enableBbsPort[i]->setChecked(aspeqtSettings->bbsListenerEnabled(i));

        m_bbsPortBox[i]->setMinimum(1024);
        m_bbsPortBox[i]->setMaximum(65535);
        m_bbsPortBox[i]->setValue(aspeqtSettings->modemListenPort(i));
        m_bbsPortBox[i]->setEnabled(aspeqtSettings->bbsListenerEnabled(i));

        m_modemPortComboBox[i]->blockSignals(true);
        m_modemPortComboBox[i]->clear();
        m_modemPortComboBox[i]->addItem(tr("None")); // <-- [FIX] Allow users to leave extra ports disconnected
        for (const QSerialPortInfo &info : infos) {
            m_modemPortComboBox[i]->addItem(info.portName(), info.systemLocation());
        }
        m_modemPortComboBox[i]->setCurrentText(aspeqtSettings->modemBridgePortName(i));

        // Custom COM port logic for the matrix
        if(0 != m_modemPortComboBox[i]->currentText().compare(aspeqtSettings->modemBridgePortName(i), Qt::CaseInsensitive)) {
            m_modemPortComboBox[i]->setEditable(true);
            m_modemPortComboBox[i]->addItem(aspeqtSettings->modemBridgePortName(i));
            m_modemPortComboBox[i]->setCurrentText(aspeqtSettings->modemBridgePortName(i));
        } else {
            m_modemPortComboBox[i]->addItem(tr("Custom"));
        }
        m_modemPortComboBox[i]->blockSignals(false);

        // Lambda to handle custom typing in any of the 4 combo boxes
        connect(m_modemPortComboBox[i], QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int index){
            bool isCustomPath = !m_modemPortComboBox[i]->itemData(index).isValid();
            m_modemPortComboBox[i]->setEditable(isCustomPath);
        });

        m_modemBaudComboBox[i]->setCurrentText(QString::number(aspeqtSettings->modemBridgeBaudRate(i)));
        m_modemFlowControlBox[i]->setChecked(aspeqtSettings->modemBridgeFlowControl(i));
        m_modemInvertCtsBox[i]->setChecked(aspeqtSettings->invertCtsLogic(i));
        m_sbStreamGuardDelay[i]->setValue(aspeqtSettings->streamGuardDelay(i));

        // Wiring up local UI rules
        connect(m_enableBbsPort[i], &QCheckBox::toggled, m_bbsPortBox[i], &QSpinBox::setEnabled);

        if (i > 0) {   // hiding for now.
            m_modemInvertCtsBox[i]->hide();
            m_sbStreamGuardDelay[i]->hide();
            m_ui->label_streamGuardDelay_R2->hide();
            m_ui->label_streamGuardDelay_R3->hide();
            m_ui->label_streamGuardDelay_R4->hide();
        }
    }

    // Wiring up global UI rules
    connect(m_ui->radioVirtual850, &QRadioButton::toggled, this, &OptionsDialog::on_transportModeChanged);
    connect(m_ui->radioHardwareBridge, &QRadioButton::toggled, this, &OptionsDialog::on_transportModeChanged);

    // ==========================================
    // 5. TNFS & Web UI
    // ==========================================
    m_ui->tnfsAutoConnectBox->setChecked(aspeqtSettings->restoreTnfsLocation());
    m_ui->eolPostCheckBox->setChecked(aspeqtSettings->translateEolOnPost());
    m_ui->eolGetCheckBox->setChecked(aspeqtSettings->translateEolOnGet());
    m_ui->cbEnableWebUi->setChecked(aspeqtSettings->isWebUiEnabled());
    m_ui->sbHttpPort->setValue(aspeqtSettings->webUiPort());
    m_ui->sbWsPort->setValue(aspeqtSettings->webUiWsPort());

    m_ui->spinVoiceVolume->setValue(aspeqtSettings->voiceVolume());
    m_ui->spinVoiceRate->setValue(aspeqtSettings->voiceRate());
    m_ui->spinVoicePitch->setValue(aspeqtSettings->voicePitch());

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

    m_ui->spinVoiceVolume->setRange(0, 10);
    m_ui->spinVoiceRate->setRange(0, 10);
    m_ui->spinVoicePitch->setRange(0, 10);

    m_ui->spinVoiceVolume->setValue(aspeqtSettings->voiceVolume());
    m_ui->spinVoiceRate->setValue(aspeqtSettings->voiceRate());
    m_ui->spinVoicePitch->setValue(aspeqtSettings->voicePitch());

    // Initialize state
    on_transportModeChanged();
    on_mDirectUart_toggled(aspeqtSettings->serialPortHardwareUart());

    m_ui->nodeTabWidget->setCurrentIndex(0);
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
    } else if (current == itemPrinter) {
        m_ui->stackedWidget->setCurrentIndex(4);
    } else if (current == itemI18n) {
        m_ui->stackedWidget->setCurrentIndex(5);
    } else if (current == itemWebUi) {
        m_ui->stackedWidget->setCurrentIndex(6);
    }
}

void OptionsDialog::on_mDirectUart_toggled(bool checked)
{
    m_ui->serialPortCompErrDelayBox->setEnabled(!checked);
    m_ui->serialPortCompErrDelayLabel->setEnabled(!checked);
    m_ui->serialPortWriteDelayCombo->setEnabled(!checked);
    m_ui->serialPortWriteDelayLabel->setEnabled(!checked);

    if (checked && m_ui->serialPortHandshakeCombo->currentIndex() == HANDSHAKE_SOFTWARE) {
        m_ui->serialPortHandshakeCombo->setCurrentIndex(HANDSHAKE_CTS);
    }

    // Update the matrix stream guard delays
    on_transportModeChanged();
}

void OptionsDialog::on_transportModeChanged()
{
    bool isBridge = m_ui->radioHardwareBridge->isChecked();
    bool hwUart = m_ui->mDirectUart->isChecked();

    for(int i = 0; i < 4; i++) {
        m_modemLocalEchoBox[i]->setEnabled(true);
        m_enableBbsPort[i]->setEnabled(true);
        m_modemPortComboBox[i]->setEnabled(isBridge );
        m_modemBaudComboBox[i]->setEnabled(isBridge );
        m_modemFlowControlBox[i]->setEnabled(isBridge );
        m_modemInvertCtsBox[i]->setEnabled(isBridge );
        m_sbStreamGuardDelay[i]->setEnabled(!hwUart );
    }
}

void OptionsDialog::OptionsDialog_accepted()
{
    // --- VALIDATION ---
    bool isBridge = m_ui->radioHardwareBridge->isChecked();
    QString sioPort = m_ui->serialPortComboBox->currentText();
    bool standardBackend = (itemStandard->checkState(0) == Qt::Checked);

    if (isBridge) {
        for (int i = 0; i < 4; i++) {
            QString port1 = m_modemPortComboBox[i]->currentText();
            if (port1 == tr("None") || port1.isEmpty()) continue;

            // 1. Check for SIO collision (Don't let ModemBridge steal the SIO port)
            if (standardBackend && (sioPort == port1)) {
                QMessageBox::critical(this, tr("Port Conflict"),
                                      tr("You cannot use the same Serial Port (%1) for both\nSIO Emulation and Modem Bridge R%2.\n\nPlease select a different port.").arg(sioPort).arg(i+1));
                return;
            }

            // 2. Check for Matrix COM Port collisions (Don't let R1 clash with R2, etc.)
            for (int j = i + 1; j < 4; j++) {
                if (port1 == m_modemPortComboBox[j]->currentText()) {
                    QMessageBox::critical(this, tr("Matrix Port Conflict"),
                                          tr("You cannot assign the same physical Serial Port (%1) to both R%2 and R%3.\n\nPlease select different ports, or set unused ports to 'None'.").arg(port1).arg(i+1).arg(j+1));
                    return;
                }
            }
        }
    }


    if (m_ui->cbEnableWebUi->isChecked()) {
        if (m_ui->sbHttpPort->value() == m_ui->sbWsPort->value()) {
            QMessageBox::critical(this, tr("Port Conflict"), tr("The HTTP Dashboard Port and WebSocket Bridge Port cannot be the same.\n\nPlease assign different ports."));
            return;
        }
    }



    // 3. Check for BBS Listener TCP Port collisions
    for (int i = 0; i < 4; i++) {
        if (!m_enableBbsPort[i]->isChecked()) continue; // Only care if listener is ON

        int tcpPort1 = m_bbsPortBox[i]->value();

        // Compare against the other R: ports
        for (int j = i + 1; j < 4; j++) {
            if (!m_enableBbsPort[j]->isChecked()) continue;

            if (tcpPort1 == m_bbsPortBox[j]->value()) {
                QMessageBox::critical(this, tr("TCP Port Conflict"),
                                      tr("You cannot assign the same BBS Listener Port (%1) to both R%2 and R%3.\n\nPlease assign different ports.").arg(tcpPort1).arg(i+1).arg(j+1));
                return;
            }
        }

        // Compare against the Web UI ports
        if (m_ui->cbEnableWebUi->isChecked()) {
            if (tcpPort1 == m_ui->sbHttpPort->value() || tcpPort1 == m_ui->sbWsPort->value()) {
                QMessageBox::critical(this, tr("TCP Port Conflict"),
                                      tr("BBS Listener Port %1 on R%2 conflicts with the Web UI ports.\n\nPlease assign different ports.").arg(tcpPort1).arg(i+1));
                return;
            }
        }
    }

    // --- SAVING CORE SETTINGS ---
    aspeqtSettings->setSerialPortName(m_ui->serialPortComboBox->currentText());
    aspeqtSettings->setSerialPortHandshakingMethod(m_ui->serialPortHandshakeCombo->currentIndex());
    aspeqtSettings->setSerialPortHardwareUart(m_ui->mDirectUart->isChecked());
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
    aspeqtSettings->setTranslateEolOnPost(m_ui->eolPostCheckBox->isChecked());
    aspeqtSettings->setTranslateEolOnGet(m_ui->eolGetCheckBox->isChecked());

    // --- SAVING 4-PORT MATRIX ---
    aspeqtSettings->setModemTransportMode(isBridge ? 1 : 0);
    aspeqtSettings->setModemBridgePhonebookPath(m_ui->modemPhonebookPathEdit->text());

    for(int i = 0; i < 4; i++) {
        aspeqtSettings->setModemBridgeLocalEcho(i, m_modemLocalEchoBox[i]->isChecked());
        aspeqtSettings->setBbsListenerEnabled(i, m_enableBbsPort[i]->isChecked());
        aspeqtSettings->setModemListenPort(i, m_bbsPortBox[i]->value());
        aspeqtSettings->setModemBridgePortName(i, m_modemPortComboBox[i]->currentText());
        aspeqtSettings->setModemBridgeBaudRate(i, m_modemBaudComboBox[i]->currentText().toInt());
        aspeqtSettings->setModemBridgeFlowControl(i, m_modemFlowControlBox[i]->isChecked());
        aspeqtSettings->setInvertCtsLogic(i, m_modemInvertCtsBox[i]->isChecked());
        aspeqtSettings->setStreamGuardDelay(i, m_sbStreamGuardDelay[i]->value());
    }

    // --- SAVING PRINTER & WEB UI ---
    aspeqtSettings->setPrinterAutoPop(m_ui->printerAutoPopBox->isChecked());
    aspeqtSettings->setPrinterFeedMode(m_ui->printerFeedCombo->currentIndex());
    aspeqtSettings->setPrinterStyle(m_ui->printerStyleCombo->currentIndex());
    aspeqtSettings->setPrinterMarginTop(m_ui->marginTop->value());
    aspeqtSettings->setPrinterMarginLeft(m_ui->marginLeft->value());
    aspeqtSettings->setPrinterMarginLength(m_ui->marginLength->value());
    aspeqtSettings->setWebUiEnabled(m_ui->cbEnableWebUi->isChecked());
    aspeqtSettings->setWebUiPort(m_ui->sbHttpPort->value());
    aspeqtSettings->setWebUiWsPort(m_ui->sbWsPort->value());

    aspeqtSettings->setVoiceVolume(m_ui->spinVoiceVolume->value());
    aspeqtSettings->setVoiceRate(m_ui->spinVoiceRate->value());
    aspeqtSettings->setVoicePitch(m_ui->spinVoicePitch->value());

    int backend = SERIAL_BACKEND_STANDARD;
    if (itemAtariSio->checkState(0) == Qt::Checked) {
        backend = SERIAL_BACKEND_SIO_DRIVER;
    }
    aspeqtSettings->setBackend(backend);
    aspeqtSettings->setI18nLanguage(m_ui->i18nLanguageCombo->itemData(m_ui->i18nLanguageCombo->currentIndex()).toString());
    aspeqtSettings->setPrinterClearRequested(true);

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

    QString fileName = QFileDialog::getSaveFileName(this, tr("Create New Dial Directory"), QDir::homePath() + "/phonebook.xml", tr("XML Files (*.xml);;All Files (*)"));

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
