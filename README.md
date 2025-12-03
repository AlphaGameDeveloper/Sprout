# WakeOnLanESP32

This project runs an ESP32 that can send Wake-on-LAN (WOL) magic packets.

Web UI / API
- The board starts a WiFi Access Point named `WOL-ESP32` (password `wakeonlan`).
- Visit http://192.168.4.1/ in a device connected to the AP to use the web UI.
- The web endpoint `/wol?mac=AA:BB:CC:DD:EE:FF` sends a magic packet to the specified MAC.

Default MAC
- A default target MAC address is provided at build time via the `DEFAULT_MAC` macro.
- To change it via PlatformIO, add a build flag in `platformio.ini` under the environment, for example:

```
[env:esp32]
platform = espressif32
framework = arduino
board = esp32dev
build_flags = -DDEFAULT_MAC=\"d8:43:ae:54:52:01\"
```

If `DEFAULT_MAC` is not provided at build time, the code falls back to `d8:43:ae:54:52:01`.
