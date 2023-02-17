# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
#set(PICO_SDK_PATH "/home/carlk/pi/pico/pico-sdk")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(dynamic_config_example C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(../FatFs_SPI build)

# Add executable. Default name is the project name, version 0.1
add_executable(dynamic_config_example 
    dynamic_config_example.cpp 
    hw_config.cpp
)

pico_set_program_name(dynamic_config_example "dynamic_config_example")
pico_set_program_version(dynamic_config_example "0.1")

# Choose source and destination for standard input and output:
#   See 4.1. Serial input and output on Raspberry Pi Pico in Getting started with Raspberry Pi Pico (https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
#   and 2.7.1. Standard Input/Output (stdio) Support in Raspberry Pi Pico C/C++ SDK (https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf):
pico_enable_stdio_uart(dynamic_config_example 1)
pico_enable_stdio_usb(dynamic_config_example 1)

# Add the standard library and FatFS/SPI to the build
target_link_libraries(dynamic_config_example 
    pico_stdlib
    FatFs_SPI
)

# add_compile_definitions(CARD2)

pico_add_extra_outputs(dynamic_config_example)

