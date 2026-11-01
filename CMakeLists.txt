cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_FETCH_FROM_GIT ON)
include(pico_sdk_import.cmake)

project(true_lxio_pico)

option(ENABLE_WS2812_SUPPORT "Enable WS2812 LED support" ON)
option(WS2812_IS_RGBW "Enable WS2812 for RGBW" OFF)
option(ENABLE_DEMO "Enable WS2812 RGB demo when not used" ON)

pico_sdk_init()

add_executable(true_lxio_pico
    main.c)

target_sources(true_lxio_pico PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/true_lxio_ws2812.c
        ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
        ${CMAKE_CURRENT_LIST_DIR}/other_drivers.c
        )

# Make sure TinyUSB can find tusb_config.h
target_include_directories(true_lxio_pico PUBLIC
        ${CMAKE_CURRENT_LIST_DIR})

pico_generate_pio_header(true_lxio_pico ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(true_lxio_pico
                      PUBLIC
                      pico_stdlib
                      pico_multicore
                      tinyusb_board
                      tinyusb_device
                      hardware_pio
                      hardware_dma
                      hardware_irq
                      hardware_flash
                      pico_flash)

if(ENABLE_WS2812_SUPPORT)
    target_compile_definitions(true_lxio_pico PRIVATE ENABLE_WS2812_SUPPORT=1)
else()
    # Core 1 is never launched, so flash_safe_execute doesn't need to lock it out
    target_compile_definitions(true_lxio_pico PRIVATE PICO_FLASH_ASSUME_CORE1_SAFE=1)
endif()

if(WS2812_IS_RGBW)
    target_compile_definitions(true_lxio_pico PRIVATE WS2812_IS_RGBW=true)
endif()

if(ENABLE_DEMO)
    target_compile_definitions(true_lxio_pico PRIVATE ENABLE_DEMO=1)
endif()

# Uncomment this line to enable fix for Errata RP2040-E5 (the fix requires use of GPIO 15)
#target_compile_definitions(true_lxio_pico PUBLIC PICO_RP2040_USB_DEVICE_ENUMERATION_FIX=1)

pico_add_extra_outputs(true_lxio_pico)
pico_enable_stdio_usb(true_lxio_pico 0)
pico_enable_stdio_uart(true_lxio_pico 0)
pico_add_extra_outputs(true_lxio_pico)

add_compile_definitions(
    PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
)
