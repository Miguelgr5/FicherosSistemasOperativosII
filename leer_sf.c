#include "ficheros_basico.h"

int main(int argc, char *argv[]) {

  if (argc != 2) {
    fprintf(stderr, RED "Uso: %s <nombre_dispositivo>\n" RESET, argv[0]);
    return EXIT_FAILURE;
  }

  // 1️⃣ Montar dispositivo
  if (bmount(argv[1]) == FALLO) {
    fprintf(stderr, RED "Error: no se pudo montar el dispositivo.\n" RESET);
    return EXIT_FAILURE;
  }

  struct superbloque sb;

  // 2️⃣ Leer superbloque
  if (bread(posSB, &sb) == -1) {
    fprintf(stderr, RED "Error: no se pudo leer el superbloque.\n" RESET);
    bumount();
    return EXIT_FAILURE;
  }

  // 3️⃣ Mostrar todos los campos del superbloque
  printf("=== Superbloque ===\n");
  printf("posPrimerBloqueMB: %u\n", sb.posPrimerBloqueMB);
  printf("posUltimoBloqueMB: %u\n", sb.posUltimoBloqueMB);
  printf("posPrimerBloqueAI: %u\n", sb.posPrimerBloqueAI);
  printf("posUltimoBloqueAI: %u\n", sb.posUltimoBloqueAI);
  printf("posPrimerBloqueDatos: %u\n", sb.posPrimerBloqueDatos);
  printf("posUltimoBloqueDatos: %u\n", sb.posUltimoBloqueDatos);
  printf("posInodoRaiz: %u\n", sb.posInodoRaiz);
  printf("posPrimerInodoLibre: %u\n", sb.posPrimerInodoLibre);
  printf("cantBloquesLibres: %u\n", sb.cantBloquesLibres);
  printf("cantInodosLibres: %u\n", sb.cantInodosLibres);
  printf("totBloques: %u\n", sb.totBloques);
  printf("totInodos: %u\n", sb.totInodos);

  // 4️⃣ Mostrar tamaño de struct inodo
  printf("sizeof(struct inodo) = %lu bytes\n", sizeof(struct inodo));

  // 5️⃣ Recorrer lista de inodos libres
  printf("\n=== Lista de inodos libres ===\n");

  struct inodo inodos[BLOCKSIZE / INODOSIZE];
  unsigned int inodoLibre = sb.posPrimerInodoLibre;
  unsigned int leidos = 0;

  while (inodoLibre != UINT_MAX && leidos < sb.totInodos) {

    unsigned int bloque =
        sb.posPrimerBloqueAI + (inodoLibre / (BLOCKSIZE / INODOSIZE));
    unsigned int indice = inodoLibre % (BLOCKSIZE / INODOSIZE);

    // Leer bloque si es la primera vez
    if (bread(bloque, inodos) == -1) {
      fprintf(stderr, RED "Error leyendo bloque de inodos.\n" RESET);
      bumount();
      return EXIT_FAILURE;
    }

    printf("Inodo %u -> punterosDirectos[0] = %u\n", inodoLibre,
           inodos[indice].punterosDirectos[0]);

    inodoLibre = inodos[indice].punterosDirectos[0];
    leidos++;
  }

  // 6️⃣ Desmontar
  if (bumount() == FALLO) {
    fprintf(stderr, RED "Error desmontando el dispositivo.\n" RESET);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
