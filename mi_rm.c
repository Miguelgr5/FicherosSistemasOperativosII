#include "directorios.h"

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, RED "Sintaxis: ./mi_rm <disco> </ruta_fichero>\n" RESET);
    return -1;
  }

  char *ruta = argv[2];

  // Verificación de sintaxis: los ficheros NO deben terminar en '/'
  if (ruta[strlen(ruta) - 1] == '/') {
    fprintf(stderr, RED "Error: mi_rm solo puede borrar ficheros. Use mi_rmdir "
                        "para directorios.\n" RESET);
    return -1;
  }

  if (bmount(argv[1]) == -1)
    return -1;

  int error = mi_unlink(ruta);
  if (error < 0) {
    mostrar_error_buscar_entrada(error);

    bumount();
    return -1;
  }

  bumount();
  return 0;
}
