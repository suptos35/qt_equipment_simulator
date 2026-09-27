/**
 * @file SecsClient.h
 * @brief Host-side SECS/GEM HSMS client for automated equipment testing and host control.
 */

#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QByteArray>

#include "SecsProtocol.h"

/**
 * @class SecsClient
 * @brief Connects to equipment server, initiates HSMS selection, and sends SECS-II requests.
 */
class SecsClient : public QObject {
    Q_OBJECT

public:
    explicit SecsClient(QObject *parent = nullptr);
    ~SecsClient() override;

    bool connectToEquipment(const QString &host, quint16 port, int timeoutMs = 3000);
    void disconnectFromEquipment();

    void sendSelectReq();
    void sendS1F1();

    [[nodiscard]] HsmsState getHsmsState() const noexcept { return m_hsmsState; }

signals:
    void hsmsStateChanged(HsmsState newState);
    void selectRspReceived();
    void s1f2Received(const QString &payload);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();

private:
    void setHsmsState(HsmsState state);
    void processPacket(const HsmsHeader &hdr, const QByteArray &body);
    void sendPacket(const HsmsHeader &hdr, const QByteArray &body = QByteArray());

    QTcpSocket *m_socket{nullptr};
    HsmsState m_hsmsState{HsmsState::NotConnected};
    QByteArray m_inBuffer;
    uint32_t m_systemBytesCounter{100};
};
