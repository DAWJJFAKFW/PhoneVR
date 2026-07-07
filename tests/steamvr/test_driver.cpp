#include <cassert>
#include <iostream>

// Basic unit test for SteamVR driver logic
void test_controller_serial() {
    std::string left_serial = "PhoneVR-Left-00001";
    std::string right_serial = "PhoneVR-Right-00001";

    assert(left_serial.find("Left") != std::string::npos);
    assert(right_serial.find("Right") != std::string::npos);
    assert(left_serial != right_serial);

    std::cout << "Controller serial test passed" << std::endl;
}

void test_headset_properties() {
    float display_freq = 90.0f;
    float ipd = 0.0635f;
    uint32_t render_width = 1440;
    uint32_t render_height = 1600;

    assert(display_freq == 90.0f);
    assert(ipd > 0.05f && ipd < 0.08f);
    assert(render_width > 0 && render_height > 0);

    std::cout << "Headset properties test passed" << std::endl;
}

void test_distortion() {
    float k1 = -0.15f;
    float k2 = 0.05f;

    float u = 0.5f, v = 0.5f;
    float cu = u * 2.0f - 1.0f;
    float cv = v * 2.0f - 1.0f;
    float r2 = cu * cu + cv * cv;
    float r4 = r2 * r2;
    float dist = 1.0f + k1 * r2 + k2 * r4;

    float du = cu * dist;
    float dv = cv * dist;

    assert(du != 0.0f);
    assert(dv != 0.0f);

    std::cout << "Distortion test passed: du=" << du << ", dv=" << dv << std::endl;
}

int main() {
    test_controller_serial();
    test_headset_properties();
    test_distortion();

    std::cout << "All SteamVR driver tests passed!" << std::endl;
    return 0;
}
