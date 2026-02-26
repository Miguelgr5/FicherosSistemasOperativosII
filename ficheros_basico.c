#include "ficheros_basico.h"
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
  if (bread(nbloqueabs, bufferMB))
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
  if (bread(nbloqueabs, bufferMB))
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
  if (escribir_bit(nbloque, 0))
    return FALLO;
  struct superbloque sb;
  if (bread(posSB, &sb) == -1)
    return FALLO;
  sb.cantBloquesLibres++;
  if (bwrite(posSB, &sb))
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
