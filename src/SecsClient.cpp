/**
 * @file SecsClient.cpp
 * @brief Implementation of Host-side SECS/GEM HSMS client.
 */

#include "SecsClient.h"
#include "Logging.h"

SecsClient::SecsClient(QObject *parent)
    : QObject(parent),
      m_socket(new QTcpSocket(this)) {
    connect(m_socket, &QTcpSocket::connected, this, &SecsClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &SecsClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &SecsClient::onReadyRead);
}

SecsClient::~SecsClient() {
    disconnectFromEquipment();
}

bool SecsClient::connectToEquipment(const QString &host, quint16 port, int timeoutMs) {
    qInfo(logSecs) << "Host Client: connecting to equipment at" << host << ":" << port;
    m_socket->connectToHost(host, port);
    if (!m_socket->waitForConnected(timeoutMs)) {
        qWarning(logSecs) << "Host Client: connection timed out or failed:" << m_socket->errorString();
        return false;
    }
    return true;
}

void SecsClient::disconnectFromEquipment() {
    if (m_socket && m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
    setHsmsState(HsmsState::NotConnected);
}

void SecsClient::setHsmsState(HsmsState state) {
    if (m_hsmsState != state) {
        qInfo(logSecs) << "Host Client: HSMS transition:"
                       << SecsProtocol::hsmsStateToString(m_hsmsState)
                       << "->" << SecsProtocol::hsmsStateToString(state);
        m_hsmsState = state;
        emit hsmsStateChanged(state);
    }
}

void SecsClient::onConnected() {
    qInfo(logSecs) << "Host Client: TCP connection established. State -> CONNECTED.";
    setHsmsState(HsmsState::Connected);
}

void SecsClient::onDisconnected() {
    qInfo(logSecs) << "Host Client: TCP connection closed.";
    setHsmsState(HsmsState::NotConnected);
}

void SecsClient::sendSelectReq() {
    HsmsHeader hdr;
    hdr.sessionId = 0xFFFF;
    hdr.stream = 0;
    hdr.function = 0;
    hdr.pType = 0;
    hdr.sType = static_cast<uint8_t>(HsmsSType::SelectReq);
    hdr.systemBytes = ++m_systemBytesCounter;

    qInfo(logSecs) << "Host Client: Sending Select.req (SystemBytes:" << hdr.systemBytes << ")";
    sendPacket(hdr);
}

void SecsClient::sendS1F1() {
    if (m_hsmsState != HsmsState::Selected) {
        qWarning(logSecs) << "Host Client: Cannot send S1F1 while not in SELECTED state!";
        return;
    }

    HsmsHeader hdr;
    hdr.sessionId = 0x0001;
    hdr.stream = 1 | 0x80; // Stream 1 with W-bit set (expecting reply)
    hdr.function = 1;      // S1F1 "Are You There"
    hdr.pType = 0;
    hdr.sType = static_cast<uint8_t>(HsmsSType::DataMessage);
    hdr.systemBytes = ++m_systemBytesCounter;

    qInfo(logSecs) << "Host Client: Sending S1F1 (Are You There) with W-bit set.";
    sendPacket(hdr);
}

void SecsClient::sendPacket(const HsmsHeader &hdr, const QByteArray &body) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    QByteArray packet = SecsProtocol::buildHsmsPacket(hdr, body);
    m_socket->write(packet);
    m_socket->flush();
}

void SecsClient::onReadyRead() {
    m_inBuffer.append(m_socket->readAll());

    while (m_inBuffer.size() >= 4) {
        const auto *data = reinterpret_cast<const uint8_t*>(m_inBuffer.constData());
        uint32_t packetLen = (static_cast<uint32_t>(data[0]) << 24) |
                             (static_cast<uint32_t>(data[1]) << 16) |
                             (static_cast<uint32_t>(data[2]) << 8)  |
                             (static_cast<uint32_t>(data[3]));

        if (packetLen < sizeof(HsmsHeader)) {
            qCritical(logSecs) << "Host Client: Invalid packet length:" << packetLen;
            m_socket->disconnectFromHost();
            return;
        }

        if (m_inBuffer.size() < static_cast<qsizetype>(4 + packetLen)) {
            break; // Awaiting more data
        }

        HsmsHeader hdr;
        std::memcpy(&hdr, m_inBuffer.constData() + 4, sizeof(HsmsHeader));

        QByteArray body = m_inBuffer.mid(4 + sizeof(HsmsHeader), packetLen - sizeof(HsmsHeader));
        m_inBuffer.remove(0, 4 + packetLen);

        processPacket(hdr, body);
    }
}

void SecsClient::processPacket(const HsmsHeader &hdr, const QByteArray &body) {
    auto sType = static_cast<HsmsSType>(hdr.sType);

    if (sType == HsmsSType::SelectRsp) {
        qInfo(logSecs) << "Host Client: Received Select.rsp. Transitioning to SELECTED.";
        setHsmsState(HsmsState::Selected);
        emit selectRspReceived();
        return;
    }

    if (sType == HsmsSType::DataMessage) {
        uint8_t stream = hdr.stream & 0x7F;
        uint8_t function = hdr.function;

        qInfo(logSecs) << "Host Client: Received SECS-II S" << stream << "F" << function
                       << "- Payload:" << body;

        if (stream == 1 && function == 2) {
            emit s1f2Received(QString::fromUtf8(body));
        }
    }
}
