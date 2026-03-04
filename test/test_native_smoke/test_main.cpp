#include <unity.h>
#include <cstring>
#include <string>

#include "hal/buzzer_logic.h"
#include "hal/neopixel_logic.h"
#include "modules/detector_logic.h"
#include "modules/flockyou_logic.h"
#include "modules/foxhunter_logic.h"
#include "modules/skyspy_logic.h"

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

// ============================================================================
// Detector Cooldown Logic Tests
// ============================================================================

void test_detector_cooldown_in_cooldown() {
    auto r = detector_logic::evaluateCooldown(true, 5000, 1000, 3000);
    TEST_ASSERT_EQUAL_INT((int)detector_logic::DetectionType::IN_COOLDOWN, (int)r.type);
}

void test_detector_cooldown_expired_re30s() {
    // Cooldown expired, last seen 35s ago
    auto r = detector_logic::evaluateCooldown(true, 2000, 1000, 36000);
    TEST_ASSERT_EQUAL_INT((int)detector_logic::DetectionType::RE_30S, (int)r.type);
    TEST_ASSERT_EQUAL_UINT32(10000, r.newCooldownMs);
}

void test_detector_cooldown_re3s() {
    // No cooldown, last seen 5s ago
    auto r = detector_logic::evaluateCooldown(false, 0, 1000, 6000);
    TEST_ASSERT_EQUAL_INT((int)detector_logic::DetectionType::RE_3S, (int)r.type);
    TEST_ASSERT_EQUAL_UINT32(3000, r.newCooldownMs);
}

void test_detector_cooldown_too_recent() {
    // Last seen 1s ago
    auto r = detector_logic::evaluateCooldown(false, 0, 5000, 6000);
    TEST_ASSERT_EQUAL_INT((int)detector_logic::DetectionType::TOO_RECENT, (int)r.type);
}

// ============================================================================
// Foxhunter Proximity Logic Tests
// ============================================================================

void test_foxhunter_prox_not_detected() {
    auto r = foxhunter_logic::evaluateProxTick(false, 10000, 5000, 0);
    TEST_ASSERT_EQUAL_INT((int)foxhunter_logic::ProxEvent::NONE, (int)r.event);
    TEST_ASSERT_FALSE(r.shouldBeep);
    TEST_ASSERT_FALSE(r.shouldPrintRSSI);
}

void test_foxhunter_prox_in_range_beep() {
    // Target detected 2s ago, last RSSI print 3s ago
    auto r = foxhunter_logic::evaluateProxTick(true, 10000, 8000, 7000);
    TEST_ASSERT_EQUAL_INT((int)foxhunter_logic::ProxEvent::NONE, (int)r.event);
    TEST_ASSERT_TRUE(r.shouldBeep);
    TEST_ASSERT_TRUE(r.shouldPrintRSSI);
}

void test_foxhunter_prox_in_range_no_print() {
    // Target detected 1s ago, last RSSI print 1s ago (too recent)
    auto r = foxhunter_logic::evaluateProxTick(true, 10000, 9000, 9000);
    TEST_ASSERT_TRUE(r.shouldBeep);
    TEST_ASSERT_FALSE(r.shouldPrintRSSI);
}

void test_foxhunter_prox_target_lost() {
    // Target detected 6s ago (>5s timeout)
    auto r = foxhunter_logic::evaluateProxTick(true, 11000, 5000, 0);
    TEST_ASSERT_EQUAL_INT((int)foxhunter_logic::ProxEvent::TARGET_LOST, (int)r.event);
    TEST_ASSERT_FALSE(r.shouldBeep);
}

void test_foxhunter_target_match_no_match() {
    auto r = foxhunter_logic::evaluateTargetMatch("AA:BB:CC:DD:EE:FF", "11:22:33:44:55:66", false,
                                                   true);
    TEST_ASSERT_EQUAL_INT((int)foxhunter_logic::TargetMatchEvent::NO_MATCH, (int)r);
}

void test_foxhunter_target_match_first_acquisition() {
    auto r = foxhunter_logic::evaluateTargetMatch("AA:BB:CC:DD:EE:FF", "aa:bb:cc:dd:ee:ff", false,
                                                   true);
    TEST_ASSERT_EQUAL_INT((int)foxhunter_logic::TargetMatchEvent::FIRST_ACQUISITION, (int)r);
}

void test_foxhunter_target_match_reacquired() {
    auto r = foxhunter_logic::evaluateTargetMatch("AA:BB:CC:DD:EE:FF", "aa:bb:cc:dd:ee:ff", false,
                                                   false);
    TEST_ASSERT_EQUAL_INT((int)foxhunter_logic::TargetMatchEvent::REACQUIRED, (int)r);
}

