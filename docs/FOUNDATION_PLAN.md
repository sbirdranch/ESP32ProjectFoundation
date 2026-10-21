# Foundation Completion and Future Wall Panel Plan

This document is the foundation acceptance checklist and the design record for
the future wall panel. **Foundation functionality accepted by the user on
2026-10-21.** Detailed test results below distinguish recorded evidence from
checks whose individual outcomes have not been supplied. User acceptance does
not establish that every Home Assistant (HA) or OTA scenario below was exercised.

## Foundation completion

### Baseline and messaging contract

- Target: Lonely Binary ESP32-S3 Gold Edition N16R8, GPIO48 WS2812.
- Framework: Arduino; LED backend: Arduino core `neopixelWrite` (not FastLED).
- Firmware version: `AppConfig::FirmwareVersion` in `include/AppConfig.h`, initially
  `0.1.0`. Printed at boot and advertised as discovery `device.sw_version`.
- Broker default: `192.168.0.179:1883`; assign a unique `APP_DEVICE_ID` per device.
- Startup: slow blue Wi-Fi flashes (500 ms on/off) for at least two seconds,
  followed by fast blue MQTT flashes (250 ms on/off) for at least two more seconds.
  Each phase continues until its connection is ready. Both connections must be
  ready before startup finishes. Pending LED commands take effect afterward.

For device `esp32-foundation-01`:

| Purpose | Topic | Payload | Retained |
| --- | --- | --- | --- |
| Command, any controller to device | `devices/esp32-foundation-01/led/set` | Exactly `ON` or `OFF` | No |
| State, device to subscribers | `devices/esp32-foundation-01/led/state` | `ON` or `OFF` | Yes |
| Availability | `devices/esp32-foundation-01/led/availability` | `online` or `offline` | Yes |
| HA discovery | `homeassistant/switch/esp32-foundation-01/led/config` | JSON configuration | Yes |

State is the logical switch state, not measured LED feedback. Startup temporarily
overrides its visible color. State is published on MQTT connection/reconnection
and after each valid command, including a repeated command. Invalid commands are
ignored. Power-on resets the switch to OFF; Wi-Fi-only recovery preserves RAM
state. The broker publishes the retained offline Last Will after an unexpected
disconnect is detected; this is not instantaneous (device keepalive is 30 seconds).
These tests cover the current QoS 0 application behavior, not guaranteed delivery.

### Prepare and run MQTT tests

Test directly against the broker first, then use HA to test discovery and UI.
Use the LED demonstration device, with no fan/window actuator attached. Pause HA
automations and other controllers for this one LED while tests run. They otherwise
could produce indistinguishable state responses: the current protocol has no
command correlation IDs. Do not stop the shared broker or clear other topics.

From the repository root in PowerShell:

```powershell
python -m venv .venv-test
.\.venv-test\Scripts\python.exe -m pip install -r tools/requirements-test.txt
.\.venv-test\Scripts\python.exe tools/test_mqtt.py --device-id esp32-foundation-01
```

Wait until the firmware reports MQTT connected before starting. The client uses
its own unique ID and clean session, waits for subscription acknowledgment, and
observes retained traffic before issuing non-retained commands. Only the selected
device's command topic is written. Ordinary observation/response windows are ten
seconds; disconnect/reconnect windows are ninety seconds. Override with
`--timeout` or `--reconnect-timeout` only when recording why in the results.
Valid commands hold their reported state for two seconds before the next command,
so ten full ON/OFF cycles take at least forty seconds. Use `--hold-seconds 5` for
a slower visual check, or `--hold-seconds 0` only for rapid protocol testing.
The initial hardware run used no hold: its ten cycles completed within about one
second and were unsuitable for judging visible flashes. Lowercase, empty and
unexpected payloads are intentional rejection tests, not valid control commands.

The automated checks verify:

1. A fresh subscriber receives retained discovery, state, and online availability;
   no retained command is present.
2. Discovery has matching physical device ID, entity unique ID, names, firmware
   version, topics, and ON/OFF and availability payload definitions.
3. ON/OFF and repeated OFF commands each produce a new, non-retained state message,
   followed by ten full ON/OFF cycles. Retained state cannot satisfy these checks.
4. Lowercase `on`/`off`, empty payload, and `TOGGLE` produce no state response during
   their observation windows. A second fresh subscriber confirms retained state is
   unchanged and commands are still not retained; a valid OFF still works.

