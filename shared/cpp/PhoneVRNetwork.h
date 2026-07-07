#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <optional>
#include <algorithm>

namespace phonevr {

#pragma pack(push, 1)
struct PacketHeader {
    uint32_t magic = 0x50485256;
    uint32_t sequence = 0;
    uint16_t type = 0;
    uint16_t flags = 0;
    uint32_t payload_size = 0;
    uint32_t crc32 = 0;

    static constexpr size_t encoded_size() { return 20; }

    bool validate() const {
        return magic == 0x50485256;
    }
};
#pragma pack(pop)

enum class PacketType : uint16_t {
    Tracking = 0x0001,
    Video = 0x0002,
    Control = 0x0003,
    Audio = 0x0004
};

enum PacketFlag : uint16_t {
    PacketFlag_Reliable = 0x0001,
    PacketFlag_Keyframe = 0x0002,
    PacketFlag_Fragment = 0x0004,
    PacketFlag_FinalFragment = 0x0008,
    PacketFlag_Encrypted = 0x0010,
    PacketFlag_Compressed = 0x0020
};

// MARK: - CRC32

inline uint32_t crc32_compute(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
    }
    return crc ^ 0xFFFFFFFF;
}

// MARK: - Packet Builder

inline std::vector<uint8_t> build_packet(PacketType type, const uint8_t* payload, size_t payload_size,
                                          uint32_t sequence, uint16_t flags = 0) {
    PacketHeader header;
    header.sequence = sequence;
    header.type = static_cast<uint16_t>(type);
    header.flags = flags;
    header.payload_size = static_cast<uint32_t>(payload_size);
    header.crc32 = crc32_compute(payload, payload_size);

    std::vector<uint8_t> packet(PacketHeader::encoded_size() + payload_size);
    std::memcpy(packet.data(), &header, PacketHeader::encoded_size());
    if (payload_size > 0) {
        std::memcpy(packet.data() + PacketHeader::encoded_size(), payload, payload_size);
    }
    return packet;
}

inline std::optional<std::pair<PacketHeader, std::vector<uint8_t>>> parse_packet(const uint8_t* data, size_t size) {
    if (size < PacketHeader::encoded_size()) return std::nullopt;

    PacketHeader header;
    std::memcpy(&header, data, PacketHeader::encoded_size());
    if (!header.validate()) return std::nullopt;

    if (size < PacketHeader::encoded_size() + header.payload_size) return std::nullopt;

    const uint8_t* payload = data + PacketHeader::encoded_size();
    uint32_t computed_crc = crc32_compute(payload, header.payload_size);
    if (computed_crc != header.crc32) return std::nullopt;

    std::vector<uint8_t> payload_vec(payload, payload + header.payload_size);
    return std::make_pair(header, std::move(payload_vec));
}

// MARK: - Encryption (AES-GCM via BCrypt/CNG)

class SecureChannel {
public:
    SecureChannel();
    ~SecureChannel();

    SecureChannel(const SecureChannel&) = delete;
    SecureChannel& operator=(const SecureChannel&) = delete;

    std::vector<uint8_t> initialize_session();
    void set_session_key(const uint8_t* key, size_t key_size);

    std::vector<uint8_t> encrypt(const uint8_t* data, size_t size);
    std::vector<uint8_t> decrypt(const uint8_t* data, size_t size);

    uint32_t next_send_sequence() { return ++send_sequence_; }

private:
    static constexpr size_t KEY_SIZE = 32;
    static constexpr size_t NONCE_SIZE = 12;
    static constexpr size_t TAG_SIZE = 16;

    bool has_key_ = false;
    uint8_t key_[KEY_SIZE];
    uint32_t send_sequence_ = 0;
    uint32_t recv_sequence_ = 0;

    void* aes_handle_ = nullptr;
};

} // namespace phonevr
