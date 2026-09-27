/**
 * @file SecsServer.cpp
 * @brief Implementation of equipment HSMS server endpoint.
 */

#include "SecsServer.h"
#include "Logging.h"

SecsServer::SecsServer(QObject *parent)
    : QObject(parent),
      m_server(new QTcpServer(this)) {
    connect(m_server, &QTcpServer::newConnection, this, &SecsServer::onNewConnection);
}

SecsServer::~SecsServer() {
    stop();
}

bool SecsServer::startListening(quint16 port) {
    if (!m_server->listen(QHostAddress::Any, port)) {
        qCritical(logSecs) << "SECS Server failed to listen on port" << port
                           << ":" << m_server->errorString();
        return false;
    }
    qInfo(logSecs) << "SECS Server listening on port" << m_server->serverPort()
                   << "- HSMS State:" << SecsProtocol::hsmsStateToString(m_hsmsState);
    return true;
}

quint16 SecsServer::getBoundPort() const {
    return m_server ? m_server->serverPort() : 0;
}

void SecsServer::stop() {
    if (m_clientSocket) {
        m_clientSocket->disconnectFromHost();
        m_clientSocket = nullptr;
    }
    if (m_server && m_server->isListening()) {
        m_server->close();
    }
    setHsmsState(HsmsState::NotConnected);
}

void SecsServer::setHsmsState(HsmsState newState) {
    if (m_hsmsState != newState) {
        qInfo(logSecs) << "SECS Server HSMS transition:"
                       << SecsProtocol::hsmsStateToString(m_hsmsState)
                       << "->" << SecsProtocol::hsmsStateToString(newState);
        m_hsmsState = newState;
        emit hsmsStateChanged(newState);
    }
}

void SecsServer::onNewConnection() {
    QTcpSocket *socket = m_server->nextPendingConnection();
    if (!socket) return;

    if (m_clientSocket) {
        qWarning(logSecs) << "SECS Server: multiple host connections rejected (single-session HSMS-SS).";
        socket->disconnectFromHost();
        socket->deleteLater();
        return;
    }

    m_clientSocket = socket;
    m_inBuffer.clear();

    connect(m_clientSocket, &QTcpSocket::readyRead, this, &SecsServer::onReadyRead);
    connect(m_clientSocket, &QTcpSocket::disconnected, this, &SecsServer::onClientDisconnected);

    qInfo(logSecs) << "SECS Server: TCP connection established from"
                   << m_clientSocket->peerAddress().toString() << ":" << m_clientSocket->peerPort();

    setHsmsState(HsmsState::Connected);
}

void SecsServer::onClientDisconnected() {
    qInfo(logSecs) << "SECS Server: Host TCP socket disconnected.";
    m_clientSocket = nullptr;
    m_inBuffer.clear();
    setHsmsState(HsmsState::NotConnected);
}

void SecsServer::onReadyRead() {
    if (!m_clientSocket) return;

    m_inBuffer.append(m_clientSocket->readAll());

    while (m_inBuffer.size() >= 4) {
        const auto *data = reinterpret_cast<const uint8_t*>(m_inBuffer.constData());
        uint32_t packetLen = (static_cast<uint32_t>(data[0]) << 24) |
                             (static_cast<uint32_t>(data[1]) << 16) |
                             (static_cast<uint32_t>(data[2]) << 8)  |
                             (static_cast<uint32_t>(data[3]));

        if (packetLen < sizeof(HsmsHeader)) {
            qCritical(logSecs) << "Malformed HSMS frame length:" << packetLen;
            m_clientSocket->disconnectFromHost();
            return;
        }

        if (m_inBuffer.size() < static_cast<qsizetype>(4 + packetLen)) {
            // Incomplete packet in buffer; wait for next readyRead
            break;
        }

        // Complete packet received
        HsmsHeader hdr;
        std::memcpy(&hdr, m_inBuffer.constData() + 4, sizeof(HsmsHeader));

        QByteArray body = m_inBuffer.mid(4 + sizeof(HsmsHeader), packetLen - sizeof(HsmsHeader));
        m_inBuffer.remove(0, 4 + packetLen);

        processPacket(hdr, body);
    }
}

void SecsServer::sendPacket(const HsmsHeader &hdr, const QByteArray &body) {
    if (!m_clientSocket || m_clientSocket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    QByteArray packet = SecsProtocol::buildHsmsPacket(hdr, body);
    m_clientSocket->write(packet);
    m_clientSocket->flush();
}

void SecsServer::processPacket(const HsmsHeader &hdr, const QByteArray &body) {
    auto sType = static_cast<HsmsSType>(hdr.sType);

    if (sType == HsmsSType::SelectReq) {
        qInfo(logSecs) << "SECS Server: Received Select.req (SystemBytes:" << hdr.systemBytes << ")";

        // Reply with Select.rsp (Status 0 = Success)
        HsmsHeader rspHdr = hdr;
        rspHdr.sType = static_cast<uint8_t>(HsmsSType::SelectRsp);

        sendPacket(rspHdr);
        setHsmsState(HsmsState::Selected);
        qInfo(logSecs) << "SECS Server: Sent Select.rsp -> Session is now SELECTED.";
        return;
    }

    if (sType == HsmsSType::LinktestReq) {
        qInfo(logSecs) << "SECS Server: Received Linktest.req";
        HsmsHeader rspHdr = hdr;
        rspHdr.sType = static_cast<uint8_t>(HsmsSType::LinktestRsp);
        sendPacket(rspHdr);
        return;
    }

    if (sType == HsmsSType::SeparateReq) {
        qInfo(logSecs) << "SECS Server: Received Separate.req from host.";
        stop();
        return;
    }

    if (sType == HsmsSType::DataMessage) {
        uint8_t stream = hdr.stream & 0x7F; // Mask W-bit
        uint8_t function = hdr.function;

        qInfo(logSecs) << "SECS Server: Received SECS-II S" << stream << "F" << function
                       << "(Payload bytes:" << body.size() << ")";

        // Stream 1, Function 1: Are You There Request (S1F1)
        if (stream == 1 && function == 1) {
            emit s1f1Received();

            // S1F2: Online Data Response (Model name and software revision)
            QByteArray s1f2Body = "MDLN:EQUIP_SIM_2000;SOFTREV:1.0.0";
            HsmsHeader rspHdr = hdr;
            rspHdr.stream = 1;
            rspHdr.function = 2; // S1F2
            rspHdr.sType = 0;    // Data message

            sendPacket(rspHdr, s1f2Body);
            emit s1f2Sent();
            qInfo(logSecs) << "SECS Server: Sent S1F2 Online Data:" << s1f2Body;
        }
    }
}
