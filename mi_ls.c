#include "directorios.h"

/**
 * mi_ls.c
 * Programa de usuario para listar el contenido de un directorio
 * o mostrar la información de un fichero.
 */

int main(int argc, char **argv) {
  if (argc < 3 || argc > 4) {
    fprintf(stderr, "Sintaxis: ./mi_ls [-l] <disco> </ruta>\n");
    return -1;
  }

  int extendido = 0;
  char *nombre_disco;
  char *ruta;

  // 1. Gestión de argumentos y flags
  if (argc == 4) {
    if (strcmp(argv[1], "-l") != 0) {
      fprintf(stderr, "Error: opción %s no válida. Use -l\n", argv[1]);
      return -1;
    }
    extendido = 1;
    nombre_disco = argv[2];
    ruta = argv[3];
  } else {
    nombre_disco = argv[1];
    ruta = argv[2];
  }

  // 2. Determinar tipo esperado según la sintaxis del camino (barra final)
  char tipo_esperado = (ruta[strlen(ruta) - 1] == '/') ? 'd' : 'f';

  // Flag para mi_dir: 'l' (long/extendido) o 's' (simple)
  char flag = (extendido) ? 'l' : 's';

  // 3. Montar el disco virtual
  if (bmount(nombre_disco) == -1) {
    return -1;
  }

  // 4. Preparar buffer y llamar a mi_dir
  // Definimos un buffer suficientemente grande (ej. 100 entradas x 256 bytes)
  char buffer[100 * 256];
  memset(buffer, 0, sizeof(buffer));

  int total_entradas = mi_dir(ruta, buffer, tipo_esperado, flag);

  // 5. Gestión de resultados e impresión
  if (total_entradas < 0) {
    // En caso de error (ruta no encontrada, permisos, etc.), mi_dir
    // o buscar_entrada ya habrán impreso el mensaje de error.
    bumount();
    return -1;
  }

  // Si todo fue bien, imprimimos el contenido del buffer
  if (total_entradas >= 0) {
    if (extendido) {
      printf("Tipo\tPermisos\tmTime\t\t\tTamaño\tNombre\n");
      printf("-----------------------------------------------------------------"
             "---------------------------\n");
    }

    // Imprimimos el buffer que ya viene formateado según el flag desde mi_dir
    if (strlen(buffer) > 0) {
      // En formato simple, nos aseguramos de que termine con un salto de línea
      if (!extendido) {
        printf("%s\n", buffer);
      } else {
        printf("%s", buffer);
      }
    }

    // El contador "Total" solo se muestra cuando listamos directorios
    if (tipo_esperado == 'd') {
      printf("Total: %d\n", total_entradas);
    }
  }

  // 6. Desmontar disco
  if (bumount() == -1) {
    return -1;
  }

  return 0;
}
