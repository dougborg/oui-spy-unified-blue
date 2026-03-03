#include <unity.h>
#include <cstring>
#include <string>

#include "hal/buzzer_logic.h"
#include "hal/neopixel_logic.h"
#include "modules/detector_logic.h"
#include "modules/flockyou_logic.h"

extern "C" {
#include "opendroneid.h"
#include "odid_wifi.h"
}

static void seed_basic_uas(ODID_UAS_Data* uas, const char* uas_id) {
    odid_initUasData(uas);
    uas->BasicIDValid[0] = 1;
    uas->BasicID[0].IDType = ODID_IDTYPE_SERIAL_NUMBER;
    uas->BasicID[0].UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
    strncpy(uas->BasicID[0].UASID, uas_id, ODID_ID_SIZE);
}

void test_basic_id_encode_decode_roundtrip() {
    ODID_BasicID_data input;
    odid_initBasicIDData(&input);
    input.IDType = ODID_IDTYPE_SERIAL_NUMBER;
    input.UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
    strncpy(input.UASID, "UAS-UNIT-TEST-0001", ODID_ID_SIZE);

    ODID_BasicID_encoded encoded;
    TEST_ASSERT_EQUAL_INT(ODID_SUCCESS, encodeBasicIDMessage(&encoded, &input));
    TEST_ASSERT_EQUAL_INT(ODID_MESSAGETYPE_BASIC_ID, encoded.MessageType);

    ODID_BasicID_data decoded;
    odid_initBasicIDData(&decoded);
    TEST_ASSERT_EQUAL_INT(ODID_SUCCESS, decodeBasicIDMessage(&decoded, &encoded));
    TEST_ASSERT_EQUAL_INT(input.IDType, decoded.IDType);
    TEST_ASSERT_EQUAL_INT(input.UAType, decoded.UAType);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(input.UASID, decoded.UASID, ODID_ID_SIZE);
}

void test_location_encode_rejects_invalid_direction() {
    ODID_Location_data location;
    odid_initLocationData(&location);
    location.Direction = 500.0f;

    ODID_Location_encoded encoded;
    TEST_ASSERT_EQUAL_INT(ODID_FAIL, encodeLocationMessage(&encoded, &location));
}

void test_message_pack_decodes_and_sets_basic_id_valid() {
    ODID_BasicID_data basic;
    odid_initBasicIDData(&basic);
    basic.IDType = ODID_IDTYPE_SERIAL_NUMBER;
    basic.UAType = ODID_UATYPE_HELICOPTER_OR_MULTIROTOR;
    strncpy(basic.UASID, "PACK-ROUNDTRIP-0001", ODID_ID_SIZE);

    ODID_BasicID_encoded basic_encoded;
    TEST_ASSERT_EQUAL_INT(ODID_SUCCESS, encodeBasicIDMessage(&basic_encoded, &basic));

    ODID_MessagePack_data pack_data;
    odid_initMessagePackData(&pack_data);
    pack_data.MsgPackSize = 1;
    memcpy(&pack_data.Messages[0], &basic_encoded, ODID_MESSAGE_SIZE);

    ODID_MessagePack_encoded pack_encoded;
    TEST_ASSERT_EQUAL_INT(ODID_SUCCESS, encodeMessagePack(&pack_encoded, &pack_data));

    ODID_UAS_Data uas;
    odid_initUasData(&uas);
    TEST_ASSERT_EQUAL_INT(ODID_SUCCESS, decodeMessagePack(&uas, &pack_encoded));
    TEST_ASSERT_EQUAL_UINT8(1, uas.BasicIDValid[0]);
    TEST_ASSERT_EQUAL_INT(ODID_IDTYPE_SERIAL_NUMBER, uas.BasicID[0].IDType);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(basic.UASID, uas.BasicID[0].UASID, ODID_ID_SIZE);
}

