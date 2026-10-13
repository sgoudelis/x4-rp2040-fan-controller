#!/usr/bin/env python3
"""Read RP2040 USB diagnostics for a bounded interval, then exit normally."""

import argparse
import os
import select
import sys
import time


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("port", help="RP2040 USB serial device")
    parser.add_argument("--seconds", type=float, default=10.0)
    args = parser.parse_args()
    if not 0 < args.seconds <= 120:
        parser.error("--seconds must be greater than 0 and at most 120")

    descriptor = os.open(args.port, os.O_RDONLY | os.O_NONBLOCK | os.O_NOCTTY)
    deadline = time.monotonic() + args.seconds
    try:
        while (remaining := deadline - time.monotonic()) > 0:
            ready, _, _ = select.select([descriptor], [], [], remaining)
            if not ready:
                break
            try:
                chunk = os.read(descriptor, 4096)
            except BlockingIOError:
                continue
            if not chunk:
                break
            sys.stdout.buffer.write(chunk)
            sys.stdout.buffer.flush()
    finally:
        os.close(descriptor)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
