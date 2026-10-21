"""Local MQTT wire tests: real Paho clients, simulated broker/device, no LAN access."""

import contextlib
import io
import json
from pathlib import Path
import socket
import socketserver
import struct
import tempfile
import threading
import unittest
from unittest.mock import patch

from tools import test_mqtt as runner


DEVICE = "local-test-only"
TOPICS = runner.topics_for(DEVICE)


def discovery():
    return {
        "name": "Test LED", "unique_id": f"{DEVICE}_led",
        "command_topic": TOPICS["command"], "state_topic": TOPICS["state"],
        "availability_topic": TOPICS["availability"],
        "payload_on": "ON", "payload_off": "OFF", "state_on": "ON", "state_off": "OFF",
        "payload_available": "online", "payload_not_available": "offline",
        "device": {"identifiers": [DEVICE], "name": "Test board", "sw_version": "0.1.0"},
    }


def field(value):
    return struct.pack("!H", len(value)) + value


def packet(header, payload):
    length = len(payload)
    encoded = bytearray([header])
    while True:
        digit = length % 128
        length //= 128
        encoded.append(digit | (128 if length else 0))
        if not length:
            return bytes(encoded) + payload


def read_exact(stream, count):
    data = stream.read(count)
    if len(data) != count:
        raise EOFError
    return data


class Handler(socketserver.StreamRequestHandler):
    def handle(self):
        self.request.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.subscriptions = set()
        with self.server.lock:
            self.server.clients.add(self)
        try:
            while True:
                header = read_exact(self.rfile, 1)[0]
                length, multiplier = 0, 1
                while True:
                    digit = read_exact(self.rfile, 1)[0]
                    length += (digit & 127) * multiplier
                    multiplier *= 128
                    if digit < 128:
                        break
                payload = read_exact(self.rfile, length)
                with self.server.lock:
                    if header >> 4 == 1:  # CONNECT
                        self.request.sendall(packet(0x20, b"\x00\x00"))
                    elif header >> 4 == 8:  # SUBSCRIBE, QoS 0 subscriptions
                        offset, topics = 2, []
                        while offset < len(payload):
                            size = int.from_bytes(payload[offset:offset + 2], "big")
                            topics.append(payload[offset + 2:offset + 2 + size].decode())
                            offset += size + 3
                        self.subscriptions.update(topics)
                        self.request.sendall(packet(0x90, payload[:2] + bytes(len(topics))))
                        for topic in topics:
                            if topic in self.server.retained:
                                self.send(topic, self.server.retained[topic], retained=True)
                    elif header >> 4 == 3:  # PUBLISH from test runner
                        size = int.from_bytes(payload[:2], "big")
                        topic = payload[2:2 + size].decode()
                        data = payload[2 + size:]
                        self.server.received.append((topic, data, bool(header & 1)))
                        if topic != TOPICS["command"]:
                            continue
                        if data in (b"ON", b"OFF"):
                            if not self.server.drop_responses:
                                self.server.emit_state(data)
                        elif self.server.accept_invalid:
                            self.server.emit_state(b"OFF")
                    elif header >> 4 == 12:  # PINGREQ
                        self.request.sendall(packet(0xD0, b""))
                    elif header >> 4 == 14:  # DISCONNECT
                        break
        except (EOFError, ConnectionError, OSError):
            pass
        finally:
            with self.server.lock:
                self.server.clients.discard(self)

    def send(self, topic, payload, retained=False):
        self.request.sendall(packet(0x31 if retained else 0x30, field(topic.encode()) + payload))


