#include "directorios.h"

int main(int argc, char **argv) {
  // 1. Validación de argumentos: ./mi_escribir <disco> <ruta> <texto> <offset>
  if (argc != 5) {
    fprintf(stderr, "Sintaxis: ./mi_escribir <nombre_dispositivo> <ruta> "
                    "<texto> <offset>\n");
    return FALLO;
  }

  // Recogida de parámetros
  char *nombre_dispositivo = argv[1];
  char *ruta = argv[2];
  char *texto = argv[3];
  unsigned int offset = atoi(argv[4]);
  int nbytes = strlen(texto);

  // 2. Montar el dispositivo
  if (bmount(nombre_dispositivo) == FALLO) {
    return FALLO;
  }

  printf("Longitud texto: %d\n", nbytes);

  // 3. Escribir el texto en el sistema de ficheros
  // mi_write() internamente llamará a buscar_entrada() y a mi_write_f()
  int escritos = mi_write(ruta, texto, offset, nbytes);

  if (escritos < 0) {
    // Manejo de errores (por ejemplo, si la ruta es un directorio o no hay
    // permisos)
    fprintf(stderr, "Error al escribir en la ruta: %s\n", ruta);
    printf("Bytes escritos: %d\n", escritos);
    // Si mi_write devuelve errores específicos (como -1), podrías gestionarlos
    // aquí
    bumount();
    return FALLO;
  }

  // 4. Mostrar la cantidad de bytes escritos (requisito del enunciado)
  printf("Bytes escritos: %d\n", escritos);

  // 5. Desmontar dispositivo
  if (bumount() == FALLO) {
    return FALLO;
  }

  return EXITO;
}
