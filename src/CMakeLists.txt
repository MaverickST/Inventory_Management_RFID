cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD "pico_w")

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)


project(InventoryManagement)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(invmanage
	main.c
	functs.c
	keypad_irq.c
	inventory.c
	nfc_rfid.c
	liquid_crystal_i2c.c
)

target_include_directories(invmanage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(invmanage 
	pico_stdlib
	pico_flash
	hardware_timer
	pico_cyw43_arch_none 
	hardware_gpio 
	hardware_pwm 
	hardware_irq 
	hardware_sync
	hardware_i2c)

pico_enable_stdio_uart(invmanage 0)
pico_enable_stdio_usb(invmanage 1)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(invmanage)

# Memory usage
SET(GCC_EXE_LINKER_FLAGS    "-Wl,--print-memory-usage")

SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_EXE_LINKER_FLAGS}")
