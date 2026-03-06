#include "skyspy_logic.h"

#include <cstring>

namespace skyspy_logic {

// ASTM/ASD-STAN Remote ID OUIs
static const uint8_t ODID_OUI_1[3] = {0x90, 0x3a, 0xe6}; // Wi-Fi Alliance
static const uint8_t ODID_OUI_2[3] = {0xfa, 0x0b, 0xbc}; // ASTM

// NAN destination address used by ODID
static const uint8_t NAN_DEST[6] = {0x51, 0x6f, 0x9a, 0x01, 0x00, 0x00};

bool isOdidVendorIE(const uint8_t* ie_data, size_t ie_len) {
    if (!ie_data || ie_len < 5)
        return false;
    return (memcmp(ie_data, ODID_OUI_1, 3) == 0) || (memcmp(ie_data, ODID_OUI_2, 3) == 0);
}

int findOdidBeaconIE(const uint8_t* payload, int length, int startOffset, int* odid_len) {
    if (!payload || !odid_len || length <= 0 || startOffset < 0 || startOffset >= length)
        return -1;

    // IE layout: [type(1)][len(1)][OUI(3)+type(1)+counter(1) + ODID payload...]
    static const int OUI_OVERHEAD = 5; // OUI(3) + type(1) + counter(1)

    int offset = startOffset;
    while (offset + 2 <= length) {
        int typ = payload[offset];
        int len = payload[offset + 1];
        if (offset + 2 + len > length)
            break;
        if (typ == 0xdd && isOdidVendorIE(&payload[offset + 2], len)) {
            if (len <= OUI_OVERHEAD) {
                offset += len + 2;
                continue;
            }
            int odid_start = offset + 2 + OUI_OVERHEAD;
            *odid_len = len - OUI_OVERHEAD;
            return odid_start;
        }
        offset += len + 2;
    }
    return -1;
}

int findOrAllocateSlot(const uint8_t slots[][6], int count, const uint8_t* mac) {
    if (!slots || !mac || count <= 0)
        return 0;

    // Find existing
    for (int i = 0; i < count; i++) {
        if (memcmp(slots[i], mac, 6) == 0)
            return i;
    }
    // Find empty
    static const uint8_t zero[6] = {0};
    for (int i = 0; i < count; i++) {
        if (memcmp(slots[i], zero, 6) == 0)
            return i;
    }
    // Evict slot 0
    return 0;
}

bool isOdidBlePayload(const uint8_t* payload, int len) {
    if (!payload || len < 6)
        return false;
    return (payload[1] == 0x16 && payload[2] == 0xFA && payload[3] == 0xFF && payload[4] == 0x0D);
}

bool isNanActionFrame(const uint8_t* payload, int length) {
    if (!payload || length < 10)
        return false;
    return memcmp(NAN_DEST, &payload[4], 6) == 0;
}

int countActiveUavs(const uint32_t* lastSeenTimes, const uint8_t macs[][6], int count, uint32_t now,
                    uint32_t timeoutMs) {
    if (!lastSeenTimes || !macs || count <= 0)
        return 0;

    static const uint8_t zero[6] = {0};
    int active = 0;
    for (int i = 0; i < count; i++) {
        if (memcmp(macs[i], zero, 6) != 0 && (now - lastSeenTimes[i]) < timeoutMs)
            active++;
    }
    return active;
}

} // namespace skyspy_logic