The test ends with OFF on success; if interrupted or failed, inspect the LED and
report before issuing further commands. It does not automatically clean retained
topics, restore prior state, or certify physical LED behavior.

For physical and reconnect tests:

```powershell
.\.venv-test\Scripts\python.exe tools/test_mqtt.py --device-id esp32-foundation-01 --interactive
```

Follow the prompts. Remove power, wait for the observed offline Last Will, then
restore power. Verify online, default OFF state, and working commands. Next,
interrupt **only the device's** Wi-Fi, keeping the test PC and broker connected;
wait for offline, restore Wi-Fi, and verify online, preserved switch state, and
working commands. If device-specific Wi-Fi isolation is unavailable, skip and
leave that check Pending. Do not replace this with a restart of the shared broker.
Confirm green for ON and off for OFF visually. Record startup flashes separately.

Default report: `.pio/mqtt-test-report.json` (overwritten each run). Use
`--report .pio/mqtt-before-ota.json` or another filename to preserve a run. Reports
contain local timestamps with UTC offset, firmware version, observations and check
statuses, but not credentials. Optional broker authentication uses `--username`
and the local `MQTT_TEST_PASSWORD` environment variable. Never commit passwords.

Exit codes: 0 = automated checks passed (manual checks may still be Pending);
1 = failure; 2 = interrupted or skipped interactive checks. HA and OTA always remain
manual checklist items. The script never grants overall foundation sign-off.

### If a retained command is found

Stop and inspect the exact device ID/topic. A retained ON/OFF can be replayed on
reconnect. The tester refuses to send further commands in this condition; it does
not clear the message automatically. In HA's MQTT publish tool or a trusted MQTT
client, deliberately publish an **empty retained payload to only that device's
`/led/set` topic**. This removes the saved command; the empty live command is
ignored by the foundation firmware. Rerun the test to verify removal. Do not use
wildcard/bulk cleanup or erase discovery/state messages.

### Home Assistant checklist

Use the existing MQTT integration connected to the same broker. The device's
retained discovery message provides topics; no manually configured test switch is
needed. Record each result in the table below:

- [ ] HA discovers exactly one LED switch grouped under the correct physical
  device, with no duplicate entities; device information shows firmware `0.1.0`.
- [ ] Toggle ON/OFF in HA: physical LED becomes green/off and state topic agrees.
- [ ] Run the Python test client and confirm HA follows the externally published
  commands. Observe the initial ON/OFF steps and repeated cycles.
- [ ] Power disconnect makes the entity unavailable after Last Will detection;
  reconnect restores available/OFF without a duplicate entity.
- [ ] Reload the MQTT integration and confirm discovery and state return without
  manual entity creation. Restart HA only if a reload is insufficient, and record it.

### OTA procedure

Use the existing `esp32-s3-ota` environment and ArduinoOTA, not a web updater.
The configured `default_16MB.csv` contains `ota_0` and `ota_1` application slots.
Keep `framework = arduino`, board/partition settings, unique device ID, Wi-Fi
credentials and the device OTA password unchanged throughout these version tests.
Keep UART recovery available. A UART cable may stay connected for power and logs;
that does not make an `espota` upload a serial upload.

1. With `FirmwareVersion` set to `0.1.0`, build/upload through UART:

   ```powershell
   pio run -e esp32-s3-uart -t upload
   pio device monitor -e esp32-s3-uart
   ```

   Confirm printed version, expected blue startup patterns, Wi-Fi/MQTT connected,
   `OTA ready`, and successful LED commands. Record the device IP. If startup logs
   were missed, reset with the monitor attached. Close the monitor before another
   tool needs the serial port.

2. Change only `AppConfig::FirmwareVersion` to `0.1.1`. Set the uploader password
   to an intentionally incorrect value **in the shell only**, then attempt OTA:

   ```powershell
   $env:ESP32_OTA_PASSWORD = 'intentionally-wrong-test-password'
   pio run -e esp32-s3-ota -t upload
   ```

   Require an authentication rejection, not just a timeout. Confirm `0.1.0` still
   responds to MQTT and remains the running version. Do not change firmware's
   `OTA_PASSWORD`. If credentials happen to match the test value, choose a different
   incorrect value.

