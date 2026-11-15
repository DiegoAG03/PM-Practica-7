/*----------------------------------------------------------------
 * Experimento 3: Control de tira LED NeoPixel por USB
 * Autor: Diego Ambrosio González
 *----------------------------------------------------------------
*/

// Librerias del Kernel
#include <FreeRTOS.h>
#include <task.h>

// Librerias de hardware
#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/mutex.h>
#include <hardware/gpio.h>

#include"neopixel.h"

// Macros para la tira LED Neopixel
#define NEOPIX_DIN 15  // Pin de datos de la tira LED Neopixel
#define NUM_PIXELS  8  // Número de LEDs de la tira LED 

// Variables compartidas por las tareas
volatile uint16_t svHue        = 0;
volatile uint32_t svDelay      = 100;

// Mutex utilizado para sincronizar el acceso a las variables compartidas
static mutex_t mutex;

// Configuración de hardware
static void setup();

// Tarea 1: Supervisión del LED Neopixel
static void neopixelTask( void *param );

// Tarea 2: Lectura del puerto USB
static void usbTask( void *param );

// Lee un valor de tipo uint32_t de stdin y lo muestra en stdout.
// Cuando esta función regresa, contiene el valor leído.
// True si se leyó un entero sin signo; False en caso contrario.
static bool readInt(uint32_t* i);

// Analiza la cadena interpretando su contenido como un número entero.
// buffer: Cadena que comienza con la representación de un número entero.
// endptr: Referencia a un objeto de tipo char*, cuyo valor es establecido por la función al siguiente carácter en str después del valor numérico.
// Si la operación tiene éxito, la función devuelve el número entero convertido como un valor de tipo uint32_t.
static uint32_t atou(char* buffer, char** endptr);

int main( void ){
	setup();

	xTaskCreate( neopixelTask,              // Función que implementa la tarea.
				 "neopixel",                // Nombre asignado a la tarea.
				 configMINIMAL_STACK_SIZE,  // Tamaño asignado en el stack para la tarea.
				 NULL,                      // Parámetro asignado a la tarea.
				 tskIDLE_PRIORITY,          // Prioridad asignada a la tarea.
				 NULL );                    // Identificador de la tarea.

	xTaskCreate( usbTask,                   // Función que implementa la tarea.
				 "usb",                     // Nombre asignado a la tarea.
				 configMINIMAL_STACK_SIZE,  // Tamaño asignado en el stack para la tarea.
				 NULL,                      // Parámetro asignado a la tarea.
				 tskIDLE_PRIORITY,          // Prioridad asignada a la tarea.
				 NULL );                    // Identificador de la tarea.

	mutex_init(&mutex);

	vTaskStartScheduler();
	for( ;; );
	return 0;
}

// Función de configuración del hardware
static void setup(){
	stdio_init_all();

	// Inicialización del pin GPIO para la tira de LED
	gpio_init(NEOPIX_DIN);
	gpio_set_dir(NEOPIX_DIN, GPIO_OUT);
	gpio_put(NEOPIX_DIN, 0);
}

// Tarea 1: Supervisión del LED Neopixel
static void neopixelTask( void *param ){
	uint16_t hue;
	uint32_t delay;
	int8_t   current_led = 0;
	int8_t   step = 1; // 1 avanza y -1 retrocede

	vTaskDelay(pdMS_TO_TICKS(500));
	printf("Starting...\n");

	NeoPixel np = NeoPixel(NEOPIX_DIN, NUM_PIXELS);

	while(true){
		mutex_enter_blocking(&mutex);
		hue   = svHue;
		delay = svDelay;
		mutex_exit(&mutex);

		for(int8_t i = 0; i < NUM_PIXELS; ++i){
			float brightness;
			if(i == current_led){
				brightness = 1.0f;  // LED principal (100%)
			} else if(i == current_led - 1 || i == current_led + 1){
				brightness = 0.75f; // LEDs adyacentes (75%)
			} else {
				brightness = 0.5f;  // Resto de la tira (50%)
			}
			np.setHSV(i, hue, 1.0f, brightness);
		}
		np.write();

		current_led += step;
		if(current_led >= NUM_PIXELS - 1){
			current_led = NUM_PIXELS - 1;
			step = -1; // Rebota hacia la izquierda
		} else if(current_led <= 0){
			current_led = 0;
			step = 1;  // Rebota hacia la derecha
		}

		vTaskDelay(pdMS_TO_TICKS(delay));
	}
}

// Tarea 2: Lectura del puerto USB
static void usbTask( void *param ){
	uint32_t hue;
	uint32_t delay;
	vTaskDelay(pdMS_TO_TICKS(550));
	while(true){
		printf("NeoPixel LED strip hue [0-360]: ");
		if(!readInt(&hue) || hue>360){
			printf("\nInvalid input (must be 0-360).\n");
			continue;
		}
		printf("Input delay (ms): ");
		if(!readInt(&delay) || delay<10){
			printf("\nInvalid value (min 10 ms).\n");
			continue;
		}
		mutex_enter_blocking(&mutex);
		svDelay = delay;
		svHue   = hue;
		mutex_exit(&mutex);
		printf("NeoPixel LED strip hue set to %lu with %lu ms delay \n", hue, delay);
	}
}

// Función para leer un valor de tipo uint32_t de stdin y mostrarlo en stdout
static bool readInt(uint32_t* i){
	char ix = 0;
	char buffer[6];
	while(ix < 5){
		buffer[ix] = getchar();
		buffer[ix+1] = 0;
		if(buffer[ix] == '\b' || buffer[ix] == '\x7f'){
			if(ix > 0){
				ix--;
				printf("\b \b");
			}
			continue;
		}
		putchar(buffer[ix]);
		if((buffer[ix] == '\n') || (buffer[ix] == '\r')){
			if(buffer[ix] == '\r') putchar('\n');
			if(ix < 1) return false;
			buffer[ix] = 0;
			*i = atou(buffer, NULL);
			return true;
		}
		else if( (buffer[ix] < '0') || (buffer[ix] > '9') ){
			return false;
		}
		++ix;
	}
	return false;
}

// Función para analizar una cadena de datos interpretando su contenido como un número entero
static uint32_t atou(char* buffer, char** endptr){
	uint32_t num = 0;
	uint32_t pow = 1;
	char *end = buffer;
	while( (*end >= '0') && (*end <= '9') )
		++end;
	if(endptr != NULL) *endptr = end-1;

	for(--end; end >= buffer; --end){
		num+= (*end - '0') * pow;
		pow*= 10;
	}
	return num;
}