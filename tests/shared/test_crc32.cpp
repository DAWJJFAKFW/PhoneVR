#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include "../../shared/cpp/PhoneVRNetwork.h"

void test_crc32() {
    const char* test_data = "PhoneVR Test Data";
    uint32_t crc = phonevr::crc32_compute(
        reinterpret_cast<const uint8_t*>(test_data),
        std::strlen(test_data));
    assert(crc != 0);
    std::cout << "CRC32 test passed: " << crc << std::endl;
}

void test_packet_build_parse() {
    const char* payload = "Hello PhoneVR";
    size_t payload_size = std::strlen(payload);

    auto packet = phonevr::build_packet(
        phonevr::PacketType::Control,
        reinterpret_cast<const uint8_t*>(payload),
        payload_size, 1, 0);

    assert(packet.size() == phonevr::PacketHeader::encoded_size() + payload_size);

    auto result = phonevr::parse_packet(packet.data(), packet.size());
    assert(result.has_value());
    assert(result->first.sequence == 1);
    assert(result->first.type == static_cast<uint16_t>(phonevr::PacketType::Control));
    assert(result->second.size() == payload_size);
    assert(std::memcmp(result->second.data(), payload, payload_size) == 0);

    std::cout << "Packet build/parse test passed" << std::endl;
}

void test_crc_validation() {
    std::vector<uint8_t> packet(phonevr::PacketHeader::encoded_size() + 4);
    auto result = phonevr::parse_packet(packet.data(), packet.size());
    assert(result.has_value());

    std::cout << "CRC validation test passed" << std::endl;
}

int main() {
    test_crc32();
    test_packet_build_parse();
    test_crc_validation();

    std::cout << "All shared tests passed!" << std::endl;
    return 0;
}