class Broker(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def __init__(self):
        super().__init__(("127.0.0.1", 0), Handler)
        self.lock = threading.RLock()
        self.clients = set()
        self.received = []
        self.drop_responses = False
        self.accept_invalid = False
        self.retained = {
            TOPICS["discovery"]: json.dumps(discovery()).encode(),
            TOPICS["state"]: b"OFF", TOPICS["availability"]: b"online",
        }

    def emit_state(self, state):
        self.emit(TOPICS["state"], state)

    def emit(self, topic, payload):
        with self.lock:
            self.retained[topic] = payload
            for client in list(self.clients):
                if topic in client.subscriptions:
                    client.send(topic, payload)


class RunnerTests(unittest.TestCase):
    def setUp(self):
        self.broker = Broker()
        self.thread = threading.Thread(target=self.broker.serve_forever, kwargs={"poll_interval": 0.01})
        self.thread.start()
        self.output = contextlib.redirect_stdout(io.StringIO())
        self.output.__enter__()

    def tearDown(self):
        self.broker.shutdown()
        self.broker.server_close()
        self.thread.join()
        self.output.__exit__(None, None, None)

    def run_script(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            code = runner.main([
                "--device-id", DEVICE, "--host", "127.0.0.1",
                "--port", str(self.broker.server_address[1]),
                "--timeout", "0.15", "--hold-seconds", "0", "--report", str(report),
            ])
            return code, json.loads(report.read_text())

    def test_complete_wire_run_and_manual_results_pending(self):
        code, report = self.run_script()
        self.assertEqual(code, 0)
        self.assertEqual(report["firmware_version"], "0.1.0")
        self.assertEqual(report["foundation_signoff"], "Pending")
        self.assertTrue(any(item["name"] == "Physical LED" and item["status"] == "Pending"
                            for item in report["checks"]))
        self.assertEqual(len(self.broker.received), 29)
        self.assertTrue(all(topic == TOPICS["command"] and not retained
                            for topic, _, retained in self.broker.received))
        self.assertEqual(self.broker.retained[TOPICS["state"]], b"OFF")
        self.assertNotIn(TOPICS["command"], self.broker.retained)

    def test_retained_command_aborts_before_any_publish(self):
        self.broker.retained[TOPICS["command"]] = b"ON"
        code, report = self.run_script()
        self.assertEqual(code, 1)
        self.assertEqual(self.broker.received, [])
        self.assertIn("Retained command exists", str(report))

    def test_retained_state_cannot_satisfy_command_response(self):
        self.broker.drop_responses = True
        self.broker.retained[TOPICS["state"]] = b"ON"
        code, report = self.run_script()
        self.assertEqual(code, 1)
        self.assertIn("Timed out", str(report))

    def test_invalid_command_acceptance_is_detected(self):
        self.broker.accept_invalid = True
        code, report = self.run_script()
        self.assertEqual(code, 1)
        self.assertIn("Invalid command caused", str(report))

    def test_missing_retained_discovery(self):
        del self.broker.retained[TOPICS["discovery"]]
        code, report = self.run_script()
        self.assertEqual(code, 1)
        self.assertEqual(self.broker.received, [])
        self.assertIn("No retained discovery", str(report))

    def test_discovery_identity_and_types(self):
        for change in ({"command_topic": "unrelated/set"}, {"device": []}, {"device": {"identifiers": DEVICE}}):
            config = discovery() | change
            with self.assertRaises(AssertionError):
                runner.validate_discovery(json.dumps(config).encode(), DEVICE, TOPICS)
        with self.assertRaises(AssertionError):
            runner.validate_discovery(b"[]", DEVICE, TOPICS)

    def test_wildcard_device_id_is_rejected(self):
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
            runner.parse_args(["--device-id", "+"])

    def test_operator_assisted_power_and_wifi_recovery(self):
        args = runner.parse_args([
            "--device-id", DEVICE, "--host", "127.0.0.1",
            "--port", str(self.broker.server_address[1]), "--timeout", "1", "--hold-seconds", "0",
        ])
        report = runner.Report(args)
        with runner.Probe(args, TOPICS) as probe:
            probe.observe(0.05) # Finish receiving initial retained messages.
            for power_cycle in (True, False):
                def operator(prompt):
                    if prompt.startswith("Restore"):
                        self.broker.emit(TOPICS["availability"], b"online")
                        self.broker.emit_state(b"OFF" if power_cycle else b"ON")
                    else:
                        self.broker.emit(TOPICS["availability"], b"offline")
                    return ""
                with patch("builtins.input", side_effect=operator):
                    runner.manual_reconnect(probe, report, power_cycle)
        self.assertEqual(len(report.data["checks"]), 8)
        self.assertTrue(all(item["status"] == "Passed" for item in report.data["checks"]))

    def test_skipped_operator_check_is_pending(self):
        args = runner.parse_args([
            "--device-id", DEVICE, "--host", "127.0.0.1",
            "--port", str(self.broker.server_address[1]), "--timeout", "1", "--hold-seconds", "0",
        ])
        report = runner.Report(args)
        with runner.Probe(args, TOPICS) as probe:
            probe.observe(0.05)
            with patch("builtins.input", return_value="s"):
                runner.manual_reconnect(probe, report, True)
        self.assertEqual(report.data["checks"][0]["status"], "Pending")

    def test_valid_command_holds_after_response(self):
        args = runner.parse_args([
            "--device-id", DEVICE, "--host", "127.0.0.1",
            "--port", str(self.broker.server_address[1]), "--timeout", "1",
        ])
        self.assertEqual(args.hold_seconds, 2)
        with runner.Probe(args, TOPICS) as probe:
            probe.observe(0.05)
            with patch.object(probe, "observe") as observe:
                probe.command(b"ON")
                observe.assert_called_once_with(2)


if __name__ == "__main__":
    unittest.main()
