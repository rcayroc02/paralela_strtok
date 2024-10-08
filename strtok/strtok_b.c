#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h> // Para medir el tiempo

#define MAX 100 // Máxima longitud de una línea
#define THREAD_COUNT 8 // Número de hilos

// Semáforos para controlar el acceso
sem_t sems[THREAD_COUNT];
double total_time[THREAD_COUNT] = {0}; // Array para almacenar el tiempo de cada hilo

// Prototipo de la función Tokenize
void* Tokenize(void* rank);

// Implementación de strtok_mio
char* strtok_mio(char* string, const char* delimiters) {
    static char* saved_string; // Mantener un puntero para recordar la posición entre llamadas

    // Si el string es NULL, reanudar desde donde se quedó la última vez
    if (string == NULL) {
        string = saved_string;
    }

    // Si el string está vacío, devolver NULL
    if (string == NULL) {
        return NULL;
    }

    // Saltar los delimitadores iniciales
    string += strspn(string, delimiters);

    // Si hemos llegado al final de la cadena, devolver NULL
    if (*string == '\0') {
        saved_string = NULL;
        return NULL;
    }

    // Encontrar el final del token (donde aparece el primer delimitador)
    char* token_end = string + strcspn(string, delimiters);

    // Si se encontró un delimitador, colocar un carácter nulo para finalizar el token
    if (*token_end != '\0') {
        *token_end = '\0';
        saved_string = token_end + 1; // Guardar la posición para la siguiente llamada
    } else {
        saved_string = NULL; // No hay más tokens
    }

    return string; // Devolver el token encontrado
}

int main() {
    pthread_t threads[THREAD_COUNT];
    long thread_rank;

    // Inicializar semáforos
    for (int i = 0; i < THREAD_COUNT; i++) {
        sem_init(&sems[i], 0, (i == 0) ? 1 : 0); // Solo el primer hilo se activa inicialmente
    }

    // Crear hilos
    for (thread_rank = 0; thread_rank < THREAD_COUNT; thread_rank++) {
        pthread_create(&threads[thread_rank], NULL, Tokenize, (void*)thread_rank);
    }

    // Esperar a que todos los hilos terminen
    for (thread_rank = 0; thread_rank < THREAD_COUNT; thread_rank++) {
        pthread_join(threads[thread_rank], NULL);
    }

    // Calcular y mostrar el tiempo total
    double total_execution_time = 0.0;
    for (int i = 0; i < THREAD_COUNT; i++) {
        total_execution_time += total_time[i];
    }
    printf("Total execution time: %.5f seconds\n", total_execution_time);

    // Destruir semáforos
    for (int i = 0; i < THREAD_COUNT; i++) {
        sem_destroy(&sems[i]);
    }

    return 0;
}

void* Tokenize(void* rank) {
    long my_rank = (long) rank;
    char my_line[MAX];
    char* my_string;
    int count;

    printf("Thread %ld started.\n", my_rank); // Impresión de depuración

    while (1) {
        // Esperar a que sea el turno de este hilo
        sem_wait(&sems[my_rank]);

        // Leer una línea de la entrada estándar
        if (fgets(my_line, MAX, stdin) == NULL) {
            sem_post(&sems[(my_rank + 1) % THREAD_COUNT]); // Notificar al siguiente hilo
            break; // Salir si no hay más líneas
        }

        printf("Thread %ld > my line = %s", my_rank, my_line);
        count = 0;

        // Medir el tiempo antes de llamar a strtok_mio
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start); // Obtiene el tiempo actual

        // Tokenizar la línea usando strtok_mio
        my_string = strtok_mio(my_line, " \t\n");

        while (my_string != NULL) {
            count++;
            printf("Thread %ld > string %d = %s\n", my_rank, count, my_string);
            my_string = strtok_mio(NULL, " \t\n"); // Obtener el siguiente token
        }

        // Medir el tiempo después de llamar a strtok_mio
        clock_gettime(CLOCK_MONOTONIC, &end); // Obtiene el tiempo actual

        // Calcular la duración en segundos
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;

        // Almacenar el tiempo de ejecución del hilo
        total_time[my_rank] = elapsed;

        // Imprimir el tiempo de ejecución
        printf("Thread %ld > strtok_mio took %.5f seconds\n", my_rank, elapsed);

        // Notificar al siguiente hilo
        sem_post(&sems[(my_rank + 1) % THREAD_COUNT]);
    }

    return NULL; // Finalizar la función del hilo
}
