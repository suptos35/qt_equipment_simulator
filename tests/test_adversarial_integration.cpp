/**
 * @file test_adversarial_integration.cpp
 * @brief Adversarial integration test suite (INT-01).
 */

#include <QObject>
#include <QTest>
#include <QPushButton>
#include <QLabel>
#include <random>

#include "MainWindow.h"
#include "Logging.h"

class TestAdversarialIntegration : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // INT-01: Randomized real click sequences on GUI buttons
    void testINT01_RandomizedMouseClickSequences();
};

void TestAdversarialIntegration::initTestCase() {
    Logging::installMessageHandler();
}

void TestAdversarialIntegration::testINT01_RandomizedMouseClickSequences() {
    MainWindow window;
    window.show();
    QTest::qWaitForWindowExposed(&window);

    QPushButton *btnStart = window.findChild<QPushButton*>("btnStart");
    QPushButton *btnStop = window.findChild<QPushButton*>("btnStop");
    QPushButton *btnHome = window.findChild<QPushButton*>("btnHome");
    QPushButton *btnReset = window.findChild<QPushButton*>("btnReset");
    QPushButton *btnTriggerError = window.findChild<QPushButton*>("btnTriggerError");
    QLabel *lblStatus = window.findChild<QLabel*>("lblStatus");

    QVERIFY(btnStart != nullptr);
    QVERIFY(btnStop != nullptr);
    QVERIFY(btnHome != nullptr);
    QVERIFY(btnReset != nullptr);
    QVERIFY(btnTriggerError != nullptr);
    QVERIFY(lblStatus != nullptr);

    std::vector<QPushButton*> buttons = {btnStart, btnStop, btnHome, btnReset, btnTriggerError};

    std::mt19937 rng(1337);
    std::uniform_int_distribution<size_t> distBtn(0, buttons.size() - 1);
    std::uniform_int_distribution<int> distDelay(5, 30);

    // Fire 60 randomized clicks with random settling times
    for (int i = 0; i < 60; ++i) {
        QPushButton *btn = buttons[distBtn(rng)];
        QTest::mouseClick(btn, Qt::LeftButton);

        int delay = distDelay(rng);
        QTest::qWait(delay);

        // Verify displayed UI label text matches StateMachine's internal state
        EquipmentStatus expectedState = window.getStateMachine().getCurrentState();
        QString expectedStr = QString(EquipmentState::statusToString(expectedState));

        QCOMPARE(lblStatus->text(), expectedStr);
    }
}

QTEST_MAIN(TestAdversarialIntegration)
#include "test_adversarial_integration.moc"
