/**
 * @file Logging.h
 * @brief Diagnostic logging subsystem and message handler definitions.
 *
 * Defines subsystem logging categories and installs a custom Qt message handler
 * that routes formatted diagnostic outputs with timestamp, category, and severity.
 */

#pragma once

#include <QLoggingCategory>
#include <QString>

Q_DECLARE_LOGGING_CATEGORY(logEquip)
Q_DECLARE_LOGGING_CATEGORY(logThread)
Q_DECLARE_LOGGING_CATEGORY(logGui)
Q_DECLARE_LOGGING_CATEGORY(logSecs)

namespace Logging {

/**
 * @brief Installs the custom Qt message handler.
 *
 * Must be invoked prior to constructing QApplication in main().
 */
void installMessageHandler();

} // namespace Logging
