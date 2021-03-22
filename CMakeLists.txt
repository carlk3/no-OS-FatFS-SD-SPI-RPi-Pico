cmake_minimum_required(VERSION 3.13)

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(no-OS-FatFS C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise the Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(no-OS-FatFS 
    no-OS-FatFS.c 
    data_log_demo.c
    tests/simple.c
    tests/app4-IO_module_function_checker.c
    tests/big_file_test.c
    tests/CreateAndVerifyExampleFiles.c
    tests/ff_stdio_tests_with_cwd.c
)
# Add the standard library to the build
target_link_libraries(no-OS-FatFS pico_stdlib)

target_compile_options(no-OS-FatFS PUBLIC -Wall -Wextra -Wno-unused-function -Wno-unused-parameter)
#target_compile_definitions(no-OS-FatFS PUBLIC DEBUG)

pico_set_program_name(no-OS-FatFS "no-OS-FatFS")
pico_set_program_version(no-OS-FatFS "0.1")

pico_enable_stdio_uart(no-OS-FatFS 1)
pico_enable_stdio_usb(no-OS-FatFS 0)

add_library(FatFs_SPI INTERFACE)
target_sources(FatFs_SPI INTERFACE
    ff14a/source/ffsystem.c
    ff14a/source/ffunicode.c
    ff14a/source/ff.c
    sd_driver/sd_spi.c
    sd_driver/demo_logging.c
    sd_driver/hw_config.c
    sd_driver/spi.c
    sd_driver/sd_card.c
    sd_driver/crc.c
    sd_driver/my_debug.c
    src/glue.c
    src/f_util.c
    src/ff_stdio.c
    src/my_debug.c
    src/rtc.c
)
target_include_directories(FatFs_SPI INTERFACE
    ff14a/source
    sd_driver
    include
)
target_link_libraries(FatFs_SPI INTERFACE
        hardware_spi
        hardware_dma
        hardware_rtc
        pico_stdlib
)

target_link_libraries(no-OS-FatFS
    FatFs_SPI
    hardware_clocks
    hardware_adc
)

pico_add_extra_outputs(no-OS-FatFS)

