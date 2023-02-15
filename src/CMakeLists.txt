add_library(FatFs_SPI INTERFACE)

pico_generate_pio_header(FatFs_SPI ${CMAKE_CURRENT_LIST_DIR}/sd_driver/SDIO/rp2040_sdio.pio)

target_compile_definitions(FatFs_SPI INTERFACE
    PICO_MAX_SHARED_IRQ_HANDLERS=8u
)

target_sources(FatFs_SPI INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/ff15/source/ff.c
    ${CMAKE_CURRENT_LIST_DIR}/ff15/source/ffsystem.c
    ${CMAKE_CURRENT_LIST_DIR}/ff15/source/ffunicode.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/sd_card.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/SDIO/rp2040_sdio.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/SDIO/sd_card_sdio.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/SPI/crc.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/SPI/sd_spi.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/SPI/sd_card_spi.c
    ${CMAKE_CURRENT_LIST_DIR}/sd_driver/SPI/spi.c
    ${CMAKE_CURRENT_LIST_DIR}/source/f_util.c
    ${CMAKE_CURRENT_LIST_DIR}/source/ff_stdio.c
    ${CMAKE_CURRENT_LIST_DIR}/source/glue.c
    ${CMAKE_CURRENT_LIST_DIR}/source/my_debug.c
    ${CMAKE_CURRENT_LIST_DIR}/source/rtc.c
)
target_include_directories(FatFs_SPI INTERFACE
    ff15/source
    sd_driver
    tests
    include
)
target_link_libraries(FatFs_SPI INTERFACE
    hardware_dma
    hardware_pio
    hardware_rtc
    hardware_spi
    pico_stdlib
    cmsis_core
)
