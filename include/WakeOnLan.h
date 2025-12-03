// WakeOnLan.h
// Simple Wake-on-LAN helper for Arduino/ESP32
#ifndef WAKEONLAN_H
#define WAKEONLAN_H

#include <Arduino.h>

class WakeOnLan
{
  public:
    // Constants
    static constexpr size_t      MAC_LEN           = 6;
    static constexpr uint16_t    DEFAULT_PORT      = 9; // standard WOL port
    static constexpr const char* DEFAULT_BROADCAST = "255.255.255.255";

    // Send magic packet to MAC address string ("AA:BB:CC:DD:EE:FF" or "AABBCCDDEEFF").
    // broadcastIp optionally specifies the target broadcast IP (default 255.255.255.255).
    // port defaults to DEFAULT_PORT
    static bool send(const char* macStr, const char* broadcastIp = DEFAULT_BROADCAST,
                     uint16_t port = DEFAULT_PORT);

    // Parse MAC string into 6-byte array. Returns true on success.
    static bool parseMac(const char* macStr, uint8_t mac[MAC_LEN]);
};

#endif // WAKEONLAN_H
