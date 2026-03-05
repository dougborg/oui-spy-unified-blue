#include "dns_server.h"
#include <WiFi.h>
#include <WiFiUdp.h>

// ============================================================================
// Captive Portal DNS Server
// Minimal DNS responder: all queries → AP IP (192.168.4.1)
// ============================================================================

namespace hal {

static WiFiUDP _dnsUDP;
static bool _dnsRunning = false;
static const int DNS_PORT = 53;

// DNS header structure (12 bytes)
struct DNSHeader {
    uint16_t id;
    uint16_t flags;
    uint16_t qdCount;
    uint16_t anCount;
    uint16_t nsCount;
    uint16_t arCount;
};

void dnsServerStart() {
    if (_dnsRunning)
        return;
    if (_dnsUDP.begin(DNS_PORT)) {
        _dnsRunning = true;
        Serial.println("[HAL] DNS captive portal started on port 53");
    } else {
        Serial.println("[HAL] DNS server failed to start");
    }
}

void dnsServerLoop() {
    if (!_dnsRunning)
        return;

    int packetSize = _dnsUDP.parsePacket();
    if (packetSize < (int)sizeof(DNSHeader))
        return;

    // Read the query
    uint8_t query[512];
    int len = _dnsUDP.read(query, sizeof(query));
    if (len < (int)sizeof(DNSHeader))
        return;

    DNSHeader* hdr = (DNSHeader*)query;

    // Only respond to standard queries (QR=0, Opcode=0)
    uint16_t flags = ntohs(hdr->flags);
    if ((flags & 0x8000) != 0)
        return; // This is a response, not a query
    if (((flags >> 11) & 0x0F) != 0)
        return; // Not a standard query

    uint16_t qdCount = ntohs(hdr->qdCount);
    if (qdCount == 0)
        return;

    // Find end of question section (skip QNAME + QTYPE + QCLASS)
    int qOffset = sizeof(DNSHeader);
    // Skip QNAME (series of length-prefixed labels ending with 0)
    while (qOffset < len && query[qOffset] != 0) {
        // Reject compression pointers in question section
        if ((query[qOffset] & 0xC0) == 0xC0)
            return;
        int labelLen = query[qOffset] & 0x3F;
        if (qOffset + labelLen + 1 >= len)
            return;
        qOffset += labelLen + 1;
    }
    qOffset++;              // skip null terminator
    int qEnd = qOffset + 4; // QTYPE(2) + QCLASS(2)
    if (qEnd > len)
        return;

    // Build DNS response
    // Response = query header (modified) + original question + answer (16 bytes)
    int questionLen = qEnd - (int)sizeof(DNSHeader);
    if ((int)sizeof(DNSHeader) + questionLen + 16 > 512)
        return; // question too large for response buffer

    uint8_t response[512];
    int respLen = 0;

    // Copy header
    memcpy(response, query, sizeof(DNSHeader));
    DNSHeader* respHdr = (DNSHeader*)response;
    respHdr->flags = htons(0x8180); // QR=1, AA=1, RD=1, RA=1
    respHdr->anCount = htons(1);
    respHdr->nsCount = 0;
    respHdr->arCount = 0;
    respLen = sizeof(DNSHeader);

    // Copy question section (questionLen already computed above for bounds check)
    memcpy(response + respLen, query + sizeof(DNSHeader), questionLen);
    respLen += questionLen;

    // Add answer: pointer to QNAME + TYPE A + CLASS IN + TTL + RDLENGTH + IP
    // Name pointer: 0xC00C points to offset 12 (start of QNAME in question)
    response[respLen++] = 0xC0;
    response[respLen++] = 0x0C;
    // TYPE A (1)
    response[respLen++] = 0x00;
    response[respLen++] = 0x01;
    // CLASS IN (1)
    response[respLen++] = 0x00;
    response[respLen++] = 0x01;
    // TTL (60 seconds)
    response[respLen++] = 0x00;
    response[respLen++] = 0x00;
    response[respLen++] = 0x00;
    response[respLen++] = 0x3C;
    // RDLENGTH (4 bytes for IPv4)
    response[respLen++] = 0x00;
    response[respLen++] = 0x04;
    // RDATA: AP IP address
    IPAddress apIP = WiFi.softAPIP();
    response[respLen++] = apIP[0];
    response[respLen++] = apIP[1];
    response[respLen++] = apIP[2];
    response[respLen++] = apIP[3];

    // Send response
    _dnsUDP.beginPacket(_dnsUDP.remoteIP(), _dnsUDP.remotePort());
    _dnsUDP.write(response, respLen);
    _dnsUDP.endPacket();
}

} // namespace hal