void test_foxhunter_target_match_update_existing() {
    auto r = foxhunter_logic::evaluateTargetMatch("AA:BB:CC:DD:EE:FF", "aa:bb:cc:dd:ee:ff", true,
                                                   false);
    TEST_ASSERT_EQUAL_INT((int)foxhunter_logic::TargetMatchEvent::UPDATE_EXISTING, (int)r);
}

void test_foxhunter_target_match_null_target() {
    auto r = foxhunter_logic::evaluateTargetMatch("AA:BB:CC:DD:EE:FF", "", false, true);
    TEST_ASSERT_EQUAL_INT((int)foxhunter_logic::TargetMatchEvent::NO_MATCH, (int)r);
}

// ============================================================================
// SkySpy Logic Tests
// ============================================================================

void test_skyspy_is_odid_vendor_ie() {
    uint8_t ie1[] = {0x90, 0x3a, 0xe6, 0x00, 0x01};
    TEST_ASSERT_TRUE(skyspy_logic::isOdidVendorIE(ie1, 5));

    uint8_t ie2[] = {0xfa, 0x0b, 0xbc, 0x00, 0x01};
    TEST_ASSERT_TRUE(skyspy_logic::isOdidVendorIE(ie2, 5));

    uint8_t ie3[] = {0x00, 0x00, 0x00, 0x00, 0x01};
    TEST_ASSERT_FALSE(skyspy_logic::isOdidVendorIE(ie3, 5));

    TEST_ASSERT_FALSE(skyspy_logic::isOdidVendorIE(ie1, 2)); // too short
}

void test_skyspy_find_odid_beacon_ie() {
    // Build a minimal beacon-like payload with a vendor IE at offset 36
    uint8_t payload[60] = {0};
    payload[0] = 0x80; // beacon frame type
    // IE at offset 36: type=0xdd, len=10, OUI=90:3a:e6, type+counter=2 bytes, then data
    payload[36] = 0xdd;
    payload[37] = 10;           // IE length
    payload[38] = 0x90;         // OUI byte 1
    payload[39] = 0x3a;         // OUI byte 2
    payload[40] = 0xe6;         // OUI byte 3
    payload[41] = 0x01;         // type
    payload[42] = 0x00;         // counter
    payload[43] = 0xAA;         // ODID data starts here (offset 43 = 36+7)

    int odid_len = 0;
    int result = skyspy_logic::findOdidBeaconIE(payload, 48, 36, &odid_len);
    TEST_ASSERT_EQUAL_INT(43, result);
    TEST_ASSERT_GREATER_THAN_INT(0, odid_len);
}

