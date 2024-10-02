#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h> // Incluir para medir el tiempo

#define MAX 100 // Máxima longitud de una línea
#define THREAD_COUNT 8
 // Número de hilos

// Semáforos para controlar el acceso
sem_t sems[THREAD_COUNT];

// Prototipo de la función Tokenize
void* Tokenize(void* rank);

// Implementación de strtok_mio
char* strtok_mio(char* string, const char* separators, char** saveptr) {
    if (string == NULL) {
        string = *saveptr; // Reanudar desde donde se quedó
    }

    // Si el string está vacío, devolver NULL
    if (string == NULL) {
        return NULL;
    }

    // Saltar los separadores iniciales
    string += strspn(string, separators);
    
    // Si hemos alcanzado el final de la cadena, devolver NULL
    if (*string == '\0') {
        *saveptr = NULL;
        return NULL;
    }

    // Encontrar el final del token
    char* token_end = string + strcspn(string, separators);
    
    // Si se encontró un separador, colocar un carácter nulo
    if (*token_end != '\0') {
        *token_end = '\0';
        *saveptr = token_end + 1; // Actualizar el puntero de estado
    } else {
        *saveptr = NULL; // No hay más tokens
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
    char* saveptr; // Para strtok_mio
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

        // Medir el tiempo de ejecución de strtok_mio
        clock_t start = clock(); // Iniciar el temporizador
        my_string = strtok_mio(my_line, " \t\n", &saveptr);

        while (my_string != NULL) {
            count++;
            printf("Thread %ld > string %d = %s\n", my_rank, count, my_string);
            my_string = strtok_mio(NULL, " \t\n", &saveptr); // Obtener el siguiente token
        }
        clock_t end = clock(); // Finalizar el temporizador

        // Calcular y mostrar el tiempo de ejecución
        double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("Thread %ld > Time taken for strtok_mio: %f seconds\n", my_rank, cpu_time_used);

        // Notificar al siguiente hilo
        sem_post(&sems[(my_rank + 1) % THREAD_COUNT]);
    }

    return NULL; // Finalizar la función del hilo
}
