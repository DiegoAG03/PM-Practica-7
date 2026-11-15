/*----------------------------------------------------------------
 * Experimento 2: Cálculo multinúcleo de pi
 * Autor: Diego Ambrosio González
 *----------------------------------------------------------------
*/

// Librerias del Kernel
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

// Librerias de hardware
#include <stdio.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <pico/sem.h>

#define PARTASKS 4

struct{
	size_t   job;
	size_t   start;
	size_t   end;
	uint32_t n;
	double   result;
}typedef work_t;

// Sincronización
SemaphoreHandle_t semb; // Core 0
semaphore_t semc; // Core 1

// Tarea principal: inicia todos los cálculos y mide el tiempo.
static void mainTask( void *param );

// Tarea secuencial para aproximar el valor de pi utilizando N tareas (función envolvente).
static void seqPiTask(void *param);

// Tarea paralela para aproximar el valor de pi utilizando N tareas (función envolvente).
static void parPiTask(void *param);

// Función para aproximar el valor de pi utilizando n términos de forma secuencial.
static double sequential_pi (uint32_t n);

// Funciones para aproximar el valor de pi utilizando n términos de forma paralela.
static void parallel_pi (void* job);
static void core1_worker (void);

// Lee un valor de tipo uint32_t de stdin y lo muestra en stdout.
// Cuando esta función regresa, contiene el valor leído.
// True si se leyó un entero sin signo; False en caso contrario.
static bool readInt(uint32_t* i);

// Analiza la cadena interpretando su contenido como un número entero.
// buffer: Cadena que comienza con la representación de un número entero.
// endptr: Referencia a un objeto de tipo char*, cuyo valor es establecido por la función al siguiente carácter en str después del valor numérico.
// Si la operación tiene éxito, la función devuelve el número entero convertido como un valor de tipo uint32_t.
static uint32_t atou(char* buffer, char** endptr);

void main( void ){
	stdio_init_all();

	xTaskCreate( mainTask,                  // Función que implementa la tarea.
			     "main",                    // Nombre asignado a la tarea.
			     1024,                      // Tamaño asignado en el stack para la tarea.
			     NULL,                      // Parámetro asignado a la tarea.
			     2,                         // Prioridad asignada a la tarea.
			     NULL );                    // Identificador de la tarea.

	multicore_launch_core1(core1_worker);
	vTaskStartScheduler();
	for( ;; );
}

// Tarea principal: inicia todos los cálculos y mide el tiempo.
static void mainTask( void *param ){
	uint32_t n;
	TaskHandle_t seqPiTaskHandle;
	TaskHandle_t parPiTaskHandle;

	semb = xSemaphoreCreateBinary();
	while(true){
		printf("\fInput integer between 1000 and 50000: ");
		if( !readInt(&n) ) continue;
		if(n < 1000) n = 1000;
		if(n > 65535) n = 65535;

		// Secuencial
		printf("Calculating pi using 1 task and %lu terms...\n", n);
		xTaskCreate(seqPiTask, "seq", 1024, (void*)n, 1, &seqPiTaskHandle);
		xSemaphoreTake(semb, portMAX_DELAY);

		// Limpieza
		vTaskDelete(seqPiTaskHandle);
		printf("\n");

		// Concurrente
		printf("Calculating pi using %d tasks and %lu terms...\n", PARTASKS, n);
		xTaskCreate(parPiTask, "par", 1024, (void*)n, 1, &parPiTaskHandle);
		xSemaphoreTake(semb, portMAX_DELAY);

		// Limpieza
		vTaskDelete(parPiTaskHandle);
	}
}