void test_skyspy_find_odid_beacon_ie_not_found() {
    uint8_t payload[50] = {0};
    payload[36] = 0x01; // Not a vendor IE
    payload[37] = 5;

    int odid_len = 0;
    int result = skyspy_logic::findOdidBeaconIE(payload, 43, 36, &odid_len);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_skyspy_slot_allocation_existing() {
    uint8_t slots[4][6] = {{0}};
    uint8_t mac[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    memcpy(slots[2], mac, 6);
    TEST_ASSERT_EQUAL_INT(2, skyspy_logic::findOrAllocateSlot(slots, 4, mac));
}

void test_skyspy_slot_allocation_empty() {
    uint8_t slots[4][6] = {{0}};
    uint8_t mac[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    TEST_ASSERT_EQUAL_INT(0, skyspy_logic::findOrAllocateSlot(slots, 4, mac));
}

void test_skyspy_slot_allocation_evict() {
    uint8_t slots[2][6];
    memset(slots, 0xFF, sizeof(slots)); // All slots non-zero
    uint8_t mac[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    TEST_ASSERT_EQUAL_INT(0, skyspy_logic::findOrAllocateSlot(slots, 2, mac));
}

void test_skyspy_is_odid_ble_payload() {
    uint8_t good[] = {0x00, 0x16, 0xFA, 0xFF, 0x0D, 0x00};
    TEST_ASSERT_TRUE(skyspy_logic::isOdidBlePayload(good, 6));

    uint8_t bad[] = {0x00, 0x16, 0xFA, 0xFF, 0x0E, 0x00};
    TEST_ASSERT_FALSE(skyspy_logic::isOdidBlePayload(bad, 6));

    TEST_ASSERT_FALSE(skyspy_logic::isOdidBlePayload(good, 5)); // too short
}

void test_skyspy_is_nan_action_frame() {
    uint8_t payload[12] = {0};
    // NAN dest at offset 4
    payload[4] = 0x51;
    payload[5] = 0x6f;
    payload[6] = 0x9a;
    payload[7] = 0x01;
    payload[8] = 0x00;
    payload[9] = 0x00;
    TEST_ASSERT_TRUE(skyspy_logic::isNanActionFrame(payload, 12));

    payload[4] = 0x00; // wrong dest
    TEST_ASSERT_FALSE(skyspy_logic::isNanActionFrame(payload, 12));
    TEST_ASSERT_FALSE(skyspy_logic::isNanActionFrame(payload, 5)); // too short
}

void test_skyspy_count_active_uavs() {
    uint8_t macs[4][6] = {{0}};
    uint32_t lastSeen[4] = {0};

    // Slot 0: active (mac set, seen 2s ago)
    macs[0][0] = 0x01;
    lastSeen[0] = 8000;

    // Slot 1: empty
    // Slot 2: stale (mac set, seen 10s ago)
    macs[2][0] = 0x02;
    lastSeen[2] = 0;

    // Slot 3: active
    macs[3][0] = 0x03;
    lastSeen[3] = 5000;

    TEST_ASSERT_EQUAL_INT(2, skyspy_logic::countActiveUavs(lastSeen, macs, 4, 10000, 7000));
}

// ============================================================================
// Flockyou Extended Logic Tests
// ============================================================================

void test_flockyou_find_detection_by_mac() {
    const char* macs[] = {"AA:BB:CC:11:22:33", "DD:EE:FF:44:55:66"};
    TEST_ASSERT_EQUAL_INT(0, flockyou_logic::findDetectionByMac(macs, 2, "aa:bb:cc:11:22:33"));
    TEST_ASSERT_EQUAL_INT(1, flockyou_logic::findDetectionByMac(macs, 2, "DD:EE:FF:44:55:66"));
    TEST_ASSERT_EQUAL_INT(-1, flockyou_logic::findDetectionByMac(macs, 2, "00:00:00:00:00:00"));
    TEST_ASSERT_EQUAL_INT(-1, flockyou_logic::findDetectionByMac(nullptr, 0, "AA:BB:CC:11:22:33"));
}

void test_flockyou_is_out_of_range() {
    TEST_ASSERT_TRUE(flockyou_logic::isOutOfRange(1000, 32000, 30000));
    TEST_ASSERT_FALSE(flockyou_logic::isOutOfRange(1000, 30000, 30000));
    TEST_ASSERT_FALSE(flockyou_logic::isOutOfRange(5000, 10000, 30000));
}

void test_flockyou_should_heartbeat() {
    TEST_ASSERT_TRUE(flockyou_logic::shouldHeartbeat(0, 11000, 10000));
    TEST_ASSERT_FALSE(flockyou_logic::shouldHeartbeat(5000, 10000, 10000));
    TEST_ASSERT_TRUE(flockyou_logic::shouldHeartbeat(0, 10000, 10000));
}

void test_flockyou_sanitize_name_char() {
    TEST_ASSERT_EQUAL_CHAR('a', flockyou_logic::sanitizeNameChar('a'));
    TEST_ASSERT_EQUAL_CHAR('_', flockyou_logic::sanitizeNameChar('"'));
    TEST_ASSERT_EQUAL_CHAR('_', flockyou_logic::sanitizeNameChar('\\'));
    TEST_ASSERT_EQUAL_CHAR(' ', flockyou_logic::sanitizeNameChar(' '));
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
    // Detector cooldown logic
    RUN_TEST(test_detector_cooldown_in_cooldown);
    RUN_TEST(test_detector_cooldown_expired_re30s);
    RUN_TEST(test_detector_cooldown_re3s);
    RUN_TEST(test_detector_cooldown_too_recent);
    // Foxhunter proximity logic
    RUN_TEST(test_foxhunter_prox_not_detected);
    RUN_TEST(test_foxhunter_prox_in_range_beep);
    RUN_TEST(test_foxhunter_prox_in_range_no_print);
    RUN_TEST(test_foxhunter_prox_target_lost);
    RUN_TEST(test_foxhunter_target_match_no_match);
    RUN_TEST(test_foxhunter_target_match_first_acquisition);
    RUN_TEST(test_foxhunter_target_match_reacquired);
    RUN_TEST(test_foxhunter_target_match_update_existing);
    RUN_TEST(test_foxhunter_target_match_null_target);
    // SkySpy logic
    RUN_TEST(test_skyspy_is_odid_vendor_ie);
    RUN_TEST(test_skyspy_find_odid_beacon_ie);
    RUN_TEST(test_skyspy_find_odid_beacon_ie_not_found);
    RUN_TEST(test_skyspy_slot_allocation_existing);
    RUN_TEST(test_skyspy_slot_allocation_empty);
    RUN_TEST(test_skyspy_slot_allocation_evict);
    RUN_TEST(test_skyspy_is_odid_ble_payload);
    RUN_TEST(test_skyspy_is_nan_action_frame);
    RUN_TEST(test_skyspy_count_active_uavs);
    // Flockyou extended logic
    RUN_TEST(test_flockyou_find_detection_by_mac);
    RUN_TEST(test_flockyou_is_out_of_range);
    RUN_TEST(test_flockyou_should_heartbeat);
    RUN_TEST(test_flockyou_sanitize_name_char);
    RUN_TEST(test_native_arithmetic_sanity);
    return UNITY_END();
}
