/**
 * @file SecsProtocol.h
 * @brief HSMS (SEMI E37) and SECS-II (SEMI E5) message definitions and framing.
 */

#pragma once

#include <cstdint>
#include <QByteArray>
#include <QString>

/**
 * @enum HsmsState
 * @brief High-Speed SECS Message Services (HSMS-SS) session connection states.
 */
enum class HsmsState {
    NotConnected, ///< No TCP connection active
    Connected,    ///< TCP socket open; awaiting Select.req
    Selected      ///< Select.req accepted; ready for SECS-II data transactions
};

/**
 * @enum HsmsSType
 * @brief HSMS Session Type definitions (Byte 5 of HSMS Header).
 */
enum class HsmsSType : uint8_t {
    DataMessage = 0, ///< Stream/Function SECS-II data message
    SelectReq   = 1, ///< Session selection request
    SelectRsp   = 2, ///< Session selection response
    DeselectReq = 3,
    DeselectRsp = 4,
    LinktestReq = 5, ///< Heartbeat link verification request
    LinktestRsp = 6, ///< Heartbeat link verification response
    SeparateReq = 9  ///< Graceful session termination
};

/**
 * @struct HsmsHeader
 * @brief 10-byte standard HSMS packet header.
 */
#pragma pack(push, 1)
struct HsmsHeader {
    uint16_t sessionId{0};   ///< Device / Session ID (Big Endian)
    uint8_t  stream{0};      ///< SECS Stream (MSB is W-bit: 0x80)
    uint8_t  function{0};    ///< SECS Function
    uint8_t  pType{0};       ///< Presentation Type (0 = SECS-II)
    uint8_t  sType{0};       ///< Session / Control Type
    uint32_t systemBytes{0}; ///< Unique transaction correlation ID (Big Endian)
};
#pragma pack(pop)

namespace SecsProtocol {

inline const char* hsmsStateToString(HsmsState state) noexcept {
    switch (state) {
    case HsmsState::NotConnected: return "NOT CONNECTED";
    case HsmsState::Connected:    return "CONNECTED";
    case HsmsState::Selected:     return "SELECTED";
    }
    return "UNKNOWN";
}

/**
 * @brief Encapsulates an HSMS frame with a 4-byte big-endian length prefix.
 */
inline QByteArray buildHsmsPacket(const HsmsHeader &hdr, const QByteArray &body = QByteArray()) {
    uint32_t length = static_cast<uint32_t>(sizeof(HsmsHeader) + body.size());

    QByteArray packet;
    packet.resize(4 + sizeof(HsmsHeader) + body.size());

    // 4-byte Big-Endian length
    packet[0] = static_cast<char>((length >> 24) & 0xFF);
    packet[1] = static_cast<char>((length >> 16) & 0xFF);
    packet[2] = static_cast<char>((length >> 8) & 0xFF);
    packet[3] = static_cast<char>(length & 0xFF);

    // 10-byte Header
    std::memcpy(packet.data() + 4, &hdr, sizeof(HsmsHeader));

    // Body payload
    if (!body.isEmpty()) {
        std::memcpy(packet.data() + 4 + sizeof(HsmsHeader), body.constData(), body.size());
    }

    return packet;
}

} // namespace SecsProtocol
