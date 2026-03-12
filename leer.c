#include "ficheros.h"

#define TAMBUFFER 1500

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr,
            RED "Sintaxis: ./leer <nombre_dispositivo> <ninodo>\n" RESET);
    return FALLO;
  }

  char *nombre_dispositivo = argv[1];
  unsigned int ninodo = atoi(argv[2]);

  if (bmount(nombre_dispositivo) == FALLO)
    return FALLO;

  char buffer_texto[TAMBUFFER];
  int offset = 0;
  int leidos = 0;
  int total_leidos = 0;

  memset(buffer_texto, 0, TAMBUFFER);

  leidos = mi_read_f(ninodo, buffer_texto, offset, TAMBUFFER);
  while (leidos > 0) {
    if (write(1, buffer_texto, leidos) < 0) {
      perror("Error en write");
      break;
    }

    total_leidos += leidos;
    offset += leidos;

    memset(buffer_texto, 0, TAMBUFFER);
    leidos = mi_read_f(ninodo, buffer_texto, offset, TAMBUFFER);
  }

  struct STAT stat;
  if (mi_stat_f(ninodo, &stat) == FALLO)
    return FALLO;

  char info[128];
  sprintf(info, "\nTotal bytes leídos: %d\n", total_leidos);
  write(2, info, strlen(info));
  sprintf(info, "tamEnBytesLog del inodo: %u\n", stat.tamEnBytesLog);
  write(2, info, strlen(info));

  if (bumount() == FALLO)
    return FALLO;

  return EXITO;
}
