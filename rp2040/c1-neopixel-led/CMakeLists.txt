cmake_minimum_required(VERSION 3.13)

# Importar el SDK de la tarjeta (debe ir antes de project)
include(../picoSDK_import.cmake)

# Importar el Kernel de FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

project(c1-neopixel-led C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

pico_sdk_init()

# --- EXPERIMENTO 1: CONTROL DEL LED NEOPIXEL ---
add_executable(c1-neopixel-led main.cpp neopixel.cpp aux.c)

# Generar el archivo de cabecera PIO «ws2812.pio.h» dentro del directorio src
pico_generate_pio_header(c1-neopixel-led ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)

# Incluir el directorio actual para que el compilador detecte FreeRTOSConfig.h
target_include_directories(c1-neopixel-led PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Ligar librerías base, el kernel de FreeRTOS, el gestor de memoria (Heap 4), el ADC y el WS2812B
target_link_libraries(c1-neopixel-led pico_stdlib hardware_pio hardware_adc FreeRTOS-Kernel FreeRTOS-Kernel-Heap4)

# Habilitar salida estándar por USB
pico_enable_stdio_usb(c1-neopixel-led 1)
pico_enable_stdio_uart(c1-neopixel-led 0)

# Generar el archivo .uf2 
pico_add_extra_outputs(c1-neopixel-led)
