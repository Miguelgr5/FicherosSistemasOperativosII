#include "ficheros_basico.h"
#include <string.h>
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
char leer_bit(unsigned int nbloque) {}
