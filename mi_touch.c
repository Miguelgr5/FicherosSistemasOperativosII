#include "directorios.h"

int main(int argc, char **argv) {
  // 1. Validar sintaxis
  if (argc != 4) {
    fprintf(stderr, RED "Sintaxis: ./mi_touch <disco> <permisos> </ruta>\n");
    return -1;
  }

  char *nombre_disco = argv[1];
  unsigned char permisos = (unsigned char)atoi(argv[2]);
  char *ruta = argv[3];

  // 2. Validar que no sea un directorio (no debe terminar en '/')
  if (ruta[strlen(ruta) - 1] == '/') {
    fprintf(
        stderr, RED
        "Error: mi_touch solo crea ficheros (la ruta no debe acabar en /)\n");
    return -1;
  }

  // 3. Validar permisos (0-7)
  if (permisos < 0 || permisos > 7) {
    fprintf(stderr, RED "Error: permisos no válidos (0-7)\n");
    return -1;
  }

  // 4. Montar el disco
  if (bmount(nombre_disco) == -1)
    return -1;

  // 5. Llamar a la función de la capa de directorios
  int error;
  if ((error = mi_touch(ruta, permisos)) < 0) {
    mostrar_error_buscar_entrada(error);
    bumount();
    return -1;
  }

  // 6. Desmontar
  bumount();
  return 0;
}
