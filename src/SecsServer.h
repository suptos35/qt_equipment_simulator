/**
 * @file SecsServer.h
 * @brief Semiconductor equipment HSMS server endpoint.
 */

#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>

#include "SecsProtocol.h"

/**
 * @class SecsServer
 * @brief Manages incoming host connections and handles HSMS state and SECS-II exchanges.
 */
class SecsServer : public QObject {
    Q_OBJECT

public:
    explicit SecsServer(QObject *parent = nullptr);
    ~SecsServer() override;

    bool startListening(quint16 port = 5000);
    void stop();

    [[nodiscard]] HsmsState getHsmsState() const noexcept { return m_hsmsState; }
    [[nodiscard]] quint16 getBoundPort() const;

signals:
    void hsmsStateChanged(HsmsState newState);
    void s1f1Received();
    void s1f2Sent();

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();

private:
    void processPacket(const HsmsHeader &hdr, const QByteArray &body);
    void setHsmsState(HsmsState newState);
    void sendPacket(const HsmsHeader &hdr, const QByteArray &body = QByteArray());

    QTcpServer *m_server{nullptr};
    QTcpSocket *m_clientSocket{nullptr};
    HsmsState m_hsmsState{HsmsState::NotConnected};
    QByteArray m_inBuffer;
};
