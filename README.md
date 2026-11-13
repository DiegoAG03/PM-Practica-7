# Práctica 7: Cómputo concurrente y paralelo con FreeRTOS en el microcontrolador RP2040 (DualMCU)

**Autor:** Diego Ambrosio González

**Asignatura:** Procesadores Multinúcleo

Repositorio creado para la Práctica 7 de Procesadores Multinúcleo, el cual contiene la implementación de un sistema operativo en tiempo real (RTOS) utilizando FreeRTOS sobre la arquitectura de un microcontrolador RP2040. El objetivo principal es realizar cómputo concurrente y paralelo en el RP2040 mediante la gestión de múltiples tareas, protegiendo los recursos compartidos mediante el uso de *Mutex* y *Semáforos binarios*.

## Estructura del Proyecto

El repositorio está dividido en cuatro experimentos incrementales:

*   **`c1-neopixel-led/`**: 
*   **`c2-pi/`**: 
*   **`c3-neopixel-strip/`**:

## Requisitos de Software

*   Entorno Ubuntu Linux.
*   Raspberry Pi Pico SDK configurado.
*   Código fuente de FreeRTOS.
*   Toolchain de compilación: `cmake`, `make`, `gcc-arm-none-eabi`.
*   Terminal serial: `gtkterm`.

## Instrucciones de Compilación y Flasheo

### 1. Compilación del código
Abre una terminal en la raíz de la carpeta `rp2040` y ejecuta los siguientes comandos para generar los archivos binarios:

```bash
mkdir build
cd build
cmake ..
make
```
Esto generará los archivos `.uf2` ejecutables dentro de las subcarpetas correspondientes a cada experimento en el directorio `build`.

### 2. Carga del binario a la DualMCU (RP2040)
1. Conecta la placa UNIT DualMCU a la computadora vía USB mientras mantienes presionado el botón **BOOT** correspondiente al RP2040.
2. El sistema montará un dispositivo de almacenamiento masivo.
3. Arrastra y suelta el archivo `.uf2` del experimento deseado (por ejemplo, `c1-neopixel-led.uf2`) dentro de la unidad de la placa.
4. La placa se reiniciará automáticamente y comenzará a ejecutar el kernel de FreeRTOS.

### 3. Ejecución de la Interfaz Serial
Para interactuar con el menú de comandos, abre la terminal serial con permisos de superusuario configurando los baudios a 115200:

```bash
sudo gtkterm -p /dev/ttyACM0 -s 115200
```
> **Nota de hardware:** Debido a la gestión de los puertos seriales virtuales, es estrictamente necesario ir a la pestaña `Configuration -> Control signals` dentro de la ventana de `gtkterm` y habilitar la señal **DTR** en cada nueva conexión para visualizar la salida estándar.
