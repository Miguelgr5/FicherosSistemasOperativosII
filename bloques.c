#include "bloques.h"
#include <fcntl.h>
#include <stdio.h>
// #define DEBUG
static int descriptor;
static sem_t *mutex;
static unsigned int inside_sc = 0; // Control de reentrada
// Función para solicitar el semáforo
void mi_waitSem() {
  if (!inside_sc) { // Si inside_sc == 0, no se ha hecho un wait todavía
    waitSem(mutex);
  }
  inside_sc++; // Incrementamos para saber que ya estamos dentro de una sección
               // crítica
}
// Función para liberar el semáforo
void mi_signalSem() {
  inside_sc--;      // Decrementamos al salir de una función
  if (!inside_sc) { // Solo si es el último nivel de salida, liberamos el
                    // semáforo real
    signalSem(mutex);
  }
}
int bmount(const char *camino) {
  umask(0000);
  descriptor = open(camino, O_RDWR | O_CREAT, 0666);
  if (descriptor == -1) {
    fprintf(stderr, RED "Error al abrir archivo:");
    perror("");
    fprintf(stderr, RESET);
    return FALLO;
  }
  if (!mutex) {        // El semáforo es único y solo se inicializa una vez
    mutex = initSem(); // Llama a sem_open() internamente
    if (mutex == SEM_FAILED) {
      return FALLO; // Error si no se puede crear el semáforo
    }
  }
  return descriptor;
}
int bumount() {
  deleteSem(mutex);
  mutex = NULL; // Evitamos punteros colgantes
  if (close(descriptor) < 0) {
    fprintf(stderr, RED "Error al cerrar archivo: ");
    perror("");
    fprintf(stderr, RESET);
  }
#if defined(DEBUG)
  fprintf(stderr, "Cerrando archivo...\n");
#endif
  return descriptor;
}
int bwrite(unsigned int nbloque, const void *buf) {
  if (lseek(descriptor, nbloque * BLOCKSIZE, SEEK_SET) < 0) {
    fprintf(stderr, RED "Error en lseek (bloque %u): ", nbloque);
    perror("");
    fprintf(stderr, RESET);
  }
  if (write(descriptor, buf, BLOCKSIZE) == -1) {
    fprintf(stderr, RED "Error al escribir en el bloque %u: ", nbloque);
    perror("");
    fprintf(stderr, RESET);
    return FALLO;
  }
  return BLOCKSIZE;
}
int bread(unsigned int nbloque, void *buf) {
  if (lseek(descriptor, nbloque * BLOCKSIZE, SEEK_SET) < 0) {
    fprintf(stderr, RED "Error en lseek:");
    perror("");
    fprintf(stderr, RESET);
  }
  if (read(descriptor, buf, BLOCKSIZE) == -1) {
    fprintf(stderr, RED "Error al leer archivo:");
    perror("");
    fprintf(stderr, RESET);
    return FALLO;
  }
  return BLOCKSIZE;
}
