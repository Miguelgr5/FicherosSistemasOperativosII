#include "ficheros_basico.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, RED "Uso: %s <nombre_dispositivo>\n" RESET, argv[0]);
    return EXIT_FAILURE;
  }

  if (bmount(argv[1]) == FALLO)
    return EXIT_FAILURE;

  struct superbloque sb;
  if (bread(posSB, &sb) == -1)
    return EXIT_FAILURE;

  // 1. Mostrar Superbloque
  printf("DATOS DEL SUPERBLOQUE\n");
  printf("posPrimerBloqueMB = %u\n", sb.posPrimerBloqueMB);
  printf("posUltimoBloqueMB = %u\n", sb.posUltimoBloqueMB);
  printf("posPrimerBloqueAI = %u\n", sb.posPrimerBloqueAI);
  printf("posUltimoBloqueAI = %u\n", sb.posUltimoBloqueAI);
  printf("posPrimerBloqueDatos = %u\n", sb.posPrimerBloqueDatos);
  printf("posUltimoBloqueDatos = %u\n", sb.posUltimoBloqueDatos);
  printf("posInodoRaiz = %u\n", sb.posInodoRaiz);
  printf("posPrimerInodoLibre = %u\n", sb.posPrimerInodoLibre);
  printf("cantBloquesLibres = %u\n", sb.cantBloquesLibres);
  printf("cantInodosLibres = %u\n", sb.cantInodosLibres);
  printf("totBloques = %u\n", sb.totBloques);
  printf("totInodos = %u\n", sb.totInodos);

  // 2. Test Reservar/Liberar
  printf("\nRESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");
  int primerLibre = reservar_bloque();
  bread(posSB, &sb); // Recargamos SB para ver cambios
  printf("Se ha reservado el bloque físico nº %d que era el 1º libre indicado "
         "por el MB\n",
         primerLibre);
  printf("SB.cantBloquesLibres = %u\n", sb.cantBloquesLibres);

  liberar_bloque(primerLibre);
  bread(posSB, &sb); // Recargamos SB
  printf("Liberamos ese bloque y después SB.cantBloquesLibres = %u\n",
         sb.cantBloquesLibres);

  // 3. Mapa de Bits con rastreo (Debug)
  printf("\nMAPA DE BITS CON BLOQUES DE METADATOS OCUPADOS\n");
  unsigned int bits_a_testear[] = {0,
                                   sb.posPrimerBloqueMB,
                                   sb.posUltimoBloqueMB,
                                   sb.posPrimerInodoLibre,
                                   sb.posUltimoBloqueAI,
                                   sb.posPrimerBloqueDatos,
                                   sb.posUltimoBloqueDatos};
  char *nombres[] = {"posSB",
                     "SB.posPrimerBloqueMB",
                     "SB.posUltimoBloqueMB",
                     "SB.posPrimerBloqueAI",
                     "SB.posUltimoBloqueAI",
                     "SB.posPrimerBloqueDatos",
                     "SB.posUltimoBloqueDatos"};

  for (int i = 0; i < 7; i++) {
    unsigned int n = bits_a_testear[i];
    unsigned int posbyteMB = n / 8;
    unsigned int posbit = n % 8;
    unsigned int nbloqueMB = posbyteMB / BLOCKSIZE;
    unsigned int nbloqueabs = sb.posPrimerBloqueMB + nbloqueMB;
    unsigned int posbyte = posbyteMB % BLOCKSIZE;

    printf("[leer_bit(%u)→ posbyteMB:%u, posbyte:%u, posbit:%u, nbloqueMB:%u, "
           "nbloqueabs:%u)]\n",
           n, posbyteMB, posbyte, posbit, nbloqueMB, nbloqueabs);
    printf("%s: %u → leer_bit(%u) = %d\n", nombres[i], n, n, leer_bit(n));
  }

  // 4. Datos Inodo Raíz
  printf("\nDATOS DEL DIRECTORIO RAIZ\n");
  struct inodo raiz;
  leer_inodo(sb.posInodoRaiz, &raiz);

  struct tm *ts;
  char atime[80], mtime[80], ctime[80], btime[80];
  ts = localtime(&raiz.atime);
  strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
  ts = localtime(&raiz.mtime);
  strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
  ts = localtime(&raiz.ctime);
  strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
  ts = localtime(&raiz.ctime);
  strftime(btime, sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);

  printf("tipo: %c\n", raiz.tipo);
  printf("permisos: %u\n", raiz.permisos);
  printf("atime: %s\nmtime: %s\nctime: %s\nbtime: %s\n", atime, mtime, ctime,
         btime);
  printf("nlinks: %u\ntamEnBytesLog: %u\nnumBloquesOcupados: %u\n", raiz.nlinks,
         raiz.tamEnBytesLog, raiz.numBloquesOcupados);

  bumount();
  return EXIT_SUCCESS;
}