3. Set `ESP32_OTA_PASSWORD` locally to the real `OTA_PASSWORD` from the device's
   configuration, without putting it in tracked files or copied logs. Upload:

   ```powershell
   pio run -e esp32-s3-ota -t upload
   ```

   The environment targets `esp32-foundation-01.local`. For another device ID,
   supply its matching hostname. If mDNS resolution fails, use the recorded IP:

   ```powershell
   pio run -e esp32-s3-ota -t upload --upload-port 192.168.0.123
   ```

   Replace the example IP with the actual device IP. For a timeout, first check the
   target address, LAN reachability, `OTA ready`, and firewall rules for the updater
   before diagnosing authentication. Do not disable the firewall globally.

4. Require successful upload and reboot into `0.1.1`, verified by serial output
   and discovery `device.sw_version`. Confirm startup patterns, Wi-Fi/MQTT, `OTA
   ready`, HA entity continuity, and ON/OFF commands. An uploader success message
   alone is insufficient evidence that the expected firmware is running.
5. Change the version to `0.1.2` and repeat successful OTA. This proves the newly
   installed firmware can itself accept another OTA update.
6. Power-cycle, verify `0.1.2` persists, and rerun the MQTT checks and HA control
   checks. Save reports for each version. Remove the shell password afterward:

   ```powershell
   Remove-Item Env:ESP32_OTA_PASSWORD
   ```

If OTA fails, retain serial/upload errors without secrets, recover by UART if
needed, fix the issue, and rerun affected checks. Do not count a UART recovery as
successful OTA. Interrupted-update testing and automatic rollback are not covered.

### Results and completion gate

Use **Pending / Passed / Failed**. A checkbox is checked only when the corresponding
row is Passed. Add runs instead of overwriting evidence; include report paths in
observations. No hardware or network results have been assumed.

| Done | Check | Status | Firmware | Date | Observations / unresolved issues |
| --- | --- | --- | --- | --- | --- |
| [x] | Host startup timing regression tests | Passed | 0.1.0 | 2026-10-21 | g++ build and execution passed; immediate/delayed connections and timer wrap tested. |
| [x] | MQTT tester local regression tests | Passed | 0.1.0 | 2026-10-21 | 10 tests passed using real Paho clients and a local simulated broker/device, including two-second pacing of valid commands; no physical device tested by these local regressions. |
| [x] | UART firmware build | Passed | 0.1.0 | 2026-10-21 | `pio run -e esp32-s3-uart` passed; not uploaded. |
| [x] | MQTT automatic checks on hardware | Passed | 0.1.0 | 2026-10-21 | Live broker/device run 12:33-12:34 EDT passed discovery, retained state/availability, no retained commands, ON/OFF/repeated commands, ten cycles and invalid payload rejection. Report: `.pio/mqtt-0.1.0-hardware.json`. Final reported state OFF; physical LED observation separate. |
| [x] | Physical LED responds to MQTT ON/OFF | Passed | 0.1.0 | 2026-10-21 | Operator confirmed correctly formatted ON/OFF commands control the physical LED. Slower visual follow-up intentionally held each state for five seconds, ran twice, then finished OFF. Other payloads were intentional rejection tests. Standard tester now holds valid states for two seconds. |
| [ ] | Startup LED sequence | Pending | 0.1.0 | | Confirm two slow blue flashes followed by four fast blue flashes for quick connections, with each phase extending if needed. |
| [ ] | Power disconnect / Last Will / restart | Pending | 0.1.0 | | |
| [ ] | Device-only Wi-Fi interruption / recovery | Pending | 0.1.0 | | |
| [ ] | HA discovery, both command sources, availability, reload | Pending | 0.1.0 | | |
| [ ] | Incorrect OTA password rejected; old version operational | Pending | 0.1.0 | | |
| [ ] | First OTA: version verified, all services recovered | Pending | 0.1.1 | | |
| [ ] | Second OTA: version verified, all services recovered | Pending | 0.1.2 | | |
| [ ] | Power-cycle persistence, MQTT and HA retest | Pending | 0.1.2 | | |
| [ ] | All failures fixed; affected checks rerun | Pending | | | |
| [x] | Foundation sign-off | Passed | 0.1.0 baseline | 2026-10-21 | User confirmed: "The foundational components work." Accepted for device-specific implementations. Individual scenarios without recorded evidence remain Pending above; no OTA version results are inferred. |

