/*----------------------------------------------------------------
 * Experimento 1: Control del LED NeoPixel por ADC y USB
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
#include <hardware/adc.h>

#include"neopixel.h"

// Asignación de pines de la DualMCU
#define NEOPIX_PWR 17  // Power del LED Neopixel (ON/OFF)
#define NEOPIX_DIN 16  // Datos del LED NEopixel
#define ADC0_PIN   26  // Pin del potenciómetro

// Variables de saturación y brillo compartidas por las tareas
volatile uint16_t svHue = 0;
volatile float    svBrightness = 0.0f;

// Mutex utilizado para sincronizar el acceso a las variables compartidas
static mutex_t mutex;

// Configuración de hardware
static void setup();

// Tarea 1: Supervisión del LED Neopixel
static void neopixelTask( void *param );

// Tarea 2: Lectura del puerto USB
static void usbTask( void *param );

// Tarea 3: Lectura del potenciómetro por ADC
static void adcTask( void *param );

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

	xTaskCreate( adcTask,                   // Función que implementa la tarea.
				 "adc",                     // Nombre asignado a la tarea.
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

	// Inicialización de los pines GPIO para el LED Neopixel
	uint8_t pins[] = {PICO_DEFAULT_LED_PIN, NEOPIX_PWR, NEOPIX_DIN};
	for (uint8_t i = 0; i < 3; ++i){
		gpio_init(pins[i]);
		gpio_set_dir(pins[i], GPIO_OUT);
		gpio_put(pins[i], 0);
	}

	// Inicialización de ADC
	adc_init();
	adc_gpio_init(ADC0_PIN); 
	adc_select_input(0);
}

// Tarea 1: Supervisión del LED Neopixel
static void neopixelTask( void *param ){
	uint16_t hue;
	float brightness;
	uint32_t delay = 10;

	gpio_put(NEOPIX_PWR, 1);
	vTaskDelay(pdMS_TO_TICKS(500));
	printf("Starting...\n");

	NeoPixel np = NeoPixel(NEOPIX_DIN, 1);

	while(true){
		mutex_enter_blocking(&mutex);
		hue = svHue;
		brightness = svBrightness;
		mutex_exit(&mutex);

		np.setHSV(0, hue, 1, brightness);
		np.write();

		vTaskDelay(pdMS_TO_TICKS(delay));
	}
}

// Tarea 2: Lectura del puerto USB
static void usbTask( void *param ){
	uint32_t hue;
	vTaskDelay(pdMS_TO_TICKS(550));
	while(true){
		printf("NeoPixel LED hue [0-360]: ");
		if(!readInt(&hue)){
			printf("\nInvalid input\n");
			continue;
		}
		mutex_enter_blocking(&mutex);
		svHue = hue;
		mutex_exit(&mutex);
		printf("NeoPixel LED hue set to %d\n", hue);
	}
}

// Tarea 3: Lectura del potenciómetro por ADC
static void adcTask( void *param ){
	uint16_t raw;
	float brightness;
	uint32_t delay = 50;

	while(true){
        raw = adc_read();
        if (raw < 0){
            brightness = 0.0f;
        } else if (raw > 4095){
        	brightness = 1.0f;
        } else{
            brightness = raw / 4095.0f;
        }

        mutex_enter_blocking(&mutex);
        svBrightness = brightness;
        mutex_exit(&mutex);

        vTaskDelay(pdMS_TO_TICKS(delay));
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