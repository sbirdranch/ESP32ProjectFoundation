# Window Opener Project Plan

## Status

Implementation is complete on the local `window-opener` branch, based on the ESP32 Project Foundation. Controller/output, MQTT service protocol (fake transport with real JSON), LED and foundation startup host tests pass; the UART firmware build passes. Hardware, live broker/HA and OTA checks are pending; no firmware upload or push has been performed. The firmware will control the left window; a right-side opener may be added later. Hardware, live Home Assistant behavior, and OTA operation must be verified on the device before sign-off.

## Goal

Control a window with a linear actuator using separate open and close relay outputs. Accept commands from Home Assistant over MQTT, report the commanded state, and automatically close on a Home Assistant rain-start event. The actuator's internal limit switches stop physical travel at each end of stroke. There is no position feedback to the ESP32, so reported endpoints are assumed rather than measured.

Device ID: `WindowOpener_Left`.

## Inherited Foundation

Retain the foundation's Wi-Fi connection and retry, MQTT transport and reconnect, ArduinoOTA, board configuration, MQTT availability, and startup LED indication. Preserve the slow-blue Wi-Fi and fast-blue MQTT flashing sequence. After startup, the onboard LED reflects window status and is exposed as a separate read-only Home Assistant sensor showing its current color. HA cannot command the LED independently of window status.

Ensure the compiled device ID is `WindowOpener_Left`, including when the ignored local `include/secrets.h` defines `APP_DEVICE_ID`. Never commit local credentials.

## Window Control Behavior

- MQTT commands are exact, non-retained `OPEN`, `CLOSE`, and `STOP` payloads.
- `OPEN` energizes only the open relay output. `CLOSE` energizes only the close relay output.
- Each `OPEN` or `CLOSE` command immediately preempts the current action. When reversing direction, de-energize both outputs for a configurable 500 ms before energizing the new direction. Start the configurable 30-second run timer when the requested direction output is energized; repeated same-direction commands restart that timer.
- When the run timer expires, de-energize both relay outputs and report the commanded endpoint (`Open` or `Closed`). The actuator's internal limit switch is expected to stop travel; firmware cannot confirm the physical position.
- `STOP` immediately de-energizes both relay outputs and reports `Unknown`.
- Invalid commands do not change the outputs or reported state.
- Keep the two direction outputs mutually exclusive and initialize both inactive before networking starts.
- On every boot, publish `Unknown` with both outputs inactive, regardless of the previous retained MQTT state.
- Wi-Fi or MQTT loss does not interrupt timed movement; the local run timer still de-energizes the outputs when it expires.
- Before an OTA update, stop both outputs, cancel pending movement, and set status to `Unknown`. Publish that status if MQTT is available; after reboot, publish `Unknown` again.

## LED Status

After startup indication, use the onboard WS2812 on GPIO48 with steady state colors:

![Window status LED guide: Open is blue, Closed is green, Unknown is red.](../assets/window-led-states.svg)

| Window status | LED |
| --- | --- |
| Open | Blue |
| Closed | Green |
| Unknown | Red |
| Opening / Closing | Preserve the previous endpoint color |

Unknown applies after boot, STOP, or OTA interruption. During movement from Unknown, keep red until a run completes. Open/Closed are assumed endpoints, not measured positions. The slow/fast blue startup flashes indicate network connection progress; steady blue after startup indicates Open.

Publish the current LED color (`blue`, `green`, or `red`) as its own read-only HA sensor entity. The sensor mirrors the color actually displayed and is not an independent light control.

## MQTT and Home Assistant

Use MQTT and Home Assistant MQTT discovery on the foundation broker. Advertise a cover entity for the window opener. Publish retained status and discovery, and retain the foundation's online/offline availability behavior. Commands and weather events are non-retained.

| Purpose | Topic | Payload / retention |
| --- | --- | --- |
| Window command | `devices/WindowOpener_Left/window/set` | Exact `OPEN`, `CLOSE`, or `STOP`; not retained |
| Window status | `devices/WindowOpener_Left/window/state` | `Opening`, `Closing`, `Open`, `Closed`, or `Unknown`; retained |
| Onboard LED color | `devices/WindowOpener_Left/led/state` | `blue`, `green`, or `red`; retained |
| Availability | `devices/WindowOpener_Left/availability` | `online` / `offline`; retained with MQTT Last Will |
| Home Assistant discovery | `homeassistant/cover/WindowOpener_Left/window/config` | Retained JSON configuration |
| LED sensor discovery | `homeassistant/sensor/WindowOpener_Left/status_led/config` | Retained JSON configuration; read-only color sensor |
| Rain-start event | `weather/events` | `{"event":"rain_started"}`; not retained |