void test_wifi_pack_build_and_process_roundtrip() {
    ODID_UAS_Data source;
    seed_basic_uas(&source, "WIFI-PACK-ROUNDTRIP");

    uint8_t pack[sizeof(ODID_MessagePack_encoded)] = {0};
    int pack_len = odid_message_build_pack(&source, pack, sizeof(pack));
    TEST_ASSERT_GREATER_THAN_INT(0, pack_len);

    ODID_UAS_Data decoded;
    int processed_len = odid_message_process_pack(&decoded, pack, (size_t)pack_len);
    TEST_ASSERT_EQUAL_INT(pack_len, processed_len);
    TEST_ASSERT_EQUAL_UINT8(1, decoded.BasicIDValid[0]);
    TEST_ASSERT_EQUAL_CHAR_ARRAY(source.BasicID[0].UASID, decoded.BasicID[0].UASID, ODID_ID_SIZE);
}

void test_wifi_nan_action_frame_roundtrip() {
    ODID_UAS_Data source;
    seed_basic_uas(&source, "NAN-ACTION-ROUNDTRP");

    char tx_mac[6] = {0x02, 0x11, 0x22, 0x33, 0x44, 0x55};
    uint8_t frame[1024] = {0};

    int frame_len = odid_wifi_build_message_pack_nan_action_frame(&source, tx_mac, 7, frame,
                                                                   sizeof(frame));
    TEST_ASSERT_GREATER_THAN_INT(0, frame_len);

    ODID_UAS_Data received;
    char rx_mac[6] = {0};
    int ret = odid_wifi_receive_message_pack_nan_action_frame(&received, rx_mac, frame,
                                                              (size_t)frame_len);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_MEMORY(tx_mac, rx_mac, sizeof(rx_mac));
    TEST_ASSERT_EQUAL_UINT8(1, received.BasicIDValid[0]);
}

