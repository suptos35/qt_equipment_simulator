/**
 * @file Logging.h
 * @brief Diagnostic logging subsystem, categories, and thread-safe dispatchers.
 */

#pragma once

#include <QLoggingCategory>
#include <QString>
#include <QObject>

Q_DECLARE_LOGGING_CATEGORY(logEquip)
Q_DECLARE_LOGGING_CATEGORY(logThread)
Q_DECLARE_LOGGING_CATEGORY(logGui)
Q_DECLARE_LOGGING_CATEGORY(logSecs)

namespace Logging {

/**
 * @brief Installs the custom Qt message handler and initializes LoggerThread.
 */
void installMessageHandler();

/**
 * @brief Registers a target QObject to receive formatted log messages on the GUI thread.
 * @param receiver Pointer to receiver QObject (usually MainWindow).
 * @param slotName Slot signature taking (int msgType, const QString &formattedMsg).
 */
void registerGuiReceiver(QObject *receiver, const char *slotName);

/**
 * @brief Unregisters GUI receiver during shutdown.
 */
void unregisterGuiReceiver();

} // namespace Logging
