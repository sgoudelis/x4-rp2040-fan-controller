# Radxa X4 RP2040 PoE HAT fan controller

Standalone firmware for the fan and DS18B20 temperature sensor on the
[Radxa 25 W PoE+ HAT for X4](https://docs.radxa.com/en/accessories/poe-hat/radxa-25w-poe-hat-for-x4).
It runs on the X4's RP2040 without a host daemon or writable host operating
system.

**Status: tested on one X4.** The full-on image spins the fan, the sensor probe
reads valid HAT temperatures, and the controller runs without a host service.
The fan was observed to slow down at 80% duty; the quieter 55% stage around
46 °C was reported as much better. The 40% and 75%
stages, threshold transitions, and behavior after a full power cycle have not
been physically checked.

## What it does

- `x4_fan_full_on.uf2` commands the stock HAT fan on continuously. Flash this
  first to check basic wiring and fan operation.
- `x4_fan_sensor_probe.uf2` also keeps the fan commanded fully on while
  reporting HAT DS18B20 readings. It checks the standalone sensor path before
  enabling the speed curve.
- `x4_fan_controller.uf2` reads the HAT's DS18B20 on RP2040 GPIO 4 and drives
  the fan control input through GPIO 13. It uses a stepped temperature curve,
  2 °C hysteresis, a 700 ms full-speed spin-up, and full speed when a sensor
  reading fails. The fan starts in the full-on state before sensor setup.
- All three images offer a USB serial diagnostic stream. The controller reports
  `hat_temp_q16` (temperature in sixteenths of a degree Celsius), the
  commanded fan percentage, and the selected stage.

`tools/read_serial.py PORT --seconds 10` reads the RP2040's USB diagnostics
for a bounded interval and exits normally. Identify `PORT` with
`/dev/serial/by-id/` so another serial peripheral is not opened by mistake.

| HAT sensor temperature | Commanded fan duty |
| --- | ---: |
| Below 38 °C | 0% |
| 38–44.9 °C | 40% |
| 45–49.9 °C | 55% |
| 50–54.9 °C | 75% |
| 55 °C and above | 100% |
| Sensor missing, CRC error, or implausible reading | 100% |

The table shows thresholds when temperature rises; a stage steps down 2 °C
below its threshold to avoid frequent switching. The HAT sensor measures its
own location, **not the Intel N100 CPU**. The stock fan has no tachometer
connection, so percentages are commands rather than measured RPM. The 55% and
80% stages have been assessed on the test X4; other stages require physical
validation on each fan.

## Build

Install CMake, an `arm-none-eabi` GCC toolchain with newlib, and Python 3.
Use [Pico SDK 2.2.0](https://github.com/raspberrypi/pico-sdk/tree/2.2.0)
with its TinyUSB submodule:

```sh
git clone --depth 1 --branch 2.2.0 https://github.com/raspberrypi/pico-sdk.git ../pico-sdk
git -C ../pico-sdk submodule update --init lib/tinyusb
export PICO_SDK_PATH="$(cd ../pico-sdk && pwd)"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

The three UF2 files appear in `build/`. Build artifacts are excluded from Git;
the source tag can be built with the commands above.

## Flash and test

Enter the X4 RP2040's BOOTSEL mode using its button or
[Radxa's host GPIO procedure](https://docs.radxa.com/en/x/x4/software/flash).
Identify the `RPI-RP2` USB device before writing. On a Linux host with USB
access, [picotool](https://github.com/raspberrypi/picotool) can load and verify
the first image:

```sh
picotool load -v -x build/x4_fan_full_on.uf2
```

After confirming that the physical fan spins, test the temperature controller
and its USB serial output. `picotool load -v -x` resets only the RP2040. The
fan can briefly stop while the RP2040 resets. A hardware BOOTSEL button is a
recovery path if application firmware fails. Linux host details are in
[docs/linux-flashing.md](docs/linux-flashing.md).

Software re-entry into BOOTSEL works on the test X4, but the tool may lose
track of the RP2040 when its USB serial identity changes. Perform re-entry
and loading as separate steps; see the Linux flashing notes.

## Sources and design provenance

The firmware is an independent implementation. These references establish the
board connections, sensor protocol, SDK calls, and flashing procedure:

| Source | What this project uses from it |
| --- | --- |
| [Radxa 25 W PoE+ HAT for X4](https://docs.radxa.com/en/accessories/poe-hat/radxa-25w-poe-hat-for-x4) | Identifies the target HAT and its onboard fan. |
| [Radxa X4 RP2040 C SDK examples, especially “PoE FAN”](https://docs.radxa.com/en/x/x4/software/c_sdk_examples#poe-fan) | Shows the HAT's DS18B20 on RP2040 GPIO 4, fan control on GPIO 13, and the active-low on/off behavior. Its temperature-switch example established the basic standalone approach. |
| [Radxa 25 W PoE+ HAT v1.2 schematic](https://dl.radxa.com/accessories/25w-poe/radxa_25w_poe_plus_hat_v1.2_schematic.pdf) | Shows the fan-control path through Q8, 5 V and ground, and no tachometer signal to the RP2040. This explains the inverted PWM output and why RPM cannot be measured here. |
| [Radxa X4 RP2040 flashing guide](https://docs.radxa.com/en/x/x4/software/flash) | Documents BOOTSEL entry and the host GPIO sequence used by `tools/enter_bootsel_x4.py`. |
| [Analog Devices DS18B20 datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ds18b20.pdf) | Defines 1-Wire timing, Skip ROM / Convert T / Read Scratchpad commands, scratchpad CRC, 12-bit conversion time, and the 85 °C power-on reading rejected by this firmware. |
| [RP2040 datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf) and [Pico SDK 2.2.0](https://github.com/raspberrypi/pico-sdk/tree/2.2.0) | Document the RP2040 PWM peripheral, GPIO, timing, and USB serial APIs used in the C implementation. |
| [Pico PWM example](https://github.com/raspberrypi/pico-examples/tree/master/pwm/led_fade) | Illustrates configuring an RP2040 PWM slice with the Pico SDK. |
| [picotool](https://github.com/raspberrypi/picotool) | Provides verified UF2 loading and software re-entry into BOOTSEL. |

Related X4 fan projects were reviewed while choosing the architecture:
[arag0re/radxa_x4-rp2040-notua-pwm](https://github.com/arag0re/radxa_x4-rp2040-notua-pwm)
uses a separately wired four-wire fan;
[0xC9C3/radxa-x4-cluster fan-manager](https://github.com/0xC9C3/radxa-x4-cluster/tree/main/fan-manager)
uses a host daemon; and
[ovirtanen/sbc-pwm-fan](https://github.com/ovirtanen/sbc-pwm-fan)
uses a host temperature service. None is a drop-in image for this stock PoE
HAT fan, and no source code was copied from them.

The 25 kHz PWM frequency, stepped quiet curve, 2 °C hysteresis, 700 ms spin-up,
and full-on sensor-fault response are choices made in this project. They are
not settings supplied by Radxa. See [docs/hardware.md](docs/hardware.md) for
the electrical interpretation and validation limits. This project is not
affiliated with Radxa.

Licensed under [MIT](LICENSE).
