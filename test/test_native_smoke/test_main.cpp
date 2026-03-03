#include <unity.h>
#include <cstring>

extern "C" {
#include "opendroneid.h"
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

void test_native_arithmetic_sanity() {
    TEST_ASSERT_EQUAL_INT(4, 2 + 2);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_basic_id_encode_decode_roundtrip);
    RUN_TEST(test_location_encode_rejects_invalid_direction);
    RUN_TEST(test_message_pack_decodes_and_sets_basic_id_valid);
    RUN_TEST(test_native_arithmetic_sanity);
    return UNITY_END();
}
