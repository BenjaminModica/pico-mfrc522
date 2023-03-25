cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_MFRC522 C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico_MFRC522
            pico_MFRC522.c
            )

pico_enable_stdio_usb(pico_MFRC522 1)
pico_enable_stdio_uart(pico_MFRC522 0)

pico_add_extra_outputs(pico_MFRC522)

target_link_libraries(pico_MFRC522 
            pico_stdlib
            hardware_spi
            )