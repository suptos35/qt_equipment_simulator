/**
 * @file test_adversarial_threading.cpp
 * @brief Adversarial threading, race condition, and lifecycle test suite (WT-01 to WT-04).
 */

#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <QThread>
#include <random>

#include "EquipmentWorker.h"
#include "Logging.h"

class TestAdversarialThreading : public QObject {
    Q_OBJECT

public:
    QThread *m_slotThread{nullptr};

public slots:
    void onDataReceived(double pos, double temp, EquipmentStatus status) {
        Q_UNUSED(pos);
        Q_UNUSED(temp);
        Q_UNUSED(status);
        m_slotThread = QThread::currentThread();
    }

private slots:
    void initTestCase();

    // WT-01: Stop mid-tick execution
    void testWT01_StopMidTick();

    // WT-02: Connection type verification across thread boundary
    void testWT02_ConnectionTypeVerification();

    // WT-03: Start/Stop with random jitter (0-5ms)
    void testWT03_StartStopWithRandomJitter();

    // WT-04: Shutdown while Running
    void testWT04_ShutdownWhileRunning();
};

void TestAdversarialThreading::initTestCase() {
    Logging::installMessageHandler();
    qRegisterMetaType<EquipmentStatus>("EquipmentStatus");
}

void TestAdversarialThreading::testWT01_StopMidTick() {
    for (int cycle = 0; cycle < 50; ++cycle) {
        QThread workerThread;
        auto *worker = new EquipmentWorker();
        worker->moveToThread(&workerThread);

        connect(&workerThread, &QThread::started, worker, &EquipmentWorker::start);
        connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);

        workerThread.start();
        QTest::qWait(5); // Let timer start

        // Stop worker immediately
        QMetaObject::invokeMethod(worker, "stop", Qt::BlockingQueuedConnection);
        workerThread.quit();
        QVERIFY(workerThread.wait(1000));
    }
}

void TestAdversarialThreading::testWT02_ConnectionTypeVerification() {
    QThread workerThread;
    auto *worker = new EquipmentWorker();
    worker->moveToThread(&workerThread);

    connect(&workerThread, &QThread::started, worker, &EquipmentWorker::start);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);

    m_slotThread = nullptr;
    QThread *mainTestThread = QThread::currentThread();

    // Connect using explicit Qt::QueuedConnection
    connect(worker, &EquipmentWorker::dataUpdated, this, &TestAdversarialThreading::onDataReceived, Qt::QueuedConnection);

    workerThread.start();

    // Wait for at least one emission
    int attempts = 0;
    while (!m_slotThread && attempts < 50) {
        QTest::qWait(50);
        attempts++;
    }

    QVERIFY(m_slotThread != nullptr);
    // Crucial: The slot MUST have executed on the main GUI thread, NOT the worker thread!
    QCOMPARE(m_slotThread, mainTestThread);
    QVERIFY(m_slotThread != &workerThread);

    // Teardown
    QMetaObject::invokeMethod(worker, "stop", Qt::BlockingQueuedConnection);
    workerThread.quit();
    QVERIFY(workerThread.wait(2000));
}

void TestAdversarialThreading::testWT03_StartStopWithRandomJitter() {
    QThread workerThread;
    auto *worker = new EquipmentWorker();
    worker->moveToThread(&workerThread);

    connect(&workerThread, &QThread::started, worker, &EquipmentWorker::start);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    workerThread.start();

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> jitter(0, 5);

    for (int i = 0; i < 200; ++i) {
        QMetaObject::invokeMethod(worker, "requestStart", Qt::QueuedConnection);
        int delayStart = jitter(rng);
        if (delayStart > 0) {
            QTest::qWait(delayStart);
        }

        QMetaObject::invokeMethod(worker, "requestStop", Qt::QueuedConnection);
        int delayStop = jitter(rng);
        if (delayStop > 0) {
            QTest::qWait(delayStop);
        }
    }

    QTest::qWait(100);

    EquipmentStatus s = worker->getStatus();
    QVERIFY(s == EquipmentStatus::Idle || s == EquipmentStatus::Running);

    QMetaObject::invokeMethod(worker, "stop", Qt::BlockingQueuedConnection);
    workerThread.quit();
    QVERIFY(workerThread.wait(2000));
}

void TestAdversarialThreading::testWT04_ShutdownWhileRunning() {
    QThread workerThread;
    auto *worker = new EquipmentWorker();
    worker->moveToThread(&workerThread);

    connect(&workerThread, &QThread::started, worker, &EquipmentWorker::start);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    workerThread.start();

    // Transition to Running
    QMetaObject::invokeMethod(worker, "requestStart", Qt::BlockingQueuedConnection);
    QCOMPARE(worker->getStatus(), EquipmentStatus::Running);

    // Shut down thread without stopping worker first
    workerThread.quit();
    QVERIFY2(workerThread.wait(2000), "Worker thread failed to terminate cleanly while Running");
}

QTEST_MAIN(TestAdversarialThreading)
#include "test_adversarial_threading.moc"
