// SPDX-License-Identifier: MIT
// First-stage hardware check: leave the stock PoE HAT fan commanded on.
#include <stdio.h>
#include "pico/stdlib.h"

#define HAT_FAN_GPIO 13u

int main(void) {
    // Radxa's HAT fan example specifies GPIO 13 low = on, high = off.
    gpio_init(HAT_FAN_GPIO);
    gpio_put(HAT_FAN_GPIO, 0);
    gpio_set_dir(HAT_FAN_GPIO, GPIO_OUT);

    stdio_init_all();
    while (true) {
        puts("x4_fan_full_on: GPIO13 low; fan commanded on");
        sleep_ms(5000);
    }
}
