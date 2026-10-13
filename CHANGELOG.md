# Changelog

## Unreleased

- Reduce fan command from 55% to 40% at HAT temperatures from 45 to 49.9 °C.
  The 75% and full-speed high-temperature stages and sensor-fault response are
  unchanged. Physical fan response at 40% is awaiting validation.

## v0.1.0 — 2026-10-13

- Standalone DS18B20-controlled PWM firmware for the Radxa X4 25 W PoE+ HAT.
- Quiet temperature curve with 2 °C hysteresis, fan spin-up, and full-on
  fallback for invalid sensor readings.
- Full-on and sensor-probe images, USB diagnostics, and BOOTSEL recovery tool.
- Tested on one X4: sensor readings, full-on operation, and physical fan
  response at 80% and 55% duty. The 55% stage was reported as much quieter.
  The 40% and 75% stages, threshold transitions, and full power-cycle behavior
  remain to be checked.
