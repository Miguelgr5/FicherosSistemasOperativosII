#include "directorios.h"

#define TAMBUFFER 1500

int main(int argc, char **argv) {
  // 1. Validación de argumentos
  if (argc != 3) {
    fprintf(stderr, "Sintaxis: ./mi_cat <nombre_dispositivo> <ruta>\n");
    return FALLO;
  }

  char *nombre_dispositivo = argv[1];
  char *ruta = argv[2];

  // 2. Montar el dispositivo
  if (bmount(nombre_dispositivo) == FALLO) {
    return FALLO;
  }

  // 3. Obtener metadatos para conocer el tamaño lógico del fichero
  struct STAT stat;
  if (mi_stat(ruta, &stat) == FALLO) {
    fprintf(stderr, "Error: No se puede obtener información de %s\n", ruta);
    bumount();
    return FALLO;
  }

  // Comprobación: ¿Es un fichero? (Asumiendo que mi_stat o buscar_entrada lo
  // valida) Nota: El tipo de entrada (fichero o directorio) suele validarse en
  // mi_stat o mi_read
  if (stat.tipo == 'd') {
    fprintf(stderr, "Error: %s es un directorio, no se puede hacer cat.\n",
            ruta);
    bumount();
    return FALLO;
  }

  // 4. Lectura del contenido
  char buffer_texto[TAMBUFFER];
  int offset = 0;
  int leidos = 0;
  int total_leidos = 0;

  memset(buffer_texto, 0, TAMBUFFER);

  // Leer hasta el final del fichero (usando el tamaño lógico obtenido en stat)
  leidos = mi_read(ruta, buffer_texto, offset, TAMBUFFER);
  while (leidos > 0) {
    // Escribir en la salida estándar (1)
    if (write(1, buffer_texto, leidos) < 0) {
      perror("Error en write");
      break;
    }

    total_leidos += leidos;
    offset += leidos;

    memset(buffer_texto, 0, TAMBUFFER);
    leidos = mi_read(ruta, buffer_texto, offset, TAMBUFFER);
  }

  // 5. Informe final
  char info[128];
  sprintf(info, "\nTotal bytes leídos: %d\n", total_leidos);
  write(2, info, strlen(info));
  sprintf(info, "Tamaño lógico del fichero: %u\n", stat.tamEnBytesLog);
  write(2, info, strlen(info));

  // 6. Desmontar
  if (bumount() == FALLO) {
    return FALLO;
  }

  return EXITO;
}
