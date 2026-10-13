cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "Set PICO_SDK_PATH to a Pico SDK checkout (tested with 2.2.0)")
endif()

set(PICO_BOARD pico CACHE STRING "RP2040 target board")
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(x4_rp2040_fan_controller C CXX ASM)
pico_sdk_init()

add_executable(x4_fan_full_on firmware/full_on.c)
set_target_properties(x4_fan_full_on PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(x4_fan_full_on pico_stdlib)
pico_enable_stdio_usb(x4_fan_full_on 1)
pico_enable_stdio_uart(x4_fan_full_on 0)
pico_add_extra_outputs(x4_fan_full_on)

add_executable(x4_fan_controller firmware/controller.c)
set_target_properties(x4_fan_controller PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(x4_fan_controller pico_stdlib hardware_pwm hardware_sync)
pico_enable_stdio_usb(x4_fan_controller 1)
pico_enable_stdio_uart(x4_fan_controller 0)
pico_add_extra_outputs(x4_fan_controller)

add_executable(x4_fan_sensor_probe firmware/controller.c)
set_target_properties(x4_fan_sensor_probe PROPERTIES LINKER_LANGUAGE C)
target_compile_definitions(x4_fan_sensor_probe PRIVATE X4_FAN_SENSOR_PROBE_ONLY=1)
target_link_libraries(x4_fan_sensor_probe pico_stdlib hardware_pwm hardware_sync)
pico_enable_stdio_usb(x4_fan_sensor_probe 1)
pico_enable_stdio_uart(x4_fan_sensor_probe 0)
pico_add_extra_outputs(x4_fan_sensor_probe)
