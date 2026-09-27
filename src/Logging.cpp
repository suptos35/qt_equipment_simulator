/**
 * @file Logging.cpp
 * @brief Implementation of diagnostic logging and message handler.
 */

#include "Logging.h"

#include <QDateTime>
#include <iostream>
#include <cstdio>

Q_LOGGING_CATEGORY(logEquip,  "equipment.core")
Q_LOGGING_CATEGORY(logThread, "equipment.threading")
Q_LOGGING_CATEGORY(logGui,    "equipment.gui")
Q_LOGGING_CATEGORY(logSecs,   "equipment.secsgem")

namespace {

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    const char *levelStr = "DEBUG";
    switch (type) {
    case QtDebugMsg:
        levelStr = "DEBUG";
        break;
    case QtInfoMsg:
        levelStr = "INFO";
        break;
    case QtWarningMsg:
        levelStr = "WARN";
        break;
    case QtCriticalMsg:
        levelStr = "CRIT";
        break;
    case QtFatalMsg:
        levelStr = "FATAL";
        break;
    }

    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    const char *category = context.category ? context.category : "default";

    QString formattedMessage = QString("[%1] [%2] [%3] %4\n")
                                   .arg(timestamp, levelStr, category, msg);

    // Print to stderr
    std::cerr << formattedMessage.toStdString();
    std::cerr.flush();

    if (type == QtFatalMsg) {
        abort();
    }
}

} // namespace

namespace Logging {

void installMessageHandler() {
    qInstallMessageHandler(customMessageHandler);
}

} // namespace Logging
