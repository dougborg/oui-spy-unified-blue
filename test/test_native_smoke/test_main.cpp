#include <unity.h>
#include <cstring>
#include <string>

#include "hal/buzzer_logic.h"
#include "hal/neopixel_logic.h"
#include "modules/detector_logic.h"
#include "modules/flockyou_logic.h"
#include "modules/foxhunter_logic.h"
#include "modules/skyspy_logic.h"
#include "modules/wardriver_logic.h"

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

// ============================================================================
// Wardriver Logic Tests
// ============================================================================

void test_wardriver_auth_mode_all_values() {
    // All ESP32 wifi_auth_mode_t values (0-8) plus unknown
    TEST_ASSERT_EQUAL_STRING("[OPEN]", wardriver_logic::authModeToString(0));
    TEST_ASSERT_EQUAL_STRING("[WEP]", wardriver_logic::authModeToString(1));
    TEST_ASSERT_EQUAL_STRING("[WPA-PSK]", wardriver_logic::authModeToString(2));
    TEST_ASSERT_EQUAL_STRING("[WPA2-PSK]", wardriver_logic::authModeToString(3));
    TEST_ASSERT_EQUAL_STRING("[WPA/WPA2-PSK]", wardriver_logic::authModeToString(4));
    TEST_ASSERT_EQUAL_STRING("[WPA2-ENTERPRISE]", wardriver_logic::authModeToString(5));
    TEST_ASSERT_EQUAL_STRING("[WPA3-PSK]", wardriver_logic::authModeToString(6));
    TEST_ASSERT_EQUAL_STRING("[WPA2/WPA3-PSK]", wardriver_logic::authModeToString(7));
    TEST_ASSERT_EQUAL_STRING("[WAPI-PSK]", wardriver_logic::authModeToString(8));
    TEST_ASSERT_EQUAL_STRING("[UNKNOWN]", wardriver_logic::authModeToString(9));
    TEST_ASSERT_EQUAL_STRING("[UNKNOWN]", wardriver_logic::authModeToString(-1));
    TEST_ASSERT_EQUAL_STRING("[UNKNOWN]", wardriver_logic::authModeToString(99));
}

void test_wardriver_channel_to_freq() {
    // All 14 valid 2.4GHz channels
    TEST_ASSERT_EQUAL_INT(2412, wardriver_logic::channelToFreqMHz(1));
    TEST_ASSERT_EQUAL_INT(2417, wardriver_logic::channelToFreqMHz(2));
    TEST_ASSERT_EQUAL_INT(2422, wardriver_logic::channelToFreqMHz(3));
    TEST_ASSERT_EQUAL_INT(2427, wardriver_logic::channelToFreqMHz(4));
    TEST_ASSERT_EQUAL_INT(2432, wardriver_logic::channelToFreqMHz(5));
    TEST_ASSERT_EQUAL_INT(2437, wardriver_logic::channelToFreqMHz(6));
    TEST_ASSERT_EQUAL_INT(2442, wardriver_logic::channelToFreqMHz(7));
    TEST_ASSERT_EQUAL_INT(2447, wardriver_logic::channelToFreqMHz(8));
    TEST_ASSERT_EQUAL_INT(2452, wardriver_logic::channelToFreqMHz(9));
    TEST_ASSERT_EQUAL_INT(2457, wardriver_logic::channelToFreqMHz(10));
    TEST_ASSERT_EQUAL_INT(2462, wardriver_logic::channelToFreqMHz(11));
    TEST_ASSERT_EQUAL_INT(2467, wardriver_logic::channelToFreqMHz(12));
    TEST_ASSERT_EQUAL_INT(2472, wardriver_logic::channelToFreqMHz(13));
    TEST_ASSERT_EQUAL_INT(2484, wardriver_logic::channelToFreqMHz(14));
    // Invalid channels
    TEST_ASSERT_EQUAL_INT(0, wardriver_logic::channelToFreqMHz(0));
    TEST_ASSERT_EQUAL_INT(0, wardriver_logic::channelToFreqMHz(15));
    TEST_ASSERT_EQUAL_INT(0, wardriver_logic::channelToFreqMHz(-1));
}

void test_wardriver_sanitize_ssid() {
    // Plain SSID passes through unchanged
    TEST_ASSERT_EQUAL_STRING("MyNetwork", wardriver_logic::sanitizeSSID("MyNetwork").c_str());
    // Empty SSID returns empty
    TEST_ASSERT_EQUAL_STRING("", wardriver_logic::sanitizeSSID("").c_str());
    TEST_ASSERT_EQUAL_STRING("", wardriver_logic::sanitizeSSID(nullptr).c_str());
    // Commas get quoted
    TEST_ASSERT_EQUAL_STRING("\"Net,work\"", wardriver_logic::sanitizeSSID("Net,work").c_str());
    // Double quotes get escaped
    TEST_ASSERT_EQUAL_STRING("\"Net\"\"work\"", wardriver_logic::sanitizeSSID("Net\"work").c_str());
    // Newlines become spaces inside quotes
    TEST_ASSERT_EQUAL_STRING("\"Net work\"", wardriver_logic::sanitizeSSID("Net\nwork").c_str());
    // Carriage returns become spaces inside quotes
    TEST_ASSERT_EQUAL_STRING("\"Net work\"", wardriver_logic::sanitizeSSID("Net\rwork").c_str());
    // Multiple special chars combined
    TEST_ASSERT_EQUAL_STRING("\"a,b\"\"c d\"",
                             wardriver_logic::sanitizeSSID("a,b\"c\nd").c_str());
}

