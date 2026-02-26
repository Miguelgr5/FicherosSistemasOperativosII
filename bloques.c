#include "bloques.h"
#include <fcntl.h>
#include <stdio.h>
static int descriptor;
int bmount(const char *camino) {
  umask(0000);
  descriptor = open(camino, O_RDWR | O_CREAT, 0666);
  if (descriptor == -1) {
    fprintf(stderr, RED);
    perror("Error al abrir archivo");
    fprintf(stderr, RESET);
    return FALLO;
  }
  return descriptor;
}
int bumount() {
  if (close(descriptor) < 0) {
    fprintf(stderr, RED);
    perror("Error al cerrar archivo\n");
    fprintf(stderr, RESET);
  }
  printf("Cerrando archivo...\n");
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
    fprintf(stderr, RED);
    perror("Error en lseek\n");
    fprintf(stderr, RESET);
  }
  if (read(descriptor, buf, BLOCKSIZE) == -1) {
    fprintf(stderr, RED);
    perror("Error al leer archivo\n");
    fprintf(stderr, RESET);
    return FALLO;
  }
  return BLOCKSIZE;
}
