/**
 * @file test_adversarial_physics.cpp
 * @brief Adversarial physics and state boundary test suite (ES-01 to ES-04).
 */

#include <QObject>
#include <QTest>
#include <QSignalSpy>

#include "EquipmentState.h"
#include "EquipmentWorker.h"
#include "Logging.h"

class TestAdversarialPhysics : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // ES-01: Boundary values on single tick
    void testES01_BoundaryValuesSingleTick();

    // ES-02: Out-of-range starting value recovery
    void testES02_OutOfRangeValueRecovery();

    // ES-03: Deterministic RNG bounds across 100 runs
    void testES03_RngBoundsAcrossManyRuns();

    // ES-04: Verification of decoupled cross-thread telemetry access
    void testES04_CrossThreadDecoupledAccess();
};

void TestAdversarialPhysics::initTestCase() {
    Logging::installMessageHandler();
}

void TestAdversarialPhysics::testES01_BoundaryValuesSingleTick() {
    EquipmentState state;
    state.setStatus(EquipmentStatus::Running);

    // 1. Exactly at MAX_POSITION (100.0)
    state.setPosition(EquipmentState::MAX_POSITION);
    QCOMPARE(state.getPosition(), EquipmentState::MAX_POSITION);
    state.tick();
    QVERIFY2(state.getPosition() <= EquipmentState::MAX_POSITION, "Position exceeded MAX_POSITION after tick at boundary");
    QVERIFY2(state.getPosition() >= EquipmentState::MIN_POSITION, "Position went below MIN_POSITION after tick at boundary");

    // 2. Exactly at MIN_POSITION (0.0)
    state.setPosition(EquipmentState::MIN_POSITION);
    QCOMPARE(state.getPosition(), EquipmentState::MIN_POSITION);
    state.tick();
    QVERIFY2(state.getPosition() >= EquipmentState::MIN_POSITION, "Position dropped below MIN_POSITION after tick at boundary");
    QVERIFY2(state.getPosition() <= EquipmentState::MAX_POSITION, "Position exceeded MAX_POSITION after tick at boundary");
}

void TestAdversarialPhysics::testES02_OutOfRangeValueRecovery() {
    EquipmentState state;

    // setPosition internally clamps out-of-range values:
    state.setPosition(150.0);
    QCOMPARE(state.getPosition(), EquipmentState::MAX_POSITION);

    state.setPosition(-50.0);
    QCOMPARE(state.getPosition(), EquipmentState::MIN_POSITION);

    // Call tick to ensure stability after boundary clamp
    state.tick();
    QVERIFY(state.getPosition() >= EquipmentState::MIN_POSITION);
    QVERIFY(state.getPosition() <= EquipmentState::MAX_POSITION);
}

void TestAdversarialPhysics::testES03_RngBoundsAcrossManyRuns() {
    // Run full 1,000-tick simulation 100 times to verify RNG never breaches physical safety limits
    for (int run = 0; run < 100; ++run) {
        EquipmentState state;
        state.setStatus(EquipmentStatus::Running);

        for (int t = 0; t < 1000; ++t) {
            state.tick();
            QVERIFY(state.getPosition() >= EquipmentState::MIN_POSITION);
            QVERIFY(state.getPosition() <= EquipmentState::MAX_POSITION);
            QVERIFY(state.getTemperature() >= 15.0);
            QVERIFY(state.getTemperature() <= EquipmentState::MAX_SAFE_TEMP);
        }
    }
}

void TestAdversarialPhysics::testES04_CrossThreadDecoupledAccess() {
    EquipmentWorker worker;
    QSignalSpy spy(&worker, &EquipmentWorker::dataUpdated);

    worker.onTick();
    QCOMPARE(spy.count(), 1);

    // Verify telemetry data is received purely by value via signal parameters
    QList<QVariant> args = spy.takeFirst();
    double pos = args.at(0).toDouble();
    double temp = args.at(1).toDouble();
    auto status = args.at(2).value<EquipmentStatus>();

    QCOMPARE(pos, worker.getPosition());
    QCOMPARE(temp, worker.getTemperature());
    QCOMPARE(status, worker.getStatus());
}

QTEST_MAIN(TestAdversarialPhysics)
#include "test_adversarial_physics.moc"