void test_wardriver_format_wigle_row_wifi() {
    std::string row =
        wardriver_logic::formatWigleRow("AA:BB:CC:DD:EE:FF", "TestNet", "[WPA2-PSK]", "WIFI", 6,
                                        -65, 47.12345678, -122.98765432, 10.0f, "2025-01-01 00:00:00");
    // Should contain all fields separated by commas
    TEST_ASSERT_TRUE(row.find("AA:BB:CC:DD:EE:FF") != std::string::npos);
    TEST_ASSERT_TRUE(row.find("TestNet") != std::string::npos);
    TEST_ASSERT_TRUE(row.find("[WPA2-PSK]") != std::string::npos);
    TEST_ASSERT_TRUE(row.find("WIFI") != std::string::npos);
    TEST_ASSERT_TRUE(row.find("-65") != std::string::npos);
    TEST_ASSERT_TRUE(row.find("2025-01-01 00:00:00") != std::string::npos);
    TEST_ASSERT_TRUE(row.find("47.12345678") != std::string::npos);
    // Count commas: MAC,SSID,Auth,Time,Chan,RSSI,Lat,Lon,Alt,Acc,Type = 10 commas
    int commas = 0;
    for (char c : row) {
        if (c == ',')
            commas++;
    }
    TEST_ASSERT_EQUAL_INT(10, commas);
}

void test_wardriver_format_wigle_row_ble() {
    std::string row =
        wardriver_logic::formatWigleRow("11:22:33:44:55:66", "MyDevice", "[BLE]", "BLE", 0,
                                        -80, 40.0, -74.0, 5.0f, "2025-06-15 12:30:00");
    TEST_ASSERT_TRUE(row.find("11:22:33:44:55:66") != std::string::npos);
    TEST_ASSERT_TRUE(row.find("MyDevice") != std::string::npos);
    TEST_ASSERT_TRUE(row.find("[BLE]") != std::string::npos);
    TEST_ASSERT_TRUE(row.find("BLE") != std::string::npos);
    // Channel 0 for BLE
    TEST_ASSERT_TRUE(row.find(",0,") != std::string::npos);
}

void test_wardriver_format_wigle_row_special_ssid() {
    // SSID with comma should be CSV-safe quoted
    std::string row =
        wardriver_logic::formatWigleRow("AA:BB:CC:DD:EE:FF", "Net,work", "[OPEN]", "WIFI", 1,
                                        -50, 0.0, 0.0, 0.0f, "2025-01-01 00:00:00");
    TEST_ASSERT_TRUE(row.find("\"Net,work\"") != std::string::npos);
    // Still 10 structural commas (the one inside quotes doesn't break field count)
    // Parse: split on commas outside quotes
    // Easier: verify the MAC is at position 0 and Type is at the end
    TEST_ASSERT_TRUE(row.substr(0, 17) == "AA:BB:CC:DD:EE:FF");
    TEST_ASSERT_TRUE(row.substr(row.size() - 4) == "WIFI");
}

void test_wardriver_hash_mac() {
    uint32_t h1 = wardriver_logic::hashMAC("AA:BB:CC:DD:EE:FF");
    uint32_t h2 = wardriver_logic::hashMAC("AA:BB:CC:DD:EE:FF");
    uint32_t h3 = wardriver_logic::hashMAC("11:22:33:44:55:66");
    // Same input -> same hash
    TEST_ASSERT_EQUAL_UINT32(h1, h2);
    // Different input -> different hash (overwhelmingly likely)
    TEST_ASSERT_NOT_EQUAL(h1, h3);
    // Never returns 0 (0 is our empty sentinel)
    TEST_ASSERT_NOT_EQUAL((uint32_t)0, h1);
    TEST_ASSERT_NOT_EQUAL((uint32_t)0, h3);
    // Empty string still produces a non-zero hash
    TEST_ASSERT_NOT_EQUAL((uint32_t)0, wardriver_logic::hashMAC(""));
}

void test_wardriver_hash_mac_case_sensitive() {
    // Hash is case-sensitive (MACs in the firmware are passed as-is)
    uint32_t h1 = wardriver_logic::hashMAC("aa:bb:cc:dd:ee:ff");
    uint32_t h2 = wardriver_logic::hashMAC("AA:BB:CC:DD:EE:FF");
    // Different case -> different hash (FNV-1a is byte-level)
    TEST_ASSERT_NOT_EQUAL(h1, h2);
}

