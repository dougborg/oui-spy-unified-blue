#include <unity.h>
#include <cstring>

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

static int build_valid_nan_action_frame(uint8_t* frame, size_t frame_size) {
    ODID_UAS_Data source;
    seed_basic_uas(&source, "NAN-PARSER-EDGE-01");

    char tx_mac[6] = {0x02, 0x44, 0x55, 0x66, 0x77, (char)0x88};
    return odid_wifi_build_message_pack_nan_action_frame(&source, tx_mac, 9, frame, frame_size);
}

void test_nan_action_parser_rejects_too_short_buffer() {
    uint8_t short_buf[8] = {0};
    ODID_UAS_Data received;
    char rx_mac[6] = {0};

    int ret = odid_wifi_receive_message_pack_nan_action_frame(&received, rx_mac, short_buf,
                                                              sizeof(short_buf));
    TEST_ASSERT_EQUAL_INT(-22, ret);  // -EINVAL
}

void test_nan_action_parser_rejects_bad_frame_control() {
    uint8_t frame[1024] = {0};
    int frame_len = build_valid_nan_action_frame(frame, sizeof(frame));
    TEST_ASSERT_GREATER_THAN_INT(0, frame_len);

    auto* mgmt = reinterpret_cast<ieee80211_mgmt*>(frame);
    mgmt->frame_control = 0;

    ODID_UAS_Data received;
    char rx_mac[6] = {0};
    int ret = odid_wifi_receive_message_pack_nan_action_frame(&received, rx_mac, frame,
                                                              static_cast<size_t>(frame_len));
    TEST_ASSERT_EQUAL_INT(-22, ret);  // -EINVAL
}

void test_nan_action_parser_rejects_bad_oui() {
    uint8_t frame[1024] = {0};
    int frame_len = build_valid_nan_action_frame(frame, sizeof(frame));
    TEST_ASSERT_GREATER_THAN_INT(0, frame_len);

    size_t offset = sizeof(ieee80211_mgmt);
    auto* nsd = reinterpret_cast<nan_service_discovery*>(frame + offset);
    nsd->oui[1] ^= 0x80;

    ODID_UAS_Data received;
    char rx_mac[6] = {0};
    int ret = odid_wifi_receive_message_pack_nan_action_frame(&received, rx_mac, frame,
                                                              static_cast<size_t>(frame_len));
    TEST_ASSERT_EQUAL_INT(-22, ret);  // -EINVAL
}

void test_nan_action_parser_rejects_bad_descriptor_attribute_id() {
    uint8_t frame[1024] = {0};
    int frame_len = build_valid_nan_action_frame(frame, sizeof(frame));
    TEST_ASSERT_GREATER_THAN_INT(0, frame_len);

    size_t offset = sizeof(ieee80211_mgmt) + sizeof(nan_service_discovery);
    auto* nsda = reinterpret_cast<nan_service_descriptor_attribute*>(frame + offset);
    nsda->header.attribute_id = 0x02;

    ODID_UAS_Data received;
    char rx_mac[6] = {0};
    int ret = odid_wifi_receive_message_pack_nan_action_frame(&received, rx_mac, frame,
                                                              static_cast<size_t>(frame_len));
    TEST_ASSERT_EQUAL_INT(-22, ret);  // -EINVAL
}

void test_nan_action_parser_rejects_bad_extension_control() {
    uint8_t frame[1024] = {0};
    int frame_len = build_valid_nan_action_frame(frame, sizeof(frame));
    TEST_ASSERT_GREATER_THAN_INT(0, frame_len);

    size_t offset = sizeof(ieee80211_mgmt) + sizeof(nan_service_discovery) +
                    sizeof(nan_service_descriptor_attribute);
    auto* service_info = reinterpret_cast<ODID_service_info*>(frame + offset);
    auto* pack = reinterpret_cast<ODID_MessagePack_encoded*>(service_info->odid_message_pack);
    size_t pack_len = sizeof(*pack) - ODID_MESSAGE_SIZE * (ODID_PACK_MAX_MESSAGES - pack->MsgPackSize);

    auto* ext = reinterpret_cast<nan_service_descriptor_extension_attribute*>(
        frame + offset + sizeof(ODID_service_info) + pack_len);
    ext->control = 0;

    ODID_UAS_Data received;
    char rx_mac[6] = {0};
    int ret = odid_wifi_receive_message_pack_nan_action_frame(&received, rx_mac, frame,
                                                              static_cast<size_t>(frame_len));
    TEST_ASSERT_EQUAL_INT(-22, ret);  // -EINVAL
}

void test_message_process_pack_rejects_zero_messages() {
    ODID_MessagePack_encoded pack = {};
    pack.MessageType = ODID_MESSAGETYPE_PACKED;
    pack.ProtoVersion = ODID_PROTOCOL_VERSION;
    pack.SingleMessageSize = ODID_MESSAGE_SIZE;
    pack.MsgPackSize = 0;

    ODID_UAS_Data out;
    int ret = odid_message_process_pack(&out, reinterpret_cast<uint8_t*>(&pack), sizeof(pack));
    TEST_ASSERT_EQUAL_INT(-1, ret);
}

void test_message_process_pack_rejects_illegal_message_type_in_pack() {
    ODID_MessagePack_encoded pack = {};
    pack.MessageType = ODID_MESSAGETYPE_PACKED;
    pack.ProtoVersion = ODID_PROTOCOL_VERSION;
    pack.SingleMessageSize = ODID_MESSAGE_SIZE;
    pack.MsgPackSize = 1;
    pack.Messages[0].rawData[0] = 0xE0;

    ODID_UAS_Data out;
    int ret = odid_message_process_pack(&out, reinterpret_cast<uint8_t*>(&pack), sizeof(pack));
    TEST_ASSERT_EQUAL_INT(-1, ret);
}

void test_encode_message_pack_rejects_too_many_messages() {
    ODID_MessagePack_data input;
    odid_initMessagePackData(&input);
    input.MsgPackSize = static_cast<uint8_t>(ODID_PACK_MAX_MESSAGES + 1);

    ODID_MessagePack_encoded out;
    TEST_ASSERT_EQUAL_INT(ODID_FAIL, encodeMessagePack(&out, &input));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_nan_action_parser_rejects_too_short_buffer);
    RUN_TEST(test_nan_action_parser_rejects_bad_frame_control);
    RUN_TEST(test_nan_action_parser_rejects_bad_oui);
    RUN_TEST(test_nan_action_parser_rejects_bad_descriptor_attribute_id);
    RUN_TEST(test_nan_action_parser_rejects_bad_extension_control);
    RUN_TEST(test_message_process_pack_rejects_zero_messages);
    RUN_TEST(test_message_process_pack_rejects_illegal_message_type_in_pack);
    RUN_TEST(test_encode_message_pack_rejects_too_many_messages);
    return UNITY_END();
}
