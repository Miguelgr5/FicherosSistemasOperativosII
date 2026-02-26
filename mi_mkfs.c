#include "ficheros_basico.h"

int main(int argc, char *argv[]) {

  // 1️⃣ Validación de argumentos
  if (argc != 3) {
    fprintf(stderr, RED "Uso: %s <ruta_fichero> <num_bloques>\n" RESET,
            argv[0]);
    return EXIT_FAILURE;
  }

  int bloques = atoi(argv[2]);
  if (bloques <= 0) {
    fprintf(stderr, RED
            "Error: El número de bloques debe ser un entero positivo.\n" RESET);
    return EXIT_FAILURE;
  }

  // 2️⃣ Montar el dispositivo
  if (bmount(argv[1]) == FALLO) {
    fprintf(stderr, RED "Error: No se pudo montar el dispositivo.\n" RESET);
    return EXIT_FAILURE;
  }

  printf("Formateando %d bloques con nivel 1...\n", bloques);

  // 3️⃣ Inicializar superbloque
  if (initSB(bloques, bloques / 4) == FALLO) { // ej: 1 inodo por 4 bloques
    fprintf(stderr, RED "Error inicializando superbloque.\n" RESET);
    bumount();
    return EXIT_FAILURE;
  }

  // 4️⃣ Inicializar mapa de bits
  if (initMB() == FALLO) {
    fprintf(stderr, RED "Error inicializando mapa de bits.\n" RESET);
    bumount();
    return EXIT_FAILURE;
  }

  // 5️⃣ Inicializar lista de inodos libres
  if (initAi() == FALLO) {
    fprintf(stderr, RED "Error inicializando inodos.\n" RESET);
    bumount();
    return EXIT_FAILURE;
  }
  // 6️⃣ Reservar inodo raíz
  printf("Creando directorio raíz...\n");
  if (reservar_inodo('d', 7) == FALLO) {
    fprintf(stderr, RED "Error al reservar el inodo raíz.\n" RESET);
    bumount();
    return EXIT_FAILURE;
  }
  // 6️⃣ Desmontar
  if (bumount() == FALLO) {
    fprintf(stderr, RED "Error desmontando el dispositivo.\n" RESET);
    return EXIT_FAILURE;
  }

  printf(GREEN "Éxito: Sistema de ficheros creado correctamente.\n" RESET);
  return EXIT_SUCCESS;
}
