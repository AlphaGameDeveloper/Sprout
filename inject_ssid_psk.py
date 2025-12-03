# Copyright (c) 2025 Damien Boisvert (AlphaGameDeveloper)
#
# This software is released under the MIT License.
# https://opensource.org/licenses/MIT
import os

try:
    with open(".env", "r") as f:
        for line in f:
            key, value = line.strip().split("=", 1)
            os.getenv(key) or os.environ.setdefault(key, value)

except FileNotFoundError:
    print("inject_ssid_psk.py: oh well no .env file found")

Import("env")

ssid = os.getenv("PLATFORMIO_WLAN_SSID")
psk = os.getenv("PLATFORMIO_WLAN_PSK")
env.Append(
    CPPDEFINES={
        "WLAN_SSID": ssid,
        "WLAN_PSK": psk,
        "WLAN_MODE": "CONNECT" if ssid and psk else "AP",
    }
)