Local regression commands (PowerShell, from repository root; host tests need g++):

```powershell
.\.venv-test\Scripts\python.exe -m unittest discover -s test/python -v
g++ -std=c++17 -Wall -Wextra -Werror -I test/host/stubs -I include -I lib/LedSwitch/src -I lib/StartupIndicator/src test/host/startup_indicator_test.cpp lib/LedSwitch/src/LedSwitch.cpp lib/StartupIndicator/src/StartupIndicator.cpp -o .pio/startup_indicator_test.exe
.\.pio\startup_indicator_test.exe
pio run -e esp32-s3-uart
```

The full validation target is to pass all acceptance rows with recorded firmware
versions/dates/observations and no unresolved failures. The user has accepted the
foundation as operational for device-specific implementations. Remaining Pending
rows track detailed validation evidence still to be recorded; implementation,
local tests, and broad acceptance do not substitute for those individual results.

## Future wall panel

This section records future requirements, not implemented functionality.

### Device identity and capabilities

A physical device has one stable device identifier and one or more independently
advertised capabilities, each with a unique entity ID. The foundation LED is a
demonstration, not a required identity or topic name for every future product.

| Physical device | Capabilities | HA discovery types |
| --- | --- | --- |
| Shed controller | Fan control, temperature, humidity | `fan`, `sensor`, `sensor` |
| Window controller | Open/close control and reported state/position | `cover` |

Replace or extend LED-specific topic names, payloads, discovery types, units, names
and command handling for each implementation. Group capabilities using the shared
physical device identifier. Sensor capabilities have state topics but no command
topic; use `OPEN`/`CLOSE`/optional `STOP` semantics for windows rather than LED ON/OFF.
Report measured window/fan state when hardware feedback is available; distinguish
requested state from measured feedback when designing those implementations.

### Panel discovery and operation

- Both HA and the panel connect directly to the broker. Neither requires the other
  to be running. The broker/network remain required.
- Each capability publishes retained discovery at
  `homeassistant/<component>/<object-id>/config`. The panel subscribes to
  `homeassistant/+/+/config`, matching this project's single-object-ID structure.
  The broader HA protocol also allows an optional node segment and device-based
  discovery; those formats are outside the initial panel support.
- Parse supported fields: stable IDs, shared device identifiers, names, component
  type, sensor units/classes, command/state/availability topics and payload
  mappings. Advertise simple scalar state payloads for the initial interoperable
  subset; do not assume the panel can evaluate arbitrary HA/Jinja templates.
- Implement `switch` (foundation demo), `fan`, `sensor`, and `cover` support
  explicitly. Ignore unsupported discovery types/configurations gracefully. MQTT
  does not automatically create touchscreen controls.
- Subscribe to advertised state/availability topics; show unavailable devices
  distinctly, and do not interpret stale retained state as proof of being online.
  Publish non-retained commands only to advertised command topics. Update displayed
  state from device reports so external HA/script commands appear on the panel too.
- A setup screen lists discovered capabilities. Users choose what to display and
  save selections by stable entity unique ID in panel storage, not hard-coded
  firmware device IDs. Resolve current topics from discovery after restart.
- Apply discovery updates without duplicate controls, unsubscribe obsolete topics,
  and remove discovered entries when their configuration is replaced by an empty
  payload. Keep unavailable or removed saved selections recognizable in setup so
  users can replace/remove them.
- Initial layout: two windows, shed ventilation fan, shed temperature, optionally
  humidity. Panel hardware, layout, persistence implementation and firmware remain
  a separate project.

The broker is the shared communication point; HA discovery is the common
description format. Do not add a second competing discovery schema for this panel.

References:

- [HA MQTT discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)
- [HA MQTT fan](https://www.home-assistant.io/integrations/fan.mqtt/)
- [HA MQTT sensor](https://www.home-assistant.io/integrations/sensor.mqtt/)
- [HA MQTT cover](https://www.home-assistant.io/integrations/cover.mqtt/)
- [Paho Python client](https://eclipse.dev/paho/files/paho.mqtt.python/html/client.html)
- [Espressif ArduinoOTA](https://github.com/espressif/arduino-esp32/tree/master/libraries/ArduinoOTA)

Future work excludes fan hardware, sensors, window actuators, panel UI,
interrupted-update testing and automatic firmware rollback from foundation sign-off.
