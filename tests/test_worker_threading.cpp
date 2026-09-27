/**
 * @file test_worker_threading.cpp
 * @brief Threading and concurrency regression tests for EquipmentWorker.
 */

#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <QThread>

#include "EquipmentWorker.h"
#include "Logging.h"

class TestWorkerThreading : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testWorkerQueuedSignalDelivery();
    void testStressStartStopTightLoop();
};

void TestWorkerThreading::initTestCase() {
    Logging::installMessageHandler();
    qRegisterMetaType<EquipmentStatus>("EquipmentStatus");
}

void TestWorkerThreading::testWorkerQueuedSignalDelivery() {
    QThread workerThread;
    auto *worker = new EquipmentWorker();
    worker->moveToThread(&workerThread);

    connect(&workerThread, &QThread::started, worker, &EquipmentWorker::start);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);

    QSignalSpy spy(worker, &EquipmentWorker::dataUpdated);
    QVERIFY(spy.isValid());

    workerThread.start();

    // Wait up to 2 seconds for periodic tick emissions across the thread boundary
    QVERIFY(spy.wait(2000));
    QVERIFY(spy.count() >= 1);

    // Verify signal payload structure
    QList<QVariant> firstEmission = spy.takeFirst();
    QCOMPARE(firstEmission.count(), 3);
    double pos = firstEmission.at(0).toDouble();
    double temp = firstEmission.at(1).toDouble();
    auto status = firstEmission.at(2).value<EquipmentStatus>();

    QVERIFY(pos >= 0.0);
    QVERIFY(temp >= 15.0);
    QVERIFY(status == EquipmentStatus::Idle || status == EquipmentStatus::Running);

    // Graceful teardown
    QMetaObject::invokeMethod(worker, "stop", Qt::BlockingQueuedConnection);
    workerThread.quit();
    QVERIFY(workerThread.wait(2000));
}

void TestWorkerThreading::testStressStartStopTightLoop() {
    QThread workerThread;
    auto *worker = new EquipmentWorker();
    worker->moveToThread(&workerThread);

    connect(&workerThread, &QThread::started, worker, &EquipmentWorker::start);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);

    workerThread.start();

    // Rapidly alternate start and stop commands across thread boundaries (500 iterations)
    for (int i = 0; i < 500; ++i) {
        QMetaObject::invokeMethod(worker, "requestStart", Qt::QueuedConnection);
        QMetaObject::invokeMethod(worker, "requestStop", Qt::QueuedConnection);
    }

    // Allow event queue on worker thread to process all queued requests
    QTest::qWait(150);

    // Verify worker didn't crash and status remained legitimate
    EquipmentStatus currentStatus = worker->getStatus();
    QVERIFY(currentStatus == EquipmentStatus::Idle || currentStatus == EquipmentStatus::Running);

    // Graceful teardown
    QMetaObject::invokeMethod(worker, "stop", Qt::BlockingQueuedConnection);
    workerThread.quit();
    QVERIFY(workerThread.wait(2000));
}

QTEST_MAIN(TestWorkerThreading)
#include "test_worker_threading.moc"
