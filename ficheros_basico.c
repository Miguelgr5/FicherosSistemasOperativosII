#include "ficheros_basico.h"
#include <stdio.h>
#define DEBUGN6
int tamMB(unsigned int nbloques) {

  int nbytes = nbloques / 8;
  if (nbloques % 8 != 0)
    nbytes++;

  int res = nbytes / BLOCKSIZE;
  if (nbytes % BLOCKSIZE != 0)
    res++;

  return res;
}
int tamAI(unsigned int ninodos) {
  int res = (ninodos * INODOSIZE) / BLOCKSIZE;
  if ((ninodos * INODOSIZE) % BLOCKSIZE != 0)
    res++;
  return res;
}
int initSB(unsigned int nbloques, unsigned int ninodos) {
  struct superbloque sb;
  sb.posPrimerBloqueMB = posSB + tamSB;
  sb.posUltimoBloqueMB = sb.posPrimerBloqueMB + tamMB(nbloques) - 1;
  sb.posPrimerBloqueAI = sb.posUltimoBloqueMB + 1;
  sb.posUltimoBloqueAI = sb.posPrimerBloqueAI + tamAI(ninodos) - 1;
  sb.posPrimerBloqueDatos = sb.posUltimoBloqueAI + 1;
  sb.posUltimoBloqueDatos = nbloques - 1;
  sb.posInodoRaiz = 0;
  sb.posPrimerInodoLibre = 0;
  sb.cantBloquesLibres = nbloques;
  sb.cantInodosLibres = ninodos;
  sb.totBloques = nbloques;
  sb.totInodos = ninodos;
  return bwrite(posSB, (char *)&sb);
}
int initMB() {
  struct superbloque sb;
  if (bread(posSB, &sb) == -1)
    return FALLO;

  unsigned int bloquesMetadatos =
      tamSB + tamMB(sb.totBloques) + tamAI(sb.totInodos);
  unsigned char bufferMB[BLOCKSIZE];
  unsigned int bloquesMB = tamMB(sb.totBloques);

  unsigned int bitsAscribir = bloquesMetadatos;

  for (int i = 0; i < bloquesMB; i++) {
    memset(bufferMB, 0, BLOCKSIZE); // Inicializamos bloque a 0 (libre)

    // Rellenamos los bytes que podamos en este bloque
    for (int j = 0; j < BLOCKSIZE && bitsAscribir > 0; j++) {
      if (bitsAscribir >= 8) {
        bufferMB[j] = 255; // 11111111
        bitsAscribir -= 8;
      } else {
        // Bits sueltos (solo ocurre una vez al final de los metadatos)
        for (int bit = 0; bit < bitsAscribir; bit++) {
          bufferMB[j] |= (1 << (7 - bit));
        }
        bitsAscribir = 0;
      }
    }
    if (bwrite(sb.posPrimerBloqueMB + i, bufferMB) == -1)
      return FALLO;
  }

  sb.cantBloquesLibres = sb.totBloques - bloquesMetadatos;
  return bwrite(posSB, &sb);
}
int initAi() {
  struct superbloque sb;
  if (bread(posSB, &sb) == -1)
    return FALLO;

  struct inodo inodos[BLOCKSIZE / INODOSIZE];
  unsigned int contInodo = 0;

  for (int i = sb.posPrimerBloqueAI; i <= sb.posUltimoBloqueAI; i++) {
    // No hace falta bread() porque vamos a limpiar todo el bloque
    for (int j = 0; j < BLOCKSIZE / INODOSIZE; j++) {
      inodos[j].tipo = 'l'; // Libre
      if (contInodo < sb.totInodos - 1) {
        inodos[j].punterosDirectos[0] = contInodo + 1; // Enlace al siguiente
      } else {
        inodos[j].punterosDirectos[0] = UINT_MAX; // Último de la lista
      }
      contInodo++;
    }
    if (bwrite(i, inodos) == -1)
      return FALLO;
  }
  return EXITO;
}
int escribir_bit(unsigned int nbloque, unsigned int bit) {
  struct superbloque sb;
  if (bread(posSB, &sb) == -1)
    return FALLO;

  unsigned int posbyteMB = nbloque / 8;
  unsigned int posbit = nbloque % 8;
  unsigned int nbloqueMB = posbyteMB / BLOCKSIZE;
  unsigned int nbloqueabs = sb.posPrimerBloqueMB + nbloqueMB;
  unsigned int posbyte = posbyteMB % BLOCKSIZE;
  unsigned char bufferMB[BLOCKSIZE];
  if (bread(nbloqueabs, bufferMB) == -1)
    return FALLO;
  unsigned char mascara = 128;
  mascara >>= posbit;
  (bit == 1) ? (bufferMB[posbyte] |= mascara) : (bufferMB[posbyte] &= ~mascara);
  return bwrite(nbloqueabs, bufferMB);
}
char leer_bit(unsigned int nbloque) {
  struct superbloque sb;
  if (bread(posSB, &sb) == -1)
    return FALLO;

  unsigned int posbyteMB = nbloque / 8;
  unsigned int posbit = nbloque % 8;
  unsigned int nbloqueMB = posbyteMB / BLOCKSIZE;
  unsigned int nbloqueabs = sb.posPrimerBloqueMB + nbloqueMB;
  unsigned int posbyte = posbyteMB % BLOCKSIZE;
  unsigned char bufferMB[BLOCKSIZE];
  if (bread(nbloqueabs, bufferMB) == -1)
    return FALLO;
  unsigned char mascara = 128;
  mascara >>= posbit;
  mascara &= bufferMB[posbyte];
  mascara >>= (7 - posbit);
  return mascara;
}
int reservar_bloque() {
  struct superbloque SB;
  if (bread(posSB, &SB) == -1)
    return FALLO;
  if (SB.cantBloquesLibres == 0) {
    return -1;
  }

  unsigned char bufferMB[BLOCKSIZE];
  unsigned char bufferAux[BLOCKSIZE];
  unsigned int totalBloquesMB =
      (SB.posUltimoBloqueMB - SB.posPrimerBloqueMB) + 1;
  memset(bufferAux, 255, BLOCKSIZE);

  unsigned int nbloqueMB = 0;
  int encontrado = 0;

  while (nbloqueMB < totalBloquesMB) {
    if (bread(SB.posPrimerBloqueMB + nbloqueMB, bufferMB) == -1)
      return -1;

    if (memcmp(bufferMB, bufferAux, BLOCKSIZE) != 0) {
      encontrado = 1;
      break;
    }
    nbloqueMB++;
  }

  if (!encontrado)
    return -1;

  int posbyte = 0;
  while (bufferMB[posbyte] == 255) {
    posbyte++;
  }

  unsigned char mascara = 128;
  int posbit = 0;
  unsigned char byteAux = bufferMB[posbyte];

  while (byteAux & mascara) {
    byteAux <<= 1;
    posbit++;
  }

  unsigned int nbloque = (nbloqueMB * BLOCKSIZE + posbyte) * 8 + posbit;

  escribir_bit(nbloque, 1);

  SB.cantBloquesLibres--;
  bwrite(posSB, &SB);

  unsigned char bufferCeros[BLOCKSIZE];
  memset(bufferCeros, 0, BLOCKSIZE);
  bwrite(nbloque, bufferCeros);

  return nbloque;
}
int liberar_bloque(unsigned int nbloque) {
  if (escribir_bit(nbloque, 0) == -1)
    return FALLO;
  struct superbloque sb;
  if (bread(posSB, &sb) == -1)
    return FALLO;
  sb.cantBloquesLibres++;
  if (bwrite(posSB, &sb) == -1)
    return FALLO;
  return nbloque;
}
int escribir_inodo(unsigned int ninodo, struct inodo *inodo) {
  struct superbloque sb;
  if (bread(posSB, &sb) == -1)
    return FALLO;
  unsigned int nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;
  unsigned int nbloqueabs = nbloqueAI + sb.posPrimerBloqueAI;
  struct inodo inodos[BLOCKSIZE / INODOSIZE];
  if (bread(nbloqueabs, inodos) == -1)
    return FALLO;
  unsigned int posinodo = ninodo % (BLOCKSIZE / INODOSIZE);
  inodos[posinodo] = *inodo;
  return bwrite(nbloqueabs, inodos);
}
int leer_inodo(unsigned int ninodo, struct inodo *inodo) {
  struct superbloque sb;
  if (bread(posSB, &sb) == -1)
    return FALLO;
  unsigned int nbloqueAI = (ninodo * INODOSIZE) / BLOCKSIZE;
  unsigned int nbloqueabs = nbloqueAI + sb.posPrimerBloqueAI;
  struct inodo inodos[BLOCKSIZE / INODOSIZE];
  if (bread(nbloqueabs, inodos) == -1)
    return FALLO;
  unsigned int posinodo = ninodo % (BLOCKSIZE / INODOSIZE);
  *inodo = inodos[posinodo];
  return EXITO;
}
int reservar_inodo(unsigned char tipo, unsigned char permisos) {
  struct superbloque sb;
  if (bread(posSB, &sb) == -1)
    return FALLO;
  if (sb.cantInodosLibres == 0)
    return FALLO;
  unsigned int posInodoReservado = sb.posPrimerInodoLibre;
  struct inodo inodoAux;
  if (leer_inodo(posInodoReservado, &inodoAux) == -1)
    return FALLO;
  sb.posPrimerInodoLibre = inodoAux.punterosDirectos[0];
  inodoAux.tipo = tipo;
  inodoAux.permisos = permisos;
  inodoAux.nlinks = 1;
  inodoAux.tamEnBytesLog = 0;
  inodoAux.atime = time(NULL);
  inodoAux.mtime = time(NULL);
  inodoAux.ctime = time(NULL);
  inodoAux.numBloquesOcupados = 0;

  for (int i = 0; i < 12; i++) {
    inodoAux.punterosDirectos[i] = 0;
  }
  for (int i = 0; i < 3; i++) {
    inodoAux.punterosIndirectos[i] = 0;
  }

  if (escribir_inodo(posInodoReservado, &inodoAux) == -1)
    return FALLO;

  sb.cantInodosLibres--;
  if (bwrite(posSB, &sb) == -1)
    return FALLO;

  return posInodoReservado;
}
int obtener_nRangoBL(struct inodo *inodo, unsigned int nblogico,
                     unsigned int *ptr) {
  if (nblogico < DIRECTOS) {
    *ptr = inodo->punterosDirectos[nblogico];
    return 0;
  } else if (nblogico < INDIRECTOS0) {
    *ptr = inodo->punterosIndirectos[0];
    return 1;
  } else if (nblogico < INDIRECTOS1) {
    *ptr = inodo->punterosIndirectos[1];
    return 2;
  } else if (nblogico < INDIRECTOS2) {
    *ptr = inodo->punterosIndirectos[2];
    return 3;
  } else {
    *ptr = 0;
    fprintf(stderr, RED "Bloque lógico fuera de rango.\n" RESET);
    return FALLO;
  }
}
int obtener_indice(unsigned int nblogico, int nivel_punteros) {
  if (nblogico < DIRECTOS) {
    return nblogico;
  } else if (nblogico < INDIRECTOS0) {
    return nblogico - DIRECTOS;
  } else if (nblogico < INDIRECTOS1) {
    if (nivel_punteros == 2) {
      return (nblogico - INDIRECTOS0) / NPUNTEROS;
    }
    return (nblogico - INDIRECTOS0) % NPUNTEROS;
  } else if (nblogico < INDIRECTOS2) {
    if (nivel_punteros == 3)
      return (nblogico - INDIRECTOS1) / (NPUNTEROS * NPUNTEROS);
    if (nivel_punteros == 2)
      return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) / NPUNTEROS;
    if (nivel_punteros == 1)
      return ((nblogico - INDIRECTOS1) % (NPUNTEROS * NPUNTEROS)) % NPUNTEROS;
  }
  fprintf(stderr, RED "Error obteniendo Índice.\n" RESET);
  return FALLO;
}
int traducir_bloque_inodo(unsigned int ninodo, unsigned int nblogico,
                          unsigned char reservar) {
  unsigned int ptr = 0, ptr_ant = 0;
  int salvar_inodo = 0, indice = 0;
  int nRangoBL, nivel_punteros;
  unsigned int buffer[BLOCKSIZE / sizeof(unsigned int)];
  struct inodo inodo;

  if (leer_inodo(ninodo, &inodo) == FALLO)
    return FALLO;

  nRangoBL = obtener_nRangoBL(&inodo, nblogico, &ptr); // 0:D, 1:I0, 2:I1, 3:I2
  nivel_punteros = nRangoBL;

  if (nRangoBL == 0) { // Caso punteros Directos
    if (ptr == 0) {    // No existe el bloque de datos
      if (reservar == 0)
        return -1;

      ptr = reservar_bloque();
      inodo.numBloquesOcupados++;
      inodo.ctime = time(NULL);
      inodo.punterosDirectos[nblogico] = ptr;
      salvar_inodo = 1;
      printf("[traducir_bloque_inodo()→ inodo.punterosDirectos[%u] = %u "
             "(reservado BF %u para BL %u)]\n",
             nblogico, ptr, ptr, nblogico);
    }
  } else { // Caso de punteros Indirectos (I0, I1, I2)
    while (nivel_punteros > 0) {
      if (ptr == 0) { // No cuelgan bloques de punteros
        if (reservar == 0)
          return -1;

        ptr = reservar_bloque();
        inodo.numBloquesOcupados++;
        inodo.ctime = time(NULL);
        salvar_inodo = 1;

        if (nivel_punteros == nRangoBL) { // Cuelga directamente del inodo
          inodo.punterosIndirectos[nRangoBL - 1] = ptr;
          printf("[traducir_bloque_inodo()→ inodo.punterosIndirectos[%u] = %u "
                 "(reservado BF %u para punteros_nivel%u)]\n",
                 nRangoBL - 1, ptr, ptr, nivel_punteros);
        } else { // Cuelga de otro bloque de punteros
          buffer[indice] = ptr;
          if (bwrite(ptr_ant, buffer) == FALLO)
            return FALLO;
          printf("[traducir_bloque_inodo()→ punteros_nivel%u [%u] = %u "
                 "(reservado BF %u para punteros_nivel%u)]\n",
                 nivel_punteros + 1, indice, ptr, ptr, nivel_punteros);
        }
        memset(buffer, 0, BLOCKSIZE); // Limpiamos el nuevo bloque de punteros
      } else {
        if (bread(ptr, buffer) == FALLO)
          return FALLO;
      }

      indice = obtener_indice(nblogico, nivel_punteros);
      ptr_ant = ptr;
      ptr = buffer[indice];
      nivel_punteros--;
    }

    // Al salir del bucle estamos al nivel de datos
    if (ptr == 0) {
      if (reservar == 0)
        return -1;

      ptr = reservar_bloque();
      if (ptr == -1) {
        fprintf(stderr, RED "Error reservando bloque.\n" RESET);
        return FALLO;
      }
      inodo.numBloquesOcupados++;
      inodo.ctime = time(NULL);
      salvar_inodo = 1;
      buffer[indice] = ptr;
      if (bwrite(ptr_ant, buffer) == FALLO)
        return FALLO;
      printf("[traducir_bloque_inodo()→ punteros_nivel1 [%u] = %u (reservado "
             "BF %u para BL %u)]\n",
             indice, ptr, ptr, nblogico);
    }
  }

  if (salvar_inodo) {
    if (escribir_inodo(ninodo, &inodo) == FALLO)
      return FALLO;
  }

  return ptr; // Retorna el bloque físico
}
int liberar_inodo(unsigned int ninodo) {
  struct inodo inodo;
  struct superbloque sb;

  // 1. Leer el inodo a liberar
  if (leer_inodo(ninodo, &inodo) == FALLO)
    return FALLO;

  // 2. Liberar todos los bloques ocupados desde el bloque lógico 0
  int liberados = liberar_bloques_inodo(0, &inodo);
  if (liberados == FALLO)
    return FALLO;

  // 3. Actualizar la cantidad de bloques ocupados (debería quedar a 0)
  inodo.numBloquesOcupados -= liberados;

  // 4. Marcar el inodo como libre y reiniciar su tamaño
  inodo.tipo = 'l';
  inodo.tamEnBytesLog = 0;

  // 5. Actualizar la lista enlazada de inodos libres en el superbloque
  if (bread(posSB, &sb) == FALLO)
    return FALLO;

  // El inodo liberado apuntará al que era el primer inodo libre
  inodo.punterosDirectos[0] = sb.posPrimerInodoLibre;

  // El superbloque ahora apunta a este inodo como el primero libre
  sb.posPrimerInodoLibre = ninodo;
  sb.cantInodosLibres++;

  // 6. Escribir el superbloque actualizado en disco
  if (bwrite(posSB, &sb) == FALLO)
    return FALLO;

  // 7. Actualizar ctime y guardar el inodo
  inodo.ctime = time(NULL);
  if (escribir_inodo(ninodo, &inodo) == FALLO)
    return FALLO;

  return ninodo;
}
int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo) {
  unsigned int nivel_punteros, nblog, ultimoBL;
  unsigned char bufAux_punteros[BLOCKSIZE];
  unsigned int bloques_punteros[3][NPUNTEROS];
  int indices_primerBL[3];
  int liberados = 0;
  int i, j, k;
  int eof = 0;
  int contador_breads = 0;
  int contador_bwrites = 0;
  int bloque_modificado[3] = {0, 0, 0};

#if defined(DEBUGN6)
  int BLliberado = 0; // Para los prints de debug
#endif

  if (inodo->tamEnBytesLog == 0)
    return 0;

  if (inodo->tamEnBytesLog % BLOCKSIZE == 0) {
    ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE - 1;
  } else {
    ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE;
  }

#if defined(DEBUGN6)
  fprintf(stderr, "[liberar_bloques_inodo()→ primer BL: %d, último BL: %d]\n",
          primerBL, ultimoBL);
#endif

  memset(bufAux_punteros, 0, BLOCKSIZE);

  // 1. Liberar bloques Directos
  if (primerBL < DIRECTOS) {
    nivel_punteros = 0;
    i = obtener_indice(primerBL, nivel_punteros);
    while (!eof && i < DIRECTOS) {
      nblog = i;
      if (nblog == ultimoBL)
        eof = 1;
      if (inodo->punterosDirectos[i]) {
        liberar_bloque(inodo->punterosDirectos[i]);
#if defined(DEBUGN6)
        fprintf(
            stderr,
            "[liberar_bloques_inodo()→ liberado BF %d de datos para BL %d]\n",
            inodo->punterosDirectos[i], nblog);
#endif
        liberados++;
        inodo->punterosDirectos[i] = 0;
      }
      i++;
    }
  }

  // 2. Liberar bloques de Indirectos[0]
  if (primerBL < INDIRECTOS0 && !eof) {
    nivel_punteros = 1;
    if (inodo->punterosIndirectos[0]) {
      bread(inodo->punterosIndirectos[0], bloques_punteros[nivel_punteros - 1]);
      bloque_modificado[nivel_punteros - 1] = 0;
      contador_breads++;

      if (primerBL >= DIRECTOS)
        i = obtener_indice(primerBL, nivel_punteros);
      else
        i = 0;

      while (!eof && i < NPUNTEROS) {
        nblog = DIRECTOS + i;
        if (nblog == ultimoBL)
          eof = 1;
        if (bloques_punteros[nivel_punteros - 1][i]) {
          liberar_bloque(bloques_punteros[nivel_punteros - 1][i]);
#if defined(DEBUGN6)
          fprintf(
              stderr,
              "[liberar_bloques_inodo()→ liberado BF %d de datos para BL %d]\n",
              bloques_punteros[nivel_punteros - 1][i], nblog);
          BLliberado = nblog;
#endif
          liberados++;
          bloques_punteros[nivel_punteros - 1][i] = 0;
          bloque_modificado[nivel_punteros - 1] = 1;
        }
        i++;
      }

      if (memcmp(bloques_punteros[nivel_punteros - 1], bufAux_punteros,
                 BLOCKSIZE) == 0) {
        liberar_bloque(inodo->punterosIndirectos[0]);
#if defined(DEBUGN6)
        fprintf(stderr,
                "[liberar_bloques_inodo()→ liberado BF %d de punteros_nivel%d "
                "correspondiente al BL %d]\n",
                inodo->punterosIndirectos[0], nivel_punteros, BLliberado);
#endif
        liberados++;
        inodo->punterosIndirectos[0] = 0;
      } else {
        if (bloque_modificado[nivel_punteros - 1]) {
          if (bwrite(inodo->punterosIndirectos[0],
                     bloques_punteros[nivel_punteros - 1]) < 0)
            return -1;
          contador_bwrites++;
        }
      }
    }
  }

  // 3. Liberar bloques de Indirectos[1]
  if (primerBL < INDIRECTOS1 && !eof) {
    nivel_punteros = 2;
    indices_primerBL[0] = 0;
    indices_primerBL[1] = 0;
    if (inodo->punterosIndirectos[1]) {
      bread(inodo->punterosIndirectos[1], bloques_punteros[nivel_punteros - 1]);
      bloque_modificado[nivel_punteros - 1] = 0;
      contador_breads++;

      if (primerBL >= INDIRECTOS0)
        i = obtener_indice(primerBL, nivel_punteros);
      else
        i = 0;

      indices_primerBL[nivel_punteros - 1] = i;
      while (!eof && i < NPUNTEROS) {
        if (bloques_punteros[nivel_punteros - 1][i]) {
          bread(bloques_punteros[nivel_punteros - 1][i],
                bloques_punteros[nivel_punteros - 2]);
          bloque_modificado[nivel_punteros - 2] = 0;
          contador_breads++;

          if (i == indices_primerBL[nivel_punteros - 1]) {
            j = obtener_indice(primerBL, nivel_punteros - 1);
            indices_primerBL[nivel_punteros - 2] = j;
          } else
            j = 0;

          while (!eof && j < NPUNTEROS) {
            nblog = INDIRECTOS0 + i * NPUNTEROS + j;
            if (nblog == ultimoBL)
              eof = 1;
            if (bloques_punteros[nivel_punteros - 2][j]) {
              liberar_bloque(bloques_punteros[nivel_punteros - 2][j]);
#if defined(DEBUGN6)
              fprintf(stderr,
                      "[liberar_bloques_inodo()→ liberado BF %d de datos para "
                      "BL %d]\n",
                      bloques_punteros[nivel_punteros - 2][j], nblog);
              BLliberado = nblog;
#endif
              liberados++;
              bloques_punteros[nivel_punteros - 2][j] = 0;
              bloque_modificado[nivel_punteros - 2] = 1;
            }
            j++;
          }
          if (memcmp(bloques_punteros[nivel_punteros - 2], bufAux_punteros,
                     BLOCKSIZE) == 0) {
            liberar_bloque(bloques_punteros[nivel_punteros - 1][i]);
#if defined(DEBUGN6)
            fprintf(stderr,
                    "[liberar_bloques_inodo()→ liberado BF %d de "
                    "punteros_nivel%d correspondiente al BL %d]\n",
                    bloques_punteros[nivel_punteros - 1][i], nivel_punteros - 1,
                    BLliberado);
#endif
            liberados++;
            bloques_punteros[nivel_punteros - 1][i] = 0;
            bloque_modificado[nivel_punteros - 1] = 1;
          } else {
            if (bloque_modificado[nivel_punteros - 2]) {
              if (bwrite(bloques_punteros[nivel_punteros - 1][i],
                         bloques_punteros[nivel_punteros - 2]) < 0)
                return -1;
              contador_bwrites++;
            }
          }
        }
        i++;
      }
      if (memcmp(bloques_punteros[nivel_punteros - 1], bufAux_punteros,
                 BLOCKSIZE) == 0) {
        liberar_bloque(inodo->punterosIndirectos[1]);
#if defined(DEBUGN6)
        fprintf(stderr,
                "[liberar_bloques_inodo()→ liberado BF %d de punteros_nivel%d "
                "correspondiente al BL %d]\n",
                inodo->punterosIndirectos[1], nivel_punteros, BLliberado);
#endif
        liberados++;
        inodo->punterosIndirectos[1] = 0;
      } else {
        if (bloque_modificado[nivel_punteros - 1]) {
          if (bwrite(inodo->punterosIndirectos[1],
                     bloques_punteros[nivel_punteros - 1]) < 0)
            return -1;
          contador_bwrites++;
        }
      }
    }
  }

  // 4. Liberar bloques de Indirectos[2]
  if (primerBL < INDIRECTOS2 && !eof) {
    nivel_punteros = 3;
    indices_primerBL[0] = 0;
    indices_primerBL[1] = 0;
    indices_primerBL[2] = 0;
    if (inodo->punterosIndirectos[2]) {
      bread(inodo->punterosIndirectos[2], bloques_punteros[nivel_punteros - 1]);
      bloque_modificado[nivel_punteros - 1] = 0;
      contador_breads++;

      if (primerBL >= INDIRECTOS1)
        i = obtener_indice(primerBL, nivel_punteros);
      else
        i = 0;

      indices_primerBL[nivel_punteros - 1] = i;
      while (!eof && i < NPUNTEROS) {
        if (bloques_punteros[nivel_punteros - 1][i]) {
          bread(bloques_punteros[nivel_punteros - 1][i],
                bloques_punteros[nivel_punteros - 2]);
          contador_breads++;

          if (i == indices_primerBL[nivel_punteros - 1]) {
            j = obtener_indice(primerBL, nivel_punteros - 1);
            indices_primerBL[nivel_punteros - 2] = j;
          } else
            j = 0;

          while (!eof && j < NPUNTEROS) {
            if (bloques_punteros[nivel_punteros - 2][j]) {
              bread(bloques_punteros[nivel_punteros - 2][j],
                    bloques_punteros[nivel_punteros - 3]);
              contador_breads++;

              if (i == indices_primerBL[nivel_punteros - 1] &&
                  j == indices_primerBL[nivel_punteros - 2]) {
                k = obtener_indice(primerBL, nivel_punteros - 2);
                indices_primerBL[nivel_punteros - 3] = k;
              } else
                k = 0;

              while (!eof && k < NPUNTEROS) {
                nblog = INDIRECTOS1 + i * NPUNTEROS2 + j * NPUNTEROS + k;
                if (nblog == ultimoBL)
                  eof = 1;
                if (bloques_punteros[nivel_punteros - 3][k]) {
                  liberar_bloque(bloques_punteros[nivel_punteros - 3][k]);
#if defined(DEBUGN6)
                  fprintf(stderr,
                          "[liberar_bloques_inodo()→ liberado BF %d de datos "
                          "para BL %d]\n",
                          bloques_punteros[nivel_punteros - 3][k], nblog);
                  BLliberado = nblog;
#endif
                  liberados++;
                  bloques_punteros[nivel_punteros - 3][k] = 0;
                  bloque_modificado[nivel_punteros - 3] = 1;
                }
                k++;
              }
              if (memcmp(bloques_punteros[nivel_punteros - 3], bufAux_punteros,
                         BLOCKSIZE) == 0) {
                liberar_bloque(bloques_punteros[nivel_punteros - 2][j]);
#if defined(DEBUGN6)
                fprintf(stderr,
                        "[liberar_bloques_inodo()→ liberado BF %d de "
                        "punteros_nivel%d correspondiente al BL %d]\n",
                        bloques_punteros[nivel_punteros - 2][j],
                        nivel_punteros - 2, BLliberado);
#endif
                liberados++;
                bloques_punteros[nivel_punteros - 2][j] = 0;
                bloque_modificado[nivel_punteros - 2] = 1;
              } else {
                if (bloque_modificado[nivel_punteros - 3]) {
                  if (bwrite(bloques_punteros[nivel_punteros - 2][j],
                             bloques_punteros[nivel_punteros - 3]) < 0)
                    return -1;
                  contador_bwrites++;
                }
              }
            }
            j++;
          }
          if (memcmp(bloques_punteros[nivel_punteros - 2], bufAux_punteros,
                     BLOCKSIZE) == 0) {
            liberar_bloque(bloques_punteros[nivel_punteros - 1][i]);
#if defined(DEBUGN6)
            fprintf(stderr,
                    "[liberar_bloques_inodo()→ liberado BF %d de "
                    "punteros_nivel%d correspondiente al BL %d]\n",
                    bloques_punteros[nivel_punteros - 1][i], nivel_punteros - 1,
                    BLliberado);
#endif
            liberados++;
            bloques_punteros[nivel_punteros - 1][i] = 0;
            bloque_modificado[nivel_punteros - 1] = 1;
          } else {
            if (bloque_modificado[nivel_punteros - 2]) {
              if (bwrite(bloques_punteros[nivel_punteros - 1][i],
                         bloques_punteros[nivel_punteros - 2]) < 0)
                return -1;
              contador_bwrites++;
            }
          }
        }
        i++;
      }
      if (memcmp(bloques_punteros[nivel_punteros - 1], bufAux_punteros,
                 BLOCKSIZE) == 0) {
        liberar_bloque(inodo->punterosIndirectos[2]);
#if defined(DEBUGN6)
        fprintf(stderr,
                "[liberar_bloques_inodo()→ liberado BF %d de punteros_nivel%d "
                "correspondiente al BL %d]\n",
                inodo->punterosIndirectos[2], nivel_punteros, BLliberado);
#endif
        liberados++;
        inodo->punterosIndirectos[2] = 0;
      } else {
        if (bloque_modificado[nivel_punteros - 1]) {
          if (bwrite(inodo->punterosIndirectos[2],
                     bloques_punteros[nivel_punteros - 1]) < 0)
            return -1;
          contador_bwrites++;
        }
      }
    }
  }

#if defined(DEBUGN6)
  fprintf(stderr,
          "[liberar_bloques_inodo()→ total bloques liberados: %d, "
          "total_breads: %d, total_bwrites:%d]\n",
          liberados, contador_breads, contador_bwrites);
#endif

  return liberados;
}
