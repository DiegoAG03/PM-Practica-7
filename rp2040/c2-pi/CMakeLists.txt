cmake_minimum_required(VERSION 3.13)

# Importar el SDK de la tarjeta (debe ir antes de project)
include(../picoSDK_import.cmake)

# Importar el Kernel de FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

project(c2-pi C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# --- EXPERIMENTO 2: CÁLCULO MULTINÚCLEO DE PI ---
add_executable(c2-pi main.c aux.c)

# Incluir el directorio actual para que el compilador detecte FreeRTOSConfig.h
target_include_directories(c2-pi PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Ligar librerías base, el kernel de FreeRTOS y el gestor de memoria (Heap 4)
target_link_libraries(c2-pi pico_stdlib FreeRTOS-Kernel FreeRTOS-Kernel-Heap4)

# Habilitar salida estándar por USB
pico_enable_stdio_usb(c2-pi 1)
pico_enable_stdio_uart(c2-pi 0)

# Generar el archivo .uf2 
pico_add_extra_outputs(c2-pi)
