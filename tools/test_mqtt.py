"""Exercise one foundation LED device. Never publishes retained commands.

Run with --help. Requires exclusive control of the selected LED during testing.
Reports protocol observations, not physical hardware or HA/OTA sign-off.
"""

import argparse
from dataclasses import dataclass
from datetime import datetime
import json
import math
import os
from pathlib import Path
import re
import threading
import time
import uuid

import paho.mqtt.client as mqtt


def timestamp():
    return datetime.now().astimezone().isoformat(timespec="seconds")


def log(message):
    print(f"{timestamp()} {message}", flush=True)


def topics_for(device_id):
    base = f"devices/{device_id}/led"
    return {
        "command": f"{base}/set",
        "state": f"{base}/state",
        "availability": f"{base}/availability",
        "discovery": f"homeassistant/switch/{device_id}/led/config",
    }


@dataclass(frozen=True)
class Observation:
    topic: str
    payload: bytes
    retained: bool
    received_at: str


class Probe:
    """Fresh, clean-session subscriber with SUBACK and message sequencing."""

    def __init__(self, args, topics):
        self.args = args
        self.topics = topics
        self.messages = []
        self.condition = threading.Condition()
        self.ready = threading.Event()
        self.error = None
        self.client = mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2,
            client_id=f"foundation-test-{uuid.uuid4().hex[:16]}",
            clean_session=True,
            reconnect_on_failure=False,
        )
        self.client.connect_timeout = args.timeout
        if args.username:
            self.client.username_pw_set(args.username, os.environ.get("MQTT_TEST_PASSWORD"))
        self.client.on_connect = self.on_connect
        self.client.on_subscribe = self.on_subscribe
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message

    def fail(self, message):
        with self.condition:
            self.error = message
            self.condition.notify_all()
        self.ready.set()

    def on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code.is_failure:
            self.fail(f"Broker connection rejected: {reason_code}")
            return
        result, _ = client.subscribe([(topic, 0) for topic in self.topics.values()])
        if result != mqtt.MQTT_ERR_SUCCESS:
            self.fail("Unable to subscribe")

    def on_subscribe(self, client, userdata, mid, reason_codes, properties):
        if any(code.is_failure for code in reason_codes):
            self.fail("Broker rejected a subscription")
        self.ready.set()

    def on_disconnect(self, client, userdata, flags, reason_code, properties):
        self.fail("Test client disconnected; results after this point are incomplete")

    def on_message(self, client, userdata, message):
        observation = Observation(message.topic, bytes(message.payload), bool(message.retain), timestamp())
        with self.condition:
            self.messages.append(observation)
            self.condition.notify_all()
        log(f"RX {observation.topic} retain={observation.retained} {observation.payload!r}")

    def __enter__(self):
        try:
            self.client.connect(self.args.host, self.args.port, keepalive=30)
            self.client.loop_start()
            if not self.ready.wait(self.args.timeout):
                raise AssertionError("Timed out waiting for broker connection/SUBACK")
            self.check()
            return self
        except BaseException:
            self.close()
            raise

    def close(self):
        self.client.disconnect()
        self.client.loop_stop()

    def __exit__(self, *exc):
        self.close()

    def check(self):
        if self.error:
            raise AssertionError(self.error)

    def mark(self):
        with self.condition:
            self.check()
            return len(self.messages)

    def since(self, mark):
        with self.condition:
            self.check()
            return list(self.messages[mark:])

    def wait_for(self, predicate, mark=0, timeout=None):
        deadline = time.monotonic() + (self.args.timeout if timeout is None else timeout)
        with self.condition:
            while True:
                self.check()
                for message in self.messages[mark:]:
                    if predicate(message):
                        return message
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise AssertionError("Timed out waiting for expected MQTT message")
                self.condition.wait(remaining)

    def observe(self, seconds):
        deadline = time.monotonic() + seconds
        with self.condition:
            while time.monotonic() < deadline:
                self.check()
                self.condition.wait(max(0, deadline - time.monotonic()))
            self.check()

    def publish(self, payload):
        mark = self.mark()
        log(f"TX {self.topics['command']} retain=False {payload!r}")
        info = self.client.publish(self.topics["command"], payload, qos=0, retain=False)
        if info.rc != mqtt.MQTT_ERR_SUCCESS:
            raise AssertionError("Command publish failed")
        info.wait_for_publish(timeout=self.args.timeout)
        if not info.is_published():
            raise AssertionError("Timed out sending command")
        return mark

    def command(self, payload):
        mark = self.publish(payload)
        # First subsequent state must match; retained startup state cannot pass.
        message = self.wait_for(
            lambda item: item.topic == self.topics["state"] and not item.retained, mark
        )
        if message.payload != payload:
            raise AssertionError(f"Command {payload!r} reported {message.payload!r}")
        # An immediate state response proves messaging, but is too quick for a
        # person to see. Hold before allowing the next valid command.
        self.observe(self.args.hold_seconds)


