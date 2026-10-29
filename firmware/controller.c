// SPDX-License-Identifier: MIT
// Standalone temperature control for the *stock* Radxa X4 25 W PoE+ HAT fan.
// DS18B20 is on RP2040 GPIO 4; HAT fan control is on GPIO 13 (active low).
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"

#define HAT_SENSOR_GPIO 4u
#define HAT_FAN_GPIO 13u
#define PWM_TOP 4999u           // 25 kHz at the default 125 MHz system clock.
#define SENSOR_INTERVAL_MS 1250u
#define SENSOR_CONVERSION_MS 800u
#define FAN_SPINUP_MS 700u

// A quiet helper-fan curve for temperature measured *on the HAT*, not the CPU.
// Escalation thresholds for stages 1..4. De-escalation has 2 C hysteresis.
static const int16_t stage_threshold_c[4] = {42, 48, 53, 58};
static const uint8_t stage_fan_percent[5] = {0, 40, 60, 80, 100};

static void fan_set_percent(uint8_t percent) {
    if (percent > 100u) percent = 100u;
    // Q8 in the HAT inverts the GPIO: low = full-on, high = off.
    // PWM output's high portion is therefore the *off* portion.
    uint16_t high_ticks = (uint16_t)(((uint32_t)(100u - percent) *
                                      (PWM_TOP + 1u)) / 100u);
    pwm_set_gpio_level(HAT_FAN_GPIO, high_ticks);
}

static void fan_gpio_full_on(void) {
    gpio_init(HAT_FAN_GPIO);
    gpio_put(HAT_FAN_GPIO, 0);
    gpio_set_dir(HAT_FAN_GPIO, GPIO_OUT);
}

static void fan_init_full_on(void) {
    // Set the safe state before switching the pin to PWM.
    fan_gpio_full_on();

    uint slice = pwm_gpio_to_slice_num(HAT_FAN_GPIO);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 1.0f);
    pwm_config_set_wrap(&config, PWM_TOP);
    pwm_init(slice, &config, false);
    fan_set_percent(100);
    gpio_set_function(HAT_FAN_GPIO, GPIO_FUNC_PWM);
    pwm_set_enabled(slice, true);
}

static void sensor_drive_low(void) {
    gpio_put(HAT_SENSOR_GPIO, 0);
    gpio_set_dir(HAT_SENSOR_GPIO, GPIO_OUT);
}

static void sensor_release(void) {
    gpio_set_dir(HAT_SENSOR_GPIO, GPIO_IN);
}

static bool onewire_reset(void) {
    uint32_t irq = save_and_disable_interrupts();
    sensor_drive_low();
    busy_wait_us(480);
    sensor_release();
    busy_wait_us(70);
    bool present = !gpio_get(HAT_SENSOR_GPIO);
    busy_wait_us(410);
    restore_interrupts(irq);
    return present && gpio_get(HAT_SENSOR_GPIO);
}

static void onewire_write_bit(bool value) {
    uint32_t irq = save_and_disable_interrupts();
    sensor_drive_low();
    busy_wait_us(value ? 6 : 60);
    sensor_release();
    busy_wait_us(value ? 64 : 10);
    restore_interrupts(irq);
}

static bool onewire_read_bit(void) {
    uint32_t irq = save_and_disable_interrupts();
    sensor_drive_low();
    busy_wait_us(3);
    sensor_release();
    busy_wait_us(12);
    bool value = gpio_get(HAT_SENSOR_GPIO);
    busy_wait_us(55);
    restore_interrupts(irq);
    return value;
}

static void onewire_write_byte(uint8_t value) {
    for (unsigned bit = 0; bit < 8; ++bit) {
        onewire_write_bit((value & 1u) != 0);
        value >>= 1;
    }
}

static uint8_t onewire_read_byte(void) {
    uint8_t value = 0;
    for (unsigned bit = 0; bit < 8; ++bit) {
        if (onewire_read_bit()) value |= (uint8_t)(1u << bit);
    }
    return value;
}

static uint8_t dallas_crc8(const uint8_t *data, unsigned length) {
    uint8_t crc = 0;
    for (unsigned i = 0; i < length; ++i) {
        uint8_t byte = data[i];
        for (unsigned bit = 0; bit < 8; ++bit) {
            uint8_t mix = (crc ^ byte) & 1u;
            crc >>= 1;
            if (mix) crc ^= 0x8cu;
            byte >>= 1;
        }
    }
    return crc;
}

static bool read_hat_temperature_q16(int16_t *out) {
    if (!onewire_reset()) return false;
    onewire_write_byte(0xcc); // SKIP ROM: the HAT has one DS18B20.
    onewire_write_byte(0x44); // CONVERT T.
    sleep_ms(SENSOR_CONVERSION_MS);

    if (!onewire_reset()) return false;
    onewire_write_byte(0xcc);
    onewire_write_byte(0xbe); // READ SCRATCHPAD.
    uint8_t scratchpad[9];
    for (unsigned i = 0; i < sizeof(scratchpad); ++i) {
        scratchpad[i] = onewire_read_byte();
    }
    if (dallas_crc8(scratchpad, sizeof(scratchpad)) != 0) return false;

    int16_t raw = (int16_t)((uint16_t)scratchpad[0] |
                            ((uint16_t)scratchpad[1] << 8));
    // Reject disconnected/noisy readings and the power-on default 85 C.
    if (raw < -20 * 16 || raw > 100 * 16 || raw == 85 * 16) return false;
    *out = raw;
    return true;
}

static uint8_t stage_for_temperature(int16_t temp_q16, uint8_t stage) {
    while (stage < 4 && temp_q16 >= stage_threshold_c[stage] * 16) {
        ++stage;
    }
    while (stage > 0 &&
           temp_q16 < (stage_threshold_c[stage - 1] - 2) * 16) {
        --stage;
    }
    return stage;
}

int main(void) {
#if X4_FAN_SENSOR_PROBE_ONLY
    fan_gpio_full_on();
#else
    fan_init_full_on();
#endif
    gpio_init(HAT_SENSOR_GPIO);
    sensor_release(); // HAT provides the external one-wire pull-up.
    stdio_init_all();
#if X4_FAN_SENSOR_PROBE_ONLY
    puts("sensor_probe: fan held on by GPIO13; sensor read begins in 5 seconds");
    sleep_ms(5000);
#endif

    uint8_t stage = 4; // Safe at startup and on sensor failure.
    uint8_t applied_percent = 100;
    while (true) {
        int16_t temperature_q16 = 0;
        if (!read_hat_temperature_q16(&temperature_q16)) {
            stage = 4;
            applied_percent = 100;
#if !X4_FAN_SENSOR_PROBE_ONLY
            fan_set_percent(100);
#endif
            puts("sensor_error fan_percent=100");
        } else {
#if X4_FAN_SENSOR_PROBE_ONLY
            stage = 4;
            applied_percent = 100;
            printf("hat_temp_q16=%d fan_percent=100 mode=sensor_probe\n",
                   temperature_q16);
#else
            uint8_t next_stage = stage_for_temperature(temperature_q16, stage);
            uint8_t next_percent = stage_fan_percent[next_stage];
            if (applied_percent == 0 && next_percent > 0) {
                fan_set_percent(100);
                sleep_ms(FAN_SPINUP_MS);
            }
            fan_set_percent(next_percent);
            stage = next_stage;
            applied_percent = next_percent;
            printf("hat_temp_q16=%d fan_percent=%u stage=%u\n",
                   temperature_q16, applied_percent, stage);
#endif
        }
        sleep_ms(SENSOR_INTERVAL_MS);
    }
}
