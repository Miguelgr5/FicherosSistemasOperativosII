#include "ficheros.h"

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(
        stderr, RED
        "Sintaxis: permitir <nombre_dispositivo> <ninodo> <permisos>\n" RESET);
    return EXIT_FAILURE;
  }

  if (bmount(argv[1]) == FALLO) {
    fprintf(stderr, RED "Error al montar el dispositivo\n" RESET);
    return EXIT_FAILURE;
  }

  int ninodo = atoi(argv[2]);
  unsigned char permisos = atoi(argv[3]);

  if (mi_chmod_f(ninodo, permisos) == FALLO) {
    fprintf(stderr, RED "Error al cambiar los permisos del inodo %d\n" RESET,
            ninodo);
    bumount();
    return EXIT_FAILURE;
  }

  if (bumount() == FALLO) {
    fprintf(stderr, RED "Error al desmontar el dispositivo\n" RESET);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
