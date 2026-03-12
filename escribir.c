#include "ficheros.h"

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Sintaxis: escribir <nombre_dispositivo> <\"$(cat "
                    "fichero)\"> <diferentes_inodos>\n");
    fprintf(stderr, "Offsets: 9000, 209000, 30725000, 409605000, 480000000\n");
    fprintf(stderr, "Si diferentes_inodos=0 se reserva un solo inodo para "
                    "todos los offsets\n");
    return FALLO;
  }

  if (bmount(argv[1]) == FALLO)
    return FALLO;

  char *texto = argv[2];
  int diferentes_inodos = atoi(argv[3]);
  int nbytes = strlen(texto);
  unsigned int offsets[] = {9000, 209000, 30725000, 409605000, 480000000};
  unsigned int ninodo;

  printf("longitud texto: %d\n", nbytes);

  if (diferentes_inodos == 0) {
    ninodo = reservar_inodo('f', 6);
  }

  for (int i = 0; i < 5; i++) {
    if (diferentes_inodos == 1) {
      ninodo = reservar_inodo('f', 6);
    }

    printf("\nNº inodo reservado: %d\n", ninodo);
    printf("offset: %u\n", offsets[i]);

    // Nota: Los mensajes de [traducir_bloque_inodo()...]
    // deberían salir desde dentro de dicha función para ser automáticos.

    int escritos = mi_write_f(ninodo, texto, offsets[i], nbytes);

    struct STAT stat;
    mi_stat_f(ninodo, &stat);

    printf("Bytes escritos: %d\n", escritos);
    printf("stat.tamEnBytesLog=%u\n", stat.tamEnBytesLog);
    printf("stat.numBloquesOcupados=%u\n", stat.numBloquesOcupados);
  }

  bumount();
  return EXITO;
}