def retained_snapshot(probe):
    # Keep observing for a full timeout, including the otherwise-silent command
    # topic. MQTT has no 'all retained messages received' completion packet.
    probe.observe(probe.args.timeout)
    messages = probe.since(0)
    if any(item.topic == probe.topics["command"] and item.retained for item in messages):
        raise AssertionError(
            "Retained command exists. STOP: see docs/FOUNDATION_PLAN.md for targeted removal."
        )
    snapshot = {}
    for key in ("discovery", "state", "availability"):
        retained = [item for item in messages if item.topic == probe.topics[key] and item.retained]
        if not retained:
            raise AssertionError(f"No retained {key} received by fresh subscriber")
        snapshot[key] = retained[-1].payload
    if snapshot["state"] not in (b"ON", b"OFF"):
        raise AssertionError("Invalid retained switch state")
    if snapshot["availability"] != b"online":
        raise AssertionError("Device is not online; start firmware and rerun")
    return snapshot


def validate_discovery(payload, device_id, topics):
    config = json.loads(payload)
    if not isinstance(config, dict):
        raise AssertionError("Discovery must be a JSON object")
    expected = {
        "unique_id": f"{device_id}_led",
        "command_topic": topics["command"],
        "state_topic": topics["state"],
        "availability_topic": topics["availability"],
        "payload_on": "ON", "payload_off": "OFF",
        "state_on": "ON", "state_off": "OFF",
        "payload_available": "online", "payload_not_available": "offline",
    }
    for key, value in expected.items():
        if config.get(key) != value:
            raise AssertionError(f"Discovery {key} must be {value!r}")
    device = config.get("device", {})
    if not isinstance(device, dict):
        raise AssertionError("Discovery device must be a JSON object")
    identifiers = device.get("identifiers", [])
    if not isinstance(identifiers, list) or device_id not in identifiers:
        raise AssertionError("Discovery physical device identifier does not match")
    if not all(isinstance(value, str) and value for value in
               (config.get("name"), device.get("name"), device.get("sw_version"))):
        raise AssertionError("Discovery must include entity/device names and firmware sw_version")
    return device["sw_version"]


class Report:
    def __init__(self, args):
        self.data = {
            "started_at": timestamp(), "device_id": args.device_id,
            "broker": f"{args.host}:{args.port}", "firmware_version": None,
            "hold_seconds": args.hold_seconds,
            "checks": [], "observations": [],
        }

    def record(self, name, status, detail=""):
        self.data["checks"].append({"name": name, "status": status, "date": timestamp(), "detail": detail})
        log(f"{status}: {name}" + (f" - {detail}" if detail else ""))

    def run(self, name, action):
        try:
            result = action()
        except (AssertionError, ValueError, OSError, RuntimeError) as exc:
            self.record(name, "Failed", str(exc))
            raise
        self.record(name, "Passed")
        return result


def manual_reconnect(probe, report, power_cycle):
    label = "Power disconnect/restart" if power_cycle else "Wi-Fi interruption/recovery"
    probe.command(b"ON")
    mark = probe.mark()
    action = "Remove device power" if power_cycle else "Disconnect ONLY the device from Wi-Fi (keep the test PC and broker online)"
    answer = input(f"{action}, then press Enter; type s to skip: ").strip().lower()
    if answer == "s":
        report.record(label, "Pending", "Operator skipped")
        return
    report.run(f"{label}: offline Last Will", lambda: probe.wait_for(
        lambda item: item.topic == probe.topics["availability"] and item.payload == b"offline" and not item.retained,
        mark, probe.args.reconnect_timeout,
    ))
    mark = probe.mark()
    input("Restore power/Wi-Fi, then press Enter: ")
    report.run(f"{label}: online", lambda: probe.wait_for(
        lambda item: item.topic == probe.topics["availability"] and item.payload == b"online" and not item.retained,
        mark, probe.args.reconnect_timeout,
    ))
    expected = b"OFF" if power_cycle else b"ON"
    def restored_state():
        message = probe.wait_for(lambda item: item.topic == probe.topics["state"] and not item.retained,
                                 mark, probe.args.reconnect_timeout)
        if message.payload != expected:
            raise AssertionError(f"Expected restored state {expected!r}, got {message.payload!r}")
    report.run(f"{label}: restored state", restored_state)
    report.run(f"{label}: commands resume", lambda: (probe.command(b"ON"), probe.command(b"OFF")))


