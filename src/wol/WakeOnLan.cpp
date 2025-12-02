// WakeOnLan.cpp
#include "WakeOnLan.h"
#include <WiFiUdp.h>

// Default WOL port and MAC length reference
static const uint16_t WOL_PORT = WakeOnLan::DEFAULT_PORT;
static const size_t MAC_LEN = WakeOnLan::MAC_LEN;

bool WakeOnLan::parseMac(const char* macStr, uint8_t mac[MAC_LEN]) {
    if (!macStr || !mac) return false;
    // Copy to a modifiable buffer
    constexpr size_t BUF_SZ = 32;
    char buf[BUF_SZ];
    size_t len = strlen(macStr);
    if (len >= sizeof(buf)) return false;
    strcpy(buf, macStr);

    // Remove separators
    char *p = buf;
    constexpr size_t CLEANED_SZ = 13; // 12 hex digits + null
    constexpr size_t CLEANED_HEX = 12;
    char cleaned[CLEANED_SZ] = {0};
    size_t ci = 0;
    while (*p && ci < CLEANED_HEX) {
        if (*p == ':' || *p == '-' || *p == ' ')
        {
            p++;
            continue;
        }
        cleaned[ci++] = *p++;
    }
    if (ci != CLEANED_HEX) return false;

    // Convert pairs
    for (size_t i = 0; i < MAC_LEN; ++i) {
        char pair[3] = { cleaned[i*2], cleaned[i*2 + 1], '\0' };
        char *endptr = nullptr;
        constexpr int STRTOL_BASE = 16;
        constexpr long BYTE_MAX = 0xFF;
        long val = strtol(pair, &endptr, STRTOL_BASE);
        if (endptr == pair || val < 0 || val > BYTE_MAX) return false;
        mac[i] = (uint8_t)val;
    }
    return true;
}

bool WakeOnLan::send(const char* macStr, const char* broadcastIp, uint16_t port) {
    if (!macStr) return false;

    uint8_t mac[MAC_LEN];
    if (!parseMac(macStr, mac)) return false;

    // Build magic packet: 6 x 0xFF followed by MAC repeated 16 times
    constexpr size_t SYNC_COUNT = 6;
    constexpr size_t MAC_REP = 16;
    constexpr uint8_t SYNC_BYTE = 0xFF;
    const size_t packetSize = SYNC_COUNT + MAC_REP * MAC_LEN;
    uint8_t packet[packetSize];
    for (size_t i = 0; i < SYNC_COUNT; ++i) packet[i] = SYNC_BYTE;
    for (size_t i = 0; i < MAC_REP; ++i) {
        memcpy(&packet[SYNC_COUNT + i * MAC_LEN], mac, MAC_LEN);
    }

    WiFiUDP udp;
    if (udp.begin(0) == 0) {
        // couldn't start UDP
        return false;
    }

    IPAddress dest;
    if (!dest.fromString(broadcastIp)) {
        // fallback to global broadcast
        constexpr uint8_t BROADCAST_OCTET = 255;
        dest = IPAddress(BROADCAST_OCTET, BROADCAST_OCTET, BROADCAST_OCTET, BROADCAST_OCTET);
    }

    udp.beginPacket(dest, port == 0 ? WOL_PORT : port);
    udp.write(packet, packetSize);
    bool ok = (udp.endPacket() == 1);
    udp.stop();
    return ok;
}
