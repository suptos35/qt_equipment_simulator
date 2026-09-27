/**
 * @file test_secsgem.cpp
 * @brief Integration tests for SEMI E37 (HSMS) and SEMI E5 (SECS-II) communication.
 */

#include <QObject>
#include <QTest>
#include <QSignalSpy>

#include "SecsServer.h"
#include "SecsClient.h"
#include "Logging.h"

class TestSecsGem : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testHsmsConnectionAndSelection();
    void testS1F1S1F2Exchange();
};

void TestSecsGem::initTestCase() {
    Logging::installMessageHandler();
    qRegisterMetaType<HsmsState>("HsmsState");
}

void TestSecsGem::testHsmsConnectionAndSelection() {
    SecsServer server;
    QVERIFY(server.startListening(0)); // 0 lets OS allocate dynamic available port
    quint16 port = server.getBoundPort();
    QVERIFY(port > 0);

    SecsClient client;
    QSignalSpy clientStateSpy(&client, &SecsClient::hsmsStateChanged);
    QSignalSpy serverStateSpy(&server, &SecsServer::hsmsStateChanged);
    QSignalSpy selectRspSpy(&client, &SecsClient::selectRspReceived);

    // 1. Establish TCP connection
    QVERIFY(client.connectToEquipment(QStringLiteral("127.0.0.1"), port, 2000));

    // Wait briefly for TCP handshake
    QTest::qWait(50);
    QCOMPARE(client.getHsmsState(), HsmsState::Connected);
    QCOMPARE(server.getHsmsState(), HsmsState::Connected);

    // 2. Perform HSMS Selection Request (Select.req / Select.rsp)
    client.sendSelectReq();

    QVERIFY(selectRspSpy.wait(2000));
    QCOMPARE(selectRspSpy.count(), 1);

    QCOMPARE(client.getHsmsState(), HsmsState::Selected);
    QCOMPARE(server.getHsmsState(), HsmsState::Selected);
}

void TestSecsGem::testS1F1S1F2Exchange() {
    SecsServer server;
    QVERIFY(server.startListening(0));
    quint16 port = server.getBoundPort();

    SecsClient client;
    QVERIFY(client.connectToEquipment(QStringLiteral("127.0.0.1"), port, 2000));

    QSignalSpy selectRspSpy(&client, &SecsClient::selectRspReceived);
    client.sendSelectReq();
    QVERIFY(selectRspSpy.wait(2000));
    QCOMPARE(client.getHsmsState(), HsmsState::Selected);

    // Send Stream 1, Function 1 (Are You There)
    QSignalSpy s1f2Spy(&client, &SecsClient::s1f2Received);
    client.sendS1F1();

    // Verify S1F2 reply arrives within 2 seconds
    QVERIFY(s1f2Spy.wait(2000));
    QCOMPARE(s1f2Spy.count(), 1);

    QString replyPayload = s1f2Spy.first().at(0).toString();
    QVERIFY(replyPayload.contains("EQUIP_SIM_2000"));
    QVERIFY(replyPayload.contains("1.0.0"));
}

QTEST_MAIN(TestSecsGem)
#include "test_secsgem.moc"
