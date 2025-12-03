// Copyright (c) 2025 Damien Boisvert (AlphaGameDeveloper)
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "WakeOnLan.h"
#include "generated/assets.h"
#include "Logger.h"
// Use the board-defined LED pin when available; fall back to GPIO2 which is
// the on-board LED on many ESP32 development boards.
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Blink interval in milliseconds
static const unsigned long BLINK_MS         = 1000UL;
static const unsigned long SERIAL_BAUD_RATE = 115200;

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

#ifndef FIRMWARE_VERSION
static const char* firmware_version_raw = "unknown";
#else
/* Stringify the macro token so we always have a literal, accepting both
   unquoted tokens (e.g. -DFIRMWARE_VERSION=v1) and quoted values. */
#define _STR_FW_HELPER(x) #x
#define _STR_FW(x) _STR_FW_HELPER(x)
static const char* firmware_version_raw = _STR_FW(FIRMWARE_VERSION);
#undef _STR_FW_HELPER
#undef _STR_FW
#endif

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

// Helper: find embedded asset by path (exact match)
static const struct asset* find_asset(const char* path)
{
    if (!path)
        return nullptr;
    // Try exact match
    for (size_t i = 0; i < embedded_assets_count; ++i)
    {
        if (strcmp(embedded_assets[i].path, path) == 0)
            return &embedded_assets[i];
    }
    // Try with leading slash (if not provided)
    if (path[0] != '/')
    {
        String withSlash = String("/") + String(path);
        for (size_t i = 0; i < embedded_assets_count; ++i)
        {
            if (String(embedded_assets[i].path) == withSlash)
                return &embedded_assets[i];
        }
    }
    return nullptr;
}

// Simple MIME type detection by extension
static const char* mime_for_path(const String& p)
{
    if (p.endsWith(".html") || p.endsWith(".htm"))
        return "text/html";
    if (p.endsWith(".js"))
        return "application/javascript";
    if (p.endsWith(".css"))
        return "text/css";
    if (p.endsWith(".json"))
        return "application/json";
    if (p.endsWith(".png"))
        return "image/png";
    if (p.endsWith(".jpg") || p.endsWith(".jpeg"))
        return "image/jpeg";
    if (p.endsWith(".svg"))
        return "image/svg+xml";
    return "text/plain";
}

// Serve an embedded asset (gzip-aware)
static void serve_embedded(const char* cpath)
{
    L_INFOF("Serving embedded asset: %s", cpath);
    const struct asset* a = find_asset(cpath);
    if (!a)
    {
        server.send(404, "text/plain", "Not found");
        return;
    }

    String      path = String(cpath);
    const char* mime = mime_for_path(path);

    // Get raw client and write full HTTP response to avoid conflicting headers
    WiFiClient client = server.client();
    if (!client || !client.connected())
    {
        // fallback to server's send
        if (a->gz)
            server.sendHeader("Content-Encoding", "gzip");
        server.send(200, mime, "");
        return;
    }

    // Build and send status + headers
    client.print("HTTP/1.1 200 OK\r\n");
    client.print("Content-Type: ");
    client.print(mime);
    client.print("\r\n");
    if (a->gz)
    {
        client.print("Content-Encoding: gzip\r\n");
    }
    client.print("Content-Length: ");
    client.print((unsigned long)a->len);
    client.print("\r\n");
    client.print("Connection: close\r\n");
    client.print("\r\n");

    // Write raw gzipped bytes
    client.write(a->data, a->len);
    client.flush();
    // close connection
    client.stop();
}

// Handler: serve root -> redirect to embedded index under /assets/
void handleRoot()
{
    serve_embedded("/index.html");
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

// Handler: POST /api/wake — accepts JSON { mac: string, broadcast?: string }
void handleApiWake()
{
    if (server.method() != HTTP_POST)
    {
        server.send(405, "text/plain", "Method Not Allowed");
        return;
    }

    String body = server.arg("plain");
    if (body.length() == 0)
    {
        server.send(400, "text/plain", "Empty body");
        return;
    }

    // Very small JSON parsing — look for "mac" and "broadcast" values.
    // This avoids pulling in a heavy JSON library on the ESP.
    auto extract = [](const String& s, const char* key) -> String
    {
        String k   = String("\"") + key + String("\"") + String(":");
        int    idx = s.indexOf(k);
        if (idx < 0)
            return String();
        idx += k.length();
        // skip whitespace
        while (idx < s.length() && isWhitespace(s[idx]))
            idx++;
        // Accept string value or bareword
        if (s[idx] == '"')
        {
            idx++;
            int end = s.indexOf('"', idx);
            if (end < 0)
                return String();
            return s.substring(idx, end);
        }
        // bare token until comma or brace
        int end = idx;
        while (end < s.length() && s[end] != ',' && s[end] != '}' && s[end] != '\n' &&
               s[end] != '\r')
            end++;
        String token = s.substring(idx, end);
        token.trim();
        return token;
    };

    String mac       = extract(body, "mac");
    String broadcast = extract(body, "broadcast");

    if (mac.length() == 0)
    {
        server.send(400, "text/plain", "Missing 'mac' in JSON body");
        return;
    }

    L_INFOF("API WOL request for %s (broadcast=%s)", mac.c_str(), broadcast.c_str());

    bool ok;
    if (broadcast.length())
        ok = WakeOnLan::send(mac.c_str(), broadcast.c_str());
    else
        ok = WakeOnLan::send(mac.c_str());

    if (ok)
    {
        server.send(200, "application/json", String("{\"status\":\"ok\",\"mac\":\"") + mac + "\"}");
    }
    else
    {
        server.send(500, "application/json",
                    String("{\"status\":\"error\",\"mac\":\"") + mac + "\"}");
    }
}

// Start WiFi (AP or CONNECT) and HTTP server
void startWebServer()
{
    server.on("/", handleRoot);
    server.on("/wol", handleWol);
    server.on("/api/wake", HTTP_POST, handleApiWake);
    server.on("/api/version",
              []()
              {
                  String v = firmware_version_raw;
                  server.send(200, "text/plain", v);
              });
    // Serve any /assets/* requests from embedded assets; fall back to 404
    server.onNotFound(
        []()
        {
            String uri = server.uri();
            // ensure paths under /assets/ are served
            if (uri == "/")
            {
                serve_embedded("/index.html");
                return;
            }
            if (uri.startsWith("/assets/"))
            {
                // strip '/assets' prefix
                String assetPath = uri.substring(8); // 8 = length of "/assets/"
                serve_embedded(assetPath.c_str());
                return;
            }
            // Not an asset -> default 404
            server.send(404, "text/plain", "Not found");
        });

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