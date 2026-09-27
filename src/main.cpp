/**
 * @file main.cpp
 * @brief Application entry point.
 */

#include "Logging.h"
#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[]) {
    // 1. Install custom message handler before QApplication construction
    Logging::installMessageHandler();

    qInfo(logEquip) << "Application started. Initializing Qt framework...";

    // 2. Construct QApplication
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("QtEquipmentSimulator"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));

    // 3. Create and show primary window
    MainWindow mainWindow;
    mainWindow.show();

    int exitCode = app.exec();

    qInfo(logEquip) << "Application exiting with code:" << exitCode;
    return exitCode;
}
