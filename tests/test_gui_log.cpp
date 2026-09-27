/**
 * @file test_gui_log.cpp
 * @brief Test verifying thread-safe queued log delivery into the UI QListWidget.
 */

#include <QObject>
#include <QTest>
#include <QListWidget>

#include "MainWindow.h"
#include "Logging.h"

class TestGuiLog : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testLogWidgetReceivesQueuedLogMessage();
    void testStateChangeAddsLogEntry();
};

void TestGuiLog::initTestCase() {
    Logging::installMessageHandler();
}

void TestGuiLog::testLogWidgetReceivesQueuedLogMessage() {
    MainWindow window;
    QListWidget *logList = window.getLogListWidget();
    QVERIFY(logList != nullptr);

    int initialCount = logList->count();

    // Emit a log message via custom category
    qInfo(logEquip) << "Unit test message for GUI event log verification.";

    // Allow Qt event loop to dispatch queued invocation to GUI thread
    QTest::qWait(150);

    QVERIFY(logList->count() > initialCount);

    bool found = false;
    for (int i = initialCount; i < logList->count(); ++i) {
        if (logList->item(i)->text().contains("Unit test message for GUI event log verification.")) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
}

void TestGuiLog::testStateChangeAddsLogEntry() {
    MainWindow window;
    QListWidget *logList = window.getLogListWidget();

    int beforeCount = logList->count();

    // Trigger state machine transition
    window.getStateMachine().requestHome();

    // Allow event queue to flush
    QTest::qWait(150);

    QVERIFY(logList->count() > beforeCount);

    bool found = false;
    for (int i = beforeCount; i < logList->count(); ++i) {
        if (logList->item(i)->text().contains("State transition ACCEPTED")) {
            found = true;
            break;
        }
    }
    QVERIFY(found);
}

QTEST_MAIN(TestGuiLog)
#include "test_gui_log.moc"
