# Shed Ventilation

An ESP32-S3 project derived from the ESP32 Project Foundation. Its goal is to ventilate a closed shed when indoor temperature is high, while reporting sensor readings and fan state to Home Assistant (HA).

## Status

This branch currently contains the foundation firmware and the shed implementation plan. The relay, SHT40 sensor, HA entities, persistent setpoints, and Tempest bridge are planned but are not implemented yet. Do not connect a fan expecting the current foundation firmware to control it.

The implementation plan is in [docs/SHED_VENTILATION_PLAN.md](docs/SHED_VENTILATION_PLAN.md). This project is maintained on the `shed-ventilation` branch so foundation updates can be merged forward.

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

## Build and Upload

The current firmware still uses the foundation's setup. Configure local Wi-Fi, OTA, and MQTT credentials in the ignored `include/secrets.h`; do not commit credentials. The foundation's default PlatformIO environment is UART:

```powershell
pio run -e esp32-s3-uart
pio run -e esp32-s3-uart -t upload
pio device monitor -e esp32-s3-uart
```

For the Native USB connector, use `esp32-s3-usb`. OTA uses `esp32-s3-ota` and requires the device to be online with OTA enabled. The OTA upload hostname must match the device ID; it will be updated for `ShedVentilation_esp32s3` as part of implementation. Until then, the inherited foundation OTA target is unchanged.

## Startup and Fan Indicator

The existing foundation startup indication uses the onboard WS2812 LED on GPIO48: slow blue flashes while Wi-Fi connects, followed by fast blue flashes while MQTT connects. The shed implementation will retain that sequence. After startup, the LED will mirror the commanded relay output: green when the fan output is on and off when it is off. It will not be an independently controllable HA entity.

The current foundation firmware still uses its original LED-switch behavior until the shed controller is implemented. Serial diagnostics use 115200 baud.

## Planned Fan and Home Assistant Behavior

The shed device will use the existing MQTT broker and HA MQTT discovery. The planned entities include a fan switch, indoor temperature and humidity, outdoor temperature, status, and editable OFF/ON setpoint numbers. HA ON/OFF commands and a local digital button will temporarily override automatic control until the next thermostat transition.

The initial setpoints are 80°F OFF and 110°F ON. Whole-degree values from 60°F through 120°F are accepted, provided OFF is at least 2°F below ON. Values will persist in ESP32 NVS. The shed will publish a retained JSON state report containing the accepted setpoints and fan/sensor status; HA will display the setpoints reported by the device. Invalid commands will leave stored values unchanged.

HA already receives Tempest data. A planned HA automation will publish the outdoor temperature to MQTT. With fresh outdoor data, the planned effective OFF threshold is the greater of the saved OFF setpoint and outdoor temperature plus 2°F. The effective ON threshold is the greater of the saved ON setpoint and effective OFF threshold plus 2°F. Stale or missing outdoor data falls back to the saved setpoints. The device will not query the HA API or consume Tempest UDP directly.

Weather MQTT design will keep retained latest-temperature state separate from non-retained event notices such as future Rain events. Topic names and payload format remain to be agreed before implementation.

The broker is currently configured in the foundation as plain MQTT/TCP on the home LAN. Do not send secrets over MQTT or expose an unauthenticated broker/device to untrusted networks.

## Development and Validation

The existing host and MQTT tests validate foundation behavior, not the planned shed controller. See [docs/FOUNDATION_PLAN.md](docs/FOUNDATION_PLAN.md) for the foundation's recorded checks. Shed-specific host tests, HA checks, hardware tests, and OTA validation will be added as implementation proceeds. The UART firmware build command is:

```powershell
pio run -e esp32-s3-uart
```

The SHT40 must be powered at 3.3V, with I²C pull-ups to 3.3V; do not apply 5V to the sensor or ESP32 GPIO. Verify the breakout board circuitry, I²C pins, relay input polarity, button polarity, and relay reset behavior before wiring. GPIO assignments and active levels are not finalized.

## Board Reference

[Lonely Binary ESP32-S3 N16R8 board guide](https://learn.lonelybinary.com/boards/esp32-s3)
