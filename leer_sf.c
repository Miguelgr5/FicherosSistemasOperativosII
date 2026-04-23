#include "directorios.h"
// #define DEBUGN2
// #define DEBUGN3
// #define DEBUGN4
// #define DEBUGN7
void mostrar_buscar_entrada(char *camino, char reservar) {
  unsigned int p_inodo_dir = 0;
  unsigned int p_inodo = 0;
  unsigned int p_entrada = 0;
  int error;
  printf("\ncamino: %s, reservar: %d\n", camino, reservar);
  if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada,
                              reservar, 6)) < 0) {
    mostrar_error_buscar_entrada(error);
  }
  printf("*********************************************************************"
         "*\n");
  return;
}

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
#if defined(DEBUGN2)
  // 2. Test Reservar/Liberar
  printf("\nRESERVAMOS UN BLOQUE Y LUEGO LO LIBERAMOS\n");
  int primerLibre = reservar_bloque();
  if (primerLibre == -1)
    return EXIT_FAILURE;
  bread(posSB, &sb); // Recargamos SB para ver cambios
  printf("Se ha reservado el bloque físico nº %d que era el 1º libre indicado "
         "por el MB\n",
         primerLibre);
  printf("SB.cantBloquesLibres = %u\n", sb.cantBloquesLibres);

  liberar_bloque(primerLibre);
  bread(posSB, &sb); // Recargamos SB
  printf("Liberamos ese bloque y después SB.cantBloquesLibres = %u\n",
         sb.cantBloquesLibres);
#endif
#if defined(DEBUGN3)
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
#endif
#if defined(DEBUGN4)
  // --- PRUEBA NIVEL 4 ---
  printf("\nINODO 1. TRADUCCION DE LOS BLOQUES LOGICOS 8, 204, 30.004, 400.004 "
         "y 468.750\n\n");

  int ninodo = reservar_inodo('f', 6);

  unsigned int bloques[] = {8, 204, 30004, 400004, 468750};
  for (int i = 0; i < 5; i++) {
    traducir_bloque_inodo(ninodo, bloques[i], 1);
    printf("\n"); // Espacio entre traducciones para claridad
  }

  // Mostrar datos del inodo reservado
  struct inodo inodo_test;
  leer_inodo(ninodo, &inodo_test);

  struct tm *ts;
  char atime[80], mtime[80], ctime[80], btime[80];
  ts = localtime(&inodo_test.atime);
  strftime(atime, sizeof(atime), "%a %Y-%m-%d %H:%M:%S", ts);
  ts = localtime(&inodo_test.mtime);
  strftime(mtime, sizeof(mtime), "%a %Y-%m-%d %H:%M:%S", ts);
  ts = localtime(&inodo_test.ctime);
  strftime(ctime, sizeof(ctime), "%a %Y-%m-%d %H:%M:%S", ts);
  ts = localtime(&inodo_test.ctime);
  strftime(btime, sizeof(btime), "%a %Y-%m-%d %H:%M:%S", ts);

  printf("DATOS DEL INODO RESERVADO %d\n", ninodo);
  printf("tipo: %c\npermisos: %u\n", inodo_test.tipo, inodo_test.permisos);
  printf("atime: %s\nmtime: %s\nctime: %s\nbtime: %s\n", atime, mtime, ctime,
         btime);
  printf("nlinks: %u\ntamEnBytesLog: %u\nnumBloquesOcupados: %u\n",
         inodo_test.nlinks, inodo_test.tamEnBytesLog,
         inodo_test.numBloquesOcupados);

  // Actualizar y mostrar el estado final del superbloque
  bread(posSB, &sb);
  printf("\nSB.posPrimerInodoLibre = %u\n", sb.posPrimerInodoLibre);
#endif
#if defined(DEBUGN7)
  // Mostrar creación directorios y errores
  mostrar_buscar_entrada("pruebas/", 1);  // ERROR_CAMINO_INCORRECTO
  mostrar_buscar_entrada("/pruebas/", 0); // ERROR_NO_EXISTE_ENTRADA_CONSULTA
  mostrar_buscar_entrada("/pruebas/docs/",
                         1); // ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO
  mostrar_buscar_entrada("/pruebas/", 1);          // creamos /pruebas/
  mostrar_buscar_entrada("/pruebas/docs/", 1);     // creamos /pruebas/docs/
  mostrar_buscar_entrada("/pruebas/docs/doc1", 1); // creamos /pruebas/docs/doc1
  mostrar_buscar_entrada("/pruebas/docs/doc1/doc11", 1);
  // ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO
  mostrar_buscar_entrada("/pruebas/", 1); // ERROR_ENTRADA_YA_EXISTENTE
  mostrar_buscar_entrada("/pruebas/docs/doc1",
                         0); // consultamos /pruebas/docs/doc1
  mostrar_buscar_entrada("/pruebas/docs/doc1", 1); // ERROR_ENTRADA_YA_EXISTENTE
  mostrar_buscar_entrada("/pruebas/casos/", 1);    // creamos /pruebas/casos/
  mostrar_buscar_entrada("/pruebas/docs/doc2", 1); // creamos /pruebas/docs/doc2
#endif
  return bumount();
}
