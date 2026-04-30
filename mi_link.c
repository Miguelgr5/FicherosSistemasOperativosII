#include "directorios.h"
int main(int argc, char **argv) {
  // 1. Validación de sintaxis de argumentos
  if (argc != 4) {
    fprintf(stderr, RED "Sintaxis: ./mi_link <disco> </ruta_fichero_original> "
                        "</ruta_enlace>\n" RESET);
    return -1;
  }

  char *nombre_disco = argv[1];
  char *ruta_original = argv[2];
  char *ruta_enlace = argv[3];

  // 2. Comprobar que ambas rutas correspondan a ficheros (no deben terminar en
  // '/')
  if (ruta_original[strlen(ruta_original) - 1] == '/' ||
      ruta_enlace[strlen(ruta_enlace) - 1] == '/') {
    fprintf(stderr, RED "Error: No se permiten enlaces a directorios.\n" RESET);
    return -1;
  }

  // 3. Montar el disco
  if (bmount(nombre_disco) == -1) {
    return -1;
  }

  // 4. Llamada a la función de la capa de directorios
  // mi_link() se encargará de verificar que ruta_original exista
  // y que ruta_enlace no exista.
  int error = mi_link(ruta_original, ruta_enlace);
  if (error < 0) {
    mostrar_error_buscar_entrada(error);
    bumount();
    return -1;
  }

  // 5. Desmontar disco
  if (bumount() == -1) {
    return -1;
  }

  return 0;
}
