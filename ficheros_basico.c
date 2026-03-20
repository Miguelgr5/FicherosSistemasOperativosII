#include "ficheros_basico.h"
#include <stdio.h>
#define DEBUGN6
// #define DEBUGSALTOS
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
#if defined(DEBUGN4)
      fprintf(stderr,
              "[traducir_bloque_inodo()→ inodo.punterosDirectos[%u] = %u "
              "(reservado BF %u para BL %u)]\n",
              nblogico, ptr, ptr, nblogico);
#endif
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
#if defined(DEBUGN4)
          printf("[traducir_bloque_inodo()→ inodo.punterosIndirectos[%u] = %u "
                 "(reservado BF %u para punteros_nivel%u)]\n",
                 nRangoBL - 1, ptr, ptr, nivel_punteros);
#endif
        } else { // Cuelga de otro bloque de punteros
          buffer[indice] = ptr;
          if (bwrite(ptr_ant, buffer) == FALLO)
            return FALLO;
#if defined(DEBUGN4)
          printf("[traducir_bloque_inodo()→ punteros_nivel%u [%u] = %u "
                 "(reservado BF %u para punteros_nivel%u)]\n",
                 nivel_punteros + 1, indice, ptr, ptr, nivel_punteros);
#endif
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
      fprintf(stderr,
              "[traducir_bloque_inodo()→ punteros_nivel1 [%u] = %u (reservado "
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
#if defined(DEBUGN6)
  fprintf(stderr,
          "[liberar_inodo()→ Tras liberar inodo: primerInodoLibre=%u]\n",
          sb.posPrimerInodoLibre);
#endif
  return ninodo;
}
int total_breads = 0;
int total_bwrites = 0;

int liberar_bloques_inodo(unsigned int primerBL, struct inodo *inodo) {
  unsigned int nBL = primerBL;
  unsigned int ultimoBL;
  int liberados = 0;
  int eof = 0;
  int nRangoBL = 0;
  unsigned int ptr_val = 0;
  total_breads = 0; // Reset contadores
  total_bwrites = 0;

  if (inodo->tamEnBytesLog == 0)
    return 0;

  if (inodo->tamEnBytesLog % BLOCKSIZE == 0) {
    ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE - 1;
  } else {
    ultimoBL = inodo->tamEnBytesLog / BLOCKSIZE;
  }

#if defined(DEBUGN6)
  fprintf(stderr, "[liberar_bloques_inodo()→ primer BL: %u, último BL: %u]\n",
          primerBL, ultimoBL);
#endif

  nRangoBL = obtener_nRangoBL(inodo, nBL, &ptr_val);
  if (nRangoBL == 0) {
    liberados += liberar_directos(&nBL, ultimoBL, inodo, &eof);
  }

  while (!eof) {
    nRangoBL = obtener_nRangoBL(inodo, nBL, &ptr_val);
    liberados += liberar_indirectos_recursivo(
        &nBL, primerBL, ultimoBL, &ptr_val, nRangoBL, nRangoBL, &eof);
  }

#if defined(DEBUGN6)
  fprintf(stderr,
          "[liberar_bloques_inodo()→ total bloques liberados: %d, "
          "total_breads: %d, total_bwrites: %d]\n",
          liberados, total_breads, total_bwrites);
#endif

  return liberados;
}

int liberar_directos(unsigned int *nBL, unsigned int ultimoBL,
                     struct inodo *inodo, int *eof) {
  int liberados = 0;
  while (*nBL < DIRECTOS && !(*eof)) {
    if (inodo->punterosDirectos[*nBL] != 0) {
      liberar_bloque(inodo->punterosDirectos[*nBL]);
#if defined(DEBUGN6)
      fprintf(stderr,
              "[liberar_bloques_inodo()→ liberado BF %u de datos para BL %u]\n",
              inodo->punterosDirectos[*nBL], *nBL);
#endif
      inodo->punterosDirectos[*nBL] = 0;
      liberados++;
    }
    (*nBL)++;
    if (*nBL > ultimoBL)
      *eof = 1;
  }
  return liberados;
}

int liberar_indirectos_recursivo(unsigned int *nBL, unsigned int primerBL,
                                 unsigned int ultimoBL, unsigned int *ptr,
                                 int nRangoBL, int nivel_punteros, int *eof) {
  int liberados = 0;
  int modificado = 0;
  unsigned int bloquePunteros[NPUNTEROS];
  unsigned int bufferCeros[NPUNTEROS];
  memset(bufferCeros, 0, BLOCKSIZE);

  if (*ptr == 0) {
#if defined(DEBUGNSALTOS)
    unsigned int BL_antes = *nBL;
#endif
    unsigned int salto = 1;
    if (nivel_punteros == 2)
      salto = NPUNTEROS;
    else if (nivel_punteros == 3)
      salto = NPUNTEROS * NPUNTEROS;

    if (nivel_punteros == nRangoBL) {
      if (nRangoBL == 1)
        *nBL = INDIRECTOS0 + NPUNTEROS;
      else if (nRangoBL == 2)
        *nBL = INDIRECTOS1 + (NPUNTEROS * NPUNTEROS);
      else if (nRangoBL == 3)
        *nBL = INDIRECTOS2 + (NPUNTEROS * NPUNTEROS * NPUNTEROS);
    } else {
      *nBL += salto;
    }

#if defined(DEBUGNSALTOS)
    fprintf(stderr, "[liberar_bloques_inodo()→ Saltamos del BL %u al BL %u]\n",
            BL_antes, *nBL - 1);
#endif
    if (*nBL > ultimoBL)
      *eof = 1;
    return 0;
  }

  if (bread(*ptr, bloquePunteros) == FALLO)
    return FALLO;
  total_breads++;

  int indice_inicial = obtener_indice(*nBL, nivel_punteros);

  for (int i = indice_inicial; i < NPUNTEROS && !(*eof); i++) {
    if (bloquePunteros[i] != 0) {
      if (nivel_punteros == 1) {
        liberar_bloque(bloquePunteros[i]);
#if defined(DEBUGN6)
        fprintf(
            stderr,
            "[liberar_bloques_inodo()→ liberado BF %u de datos para BL %u]\n",
            bloquePunteros[i], *nBL);
#endif
        bloquePunteros[i] = 0;
        modificado = 1;
        liberados++;
        (*nBL)++;
      } else {
        unsigned int ptr_antes = bloquePunteros[i];
        liberados += liberar_indirectos_recursivo(nBL, primerBL, ultimoBL,
                                                  &bloquePunteros[i], nRangoBL,
                                                  nivel_punteros - 1, eof);
        if (bloquePunteros[i] != ptr_antes)
          modificado = 1;
      }
    } else {

#if defined(DEBUGNSALTOS)
      unsigned int BL_antes = *nBL;
#endif
      switch (nivel_punteros) {
      case 1:
        (*nBL)++;
        break;
      case 2:
        (*nBL) += NPUNTEROS;
        break;
      case 3:
        (*nBL) += (NPUNTEROS * NPUNTEROS);
        break;
      }
#if defined(DEBUGNSALTOS)
      fprintf(stderr,
              "[liberar_bloques_inodo()→ Saltamos del BL %u al BL %u]\n",
              BL_antes, *nBL - 1);
#endif
    }
    if (*nBL > ultimoBL)
      *eof = 1;
  }

  if (memcmp(bloquePunteros, bufferCeros, BLOCKSIZE) == 0) {
    liberar_bloque(*ptr);
#if defined(DEBUGN6)
    fprintf(stderr,
            "[liberar_bloques_inodo()→ liberado BF %u de punteros_nivel%d "
            "correspondiente al BL %u]\n",
            *ptr, nivel_punteros, *nBL - 1);
#endif
    *ptr = 0;
    liberados++;
  } else if (modificado) {
    if (bwrite(*ptr, bloquePunteros) == FALLO)
      return FALLO;
    total_bwrites++;
#if defined(DEBUGN6)
    fprintf(stderr,
            "[liberar_bloques_inodo()→ salvado BF %u de punteros_nivel%d "
            "correspondiente al BL %u]\n",
            *ptr, nivel_punteros, *nBL - 1);
#endif
  }

  return liberados;
}
