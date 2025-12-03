// Copyright (c) 2025 Damien Boisvert (AlphaGameDeveloper)
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <Arduino.h>
#include "Logger.h"
// Use the board-defined LED pin when available; fall back to GPIO2 which is
// the on-board LED on many ESP32 development boards.
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Blink interval in milliseconds
static const unsigned long BLINK_MS         = 1000UL;
static const unsigned long SERIAL_BAUD_RATE = 115200;

#include <WiFi.h>
#include <WebServer.h>
#include "WakeOnLan.h"

// Default MAC at build time: pass -DDEFAULT_MAC="\"d8:43:ae:54:52:01\"" in build flags
#ifndef DEFAULT_MAC
// fallback default
#define DEFAULT_MAC_LITERAL "d8:43:ae:54:52:01"
#else
#define DEFAULT_MAC_LITERAL DEFAULT_MAC
#endif

// WLAN compile-time configuration
// To be tolerant of how WLAN_MODE is passed (numeric, unquoted token, or quoted
// string), stringify the macro token and decide mode at runtime. Example usages:
//   -DWLAN_MODE=CONNECT   (unquoted token)
//   -DWLAN_MODE="CONNECT" (quoted)
//   -DWLAN_MODE=2         (numeric)
#ifdef WLAN_MODE
#define _STR_HELPER(x) #x
#define _STR(x) _STR_HELPER(x)
static const char* wlan_mode_raw = _STR(WLAN_MODE);
#undef WLAN_MODE
#else
static const char* wlan_mode_raw = "AP";
#endif

// WLAN SSID/PSK may be provided at build time. Stringify the macro token so
// we always have a literal. Users should provide build_flags like:
//   -DWLAN_SSID=MySSID -DWLAN_PSK=MyPass
// or quoted values; this code will normalize surrounding quotes if present.
#ifdef WLAN_SSID
#define _STR_HELPER(x) #x
#define _STR(x) _STR_HELPER(x)
static const char* wlan_ssid_raw = _STR(WLAN_SSID);
#undef WLAN_SSID
#else
static const char* wlan_ssid_raw = nullptr;
#endif

#ifdef WLAN_PSK
#define _STR_PSK_HELPER(x) #x
#define _STR_PSK(x) _STR_PSK_HELPER(x)
static const char* wlan_psk_raw = _STR_PSK(WLAN_PSK);
#undef WLAN_PSK
#else
static const char* wlan_psk_raw = nullptr;
#endif

#include <cstring>

// Normalize runtime string: strip surrounding double-quotes if present
static void normalize_copy(const char* src, char* dst, size_t dst_sz)
{
    if (!src || !dst || dst_sz == 0)
        return;
    size_t      len   = strlen(src);
    const char* begin = src;
    size_t      n     = len;
    if (len >= 2 && src[0] == '"' && src[len - 1] == '"')
    {
        begin = src + 1;
        n     = len - 2;
    }
    if (n >= dst_sz)
        n = dst_sz - 1;
    memcpy(dst, begin, n);
    dst[n] = '\0';
}

static const char* get_wlan_ssid()
{
    static char buf[64];
    if (!wlan_ssid_raw)
        return "WOL-ESP32";
    normalize_copy(wlan_ssid_raw, buf, sizeof(buf));
    return buf;
}

static const char* get_wlan_psk()
{
    static char buf[64];
    if (!wlan_psk_raw)
        return "wakeonlan";
    normalize_copy(wlan_psk_raw, buf, sizeof(buf));
    return buf;
}

WebServer server(80);

// HTML page served at /
const char* INDEX_HTML =
    "<!doctype html><html><head><meta charset=\"utf-8\"><title>WakeOnLan ESP32</title>"
    "</head><body><h1>Wake On LAN</h1>"
    "<p>Default target MAC: <code>" DEFAULT_MAC_LITERAL "</code></p>"
    "<form action=\"/wol\" method=\"get\">"
    "MAC: <input name=mac placeholder=\"AA:BB:CC:DD:EE:FF\" value=\"" DEFAULT_MAC_LITERAL "\">"
    "<input type=\"submit\" value=\"Wake\">"
    "</form>"
    "</body></html>";

// Handler: serve the index
void handleRoot()
{
    server.send(200, "text/html", INDEX_HTML);
}

// Handler: /wol?mac=...
void handleWol()
{
    String mac = server.arg("mac");
    if (mac.length() == 0)
        mac = String(DEFAULT_MAC_LITERAL);

    L_INFOF("Received WOL request for %s", mac.c_str());

    bool ok = WakeOnLan::send(mac.c_str());
    if (ok)
    {
        server.send(200, "text/plain", String("Magic packet sent to ") + mac);
        L_INFOF("Magic packet sent to %s", mac.c_str());
    }
    else
    {
        server.send(500, "text/plain", String("Failed to send packet to ") + mac);
        L_ERRORF("Failed to send magic packet to %s", mac.c_str());
    }
}

// Start WiFi (AP or CONNECT) and HTTP server
void startWebServer()
{
    server.on("/", handleRoot);
    server.on("/wol", handleWol);

    // Decide mode based on runtime wlan_mode_raw string. Accepts: CONNECT or AP or numeric '2'/'1'.
    bool want_connect = false;
    if (wlan_mode_raw[0] == 'C' || wlan_mode_raw[0] == 'c' || wlan_mode_raw[0] == '2')
        want_connect = true;
    if (want_connect)
    {
        // Attempt to connect as station
        const char* ssid = get_wlan_ssid();
        const char* psk  = get_wlan_psk();
        L_INFOF("Attempting to connect to SSID '%s'", ssid);
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid, psk);

        const unsigned long CONNECT_TIMEOUT_MS = 20000UL; // 20s
        unsigned long       start              = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - start) < CONNECT_TIMEOUT_MS)
        {
            delay(200);
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            L_INFOF("Connected to SSID '%s' IP %s", ssid, WiFi.localIP().toString().c_str());
            server.begin();
            L_INFO("HTTP server started (STA)");
            return;
        }
        else
        {
            L_WARNINGF("Failed to connect to '%s' within timeout; falling back to AP mode", ssid);
            // fallthrough to AP mode
        }
    }

    // Start AP mode
    WiFi.mode(WIFI_AP);
    const char* ap_ssid   = get_wlan_ssid();
    const char* ap_psk    = get_wlan_psk();
    bool        apStarted = WiFi.softAP(ap_ssid, ap_psk);
    if (!apStarted)
    {
        L_ERROR("Failed to start WiFi AP");
        return;
    }
    IPAddress ip = WiFi.softAPIP();
    L_INFOF("Started AP '%s' at %s", ap_ssid, ip.toString().c_str());

    server.begin();
    L_INFO("HTTP server started (AP)");
}

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    pinMode(LED_BUILTIN, OUTPUT);
    delay(100);
    startWebServer();
}

void loop()
{
    server.handleClient();
}