void test_wardriver_dedup_basic() {
    const int CAP = 64;
    uint32_t set[CAP];
    memset(set, 0, sizeof(set));

    // Empty set has count 0
    TEST_ASSERT_EQUAL_INT(0, wardriver_logic::dedupCount(set, CAP));

    uint32_t h1 = wardriver_logic::hashMAC("AA:BB:CC:DD:EE:FF");
    uint32_t h2 = wardriver_logic::hashMAC("11:22:33:44:55:66");

    // First insert returns true (new)
    TEST_ASSERT_TRUE(wardriver_logic::dedupInsert(set, CAP, h1));
    TEST_ASSERT_EQUAL_INT(1, wardriver_logic::dedupCount(set, CAP));

    // Duplicate insert returns false
    TEST_ASSERT_FALSE(wardriver_logic::dedupInsert(set, CAP, h1));
    TEST_ASSERT_EQUAL_INT(1, wardriver_logic::dedupCount(set, CAP));

    // Contains works
    TEST_ASSERT_TRUE(wardriver_logic::dedupContains(set, CAP, h1));
    TEST_ASSERT_FALSE(wardriver_logic::dedupContains(set, CAP, h2));

    // Insert second
    TEST_ASSERT_TRUE(wardriver_logic::dedupInsert(set, CAP, h2));
    TEST_ASSERT_EQUAL_INT(2, wardriver_logic::dedupCount(set, CAP));
    TEST_ASSERT_TRUE(wardriver_logic::dedupContains(set, CAP, h2));
}

void test_wardriver_dedup_full_table() {
    // Small capacity to test full-table behavior
    const int CAP = 4;
    uint32_t set[CAP];
    memset(set, 0, sizeof(set));

    // Fill the table completely
    char mac[20];
    int inserted = 0;
    for (int i = 0; i < 100 && inserted < CAP; i++) {
        snprintf(mac, sizeof(mac), "AA:BB:CC:DD:%02X:%02X", i / 256, i % 256);
        uint32_t h = wardriver_logic::hashMAC(mac);
        if (wardriver_logic::dedupInsert(set, CAP, h))
            inserted++;
    }
    TEST_ASSERT_EQUAL_INT(CAP, wardriver_logic::dedupCount(set, CAP));

    // After table is full, new inserts return false
    uint32_t newHash = wardriver_logic::hashMAC("FF:FF:FF:FF:FF:FF");
    bool result = wardriver_logic::dedupInsert(set, CAP, newHash);
    // Either it was already (by collision) or table is full — either way false
    TEST_ASSERT_FALSE(result);
}

void test_wardriver_dedup_zero_hash_rejected() {
    const int CAP = 8;
    uint32_t set[CAP];
    memset(set, 0, sizeof(set));

    // Zero hash is rejected (it's our empty sentinel)
    TEST_ASSERT_FALSE(wardriver_logic::dedupInsert(set, CAP, 0));
    TEST_ASSERT_FALSE(wardriver_logic::dedupContains(set, CAP, 0));
    TEST_ASSERT_EQUAL_INT(0, wardriver_logic::dedupCount(set, CAP));
}

void test_wardriver_dedup_many_entries() {
    // Stress test with realistic capacity
    const int CAP = 256;
    uint32_t set[CAP];
    memset(set, 0, sizeof(set));

    // Insert 128 unique MACs (50% load factor - should have zero collisions issues)
    char mac[20];
    for (int i = 0; i < 128; i++) {
        snprintf(mac, sizeof(mac), "AA:BB:%02X:%02X:%02X:%02X",
                 (i >> 12) & 0xFF, (i >> 8) & 0xFF, (i >> 4) & 0xFF, i & 0xFF);
        uint32_t h = wardriver_logic::hashMAC(mac);
        wardriver_logic::dedupInsert(set, CAP, h);
    }

    // All 128 should be findable
    int found = 0;
    for (int i = 0; i < 128; i++) {
        snprintf(mac, sizeof(mac), "AA:BB:%02X:%02X:%02X:%02X",
                 (i >> 12) & 0xFF, (i >> 8) & 0xFF, (i >> 4) & 0xFF, i & 0xFF);
        uint32_t h = wardriver_logic::hashMAC(mac);
        if (wardriver_logic::dedupContains(set, CAP, h))
            found++;
    }
    TEST_ASSERT_EQUAL_INT(128, found);
    // Count should match
    TEST_ASSERT_EQUAL_INT(128, wardriver_logic::dedupCount(set, CAP));
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
    // Wardriver logic
    RUN_TEST(test_wardriver_auth_mode_all_values);
    RUN_TEST(test_wardriver_channel_to_freq);
    RUN_TEST(test_wardriver_sanitize_ssid);
    RUN_TEST(test_wardriver_format_wigle_row_wifi);
    RUN_TEST(test_wardriver_format_wigle_row_ble);
    RUN_TEST(test_wardriver_format_wigle_row_special_ssid);
    RUN_TEST(test_wardriver_hash_mac);
    RUN_TEST(test_wardriver_hash_mac_case_sensitive);
    RUN_TEST(test_wardriver_dedup_basic);
    RUN_TEST(test_wardriver_dedup_full_table);
    RUN_TEST(test_wardriver_dedup_zero_hash_rejected);
    RUN_TEST(test_wardriver_dedup_many_entries);
    RUN_TEST(test_native_arithmetic_sanity);
    return UNITY_END();
}
