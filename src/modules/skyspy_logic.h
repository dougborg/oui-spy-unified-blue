#pragma once

#include <cstddef>
#include <cstdint>

namespace skyspy_logic {

// Known ODID vendor OUIs for beacon IE matching
struct VendorOUI {
    uint8_t bytes[3];
};

// Check if a vendor-specific IE matches a known ODID OUI.
// ie_data points to the IE payload (after type+length bytes), ie_len is its length.
bool isOdidVendorIE(const uint8_t* ie_data, size_t ie_len);

// Walk beacon IEs starting at 'offset' in payload of 'length' bytes.
// Returns offset of first ODID vendor IE payload (ie_data + 5, i.e. after OUI+type),
// or -1 if not found. Sets *odid_len to the length of the ODID payload.
int findOdidBeaconIE(const uint8_t* payload, int length, int startOffset, int* odid_len);

// UAV slot allocation: find existing slot by MAC, find empty, or return slot 0 (evict oldest).
// slots: array of MAC addresses (each 6 bytes), count: number of slots
// Returns the index of the slot to use.
int findOrAllocateSlot(const uint8_t slots[][6], int count, const uint8_t* mac);

// Check if a BLE payload looks like an ODID advertisement.
// Returns true if the service data header matches ODID BLE signature.
bool isOdidBlePayload(const uint8_t* payload, int len);

// Check if a WiFi frame is a NAN action frame by comparing destination at offset 4.
bool isNanActionFrame(const uint8_t* payload, int length);

// Count active UAVs (those with non-zero MAC and lastSeen within timeoutMs of now).
int countActiveUavs(const uint32_t* lastSeenTimes, const uint8_t macs[][6], int count, uint32_t now,
                    uint32_t timeoutMs = 7000);

} // namespace skyspy_logic