void test_wifi_beacon_builder_rejects_invalid_ssid_len() {
    ODID_UAS_Data source;
    seed_basic_uas(&source, "BEACON-SSID-INVALID");

    char mac[6] = {0x02, 0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t frame[1024] = {0};
    const char* ssid = "invalid-ssid";
    int ret = odid_wifi_build_message_pack_beacon_frame(&source, mac, ssid, 33, 100, 1, frame,
                                                         sizeof(frame));
    TEST_ASSERT_LESS_THAN_INT(0, ret);
}

void test_wifi_process_pack_rejects_truncated_buffer() {
    ODID_UAS_Data source;
    seed_basic_uas(&source, "TRUNC-PACK-CHECK-01");

    uint8_t pack[sizeof(ODID_MessagePack_encoded)] = {0};
    int pack_len = odid_message_build_pack(&source, pack, sizeof(pack));
    TEST_ASSERT_GREATER_THAN_INT(0, pack_len);

    ODID_UAS_Data decoded;
    int ret = odid_message_process_pack(&decoded, pack, (size_t)(pack_len - 1));
    TEST_ASSERT_EQUAL_INT(-12, ret);  // -ENOMEM
}

void test_wifi_process_pack_rejects_invalid_single_message_size() {
    ODID_UAS_Data source;
    seed_basic_uas(&source, "BAD-SIZE-PACK-0001");

    uint8_t pack[sizeof(ODID_MessagePack_encoded)] = {0};
    int pack_len = odid_message_build_pack(&source, pack, sizeof(pack));
    TEST_ASSERT_GREATER_THAN_INT(0, pack_len);

    auto* msg_pack = reinterpret_cast<ODID_MessagePack_encoded*>(pack);
    msg_pack->SingleMessageSize = static_cast<uint8_t>(ODID_MESSAGE_SIZE - 1);

    ODID_UAS_Data decoded;
    int ret = odid_message_process_pack(&decoded, pack, (size_t)pack_len);
    TEST_ASSERT_EQUAL_INT(-1, ret);
}

void test_wifi_nan_action_frame_rejects_wrong_destination() {
    ODID_UAS_Data source;
    seed_basic_uas(&source, "NAN-DST-CHECK-0001");

    char tx_mac[6] = {0x02, 0x10, 0x20, 0x30, 0x40, 0x50};
    uint8_t frame[1024] = {0};
    int frame_len = odid_wifi_build_message_pack_nan_action_frame(&source, tx_mac, 2, frame,
                                                                   sizeof(frame));
    TEST_ASSERT_GREATER_THAN_INT(0, frame_len);

    auto* mgmt = reinterpret_cast<ieee80211_mgmt*>(frame);
    mgmt->da[0] ^= 0x01;

    ODID_UAS_Data received;
    char rx_mac[6] = {0};
    int ret = odid_wifi_receive_message_pack_nan_action_frame(&received, rx_mac, frame,
                                                              (size_t)frame_len);
    TEST_ASSERT_EQUAL_INT(-22, ret);  // -EINVAL
}

void test_wifi_nan_action_frame_rejects_bad_service_info_length() {
    ODID_UAS_Data source;
    seed_basic_uas(&source, "NAN-SILEN-CHECK001");

    char tx_mac[6] = {0x02, (char)0xAA, (char)0xBB, (char)0xCC, (char)0xDD, (char)0xEE};
    uint8_t frame[1024] = {0};
    int frame_len = odid_wifi_build_message_pack_nan_action_frame(&source, tx_mac, 3, frame,
                                                                   sizeof(frame));
    TEST_ASSERT_GREATER_THAN_INT(0, frame_len);

    size_t offset = sizeof(ieee80211_mgmt) + sizeof(nan_service_discovery);
    auto* nsda = reinterpret_cast<nan_service_descriptor_attribute*>(frame + offset);
    nsda->service_info_length = static_cast<uint8_t>(nsda->service_info_length + 1);

    ODID_UAS_Data received;
    char rx_mac[6] = {0};
    int ret = odid_wifi_receive_message_pack_nan_action_frame(&received, rx_mac, frame,
                                                              (size_t)frame_len);
    TEST_ASSERT_EQUAL_INT(-22, ret);  // -EINVAL
}

void test_encode_message_pack_rejects_packed_message_inside_pack() {
    ODID_MessagePack_data input;
    odid_initMessagePackData(&input);
    input.MsgPackSize = 1;
    input.Messages[0].rawData[0] = static_cast<uint8_t>(ODID_MESSAGETYPE_PACKED << 4);

    ODID_MessagePack_encoded encoded;
    TEST_ASSERT_EQUAL_INT(ODID_FAIL, encodeMessagePack(&encoded, &input));
}

void test_decode_open_drone_id_rejects_invalid_message_type() {
    ODID_UAS_Data uas;
    odid_initUasData(&uas);

    uint8_t invalid_msg[ODID_MESSAGE_SIZE] = {0};
    invalid_msg[0] = 0xE0;

    TEST_ASSERT_EQUAL_INT(ODID_MESSAGETYPE_INVALID, decodeOpenDroneID(&uas, invalid_msg));
}

void test_detector_logic_normalize_and_validate() {
    std::string normalized = detector_logic::normalizeMac("AA-BB-CC DD:EE:FF");
    TEST_ASSERT_EQUAL_STRING("aa:bb:ccdd:ee:ff", normalized.c_str());

    TEST_ASSERT_TRUE(detector_logic::isValidMac("aa:bb:cc"));
    TEST_ASSERT_TRUE(detector_logic::isValidMac("AA-BB-CC-DD-EE-FF"));
    TEST_ASSERT_FALSE(detector_logic::isValidMac("not-a-mac"));
}

void test_detector_logic_filter_matching() {
    TEST_ASSERT_TRUE(detector_logic::matchesFilter("AA:BB:CC:11:22:33", "aa:bb:cc", false));
    TEST_ASSERT_TRUE(
        detector_logic::matchesFilter("AA:BB:CC:11:22:33", "aa-bb-cc-11-22-33", true));
    TEST_ASSERT_FALSE(
        detector_logic::matchesFilter("AA:BB:CC:11:22:33", "aa:bb:dd:11:22:33", true));
}

void test_flockyou_logic_matching_helpers() {
    const char* macPrefixes[] = {"58:8e:81", "cc:cc:cc"};
    uint8_t mac[6] = {0x58, 0x8e, 0x81, 0x01, 0x02, 0x03};
    TEST_ASSERT_TRUE(flockyou_logic::macMatchesPrefixes(mac, macPrefixes, 2));

    const char* names[] = {"Flock", "Penguin"};
    TEST_ASSERT_TRUE(flockyou_logic::nameMatchesPatterns("my flock camera", names, 2));
    TEST_ASSERT_FALSE(flockyou_logic::nameMatchesPatterns("random", names, 2));

    const uint16_t mfr[] = {0x09C8, 0x1234};
    TEST_ASSERT_TRUE(flockyou_logic::manufacturerMatches(0x09C8, mfr, 2));
    TEST_ASSERT_FALSE(flockyou_logic::manufacturerMatches(0xFFFF, mfr, 2));

    const char* uuids[] = {"00003100-0000-1000-8000-00805f9b34fb"};
    TEST_ASSERT_TRUE(flockyou_logic::uuidEqualsAny("00003100-0000-1000-8000-00805F9B34FB", uuids,
                                                   1));
}

void test_flockyou_logic_fw_estimation() {
    TEST_ASSERT_EQUAL_STRING("1.1.x", flockyou_logic::estimateRavenFirmware(false, true, false));
    TEST_ASSERT_EQUAL_STRING("1.2.x", flockyou_logic::estimateRavenFirmware(true, false, false));
    TEST_ASSERT_EQUAL_STRING("1.3.x", flockyou_logic::estimateRavenFirmware(true, false, true));
    TEST_ASSERT_EQUAL_STRING("?", flockyou_logic::estimateRavenFirmware(false, false, false));
}

void test_buzzer_logic_proximity_interval() {
    TEST_ASSERT_EQUAL_INT(10, hal::buzzer_logic::calcProximityIntervalMs(-25));
    TEST_ASSERT_EQUAL_INT(200, hal::buzzer_logic::calcProximityIntervalMs(-65));
    TEST_ASSERT_EQUAL_INT(3000, hal::buzzer_logic::calcProximityIntervalMs(-95));
}

void test_neopixel_logic_breathing_and_flash() {
    auto up = hal::neopixel_logic::nextBreathing({0.99f, true});
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, up.brightness);
    TEST_ASSERT_FALSE(up.increasing);

    auto down = hal::neopixel_logic::nextBreathing({0.1f, false});
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.1f, down.brightness);
    TEST_ASSERT_TRUE(down.increasing);

    auto frame = hal::neopixel_logic::computeFlashFrame(260, 200, 12);
    TEST_ASSERT_TRUE(frame.active);
    TEST_ASSERT_EQUAL_INT(1, frame.frameIndex);

    auto done = hal::neopixel_logic::computeFlashFrame(760, 200, 12);
    TEST_ASSERT_FALSE(done.active);
}

