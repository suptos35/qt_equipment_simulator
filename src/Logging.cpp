/**
 * @file Logging.cpp
 * @brief Implementation of unified diagnostic logging and queued GUI dispatch.
 */

#include "Logging.h"
#include "LoggerThread.h"

#include <QDateTime>
#include <QPointer>
#include <QMetaObject>
#include <iostream>
#include <mutex>
#include <string>

Q_LOGGING_CATEGORY(logEquip,  "equipment.core")
Q_LOGGING_CATEGORY(logThread, "equipment.threading")
Q_LOGGING_CATEGORY(logGui,    "equipment.gui")
Q_LOGGING_CATEGORY(logSecs,   "equipment.secsgem")

namespace {

std::mutex s_receiverMutex;
QPointer<QObject> s_guiReceiver = nullptr;
std::string s_slotName;

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

    QString formattedMessage = QString("[%1] [%2] [%3] %4")
                                   .arg(timestamp, levelStr, category, msg);

    // 1. Output to standard error
    std::cerr << formattedMessage.toStdString() << "\n";
    std::cerr.flush();

    // 2. Thread-safely push to asynchronous disk logger
    LoggerThread::instance().push(formattedMessage.toStdString());

    // 3. Thread-safely ferry log to GUI via Qt::QueuedConnection
    {
        std::lock_guard<std::mutex> lock(s_receiverMutex);
        if (s_guiReceiver && !s_slotName.empty()) {
            QMetaObject::invokeMethod(
                s_guiReceiver.data(),
                s_slotName.c_str(),
                Qt::QueuedConnection,
                Q_ARG(int, static_cast<int>(type)),
                Q_ARG(QString, formattedMessage)
            );
        }
    }

    if (type == QtFatalMsg) {
        abort();
    }
}

} // namespace

namespace Logging {

void installMessageHandler() {
    LoggerThread::instance().start();
    qInstallMessageHandler(customMessageHandler);
}

void registerGuiReceiver(QObject *receiver, const char *slotName) {
    std::lock_guard<std::mutex> lock(s_receiverMutex);
    s_guiReceiver = receiver;
    s_slotName = slotName ? slotName : "";
}

void unregisterGuiReceiver() {
    std::lock_guard<std::mutex> lock(s_receiverMutex);
    s_guiReceiver = nullptr;
    s_slotName.clear();
}

} // namespace Logging
