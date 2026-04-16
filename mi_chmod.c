#include "directorios.h"

int main(int argc, char **argv) {
  // 1. Validar sintaxis
  if (argc != 4) {
    fprintf(stderr, RED "Sintaxis: ./mi_chmod <disco> <permisos> </ruta>\n");
    return -1;
  }

  // 2. Obtener y validar permisos (0-7)
  // El atoi lo convierte a entero, pero recuerda que el usuario lo pasa en
  // octal
  unsigned char permisos = (unsigned char)atoi(argv[2]);
  if (permisos < 0 || permisos > 7) {
    fprintf(stderr, "Error: permisos no válidos (0-7)\n");
    return -1;
  }

  char *nombre_disco = argv[1];
  char *ruta = argv[3];

  // 3. Montar el dispositivo
  if (bmount(nombre_disco) == -1) {
    fprintf(stderr, "Error al montar el disco\n");
    return -1;
  }

  // 4. Llamar a la función de la capa de directorios
  int resultado = mi_chmod(ruta, permisos);
  if (resultado < 0) {
    // Aquí podrías usar tu función de mostrar errores según el código devuelto
    fprintf(stderr, "Error al cambiar permisos de %s\n", ruta);
    bumount();
    return -1;
  }

  // 5. Desmontar
  bumount();
  return 0;
}
