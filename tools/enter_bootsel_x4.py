#!/usr/bin/env python3
"""Pulse the Radxa X4 host GPIOs documented for RP2040 BOOTSEL entry.

Requires libgpiod 2.x Python bindings and direct access to the host GPIO chip.
Run only after confirming gpiochip0 offsets 17 and 7 belong to this X4.
"""

import argparse
import time

import gpiod
from gpiod.line import Direction, Value


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--chip", default="/dev/gpiochip0")
    args = parser.parse_args()

    low = gpiod.LineSettings(direction=Direction.OUTPUT, output_value=Value.INACTIVE)
    with gpiod.request_lines(
        args.chip,
        consumer="x4-rp2040-bootsel",
        config={17: low, 7: low},
    ) as lines:
        # Follow Radxa's order: select RP2040 USB, pulse its reset, release.
        lines.set_value(17, Value.ACTIVE)
        time.sleep(0.05)
        lines.set_value(7, Value.ACTIVE)
        time.sleep(1.0)
        lines.set_value(17, Value.INACTIVE)
        time.sleep(0.05)
        lines.set_value(7, Value.INACTIVE)
        time.sleep(0.1)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
