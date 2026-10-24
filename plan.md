# Window Opener Project Plan

## Status

Planning is complete. This project will build the first window opener from the ESP32 Project Foundation on the local `Window-opener` branch. The firmware will control the left window; a right-side opener may be added later. Hardware, live Home Assistant behavior, and OTA operation must be verified on the device before sign-off.

## Goal

Control a window with a linear actuator using separate open and close relay outputs. Accept commands from Home Assistant over MQTT, report the commanded state, and automatically close on a Home Assistant rain-start event. The actuator's internal limit switches stop physical travel at each end of stroke. There is no position feedback to the ESP32, so reported endpoints are assumed rather than measured.

Device ID: `WindowOpener_Left`.

## Inherited Foundation

Retain the foundation's Wi-Fi connection and retry, MQTT transport and reconnect, ArduinoOTA, board configuration, MQTT availability, and startup LED indication. Preserve the slow-blue Wi-Fi and fast-blue MQTT flashing sequence. After startup, the onboard LED reflects window status and is not a separate HA switch.

Ensure the compiled device ID is `WindowOpener_Left`, including when the ignored local `include/secrets.h` defines `APP_DEVICE_ID`. Never commit local credentials.

## Window Control Behavior

- MQTT commands are exact, non-retained `OPEN`, `CLOSE`, and `STOP` payloads.
- `OPEN` energizes only the open relay output. `CLOSE` energizes only the close relay output.
- Each `OPEN` or `CLOSE` command immediately preempts the current action and restarts a configurable 30-second run timer.
- When the run timer expires, de-energize both relay outputs and report the commanded endpoint (`Open` or `Closed`). The actuator's internal limit switch is expected to stop travel; firmware cannot confirm the physical position.
- `STOP` immediately de-energizes both relay outputs and reports `Unknown`.
- Invalid commands do not change the outputs or reported state.
- Keep the two direction outputs mutually exclusive and initialize both inactive before networking starts.

## LED Status

After startup indication, use the onboard WS2812 on GPIO48:

| Window status | LED |
| --- | --- |
| Open | Blue |
| Closed | Red |
| Unknown | Green |
| Opening / Closing | Preserve the previous endpoint color |

## MQTT and Home Assistant

Use MQTT and Home Assistant MQTT discovery on the foundation broker. Advertise a cover entity for the window opener. Publish retained status and discovery, and retain the foundation's online/offline availability behavior. Commands and weather events are non-retained.

| Purpose | Topic | Payload / retention |
| --- | --- | --- |
| Window command | `devices/WindowOpener_Left/window/set` | Exact `OPEN`, `CLOSE`, or `STOP`; not retained |
| Window status | `devices/WindowOpener_Left/window/state` | `Opening`, `Closing`, `Open`, `Closed`, or `Unknown`; retained |
| Availability | `devices/WindowOpener_Left/availability` | `online` / `offline`; retained with MQTT Last Will |
| Home Assistant discovery | `homeassistant/cover/WindowOpener_Left/window/config` | Retained JSON configuration |
| Rain-start event | `weather/events` | `{"event":"rain_started"}`; not retained |

On each rain-start event, start the same timed close operation as a `CLOSE` command. A later explicit `OPEN`, `CLOSE`, or `STOP` command preempts it; another rain-start event starts another close operation. There is no continuing-rain lockout. The event is intentionally non-retained, so an opener that is offline when rain starts can miss it. Home Assistant automation will be set up separately later.

Use exact payload matching and ignore malformed or unrelated weather messages. The retained Celsius temperature topic `weather/outdoor_temperature_c` is not used by this project.

## Hardware and Configuration

- Target board: Lonely Binary ESP32-S3 Gold Edition N16R8.
- Onboard WS2812 LED: GPIO48.
- Two configurable digital relay outputs, one for each direction; both active-high.
- The open and close GPIO assignments and the run duration must be configurable in `include/AppConfig.h`.
- Verify GPIO availability, relay polarity, and relay behavior during ESP32 reset/boot before connecting the actuator.
- Confirm the relay hardware and actuator power are appropriately rated. GPIO pins are logic outputs only.
- Enclosure design is out of scope for the first implementation.

## Implementation Sequence

1. Add the `WindowOpener_Left` identity and opener-specific configuration while retaining local-only credentials.
2. Implement a testable, nonblocking window actuator controller with timer reset, command preemption, Stop, and mutually exclusive outputs.
3. Add MQTT command, weather-event subscription, retained state, availability, and Home Assistant cover discovery while preserving Wi-Fi, OTA, and startup behavior.
4. Adapt the onboard LED to show window status after startup.
5. Add focused host and MQTT protocol tests.
6. Replace the foundation README content with window-opener documentation following the supplied Shed_Ventilation README structure: status, startup/indicator, HA behavior and MQTT topics, wiring/safety, board/programming, build, and validation. Include the rain-event payload and offline-event limitation.

## Validation

- Host tests: timer expiry and reset; repeated and reversing commands; immediate preemption; Stop; rain close; output exclusivity; status and LED mapping; timer wraparound where applicable.
- MQTT tests: cover discovery identity and topics; exact command handling; retained status; non-retained commands/events; Rain-start close behavior; invalid command/event rejection; state updates after timeout and Stop.
- Build: `pio run -e esp32-s3-uart`.
- Hardware: confirm Wi-Fi/MQTT startup flashes, relay outputs inactive at boot, correct direction and polarity, actuator internal end stops, 30-second timeout, command preemption, Stop, rain close, and LED colors.
- HA: verify cover discovery, command/state behavior, availability, and later configure the automation to publish `{"event":"rain_started"}` to `weather/events` on the rain-start transition.
- OTA: verify the uploader hostname matches the device's OTA hostname and that MQTT/HA entities return after update.
- Commit the verified changes locally on `Window-opener`; do not push.

## Decisions

- Device ID: `WindowOpener_Left`; a right-side opener is a future separate device.
- Active-high open and close relay outputs, never energized simultaneously.
- `OPEN`/`CLOSE` start or replace a 30-second timed run; `STOP` turns outputs off and reports `Unknown`.
- No position sensor: endpoint state after timeout is the commanded endpoint, assumed based on the actuator's internal limit switches.
- Rain event: non-retained JSON `{"event":"rain_started"}` on `weather/events`; offline event loss is accepted.
- LED is blue at `Open`, red at `Closed`, green at `Unknown`, and preserves the last endpoint color during motion.