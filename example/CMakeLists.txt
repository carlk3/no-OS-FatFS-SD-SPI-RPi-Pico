cmake_minimum_required(VERSION 3.13)

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(FatFS_SPI_example C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise the Pico SDK
pico_sdk_init()

add_subdirectory(../FatFs_SPI build)

# Add executable. Default name is the project name, version 0.1
add_executable(FatFS_SPI_example 
    FatFS_SPI_example.c 
    data_log_demo.c
    tests/simple.c
    tests/app4-IO_module_function_checker.c
    tests/big_file_test.c
    tests/CreateAndVerifyExampleFiles.c
    tests/ff_stdio_tests_with_cwd.c
)
# Add the standard library to the build
target_link_libraries(FatFS_SPI_example pico_stdlib)

target_compile_options(FatFS_SPI_example PUBLIC -Wall -Wextra -Wno-unused-function -Wno-unused-parameter)
IF (NOT DEFINED N_SD_CARDS)
    SET(N_SD_CARDS 1)
ENDIF()
target_compile_definitions(FatFS_SPI_example PUBLIC DEBUG N_SD_CARDS=${N_SD_CARDS})

pico_set_program_name(FatFS_SPI_example "FatFS_SPI_example")
pico_set_program_version(FatFS_SPI_example "0.1")

pico_enable_stdio_uart(FatFS_SPI_example 1)
pico_enable_stdio_usb(FatFS_SPI_example 0)

target_link_libraries(FatFS_SPI_example
    FatFs_SPI
    hardware_clocks
    hardware_adc
)

pico_add_extra_outputs(FatFS_SPI_example)