def exercise(args, report):
    topics = topics_for(args.device_id)
    with Probe(args, topics) as probe:
        try:
            snapshot = report.run("Fresh retained snapshot and no retained command", lambda: retained_snapshot(probe))
            version = report.run("Discovery contract", lambda: validate_discovery(snapshot["discovery"], args.device_id, topics))
            report.data["firmware_version"] = version
            report.run("ON/OFF responses", lambda: (probe.command(b"ON"), probe.command(b"OFF")))
            report.run("Repeated OFF response", lambda: probe.command(b"OFF"))
            def cycles():
                for _ in range(10):
                    probe.command(b"ON")
                    probe.command(b"OFF")
            report.run("Ten ON/OFF cycles", cycles)
            probe.command(b"ON")
            for payload in (b"on", b"off", b"", b"TOGGLE"):
                def invalid(payload=payload):
                    mark = probe.publish(payload)
                    probe.observe(args.timeout)
                    if any(item.topic == topics["state"] for item in probe.since(mark)):
                        raise AssertionError("Invalid command caused an unexpected state response")
                report.run(f"Reject invalid command {payload!r}", invalid)
            with Probe(args, topics) as fresh:
                def final_snapshot():
                    current = retained_snapshot(fresh)
                    validate_discovery(current["discovery"], args.device_id, topics)
                    if current["state"] != b"ON":
                        raise AssertionError("Invalid commands changed retained state")
                report.run("Fresh retained state unchanged; commands remain unretained", final_snapshot)
            report.run("Valid command still works after invalid commands", lambda: probe.command(b"OFF"))
            if args.interactive:
                for power_cycle in (True, False):
                    manual_reconnect(probe, report, power_cycle)
                answer = input("Did you observe green for ON and off for OFF? [y/n/s]: ").strip().lower()
                report.record("Physical LED", {"y": "Passed", "n": "Failed"}.get(answer, "Pending"))
            else:
                for name in ("Power disconnect/restart", "Wi-Fi interruption/recovery", "Physical LED"):
                    report.record(name, "Pending", "Rerun with --interactive and an operator")
        finally:
            report.data["observations"].extend(vars(item) | {"payload": item.payload.decode("utf-8", errors="replace")}
                                               for item in probe.messages)


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--device-id", required=True, help="Explicit target APP_DEVICE_ID; test controls its LED")
    parser.add_argument("--host", default="192.168.0.179")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--username", help="Optional broker user; password comes from MQTT_TEST_PASSWORD")
    parser.add_argument("--timeout", type=float, default=10, help="Ordinary response/observation seconds (default 10)")
    parser.add_argument("--hold-seconds", type=float, default=2,
                        help="Hold each valid command's state before continuing (default 2; 0 for rapid protocol tests)")
    parser.add_argument("--reconnect-timeout", type=float, default=90, help="Disconnect/reconnect seconds (default 90)")
    parser.add_argument("--interactive", action="store_true", help="Include operator-assisted disconnect and LED checks")
    parser.add_argument("--report", type=Path, default=Path(".pio/mqtt-test-report.json"))
    args = parser.parse_args(argv)
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", args.device_id):
        parser.error("device-id must contain only letters, digits, underscores and hyphens")
    if (not all(math.isfinite(value) for value in (args.timeout, args.reconnect_timeout, args.hold_seconds))
            or args.timeout <= 0 or args.reconnect_timeout <= 0 or args.hold_seconds < 0
            or not 1 <= args.port <= 65535):
        parser.error("timeouts must be finite and positive, hold-seconds finite and nonnegative, and port 1..65535")
    return args


def main(argv=None):
    args = parse_args(argv)
    report = Report(args)
    exit_code = 0
    try:
        log("Pause other controllers/automations for this LED. Tests finish with OFF on success.")
        exercise(args, report)
    except (AssertionError, ValueError, OSError, RuntimeError) as exc:
        report.record("Run completion", "Failed", str(exc))
        exit_code = 1
    except (EOFError, KeyboardInterrupt):
        report.record("Run completion", "Pending", "Interrupted; remaining checks not performed")
        exit_code = 2
    finally:
        for name in ("Home Assistant checklist", "OTA checklist"):
            report.record(name, "Pending", "Record manual results in docs/FOUNDATION_PLAN.md")
        report.data["finished_at"] = timestamp()
        report.data["foundation_signoff"] = "Pending"
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report.data, indent=2) + "\n", encoding="utf-8")
        log(f"Report saved to {args.report}; foundation sign-off remains Pending")
    statuses = [item["status"] for item in report.data["checks"]]
    if "Failed" in statuses:
        return 1
    if args.interactive and "Pending" in [item["status"] for item in report.data["checks"]
                                          if item["name"] not in ("Home Assistant checklist", "OTA checklist")]:
        return 2
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
