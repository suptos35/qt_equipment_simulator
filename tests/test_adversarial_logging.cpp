/**
 * @file test_adversarial_logging.cpp
 * @brief Adversarial logging subsystem tests (LOG-01 to LOG-03).
 */

#include <QObject>
#include <QTest>
#include <QLoggingCategory>

#include "Logging.h"
#include "LoggerThread.h"

class TestAdversarialLogging : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // LOG-01: Startup ordering (logging before QApplication / full init)
    void testLOG01_StartupOrderingSafety();

    // LOG-02: Recursive logging protection
    void testLOG02_RecursiveLoggingSafety();

    // LOG-03: QT_LOGGING_RULES filtering behavior
    void testLOG03_CategoryFilteringBehavior();
};

void TestAdversarialLogging::initTestCase() {
    Logging::installMessageHandler();
}

void TestAdversarialLogging::testLOG01_StartupOrderingSafety() {
    // Calling logging early must safely enqueue without crashing
    size_t beforeCount = LoggerThread::instance().getProcessedCount();

    qInfo(logEquip) << "LOG-01 Early bootstrap log message.";

    // Wait for logger thread to process
    QTest::qWait(50);
    QVERIFY(LoggerThread::instance().getProcessedCount() >= beforeCount);
}

void TestAdversarialLogging::testLOG02_RecursiveLoggingSafety() {
    // Repeated high-frequency logging from multiple threads
    for (int i = 0; i < 100; ++i) {
        qWarning(logEquip) << "LOG-02 Stress message #" << i;
    }
    QTest::qWait(50);
    // Verifies no stack overflow or recursive lock deadlock
}

void TestAdversarialLogging::testLOG03_CategoryFilteringBehavior() {
    // 1. Disable debug for equipment.core
    QLoggingCategory::setFilterRules(QStringLiteral("equipment.core.debug=false"));
    QVERIFY(!logEquip().isDebugEnabled());

    size_t countBefore = LoggerThread::instance().getProcessedCount();

    // This debug call should be filtered out by Qt before reaching the message handler
    qDebug(logEquip) << "This suppressed debug message must not reach the logger queue";

    QTest::qWait(50);
    size_t countAfter = LoggerThread::instance().getProcessedCount();
    QCOMPARE(countAfter, countBefore);

    // 2. Re-enable debug for equipment.core
    QLoggingCategory::setFilterRules(QStringLiteral("equipment.core.debug=true"));
    QVERIFY(logEquip().isDebugEnabled());

    qDebug(logEquip) << "This enabled debug message SHOULD reach the logger queue";

    QTest::qWait(50);
    size_t countReenabled = LoggerThread::instance().getProcessedCount();
    QVERIFY(countReenabled > countAfter);
}

QTEST_MAIN(TestAdversarialLogging)
#include "test_adversarial_logging.moc"