On each rain-start event, start the same timed close operation as a `CLOSE` command. A later explicit `OPEN`, `CLOSE`, or `STOP` command preempts it; another rain-start event starts another close operation. There is no continuing-rain lockout. The event is intentionally non-retained, so an opener that is offline when rain starts can miss it. Home Assistant automation will be set up separately later.

Use exact payload matching and ignore malformed or unrelated weather messages. The retained Celsius temperature topic `weather/outdoor_temperature_c` is not used by this project.

Publishers are responsible for ensuring commands and rain events are non-retained. Firmware is not required to detect or reject retained deliveries.

## Hardware and Configuration

- Target board: Lonely Binary ESP32-S3 Gold Edition N16R8.
- Onboard WS2812 LED: GPIO48.
- Two configurable digital relay outputs, one for each direction; both active-high.
- Default OPEN output: GPIO10. Default CLOSE output: GPIO11.
- Default direction-reversal pause: 500 ms with both outputs inactive; configurable in `include/AppConfig.h`.
- The open and close GPIO assignments and the run duration must be configurable in `include/AppConfig.h`.
- Verify GPIO availability, relay polarity, and relay behavior during ESP32 reset/boot before connecting the actuator.
- Confirm the relay hardware and actuator power are appropriately rated. GPIO pins are logic outputs only.
- Enclosure design is out of scope for the first implementation.

## Implementation Sequence

1. Add the `WindowOpener_Left` identity and opener-specific configuration while retaining local-only credentials.
2. Implement a testable, nonblocking window actuator controller with timer reset, command preemption, Stop, and mutually exclusive outputs.
3. Add MQTT command, weather-event subscription, retained state, availability, Home Assistant cover discovery, and read-only LED-color sensor discovery while preserving Wi-Fi, OTA, and startup behavior.
4. Adapt the onboard LED to show window status after startup and publish its color to the LED sensor whenever that color changes.
5. Add focused host and MQTT protocol tests.
6. Replace the foundation README content with window-opener documentation following the supplied Shed_Ventilation README structure: status, startup/indicator, HA behavior and MQTT topics, wiring/safety, board/programming, build, and validation. Include the rain-event payload and offline-event limitation.

## Validation

- Host tests: timer expiry and reset; repeated and reversing commands; immediate preemption; Stop; rain close; output exclusivity; status and LED mapping; timer wraparound where applicable.
- Include reversal-delay timing, commands/Stop during the reversal pause, boot-to-Unknown, continued movement during network loss, and OTA cancellation of active/pending movement.
- MQTT tests: cover and LED sensor discovery identity/topics; exact command handling; retained window and LED-color states; non-retained commands/events; Rain-start close behavior; invalid command/event rejection; state updates after timeout and Stop.
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
- LED is blue at `Open`, green at `Closed`, red at `Unknown`, and preserves the last endpoint color during motion.
- Expose LED color as a separate read-only HA sensor (`blue`, `green`, `red`); it is status-driven and has no command topic.
- OPEN uses GPIO10 and CLOSE uses GPIO11 by default; both are configurable.
- Direction reversal includes a configurable 500 ms pause with both outputs inactive; the run timer starts upon energizing the new direction.
- Boot always starts with outputs inactive and state `Unknown`.
- Network loss allows timed movement to finish. OTA stops outputs, cancels pending movement, and sets state to `Unknown`.
- Non-retained command/event enforcement belongs to publishers.

## Implementation Validation ? 2026-10-24

- Passed: `tools/test_window.ps1` ? controller/output timing and exclusivity (including uint32 wraparound), MQTT service protocol with fake transport and real ArduinoJson, LED colors/startup handoff, and foundation startup regression.
- Passed: 10 existing foundation Python MQTT wire regression tests using workspace-local Paho dependency.
- Passed: `pio run -e esp32-s3-uart` ? RAM 49,160 bytes, flash 774,585 bytes.
- Passed: `git diff --check`.
- Firmware uses cooperative loop timing; inherited synchronous network calls can briefly delay timer checks. The actuator's internal limit switches remain essential.
- Pending on-device validation: wiring/reset behavior, relay direction/polarity and mechanical end stops, live HA/broker discovery and rain commands, and OTA stop/reboot/reconnect. The HA rain automation remains a separate later setup.
- No firmware upload or remote push performed. Implementation and local software validation are ready for the planned local commit; physical sign-off is pending.
