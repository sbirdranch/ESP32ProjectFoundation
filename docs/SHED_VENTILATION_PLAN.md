# Shed Ventilation Project Plan

## Status

This project branch starts from the ESP32 Project Foundation. Shed-specific firmware is implemented and passes the host regression tests and ESP32-S3 UART build. Physical hardware, OTA, and end-to-end HA behavior remain unverified until tested on the board and broker.

## Goal

Run a ventilation fan in a closed shed to reduce internal temperature. A SHT40 I2C sensor measures indoor temperature and humidity. The ESP32-S3 drives a non-latching relay, exposes device status and readings to Home Assistant (HA), and accepts HA and local-button fan control.

The device ID is `ShedVentilation_esp32s3`. The solution lives on the `shed-ventilation` branch in the foundation repository so later foundation changes can be merged into it.

## Inherited Foundation

Retain the foundation's Wi-Fi connection/retry, MQTT transport and reconnect, ArduinoOTA, board configuration, and blue startup indication. Adapt the application-facing LED/MQTT behavior for the ventilation device. After startup indication, the onboard LED mirrors the fan output: green when the relay output is on and off when the relay output is off. It is not a separate HA switch.

## Control Behavior

### Setpoints

- Initial persisted values: OFF at 80°F and ON at 110°F.
- Both values are whole degrees Fahrenheit, inclusive range 60°F–120°F.
- Store values in ESP32 Preferences/NVS. Load saved values at startup; use defaults when no valid saved pair exists.
- Accept MQTT updates only when the value is numeric, in range, and leaves at least a 2°F gap (`OFF + 2°F <= ON`). Rejected updates do not alter storage.
- Expose editable HA number entities. The shed is authoritative: a retained device state report includes the last accepted OFF and ON values, and HA displays those reported values, not merely the last command sent.
- Publish the authoritative state after connection and command handling. Commands are not retained. Accepted updates apply to automatic control immediately.
- If a user changes both values separately, update ON first when raising the pair, or OFF first when lowering it, so the gap validation remains true at each step.

### Automatic Fan Control

With a fresh outdoor temperature received from HA:

- Effective OFF threshold = `max(saved OFF setpoint, outdoor temperature + 2°F)`.
- Effective ON threshold = `max(saved ON setpoint, effective OFF threshold + 2°F)`.
- Turn the fan on when indoor temperature reaches the effective ON threshold; turn it off when indoor temperature reaches or falls below the effective OFF threshold.

This keeps a 2°F restart gap and avoids starting the fan when outside air is too warm. If outdoor data is missing, invalid, or stale, report it unavailable and use the saved OFF/ON setpoints without outdoor adjustment.

An HA ON/OFF command or local button action temporarily overrides automation until the next automatic thermostat state transition. A SHT40 read fault preserves the current relay output and reports the fault.

## MQTT and Home Assistant

Use MQTT and HA MQTT discovery on the foundation's existing broker. The shed publishes discovery for a fan switch, OFF/ON number controls, indoor temperature/humidity, outdoor temperature, availability, and relevant fault/freshness status.

Publish a retained JSON state message from the shed that includes at least the commanded relay state, last valid indoor readings and their validity/age, outdoor temperature and freshness, and the last accepted OFF/ON setpoints. HA discovery value templates extract each entity's value from this message. The state represents the commanded relay GPIO state; there is no relay-contact or airflow feedback.

HA already receives Tempest data. Add an HA automation to publish outdoor temperature to MQTT when it changes and after HA startup/recovery. The ESP32 consumes that MQTT value; it does not call the HA API or parse Tempest UDP in this version.

Weather MQTT channels:

- Retained numeric Celsius outdoor-temperature state on `weather/outdoor_temperature_c`, so a device reconnect can recover the current value.
- Non-retained Weather event notices on `weather/events`, reserved for future events such as Rain that other projects may consume.

An HA automation publishes Tempest updates and republishes after HA startup. Convert the source entity to Celsius before publishing. A retained temperature state must not be overwritten by a Rain notice.

Keep HA fan commands and setpoint commands non-retained. Publish authoritative retained state/discovery and retain the foundation's online/offline availability behavior. The foundation currently uses plain MQTT/TCP on the home LAN; do not expose it to untrusted networks without addressing broker authentication and transport security.

## Hardware and Configuration

- Target board: Lonely Binary ESP32-S3 Gold Edition N16R8.
- Sensor: SHT40, I2C address selectable between `0x44` and `0x45` (default `0x44`).
- Power the bare SHT40 at 3.3V; keep I2C pull-ups at 3.3V. Never apply more than 3.6V to the sensor or 5V to ESP32 GPIO. Verify the breakout board's power and pull-up circuitry before wiring.
- Current configurable pin defaults: SDA GPIO8, SCL GPIO9, relay GPIO10 active-high, and button GPIO11 active-low.
- Keep relay/button active levels and GPIO selections configurable. Confirm the exact modules, pin availability, and relay input behavior during ESP32 reset/boot before attaching the fan.
- Enclosure design is out of scope for the first implementation.

## Implementation Sequence

1. Add device identity/configuration and align the OTA target with `ShedVentilation_esp32s3`. Implemented; UART build passes, OTA remains unverified.
2. Add the setpoint module for defaults, range/gap validation, Preferences/NVS load and writes. Implemented and host-tested.
3. Add SHT40 sampling and validated indoor readings with fault and age reporting. Implemented; sensor hardware remains unverified.
4. Add relay output, button debounce, LED mirroring, and the testable temperature controller; initialize relay OFF before networking. Implemented and host-tested; confirm pin polarity on hardware.
5. Add product MQTT/HA service, retained JSON state, fan and setpoint commands, discovery, availability, and Tempest MQTT bridge. Implemented; live HA behavior remains unverified.
6. Update project documentation and add focused host tests. Implemented; host suites and UART build pass.

## Validation

- Host tests: Fahrenheit/Celsius conversion; ON/OFF boundaries; outdoor hotter/equal/cooler cases; high outdoor thresholds; stale/invalid outdoor fallback; sensor-fault behavior; manual override resumption; button debounce; output/LED mirroring; setpoint defaults, parsing, bounds, gap validation, invalid update rejection, and persistence/reload.
- Build: `pio run -e esp32-s3-uart`.
- MQTT/HA: verify discovery, retained JSON state (including device-reported setpoints), editable number controls, fan commands, availability, non-retained commands, rejected updates, and recovery of retained outdoor temperature after reconnect.
- Hardware: verify relay OFF at boot and correct polarity; SHT40 address/readings; button; startup sequence and LED mirroring; temperature transitions; indoor-sensor fault behavior; stale outdoor fallback; and persistence across power cycle/OTA.
- OTA: verify the uploader hostname matches the device's OTA hostname and that HA entities return after update.

## Decisions and Remaining Details

Decided: same-repository branch `shed-ventilation`; device ID `ShedVentilation_esp32s3`; 80°F/110°F initial defaults; 60°F–120°F bounds; 2°F minimum setpoint gap; NVS persistence; HA number controls backed by device-reported state; retained Celsius Weather state on `weather/outdoor_temperature_c` with future events separated on `weather/events`; HA-to-MQTT Tempest input; 2°F outdoor margin and 2°F restart gap; preserve relay output on indoor sensor fault; keep the startup LED sequence and then mirror fan output.

Before hardware sign-off, confirm the Weather topic/payload names with other consumers, tune the ten-minute outdoor freshness timeout to the Tempest update cadence, verify GPIO availability, and confirm relay input polarity/reset behavior. Then test HA discovery and commands, sensor readings, physical fan switching, and OTA on-device.
