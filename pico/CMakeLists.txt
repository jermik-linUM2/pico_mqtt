# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(bme280_mqtt C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(bme280_mqtt
	main.c
    bme280.c)

target_include_directories(bme280_mqtt PRIVATE ${CMAKE_CURRENT_LIST_DIR} )

target_compile_definitions(bme280_mqtt PRIVATE
WIFI_SSID=\"${WIFI_SSID}\"
WIFI_PASSWORD=\"${WIFI_PASSWORD}\")

# pull in common dependencies and additional spi hardware support
target_link_libraries(bme280_mqtt 
pico_stdlib 
hardware_spi 
pico_cyw43_arch_lwip_threadsafe_background 
pico_lwip_mqtt)

# create map/bin/hex file etc.
pico_add_extra_outputs(bme280_mqtt)

pico_enable_stdio_usb(bme280_mqtt 1)
pico_enable_stdio_uart(bme280_mqtt 0)

# add url via pico_set_program_url
