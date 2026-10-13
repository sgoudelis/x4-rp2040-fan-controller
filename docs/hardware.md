# Hardware interface

The [Radxa X4 RP2040 examples](https://docs.radxa.com/en/x/x4/software/c_sdk_examples)
document the 25 W PoE+ HAT's DS18B20 on RP2040 GPIO 4 and its fan control on
GPIO 13. Radxa's example drives GPIO 13 **low for full-on** and **high for
off**. The HAT's [v1.2 schematic](https://dl.radxa.com/accessories/25w-poe/radxa_25w_poe_plus_hat_v1.2_schematic.pdf)
shows that header pin 33 passes through transistor Q8 to the stock fan's
control connection. Q8 inverts the RP2040 output. The other fan connections
are 5 V power and ground. There is no tachometer signal routed to the RP2040.

The controller uses 25 kHz hardware PWM on GPIO 13, with inverted duty to
match the HAT circuit. At 100% fan command, GPIO 13 stays low. At 0%, it
stays high. The supplied fan slowed at 80% duty compared with full-on, and
the operator found the 55% setting much quieter. The 40% and 75% stages still
need physical validation; no RPM can be inferred from the PWM register.

The DS18B20 is powered on the HAT and communicates over one-wire. Firmware
initiates a conversion, waits 800 ms, reads all nine scratchpad bytes, and
checks Dallas CRC-8. Loss of sensor presence, CRC failure, or an implausible
reading forces 100% fan command. The sensor's temperature is an indirect
cooling guide for the X4 because it is on the PoE HAT, away from the CPU die.
Short one-wire timing slots use `busy_wait_us()` while interrupts are masked;
SDK `sleep_us()` cannot be used there because it waits for an alarm interrupt.

The X4 has other RP2040 GPIO users in some builds. Flashing replaces the
RP2040 application, so review any existing RP2040 firmware functions before
using these images on a configured board.
