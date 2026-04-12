#include "bloques.h"
#include <fcntl.h>
#include <stdio.h>
// #define DEBUG
static int descriptor;
int bmount(const char *camino) {
  umask(0000);
  descriptor = open(camino, O_RDWR | O_CREAT, 0666);
  if (descriptor == -1) {
    fprintf(stderr, RED "Error al abrir archivo:");
    perror("");
    fprintf(stderr, RESET);
    return FALLO;
  }
  return descriptor;
}
int bumount() {
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
