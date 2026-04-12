#include "ficheros.h"

int main(int argc, char *argv[]) {
  if (argc != 4) {
    fprintf(
        stderr,
        "Error de sintaxis: truncar <nombre_dispositivo> <ninodo> <nbytes>\n");
    return FALLO;
  }

  char *nombre_dispositivo = argv[1];
  unsigned int ninodo = atoi(argv[2]);
  unsigned int nbytes = atoi(argv[3]);

  if (bmount(nombre_dispositivo) == FALLO) {
    return FALLO;
  }

  // 1. Lógica de truncado o liberación
  if (nbytes == 0) {
    if (liberar_inodo(ninodo) == FALLO) {
      fprintf(stderr, "Error al liberar el inodo %u\n", ninodo);
      bumount();
      return FALLO;
    }
  } else {
    if (mi_truncar_f(ninodo, nbytes) == FALLO) {
      fprintf(stderr, "Error al truncar el inodo %u\n", ninodo);
      bumount();
      return FALLO;
    }
  }

  // 2. MOSTRAR DATOS DEL INODO TRAS LA OPERACIÓN
  struct inodo inodo;
  if (leer_inodo(ninodo, &inodo) == FALLO) {
    bumount();
    return FALLO;
  }

  printf("\nDATOS INODO %u:\n", ninodo);
  printf("tipo=%c\n", inodo.tipo);
  printf("permisos=%u\n", inodo.permisos);

  char atime[80], mtime[80], ctime[80], btime[80];
  struct tm *ts;
  ts = localtime(&inodo.atime);
  strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
  ts = localtime(&inodo.mtime);
  strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
  ts = localtime(&inodo.ctime);
  strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
  ts = localtime(&inodo.btime);
  strftime(btime, sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);

  printf("atime: %s\n", atime);
  printf("mtime: %s\n", mtime);
  printf("ctime: %s\n", ctime);
  printf("btime: %s\n", btime);
  printf("nlinks=%u\n", inodo.nlinks);
  printf("tamEnBytesLog=%u\n", inodo.tamEnBytesLog);
  printf("numBloquesOcupados=%u\n", inodo.numBloquesOcupados);

  if (bumount() == FALLO)
    return FALLO;
  return EXITO;
}
