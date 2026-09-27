/**
 * @file test_adversarial_secsgem.cpp
 * @brief Adversarial protocol tests for HSMS and SECS-II engine (SECS-01 to SECS-05).
 */

#include <QObject>
#include <QTest>
#include <QSignalSpy>
#include <QTcpSocket>

#include "SecsServer.h"
#include "SecsClient.h"
#include "Logging.h"

class TestAdversarialSecsGem : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();

    // SECS-01: Out-of-order protocol requests (S1F1 before Select.req)
    void testSECS01_S1F1BeforeSelectionRejected();

    // SECS-02: Malformed and truncated frame handling
    void testSECS02_MalformedAndTruncatedFrames();

    // SECS-03: Unsupported SECS-II Stream/Function
    void testSECS03_UnsupportedStreamFunction();

    // SECS-04: Disconnect mid-handshake
    void testSECS04_DisconnectMidHandshake();

    // SECS-05: Connect without selecting
    void testSECS05_ConnectWithoutSelecting();
};

void TestAdversarialSecsGem::initTestCase() {
    Logging::installMessageHandler();
    qRegisterMetaType<HsmsState>("HsmsState");
}

void TestAdversarialSecsGem::testSECS01_S1F1BeforeSelectionRejected() {
    SecsServer server;
    QVERIFY(server.startListening(0));
    quint16 port = server.getBoundPort();

    QTcpSocket rawSocket;
    rawSocket.connectToHost(QStringLiteral("127.0.0.1"), port);
    QVERIFY(rawSocket.waitForConnected(2000));
    QTest::qWait(50);

    QCOMPARE(server.getHsmsState(), HsmsState::Connected);

    // Send S1F1 (DataMessage) directly WITHOUT sending Select.req first!
    HsmsHeader s1f1Hdr;
    s1f1Hdr.sessionId = 0x0001;
    s1f1Hdr.stream = 1;
    s1f1Hdr.function = 1;
    s1f1Hdr.pType = 0;
    s1f1Hdr.sType = static_cast<uint8_t>(HsmsSType::DataMessage);
    s1f1Hdr.systemBytes = 12345;

    QByteArray packet = SecsProtocol::buildHsmsPacket(s1f1Hdr);

    QSignalSpy s1f1Spy(&server, &SecsServer::s1f1Received);
    QSignalSpy s1f2Spy(&server, &SecsServer::s1f2Sent);

    rawSocket.write(packet);
    rawSocket.flush();

    // Wait up to 500ms
    QTest::qWait(500);

    // Per HSMS specification SEMI E37, data messages received before SELECTED
    // MUST NOT be accepted or processed as valid online requests!
    QVERIFY2(s1f1Spy.count() == 0, "Server processed S1F1 while NOT in SELECTED state! (HSMS violation)");
    QVERIFY2(s1f2Spy.count() == 0, "Server emitted S1F2 while NOT in SELECTED state!");

    rawSocket.disconnectFromHost();
}

void TestAdversarialSecsGem::testSECS02_MalformedAndTruncatedFrames() {
    SecsServer server;
    QVERIFY(server.startListening(0));
    quint16 port = server.getBoundPort();

    QTcpSocket rawSocket;
    rawSocket.connectToHost(QStringLiteral("127.0.0.1"), port);
    QVERIFY(rawSocket.waitForConnected(2000));

    // 1. Partial length prefix (< 4 bytes)
    QByteArray partialLength;
    partialLength.append('\x00');
    partialLength.append('\x00');
    rawSocket.write(partialLength);
    rawSocket.flush();
    QTest::qWait(50);
    // Server must not crash

    // 2. Huge packet length claiming 100,000 bytes with only 10 bytes sent
    QByteArray hugeClaim;
    uint32_t claimed = 100000;
    hugeClaim.append(static_cast<char>((claimed >> 24) & 0xFF));
    hugeClaim.append(static_cast<char>((claimed >> 16) & 0xFF));
    hugeClaim.append(static_cast<char>((claimed >> 8) & 0xFF));
    hugeClaim.append(static_cast<char>(claimed & 0xFF));
    hugeClaim.append("0123456789"); // Only 10 bytes body
    rawSocket.write(hugeClaim);
    rawSocket.flush();
    QTest::qWait(50);

    // Server must remain alive and responsive
    QVERIFY(server.getHsmsState() == HsmsState::Connected || server.getHsmsState() == HsmsState::NotConnected);
    rawSocket.disconnectFromHost();
}

void TestAdversarialSecsGem::testSECS03_UnsupportedStreamFunction() {
    SecsServer server;
    QVERIFY(server.startListening(0));
    quint16 port = server.getBoundPort();

    SecsClient client;
    QVERIFY(client.connectToEquipment(QStringLiteral("127.0.0.1"), port, 2000));
    QSignalSpy selectRspSpy(&client, &SecsClient::selectRspReceived);
    client.sendSelectReq();
    QVERIFY(selectRspSpy.wait(2000));
    QCOMPARE(server.getHsmsState(), HsmsState::Selected);

    // Send unsupported message: S2F17 (Date & Time Request)
    HsmsHeader unsupportedHdr;
    unsupportedHdr.sessionId = 0x0001;
    unsupportedHdr.stream = 2;
    unsupportedHdr.function = 17;
    unsupportedHdr.pType = 0;
    unsupportedHdr.sType = static_cast<uint8_t>(HsmsSType::DataMessage);
    unsupportedHdr.systemBytes = 999;

    QByteArray packet = SecsProtocol::buildHsmsPacket(unsupportedHdr);

    QTcpSocket rawSocket;
    rawSocket.connectToHost(QStringLiteral("127.0.0.1"), port);
    QVERIFY(rawSocket.waitForConnected(2000));

    // Send unsupported message
    rawSocket.write(packet);
    rawSocket.flush();
    QTest::qWait(100);

    // Server must not crash
    QVERIFY(server.getHsmsState() == HsmsState::Selected || server.getHsmsState() == HsmsState::Connected);
}

void TestAdversarialSecsGem::testSECS04_DisconnectMidHandshake() {
    SecsServer server;
    QVERIFY(server.startListening(0));
    quint16 port = server.getBoundPort();

    {
        QTcpSocket socket;
        socket.connectToHost(QStringLiteral("127.0.0.1"), port);
        QVERIFY(socket.waitForConnected(2000));
        QTest::qWait(50);
        QCOMPARE(server.getHsmsState(), HsmsState::Connected);

        // Abruptly disconnect
        socket.disconnectFromHost();
    }

    QTest::qWait(100);
    // Server must clean up and return to NotConnected
    QCOMPARE(server.getHsmsState(), HsmsState::NotConnected);
}

void TestAdversarialSecsGem::testSECS05_ConnectWithoutSelecting() {
    SecsServer server;
    QVERIFY(server.startListening(0));
    quint16 port = server.getBoundPort();

    QTcpSocket socket;
    socket.connectToHost(QStringLiteral("127.0.0.1"), port);
    QVERIFY(socket.waitForConnected(2000));
    QTest::qWait(50);

    // Remains Connected, never reaches Selected
    QCOMPARE(server.getHsmsState(), HsmsState::Connected);
    QVERIFY(server.getHsmsState() != HsmsState::Selected);

    socket.disconnectFromHost();
    QTest::qWait(50);
    QCOMPARE(server.getHsmsState(), HsmsState::NotConnected);
}

QTEST_MAIN(TestAdversarialSecsGem)
#include "test_adversarial_secsgem.moc"
