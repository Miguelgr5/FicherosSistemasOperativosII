#include "directorios.h"

int main(int argc, char **argv) {
  // 1. Validar sintaxis
  if (argc != 3) {
    fprintf(stderr, "Sintaxis: ./mi_stat <disco> </ruta>\n");
    return -1;
  }

  char *nombre_disco = argv[1];
  char *ruta = argv[2];

  // 2. Montar el disco
  if (bmount(nombre_disco) == -1) {
    return -1;
  }

  // 3. Llamar a mi_stat para obtener los metadatos
  struct STAT stat;
  int p_inodo = mi_stat(ruta, &stat);
  if (p_inodo < 0) {
    // mostrar_error_buscar_entrada(p_inodo); // Si tienes esta función de ayuda
    fprintf(stderr, "Error al obtener información de la ruta %s\n", ruta);
    bumount();
    return -1;
  }

  // 4. Mostrar la información por pantalla
  printf("Nº de inodo: %d\n", p_inodo);
  printf("tipo: %c\n", stat.tipo);
  printf("permisos: %d\n", stat.permisos);

  char tmp[100];
  struct tm *tm;

  // atime: Último acceso a los datos
  tm = localtime(&stat.atime);
  strftime(tmp, sizeof(tmp), "%a %Y-%m-%d %H:%M:%S", tm);
  printf("atime: %s\n", tmp);

  // mtime: Última modificación de los datos
  tm = localtime(&stat.mtime);
  strftime(tmp, sizeof(tmp), "%a %Y-%m-%d %H:%M:%S", tm);
  printf("mtime: %s\n", tmp);

  // ctime: Último cambio en los metadatos (inodo)
  tm = localtime(&stat.ctime);
  strftime(tmp, sizeof(tmp), "%a %Y-%m-%d %H:%M:%S", tm);
  printf("ctime: %s\n", tmp);

  // btime: Fecha de creación (Birth time)
  tm = localtime(&stat.btime);
  strftime(tmp, sizeof(tmp), "%a %Y-%m-%d %H:%M:%S", tm);
  printf("btime: %s\n", tmp);

  printf("nlinks: %d\n", stat.nlinks);
  printf("tamEnBytesLog: %d\n", stat.tamEnBytesLog);
  printf("numBloquesOcupados: %d\n", stat.numBloquesOcupados);

  // 5. Desmontar
  bumount();
  return 0;
}
