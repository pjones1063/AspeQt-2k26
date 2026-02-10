/*
 * bootoptionsdialog.cpp
 */

#include "bootoptionsdialog.h"
#include "ui_bootoptionsdialog.h"
#include "mainwindow.h"

#include <QTranslator>
#include <QDir>
#include <QtDebug> // Added for debug output if needed

extern QString g_aspeQtAppPath;
extern bool g_disablePicoHiSpeed;

QString selectedDOS, bootDir;

BootOptionsDialog::BootOptionsDialog(const QString& bootFolderPath, QWidget *parent) :
    QDialog(parent),
    bootFolderPath_(bootFolderPath),
    m_ui(new Ui::BootOptionsDialog)
{
    Qt::WindowFlags flags = windowFlags();
    flags = flags & (~Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);

    m_ui->setupUi(this);

    connect(m_ui->myPicoDOS, SIGNAL(toggled(bool)), this, SLOT(picoDOSToggled()));
}

BootOptionsDialog::~BootOptionsDialog()
{
    delete m_ui;
}

void BootOptionsDialog::changeEvent(QEvent *e)
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

void BootOptionsDialog::accept()
{
    QDir dir;
    QFile file;
    QStringList filters;
    QStringList allFiles;
    QString fileName;

    // 1. Determine which Internal Resource to use
    if(m_ui->atariDOS->isChecked()) selectedDOS = ":/boot_templates/$bootata";
    if(m_ui->myDOS->isChecked()) selectedDOS = ":/boot_templates/$bootmyd";
    if(m_ui->dosXL->isChecked()) selectedDOS = ":/boot_templates/$bootdxl";
    if(m_ui->smartDOS->isChecked()) selectedDOS = ":/boot_templates/$bootsma";
    if(m_ui->spartaDOS->isChecked()) selectedDOS = ":/boot_templates/$bootspa";
    if(m_ui->myPicoDOS->isChecked()) {
        selectedDOS = ":/boot_templates/$bootpic";
        g_disablePicoHiSpeed = m_ui->disablePicoHiSpeed->isChecked();
    }

    // FIX: Do NOT prepend g_aspeQtAppPath.
    // The resource path (e.g. ":/boot_templates/...") is absolute in the Qt Resource System.
    bootDir = selectedDOS;

    // 2. First delete existing boot files in the physical Folder Image
    dir.setPath(bootFolderPath_);
    filters << "*dos.sys" << "dup.sys" << "dosxl.sys"
            << "autorun.sys" << "ramdisk.com" << "menu.com"
            << "startup.exc" << "x*.dos" << "startup.bat" << "$*.bin";

    allFiles = dir.entryList(filters, QDir::Files);
    foreach(fileName, allFiles) {
        file.remove(bootFolderPath_ + "/" + fileName);
    }

    // 3. Now copy new boot files from the internal Resource to the physical folder
    // FIX: Use the resource path directly
    dir.setPath(selectedDOS);

    // Grab all files in that resource folder
    allFiles = dir.entryList(QDir::NoDotAndDotDot | QDir::Files);

    foreach(fileName, allFiles) {
        // Copy: Source (Resource) -> Dest (Physical Folder)
        if (!file.copy(dir.path() + "/" + fileName, bootFolderPath_ + "/" + fileName)) {
            qWarning() << "Failed to copy boot file:" << fileName;
        }
    }

    QDialog::accept();
}

void BootOptionsDialog::picoDOSToggled()
{
    bool enable = m_ui->myPicoDOS->isChecked();
    m_ui->disablePicoHiSpeed->setEnabled(enable);
}
