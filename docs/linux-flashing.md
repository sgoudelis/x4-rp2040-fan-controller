# Flashing from a Linux host

The X4's RP2040 is a separate microcontroller. Loading a UF2 resets that
microcontroller and briefly interrupts the PoE HAT fan, but does not reflash
the X4's main storage. Save any existing RP2040 application before replacing
it if your board already uses other RP2040 features.

Install [picotool](https://github.com/raspberrypi/picotool), make the built
UF2 available on the X4, and enter RP2040 BOOTSEL using the board button or
[Radxa's host GPIO procedure](https://docs.radxa.com/en/x/x4/software/flash).
Check that the RP2040 appears as USB `2e8a:0003` (`RPI-RP2`) before loading:

```sh
lsusb -d 2e8a:0003
picotool load -v -x build/x4_fan_full_on.uf2
```

The `-v` option verifies flash contents and `-x` starts the application. The
firmware normally reappears as USB `2e8a:000a` with a serial port. Confirm
that the fan spins before loading `x4_fan_controller.uf2`. Identify its port
through `/dev/serial/by-id/` when other USB serial devices are attached:

```sh
ls -l /dev/serial/by-id/
python3 tools/read_serial.py /dev/serial/by-id/<rp2040-port> --seconds 10
```

The diagnostic `fan_percent` is commanded duty, not measured RPM. Check the
physical fan at each new duty level. The HAT temperature is not a CPU die
reading.

To update running firmware, enter BOOTSEL and load in **separate commands**.
The RP2040 changes USB identity on reset, so a tool may not follow it in one
operation:

```sh
picotool reboot -f -u
lsusb -d 2e8a:0003
picotool load -v -x build/x4_fan_controller.uf2
```

If application firmware does not enumerate, use the physical BOOTSEL button.
On an X4 with access to host GPIO, `tools/enter_bootsel_x4.py` also implements
Radxa's GPIO 17/7 recovery sequence; it requires Python libgpiod 2.x and
access to `/dev/gpiochip0`. The command should then be followed by a separate
`picotool load -v -x` of the known-good full-on UF2.

On container-based Linux systems, the container running picotool needs access
to the host's USB bus. A serial diagnostic container needs only the RP2040
serial device. Container paths and permissions vary by distribution; verify
the USB identity before passing a device through.
