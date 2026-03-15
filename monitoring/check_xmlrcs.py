#!/usr/bin/env python3
"""Nagios check for XmlRcs availability and progress.

Checks:
1. TCP connectivity to XmlRcs service.
2. Retrieves `stat` output and parses the `io` counter.
3. Compares `io` with previous run from a state file.
"""

from __future__ import annotations

import argparse
import re
import socket
import sys
from pathlib import Path


OK = 0
WARNING = 1
CRITICAL = 2
UNKNOWN = 3


def nagios_exit(code: int, message: str) -> None:
    print(message)
    sys.exit(code)


def fetch_stat(host: str, port: int, timeout: float) -> str:
    try:
        with socket.create_connection((host, port), timeout=timeout) as sock:
            sock.settimeout(timeout)
            sock.sendall(b"stat\n")

            chunks = []
            payload = b""
            while True:
                try:
                    data = sock.recv(4096)
                except socket.timeout:
                    break
                if not data:
                    break
                chunks.append(data)
                payload += data
                # XmlRcs replies with a single <stat>...</stat> block. Stop
                # reading as soon as we have the closing tag to avoid waiting
                # for the socket timeout when server keeps connection open.
                if b"</stat>" in payload:
                    break

            if not chunks:
                raise RuntimeError("received empty response")

            return payload.decode("utf-8", errors="replace")
    except (OSError, RuntimeError) as exc:
        nagios_exit(CRITICAL, f"CRITICAL - cannot query {host}:{port}: {exc}")


def parse_io(stat_text: str) -> int:
    # Typical response contains: "io 124661" inside <stat>...</stat>
    match = re.search(r"\bio\s+(\d+)\b", stat_text)
    if not match:
        nagios_exit(UNKNOWN, f"UNKNOWN - could not parse io counter from response: {stat_text!r}")
    return int(match.group(1))


def read_previous_value(state_file: Path) -> int | None:
    if not state_file.exists():
        return None

    try:
        raw = state_file.read_text(encoding="ascii").strip()
        return int(raw)
    except Exception as exc:
        nagios_exit(UNKNOWN, f"UNKNOWN - failed reading state file {state_file}: {exc}")


def write_current_value(state_file: Path, value: int) -> None:
    try:
        state_file.parent.mkdir(parents=True, exist_ok=True)
        state_file.write_text(f"{value}\n", encoding="ascii")
    except Exception as exc:
        nagios_exit(UNKNOWN, f"UNKNOWN - failed writing state file {state_file}: {exc}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Nagios check for XmlRcs")
    parser.add_argument("--host", default="huggle-rc.wmflabs.org")
    parser.add_argument("--port", type=int, default=8822)
    parser.add_argument("--timeout", type=float, default=5.0)
    parser.add_argument("--state-file", default="/tmp/xmlrcs_test/last_io")

    args = parser.parse_args()
    state_file = Path(args.state_file)

    stat_response = fetch_stat(args.host, args.port, args.timeout)
    io_value = parse_io(stat_response)

    previous = read_previous_value(state_file)
    write_current_value(state_file, io_value)

    if previous is None:
        nagios_exit(
            OK,
            f"OK - XmlRcs reachable, io={io_value} (initialized baseline)|io={io_value}",
        )

    if io_value > previous:
        nagios_exit(
            OK,
            f"OK - XmlRcs reachable, io increased {previous}->{io_value}|io={io_value};prev_io={previous}",
        )

    nagios_exit(
        CRITICAL,
        f"CRITICAL - XmlRcs reachable but io did not increase (prev={previous}, current={io_value})|io={io_value};prev_io={previous}",
    )


if __name__ == "__main__":
    main()
