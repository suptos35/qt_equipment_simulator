/**
 * @file test_statemachine.cpp
 * @brief Unit tests for equipment finite state machine lifecycle and transition validation.
 */

#include <QObject>
#include <QTest>
#include <QSignalSpy>

#include "StateMachine.h"
#include "Logging.h"

class TestStateMachine : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testInitialStateIsIdle();
    void testHomingLifecycle();
    void testRunningLifecycle();
    void testFaultFromIdle();
    void testFaultFromHoming();
    void testFaultFromRunning();
    void testRejectedInvalidTransitions();
};

void TestStateMachine::initTestCase() {
    Logging::installMessageHandler();
    qRegisterMetaType<EquipmentStatus>("EquipmentStatus");
}

void TestStateMachine::testInitialStateIsIdle() {
    StateMachine sm;
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);
}

void TestStateMachine::testHomingLifecycle() {
    StateMachine sm;
    QSignalSpy changeSpy(&sm, &StateMachine::stateChanged);

    QVERIFY(sm.requestHome());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Homing);
    QCOMPARE(changeSpy.count(), 1);

    QVERIFY(sm.onHomeComplete());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);
    QCOMPARE(changeSpy.count(), 2);
}

void TestStateMachine::testRunningLifecycle() {
    StateMachine sm;
    QSignalSpy changeSpy(&sm, &StateMachine::stateChanged);

    QVERIFY(sm.requestStart());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Running);
    QCOMPARE(changeSpy.count(), 1);

    QVERIFY(sm.requestStop());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);
    QCOMPARE(changeSpy.count(), 2);
}

void TestStateMachine::testFaultFromIdle() {
    StateMachine sm;
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);

    QVERIFY(sm.triggerFault(QStringLiteral("E-Stop in Idle")));
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Error);

    QVERIFY(sm.requestReset());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);
}

void TestStateMachine::testFaultFromHoming() {
    StateMachine sm;
    QVERIFY(sm.requestHome());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Homing);

    QVERIFY(sm.triggerFault(QStringLiteral("Homing Sensor Dropout")));
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Error);

    QVERIFY(sm.requestReset());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);
}

void TestStateMachine::testFaultFromRunning() {
    StateMachine sm;
    QVERIFY(sm.requestStart());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Running);

    QVERIFY(sm.triggerFault(QStringLiteral("Chamber Over-temperature")));
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Error);

    QVERIFY(sm.requestReset());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);
}

void TestStateMachine::testRejectedInvalidTransitions() {
    StateMachine sm;
    QSignalSpy rejectSpy(&sm, &StateMachine::transitionRejected);

    // 1. Idle: Cannot stop or reset
    QVERIFY(!sm.requestStop());
    QVERIFY(!sm.requestReset());
    QCOMPARE(rejectSpy.count(), 2);

    // 2. Transition to Homing
    QVERIFY(sm.requestHome());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Homing);

    // In Homing: cannot start, cannot stop, cannot reset
    QVERIFY(!sm.requestStart());
    QVERIFY(!sm.requestStop());
    QVERIFY(!sm.requestReset());
    QCOMPARE(rejectSpy.count(), 5);

    // Complete homing back to Idle
    QVERIFY(sm.onHomeComplete());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);

    // 3. Transition to Running
    QVERIFY(sm.requestStart());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Running);

    // In Running: cannot start again, cannot home, cannot reset
    QVERIFY(!sm.requestStart());
    QVERIFY(!sm.requestHome());
    QVERIFY(!sm.requestReset());
    QCOMPARE(rejectSpy.count(), 8);

    // Trigger Fault to Error
    QVERIFY(sm.triggerFault(QStringLiteral("Test fault")));
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Error);

    // In Error: CANNOT jump straight to Running or Homing without requestReset()
    QVERIFY(!sm.requestStart());
    QVERIFY(!sm.requestHome());
    QVERIFY(!sm.requestStop());
    QCOMPARE(rejectSpy.count(), 11);

    // Finally recover
    QVERIFY(sm.requestReset());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);
}

QTEST_MAIN(TestStateMachine)
#include "test_statemachine.moc"
