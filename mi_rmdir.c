#include "directorios.h"

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr,
            RED "Sintaxis: ./mi_rmdir <disco> </ruta_directorio/>\n" RESET);
    return -1;
  }

  char *ruta = argv[2];

  // Verificación de sintaxis: los directorios deben terminar en '/'
  if (ruta[strlen(ruta) - 1] != '/') {
    fprintf(stderr, RED "Error: La ruta debe corresponder a un directorio "
                        "(terminar en /).\n" RESET);
    return -1;
  }

  if (bmount(argv[1]) == -1)
    return -1;

  // mi_unlink ya comprueba internamente si el directorio está vacío
  if (mi_unlink(ruta) < 0) {
    bumount();
    return -1;
  }

  bumount();
  return 0;
}
