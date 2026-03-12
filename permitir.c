#include "ficheros.h"

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, RED "Sintaxis: ./escribir <nombre_dispositivo> <\"texto\"> "
                        "<diferentes_inodos>\n" RESET);
    return FALLO;
  }

  char *nombre_dispositivo = argv[1];
  char *texto = argv[2];
  int diferentes_inodos = atoi(argv[3]);
  int nbytes = strlen(texto);

  unsigned int offsets[] = {9000, 209000, 30725000, 409605000, 480000000};
  int num_offsets = sizeof(offsets) / sizeof(unsigned int);

  if (bmount(nombre_dispositivo) == FALLO)
    return FALLO;

  unsigned int ninodo;
  if (diferentes_inodos == 0) {
    ninodo = reservar_inodo('f', 6);
    if (ninodo == FALLO)
      return FALLO;
  }

  for (int i = 0; i < num_offsets; i++) {
    if (diferentes_inodos == 1) {
      ninodo = reservar_inodo('f', 6);
      if (ninodo == FALLO)
        break;
    }

    printf("\nInodo: %d, Offset: %u, Bytes: %d\n", ninodo, offsets[i], nbytes);

    int escritos = mi_write_f(ninodo, texto, offsets[i], nbytes);
    if (escritos < 0)
      continue;

    struct STAT stat;
    if (mi_stat_f(ninodo, &stat) != FALLO) {
      printf("tamEnBytesLog: %u\n", stat.tamEnBytesLog);
      printf("numBloquesOcupados: %u\n", stat.numBloquesOcupados);
    }
  }

  bumount();
  return EXITO;
}