void test_native_arithmetic_sanity() {
    TEST_ASSERT_EQUAL_INT(4, 2 + 2);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_basic_id_encode_decode_roundtrip);
    RUN_TEST(test_location_encode_rejects_invalid_direction);
    RUN_TEST(test_message_pack_decodes_and_sets_basic_id_valid);
    RUN_TEST(test_wifi_pack_build_and_process_roundtrip);
    RUN_TEST(test_wifi_nan_action_frame_roundtrip);
    RUN_TEST(test_wifi_beacon_builder_rejects_invalid_ssid_len);
    RUN_TEST(test_wifi_process_pack_rejects_truncated_buffer);
    RUN_TEST(test_wifi_process_pack_rejects_invalid_single_message_size);
    RUN_TEST(test_wifi_nan_action_frame_rejects_wrong_destination);
    RUN_TEST(test_wifi_nan_action_frame_rejects_bad_service_info_length);
    RUN_TEST(test_encode_message_pack_rejects_packed_message_inside_pack);
    RUN_TEST(test_decode_open_drone_id_rejects_invalid_message_type);
    RUN_TEST(test_detector_logic_normalize_and_validate);
    RUN_TEST(test_detector_logic_filter_matching);
    RUN_TEST(test_flockyou_logic_matching_helpers);
    RUN_TEST(test_flockyou_logic_fw_estimation);
    RUN_TEST(test_buzzer_logic_proximity_interval);
    RUN_TEST(test_neopixel_logic_breathing_and_flash);
    RUN_TEST(test_native_arithmetic_sanity);
    return UNITY_END();
}
