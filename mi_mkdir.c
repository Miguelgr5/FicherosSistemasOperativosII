#include "directorios.h"

int main(int argc, char **argv) {
  // 1. Comprobar sintaxis (argc != 4)
  if (argc != 4) {
    fprintf(stderr, RED "Sintaxis: ./mi_mkdir <disco> <permisos> </ruta>\n");
    return -1;
  }

  char *nombre_disco = argv[1];
  unsigned char permisos = atoi(argv[2]);
  char *ruta = argv[3];

  // 2. Validar permisos (0-7)
  if (permisos < 0 || permisos > 7) {
    fprintf(stderr, RED "Error: Permisos no válidos (0-7)\n");
    return -1;
  }

  // 4. Montar el dispositivo (bmount)
  if (bmount(nombre_disco) == -1)
    return -1;

  // 5. Llamar a mi_creat (esta función se encarga de buscar el inodo padre y
  // crear la entrada)
  int error;
  if ((error = mi_creat(ruta, permisos)) < 0) {
    // Gestionar errores según el código devuelto (ej. NO_PERMISSION,
    // ALREADY_EXISTS)
    mostrar_error_buscar_entrada(error);
    bumount();
    return -1;
  }

  // 6. Desmontar (bumount)
  bumount();
  return 0;
}
