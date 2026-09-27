/**
 * @file test_adversarial_statemachine.cpp
 * @brief Adversarial test suite for StateMachine (SM-01 to SM-06).
 */

#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <QThread>

#include "StateMachine.h"
#include "EquipmentWorker.h"
#include "Logging.h"

class TestAdversarialStateMachine : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // SM-01: Full invalid-transition matrix
    void testSM01_FullInvalidTransitionMatrix();

    // SM-02: Self-transitions and repeated commands
    void testSM02_SelfTransitions();

    // SM-03: Double fault
    void testSM03_DoubleFault();

    // SM-04: Reset from non-Error states
    void testSM04_ResetFromNonErrorStates();

    // SM-05: Rejection side effects on EquipmentState
    void testSM05_RejectionSideEffects();

    // SM-06: Reset-then-command race across queued connection
    void testSM06_ResetThenCommandQueueOrder();
};

void TestAdversarialStateMachine::initTestCase() {
    Logging::installMessageHandler();
    qRegisterMetaType<EquipmentStatus>("EquipmentStatus");
}

void TestAdversarialStateMachine::testSM01_FullInvalidTransitionMatrix() {
    // Test every (state, invalid transition) combination
    // 1. From Idle
    {
        StateMachine sm;
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);
        QVERIFY(!sm.requestStop());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);
        QVERIFY(!sm.onHomeComplete());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);
        QVERIFY(!sm.requestReset());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);
    }

    // 2. From Homing
    {
        StateMachine sm;
        QVERIFY(sm.requestHome());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Homing);

        QVERIFY(!sm.requestStart());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Homing);
        QVERIFY(!sm.requestHome()); // repeat
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Homing);
        QVERIFY(!sm.requestStop());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Homing);
        QVERIFY(!sm.requestReset());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Homing);
    }

    // 3. From Running
    {
        StateMachine sm;
        QVERIFY(sm.requestStart());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Running);

        QVERIFY(!sm.requestStart()); // repeat
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Running);
        QVERIFY(!sm.requestHome());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Running);
        QVERIFY(!sm.onHomeComplete());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Running);
        QVERIFY(!sm.requestReset());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Running);
    }

    // 4. From Error
    {
        StateMachine sm;
        QVERIFY(sm.triggerFault("Fault from Idle"));
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Error);

        QVERIFY(!sm.requestStart());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Error);
        QVERIFY(!sm.requestHome());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Error);
        QVERIFY(!sm.requestStop());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Error);
        QVERIFY(!sm.onHomeComplete());
        QCOMPARE(sm.getCurrentState(), EquipmentStatus::Error);
    }
}

void TestAdversarialStateMachine::testSM02_SelfTransitions() {
    StateMachine sm;

    // Running self-transition
    QVERIFY(sm.requestStart());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Running);
    QVERIFY(!sm.requestStart()); // duplicate
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Running);

    // Stop back to Idle
    QVERIFY(sm.requestStop());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);

    // Homing self-transition
    QVERIFY(sm.requestHome());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Homing);
    QVERIFY(!sm.requestHome()); // duplicate
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Homing);
}

void TestAdversarialStateMachine::testSM03_DoubleFault() {
    StateMachine sm;
    QVERIFY(sm.triggerFault("Initial Fault"));
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Error);

    // Second fault while already in Error
    QSignalSpy changeSpy(&sm, &StateMachine::stateChanged);
    QSignalSpy rejectSpy(&sm, &StateMachine::transitionRejected);

    bool secondFaultResult = sm.triggerFault("Secondary Fault");

    // Must not crash, must not trigger state change, must remain Error
    QVERIFY(!secondFaultResult);
    QCOMPARE(changeSpy.count(), 0);
    QCOMPARE(rejectSpy.count(), 1);
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Error);
}

void TestAdversarialStateMachine::testSM04_ResetFromNonErrorStates() {
    StateMachine sm;

    // Reset from Idle
    QVERIFY(!sm.requestReset());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Idle);

    // Reset from Homing
    QVERIFY(sm.requestHome());
    QVERIFY(!sm.requestReset());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Homing);
    QVERIFY(sm.onHomeComplete());

    // Reset from Running
    QVERIFY(sm.requestStart());
    QVERIFY(!sm.requestReset());
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Running);
}

void TestAdversarialStateMachine::testSM05_RejectionSideEffects() {
    StateMachine sm;
    EquipmentWorker worker;

    // Put both in Homing
    QVERIFY(sm.requestHome());
    worker.requestHome();
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Homing);
    QCOMPARE(worker.getStatus(), EquipmentStatus::Homing);

    // Attempt illegal requestStart()
    bool accepted = sm.requestStart();
    QVERIFY(!accepted);

    // Check no mutation occurred
    QCOMPARE(sm.getCurrentState(), EquipmentStatus::Homing);
    QCOMPARE(worker.getStatus(), EquipmentStatus::Homing);
}

void TestAdversarialStateMachine::testSM06_ResetThenCommandQueueOrder() {
    QThread workerThread;
    auto *worker = new EquipmentWorker();
    worker->moveToThread(&workerThread);

    connect(&workerThread, &QThread::started, worker, &EquipmentWorker::start);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    workerThread.start();

    // Trigger fault
    QMetaObject::invokeMethod(worker, "requestFault", Qt::BlockingQueuedConnection);
    QCOMPARE(worker->getStatus(), EquipmentStatus::Error);

    // Rapidly enqueue reset immediately followed by start (simulating fast double click)
    QMetaObject::invokeMethod(worker, "requestReset", Qt::QueuedConnection);
    QMetaObject::invokeMethod(worker, "requestStart", Qt::QueuedConnection);

    // Wait for worker event loop to process in FIFO order
    QTest::qWait(150);

    // FIFO guarantees Error -> Idle (reset) -> Running (start)
    QCOMPARE(worker->getStatus(), EquipmentStatus::Running);

    // Clean teardown
    QMetaObject::invokeMethod(worker, "stop", Qt::BlockingQueuedConnection);
    workerThread.quit();
    QVERIFY(workerThread.wait(2000));
}

QTEST_MAIN(TestAdversarialStateMachine)
#include "test_adversarial_statemachine.moc"
