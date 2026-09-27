/**
 * @file test_equipmentstate.cpp
 * @brief Unit tests for pure equipment physics and state transitions.
 */

#include <QObject>
#include <QTest>
#include "EquipmentState.h"

class TestEquipmentState : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testInitialState();
    void testTicksWithinBounds();
    void testHomingRoutine();
    void testRunningTemperatureAndPosition();
    void testStatusStringRepresentations();
};

void TestEquipmentState::initTestCase() {
    // Basic test case initialization
}

void TestEquipmentState::testInitialState() {
    EquipmentState state;
    QCOMPARE(state.getStatus(), EquipmentStatus::Idle);
    QVERIFY(state.getPosition() >= EquipmentState::MIN_POSITION);
    QVERIFY(state.getPosition() <= EquipmentState::MAX_POSITION);
    QVERIFY(std::abs(state.getTemperature() - EquipmentState::AMBIENT_TEMP) < 0.1);
}

void TestEquipmentState::testTicksWithinBounds() {
    EquipmentState state;
    state.setStatus(EquipmentStatus::Running);

    // Run tick 1,000 times
    for (int i = 0; i < 1000; ++i) {
        state.tick();
        QVERIFY(state.getPosition() >= EquipmentState::MIN_POSITION);
        QVERIFY(state.getPosition() <= EquipmentState::MAX_POSITION);
        QVERIFY(state.getTemperature() >= 15.0);
        QVERIFY(state.getTemperature() <= EquipmentState::MAX_SAFE_TEMP);

        // Status enum must remain valid
        EquipmentStatus s = state.getStatus();
        QVERIFY(s == EquipmentStatus::Idle || s == EquipmentStatus::Homing ||
                s == EquipmentStatus::Running || s == EquipmentStatus::Error);
    }
}

void TestEquipmentState::testHomingRoutine() {
    EquipmentState state;
    state.setPosition(50.0);
    state.setStatus(EquipmentStatus::Homing);

    // Homing should progressively move position towards 0.0 and complete to Idle
    int ticks = 0;
    while (state.getStatus() == EquipmentStatus::Homing && ticks < 200) {
        state.tick();
        ticks++;
    }

    QCOMPARE(state.getStatus(), EquipmentStatus::Idle);
    QCOMPARE(state.getPosition(), 0.0);
    QVERIFY(ticks > 0);
}

void TestEquipmentState::testRunningTemperatureAndPosition() {
    EquipmentState state;
    state.setStatus(EquipmentStatus::Running);

    // Check that temperature gradually increases toward target
    double initialTemp = state.getTemperature();
    for (int i = 0; i < 100; ++i) {
        state.tick();
    }
    double finalTemp = state.getTemperature();
    QVERIFY(finalTemp > initialTemp);
}

void TestEquipmentState::testStatusStringRepresentations() {
    QCOMPARE(QString(EquipmentState::statusToString(EquipmentStatus::Idle)), QStringLiteral("Idle"));
    QCOMPARE(QString(EquipmentState::statusToString(EquipmentStatus::Homing)), QStringLiteral("Homing"));
    QCOMPARE(QString(EquipmentState::statusToString(EquipmentStatus::Running)), QStringLiteral("Running"));
    QCOMPARE(QString(EquipmentState::statusToString(EquipmentStatus::Error)), QStringLiteral("Error"));
}

QTEST_MAIN(TestEquipmentState)
#include "test_equipmentstate.moc"
