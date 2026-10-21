# ESP32 Project Foundation

A reusable PlatformIO/Arduino foundation for ESP32 projects using Wi-Fi, OTA firmware updates, MQTT, and Home Assistant MQTT discovery.

## Starting Board

The initial target is the Lonely Binary ESP32-S3 Gold Edition N16R8: an ESP32-S3-DEVKITC-1 V1.6 with 16 MB QIO flash, 8 MB OPI PSRAM, two USB-C ports, and one WS2812 RGB LED on GPIO48.

![Lonely Binary ESP32-S3 N16R8 pinout](assets/ESP32_S3_N16R8_LonelyBinary.jpg)

PlatformIO uses the `esp32-s3-devkitc-1` board profile, with N16R8 flash and PSRAM settings in `platformio.ini`.

## Programming and Serial Console

Start with the USB-C port labeled **UART**. This is the default programming and serial-monitor path through the CP2102 bridge:

```powershell
pio run -e esp32-s3-uart -t upload
pio device monitor -e esp32-s3-uart
```

If using the other USB-C port labeled **USB**, select the Native USB environment:

```powershell
pio run -e esp32-s3-usb -t upload
pio device monitor -e esp32-s3-usb
```

If both ports or multiple boards are connected, select the matching serial port in PlatformIO before uploading or opening the monitor.

## Setup

1. Copy `include/secrets.example.h` to `include/secrets.h`.
2. Set the Wi-Fi password and a unique OTA password in `include/secrets.h`. The SSID is set to `ElCamino` in the example.
3. Build and upload using the USB environment above. Use the UART environment only if USB programming is unavailable.
3. Start with the default UART environment for programming and serial monitoring. Select the Native USB environment only when connected to the other USB-C socket.
4. To update over Wi-Fi, set `ESP32_OTA_PASSWORD` in the current PowerShell session to the same value as `OTA_PASSWORD` in `include/secrets.h`, then run `pio run -e esp32-s3-ota -t upload`. The device must already be running firmware with OTA enabled on the same LAN.

The local `include/secrets.h` file is ignored by Git. Do not commit Wi-Fi or OTA passwords.

## RGB LED and Startup Pattern

The LED uses the ESP32 Arduino core's `neopixelWrite` RMT driver with GRB byte order and brightness 15. The hardware isolation test worked with this driver after the FastLED-based test remained white. Wi-Fi, MQTT, OTA, and the startup indicator are enabled.

The onboard WS2812 pixel is on GPIO48. After each power-on or reset, expect these LED sequences in order:

| Stage | Visible LED sequence | Duration and completion |
| --- | --- | --- |
| 1. Wi-Fi | Slow blue flashes: 500 ms on, 500 ms off | At least 2 seconds (normally 2 full flashes); continues until Wi-Fi connects. |
| 2. MQTT | Fast blue flashes: 250 ms on, 250 ms off | At least another 2 seconds (normally 4 full flashes); continues until both Wi-Fi and MQTT are connected. |
| 3. Ready | Blue flashing stops; LED is off by default | MQTT `ON` turns it steady green; MQTT `OFF` turns it off. |

With quick connections, the expected sequence is **2 slow blue flashes, then 4 fast blue flashes, then off**. Both blue patterns run even if the connections complete immediately, giving at least four seconds of startup indication. A connection that takes longer extends its corresponding pattern.

Network services continue during the sequence. MQTT LED commands received during startup update the saved switch state; when the blue flashing finishes, the LED displays the latest state (green for `ON`, off for `OFF`). These patterns run at startup only; later disconnections do not restart them.

Connection transitions, reconnect attempts, received MQTT topics and payloads, and errors are printed to the serial console at 115200 baud. Select the USB or UART monitor environment to match the cable in use.

At boot, the console also prints detected flash and PSRAM sizes and reports if PSRAM is missing.

## MQTT and Home Assistant

The initial broker is `192.168.0.179:1883` over plain MQTT/TCP, with anonymous access. Home Assistant is already connected to this broker; the firmware publishes MQTT switch discovery for that existing integration.

| Purpose | Topic | Payload |
| --- | --- | --- |
| Command | `devices/esp32-foundation-01/led/set` | `ON` or `OFF` |
| Reported state | `devices/esp32-foundation-01/led/state` | `ON` or `OFF` |
| Availability | `devices/esp32-foundation-01/led/availability` | `online` or `offline` |
| Home Assistant discovery | `homeassistant/switch/esp32-foundation-01/led/config` | Retained JSON configuration |

The device publishes reported state and discovery retained, and uses an MQTT Last Will for offline availability. Commands should not be published retained. The broker permits anonymous, unrestricted publish/subscribe on the home LAN, so do not send secrets over MQTT or expose this device to untrusted networks.

## Foundation Validation and Future Wall Panel

See [Foundation Completion and Future Wall Panel Plan](docs/FOUNDATION_PLAN.md)
for the MQTT test runner, HA checklist, OTA version tests, acceptance results,
and the future panel discovery design. Firmware starts at version `0.1.0`, printed
at boot and included in HA discovery as `device.sw_version`.

```powershell
python -m venv .venv-test
.\.venv-test\Scripts\python.exe -m pip install -r tools/requirements-test.txt
.\.venv-test\Scripts\python.exe tools/test_mqtt.py --device-id esp32-foundation-01
```

The test controls the selected device's LED. Pause competing controllers first;
add `--interactive` for operator-assisted power/Wi-Fi and physical LED checks.
Reports go to `.pio/mqtt-test-report.json`. Hardware, HA and OTA checks remain
Pending until actually observed; a successful automated run does not grant sign-off.

## Board Reference

[Lonely Binary ESP32-S3 N16R8 board guide](https://learn.lonelybinary.com/boards/esp32-s3)
