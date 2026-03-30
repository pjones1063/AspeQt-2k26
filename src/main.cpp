#include <QApplication>
#include <QLockFile>
#include <QDir>
#include <QMessageBox>
#include <QStringList>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // 1. Initialize the Qt Application first
    QApplication a(argc, argv);

    // 2. Parse command-line arguments for the power-user override
    QStringList args = QCoreApplication::arguments();
    bool allowMultiple = args.contains("--multi") || args.contains("-m");
    QLockFile lockFile(QDir::tempPath() + "/aspeqt_2k26_instance.lock");

    // 3. Only attempt to lock if the user DID NOT pass the override flag
    if (!allowMultiple) {
        // Try to lock it (wait up to 100 milliseconds)
        if (!lockFile.tryLock(100)) {
            QMessageBox::critical(nullptr,
                                  "AspeQt-2k26",
                                  "Another instance of AspeQt is already running.\n\n"
                                  "Please close the existing instance before starting a new one to prevent serial port conflicts.\n\n"
                                  "(Power users: launch with --multi or -m to bypass this check)");
            return 1; // Exit immediately!
        }
    }
    // ------------------------------------

    // 4. Normal App Startup continues...
    MainWindow w;
    w.show();

    // The application runs here.
    // If we locked the file, it stays locked until a.exec() finishes and main() returns.
    return a.exec();
}
