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

    printf("\n------------------------------------------\n");
    printf("Escribiendo en inodo %d, offset %u, longitud %d\n", ninodo,
           offsets[i], nbytes);

    int escritos = mi_write_f(ninodo, texto, offsets[i], nbytes);
    if (escritos < 0) {
      fprintf(stderr, "Error en mi_write_f\n");
      continue;
    }
    printf("Bytes escritos: %d\n", escritos);

    struct STAT stat;
    if (mi_stat_f(ninodo, &stat) == FALLO)
      return FALLO;
    printf("tamEnBytesLog: %u\n", stat.tamEnBytesLog);
    printf("numBloquesOcupados: %u\n", stat.numBloquesOcupados);

    /* --- TEST DE LECTURA INMEDIATA --- */
    /*
    void *buffer_lectura = malloc(nbytes);
    memset(buffer_lectura, 0, nbytes);
    if (mi_read_f(ninodo, buffer_lectura, offsets[i], nbytes) > 0) {
        write(1, "Validación lectura: ", 20);
        write(1, buffer_lectura, nbytes);
        write(1, "\n", 1);
    }
    free(buffer_lectura);
    */
  }

  if (bumount() == FALLO)
    return FALLO;

  return EXITO;
}
