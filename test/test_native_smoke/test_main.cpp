#include <unity.h>
#include <cstring>

extern "C" {
#include "opendroneid.h"
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
    RUN_TEST(test_native_arithmetic_sanity);
    return UNITY_END();
}