// Tarea paralela para aproximar el valor de pi utilizando N tareas (función envolvente).
static void parPiTask(void *param){
	double pi = 0.0;
	uint32_t n = (uint32_t)param;
	TickType_t start, end, diff;
	work_t jobs[PARTASKS];
	TaskHandle_t handle[PARTASKS];

	start = xTaskGetTickCount();

	// Inicializar el semaforo nativo de Pico-SDK en 0
	sem_init(&semc, 0, PARTASKS);

	// Repartir tareas entre Core 0 y Core 1
	for(uint32_t i = 0; i < PARTASKS; ++i){
		jobs[i].job   = i + 1;
		jobs[i].start = i * n / PARTASKS;
		jobs[i].end   = (i+1) * n / PARTASKS;
		jobs[i].n     = n;
		
		if(i % 2 == 0){
			// Tareas 1 y 3 (i=0, i=2): Se ejecutan como tareas en el Core 0
			xTaskCreate(parallel_pi, "job", 1024, &jobs[i], 1, &handle[i]);
		} else {
			// Tareas 2 y 4 (i=1, i=3): Se envía su dirección por el buzón al Core 1
			multicore_fifo_push_blocking((uintptr_t)&jobs[i]);
		}

	}
	// Esperar a que las 4 tareas (2 del Core 0 y 2 del Core 1) liberen semc
	for(uint32_t i = 0; i < PARTASKS; ++i){
		while(!sem_try_acquire(&semc)){
        	vTaskDelay(1);
    	}
    }

	// Limpiar solo las tareas creadas en el Core 0 y sumar los 4 resultados
	for(uint32_t i = 0; i < PARTASKS; ++i){
		if(i % 2 == 0){
			vTaskDelete(handle[i]);
		}
		pi += jobs[i].result;
	}
	end = xTaskGetTickCount();

	diff = end - start;
	printf("Pi:      %0.6f\n", pi);
	printf("Tasks:   %d\n", PARTASKS);
	printf("Elapsed: %lu ticks (%0.0fms)\n", diff, diff * (1000.0/configTICK_RATE_HZ));

	xSemaphoreGive(semb);
	vTaskSuspend(NULL);
}

// Tarea secuencial para aproximar el valor de pi utilizando N tareas (función envolvente).
static void seqPiTask(void *param){
	double pi = 0.0;
	TickType_t start, end, diff;

	start = xTaskGetTickCount();
	pi = sequential_pi( (uint32_t)param );
	end = xTaskGetTickCount();
	diff = end - start;
	printf("Pi:      %0.6f\n", pi);
	printf("Tasks:   1\n");
	printf("Elapsed: %lu ticks (%0.0fms)\n", diff, diff * (1000.0/configTICK_RATE_HZ));

	xSemaphoreGive(semb);
	vTaskSuspend(NULL);
}

// Función para aproximar el valor de pi utilizando n términos de forma paralela.
static void parallel_pi(void *param){
	work_t* job = (work_t*)param;

	printf("Running job %lu on core %lu\n", job->job, get_core_num());

	double x, dx = 1.0/job->n, sum = 0;

	for (size_t i = job->start; i < job->end; ++i){
		x = dx * i;
		sum+= 4.0/(1.0 + x*x);
	}
	job->result = dx * sum;
	sem_release(&semc);
	
	// Solo las tareas del Core 0 (FreeRTOS) se quedan esperando a vTaskDelete.
	// El Core 1 termina la función normalmente para regresar al while(true) de core1_worker.
	if (get_core_num() == 0) {
		vTaskSuspend(NULL);
	}
}

// Función principal permanente del Core 1 (Bare-metal).
static void core1_worker(void){
	while(true){
		work_t* job = (work_t*) multicore_fifo_pop_blocking();
		parallel_pi(job);
	}
}

// Función para aproximar el valor de pi utilizando n términos de forma secuencial.
static double sequential_pi (uint32_t n) {
	double x, sum = 0.0, dx = 1.0/n;

	for (size_t i = 0; i < n; ++i){
		x = dx * i;
		sum+= 4.0/(1.0 + x*x);
	}
	return dx * sum;
}

// Función para leer un valor de tipo uint32_t de stdin y mostrarlo en stdout
static bool readInt(uint32_t* i){
	char ix = 0;
	char buffer[7];
	while(ix < 6){
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