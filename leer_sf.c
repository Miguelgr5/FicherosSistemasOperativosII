#include "ficheros_basico.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, RED "Uso: %s <nombre_dispositivo>\n" RESET, argv[0]);
    return EXIT_FAILURE;
  }

  if (bmount(argv[1]) == FALLO)
    return EXIT_FAILURE;

  struct superbloque sb;
  if (bread(posSB, &sb) == -1) {
    bumount();
    return EXIT_FAILURE;
  }

  printf("=== DATOS DEL SUPERBLOQUE ===\n");
  printf("posPrimerBloqueMB: %u\nposUltimoBloqueMB: %u\n", sb.posPrimerBloqueMB,
         sb.posUltimoBloqueMB);
  printf("posPrimerBloqueAI: %u\nposUltimoBloqueAI: %u\n", sb.posPrimerBloqueAI,
         sb.posUltimoBloqueAI);
  printf("posPrimerBloqueDatos: %u\nposUltimoBloqueDatos: %u\n",
         sb.posPrimerBloqueDatos, sb.posUltimoBloqueDatos);
  printf("cantBloquesLibres: %u\ncantInodosLibres: %u\n", sb.cantBloquesLibres,
         sb.cantInodosLibres);
  printf("posInodoRaiz: %u\nposPrimerInodoLibre: %u\n", sb.posInodoRaiz,
         sb.posPrimerInodoLibre);

  printf("\n=== TEST MAPA DE BITS (leer_bit) ===\n");
  printf("Bit %u (SB): %d\n", posSB, leer_bit(posSB));
  printf("Bit %u (Inicio AI): %d\n", sb.posPrimerBloqueAI,
         leer_bit(sb.posPrimerBloqueAI));
  printf("Bit %u (Inicio Datos): %d\n", sb.posPrimerBloqueDatos,
         leer_bit(sb.posPrimerBloqueDatos));
  printf("Bit %u (Último bloque): %d\n", sb.totBloques - 1,
         leer_bit(sb.totBloques - 1));

  printf("\n=== TEST RESERVAR/LIBERAR BLOQUE ===\n");
  int bloqueReservado = reservar_bloque();
  bread(posSB, &sb); // Actualizar info tras reservar
  printf("Bloque reservado: %d. Bloques libres: %u\n", bloqueReservado,
         sb.cantBloquesLibres);

  liberar_bloque(bloqueReservado);
  bread(posSB, &sb); // Actualizar info tras liberar
  printf("Bloque liberado: %d. Bloques libres: %u\n", bloqueReservado,
         sb.cantBloquesLibres);

  printf("\n=== DATOS DEL INODO RAÍZ ===\n");
  struct inodo raiz;
  leer_inodo(sb.posInodoRaiz, &raiz);

  struct tm *ts;
  char atime[80], mtime[80], ctime[80];
  ts = localtime(&raiz.atime);
  strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
  ts = localtime(&raiz.mtime);
  strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
  ts = localtime(&raiz.ctime);
  strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);

  printf("Tipo: %c\nPermisos: %u\n", raiz.tipo, raiz.permisos);
  printf("ATIME: %s\nMTIME: %s\nCTIME: %s\n", atime, mtime, ctime);
  printf("nlinks: %u\ntamEnBytesLog: %u\nnumBloquesOcupados: %u\n", raiz.nlinks,
         raiz.tamEnBytesLog, raiz.numBloquesOcupados);

  bumount();
  return EXIT_SUCCESS;
}
