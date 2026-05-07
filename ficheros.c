#include "ficheros.h"
int mi_write_f(unsigned int ninodo, const void *buf_original,
               unsigned int offset, unsigned int nbytes) {
  struct inodo inodo;
  unsigned char buf_bloque[BLOCKSIZE];
  int nbfisico;
  int escritos = 0;

  // 1. Leer inodo y comprobar permisos de escritura
  mi_waitSem();
  if (leer_inodo(ninodo, &inodo) == FALLO) {
    mi_signalSem();
    return FALLO;
  }
  mi_signalSem();
  if ((inodo.permisos & 2) != 2) {
    fprintf(stderr, RED "No hay permisos de escritura\n" RESET);
    return FALLO;
  }

  // 2. Cálculos de bloques lógicos y desplazamientos
  unsigned int primerBL = offset / BLOCKSIZE;
  unsigned int ultimoBL = (offset + nbytes - 1) / BLOCKSIZE;
  unsigned int desp1 = offset % BLOCKSIZE;
  unsigned int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

  // 3. Escritura bloque a bloque
  // CASO A: La escritura cabe en un solo bloque lógico
  if (primerBL == ultimoBL) {
    mi_waitSem();
    nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
    mi_signalSem();
    if (nbfisico == FALLO) {
      return FALLO;
    }
    if (bread(nbfisico, buf_bloque) == FALLO) {
      return FALLO;
    }
    memcpy(buf_bloque + desp1, buf_original, nbytes);
    if (bwrite(nbfisico, buf_bloque) == FALLO) {
      return FALLO;
    }
    escritos = nbytes;
  }
  // CASO B: La escritura afecta a varios bloques
  else {
    // 3.1. Fase 1: Primer bloque lógico
    mi_waitSem();
    nbfisico = traducir_bloque_inodo(ninodo, primerBL, 1);
    mi_signalSem();
    if (nbfisico != FALLO) {
      if (bread(nbfisico, buf_bloque) == FALLO) {
        return FALLO;
      }
      memcpy(buf_bloque + desp1, buf_original, BLOCKSIZE - desp1);
      if (bwrite(nbfisico, buf_bloque) == FALLO) {
        return FALLO;
      }
      escritos += (BLOCKSIZE - desp1);
    }

    // 3.2. Fase 2: Bloques lógicos intermedios
    for (unsigned int bl = primerBL + 1; bl < ultimoBL; bl++) {
      mi_waitSem();
      nbfisico = traducir_bloque_inodo(ninodo, bl, 1);
      mi_signalSem();
      if (nbfisico != FALLO) {
        // Escribimos directamente desde el buffer original sin bread previo
        if (bwrite(nbfisico, buf_original + (BLOCKSIZE - desp1) +
                                 (bl - primerBL - 1) * BLOCKSIZE) == FALLO) {
          return escritos; // Devolvemos lo que se haya podido escribir
        }
        escritos += BLOCKSIZE;
      }
    }

    // 3.3. Fase 3: Último bloque lógico
    mi_waitSem();
    nbfisico = traducir_bloque_inodo(ninodo, ultimoBL, 1);
    mi_signalSem();
    if (nbfisico != FALLO) {
      if (bread(nbfisico, buf_bloque) == FALLO) {
        return FALLO;
      }
      // El origen es: buf_original + (bytes ya escritos)
      memcpy(buf_bloque, buf_original + (nbytes - (desp2 + 1)), desp2 + 1);
      if (bwrite(nbfisico, buf_bloque) == FALLO) {
        return FALLO;
      }
      escritos += (desp2 + 1);
    }
  }

  // 4. Actualizar metainformación del inodo
  // Volvemos a leerlo por si traducir_bloque_inodo cambió nbloques
  mi_waitSem();
  if (leer_inodo(ninodo, &inodo) == FALLO) {
    mi_signalSem();
    return FALLO;
  }
  // Solo actualizamos el tamaño si hemos escrito más allá del EOF
  if (offset + nbytes > inodo.tamEnBytesLog) {
    inodo.tamEnBytesLog = offset + nbytes;
  }

  inodo.mtime = time(NULL);
  inodo.ctime = time(NULL);

  if (escribir_inodo(ninodo, &inodo) == FALLO) {
    mi_signalSem();
    return FALLO;
  }
  mi_signalSem();
  return escritos;
}
int mi_read_f(unsigned int ninodo, void *buf_original, unsigned int offset,
              unsigned int nbytes) {
  mi_waitSem();
  struct inodo inodo;
  int leidos = 0;

  // 1. Leer el inodo del dispositivo
  if (leer_inodo(ninodo, &inodo) == FALLO) {
    mi_signalSem();
    return FALLO;
  }

  // 2. Control de permisos (debe tener el bit 'r' activo, que es el 4: 100 en
  // binario)
  if ((inodo.permisos & 4) != 4) {
    fprintf(stderr, RED "Error: No hay permisos de lectura\n" RESET);
    mi_signalSem();
    return FALLO;
  }

  // 3. Control de EOF (End of File)
  if (offset >= inodo.tamEnBytesLog) {
    mi_signalSem();
    return 0; // No hay nada más que leer
  }
  if (offset + nbytes >= inodo.tamEnBytesLog) {
    nbytes = inodo.tamEnBytesLog -
             offset; // Ajustamos para no leer más allá del tamaño real
  }

  // 4. Preparación de variables para el bucle de lectura
  unsigned int primer_BL = offset / BLOCKSIZE;
  unsigned int ultimo_BL = (offset + nbytes - 1) / BLOCKSIZE;
  unsigned int desp1 = offset % BLOCKSIZE;
  unsigned int desp2 = (offset + nbytes - 1) % BLOCKSIZE;

  unsigned char buf_bloque[BLOCKSIZE];
  int nbfisico;

  // 5. Bucle de lectura bloque a bloque
  // Caso A: La lectura cabe en un solo bloque
  if (primer_BL == ultimo_BL) {
    nbfisico = traducir_bloque_inodo(ninodo, primer_BL, 0); // reservar = 0
    if (nbfisico != FALLO) {
      if (bread(nbfisico, buf_bloque) == FALLO) {
        mi_signalSem();
        return FALLO;
      }
      memcpy(buf_original, buf_bloque + desp1, nbytes);
    }
    leidos = nbytes;
  } else {
    // Caso B: La lectura abarca varios bloques
    // 5.1. Primer bloque
    nbfisico = traducir_bloque_inodo(ninodo, primer_BL, 0);
    if (nbfisico != FALLO) {
      if (bread(nbfisico, buf_bloque) == FALLO) {
        mi_signalSem();
        return FALLO;
      }
      memcpy(buf_original, buf_bloque + desp1, BLOCKSIZE - desp1);
    }
    leidos = BLOCKSIZE - desp1;

    // 5.2. Bloques intermedios
    for (int bl = primer_BL + 1; bl < ultimo_BL; bl++) {
      nbfisico = traducir_bloque_inodo(ninodo, bl, 0);
      if (nbfisico != FALLO) {
        if (bread(nbfisico, buf_bloque) == FALLO) {
          mi_signalSem();
          return FALLO;
        }
        memcpy(buf_original + leidos, buf_bloque, BLOCKSIZE);
      }
      leidos += BLOCKSIZE;
    }

    // 5.3. Último bloque
    nbfisico = traducir_bloque_inodo(ninodo, ultimo_BL, 0);
    if (nbfisico != FALLO) {
      if (bread(nbfisico, buf_bloque) == FALLO) {
        mi_signalSem();
        return FALLO;
      }
      memcpy(buf_original + leidos, buf_bloque, desp2 + 1);
    }
    leidos += (desp2 + 1);
  }

  // 6. Actualizar atime y guardar inodo
  inodo.atime = time(NULL);
  if (escribir_inodo(ninodo, &inodo) == FALLO) {
    mi_signalSem();
    return FALLO;
  }
  mi_signalSem();
  return leidos;
}
int mi_stat_f(unsigned int ninodo, struct STAT *p_stat) {
  struct inodo inodo;

  // 1. Leer el inodo del dispositivo
  if (leer_inodo(ninodo, &inodo) == FALLO) {
    return FALLO;
  }

  // 2. Copiar metadatos (usamos el operador -> por ser p_stat un puntero)
  p_stat->tipo = inodo.tipo;
  p_stat->permisos = inodo.permisos;

  // Los campos de tiempo
  p_stat->atime = inodo.atime;
  p_stat->mtime = inodo.mtime;
  p_stat->ctime = inodo.ctime;

  p_stat->btime = inodo.btime;

  p_stat->nlinks = inodo.nlinks;
  p_stat->tamEnBytesLog = inodo.tamEnBytesLog;

  // Importante: numBloquesOcupados corresponde a nbloques del inodo
  p_stat->numBloquesOcupados = inodo.numBloquesOcupados;

  return EXITO;
}
int mi_chmod_f(unsigned int ninodo, unsigned char permisos) {
  mi_waitSem();
  struct inodo inodo;

  // 1. Leer el inodo del dispositivo
  if (leer_inodo(ninodo, &inodo) == FALLO) {
    mi_signalSem();
    return FALLO;
  }

  // 2. Actualizar los permisos con el nuevo valor
  inodo.permisos = permisos;

  // 3. Actualizar el timestamp de cambio de metadatos (ctime)
  inodo.ctime = time(NULL);

  // 4. Escribir el inodo actualizado de vuelta al dispositivo
  if (escribir_inodo(ninodo, &inodo) == FALLO) {
    mi_signalSem();
    return FALLO;
  }
  mi_signalSem();
  return EXITO;
}
int mi_truncar_f(unsigned int ninodo, unsigned int nbytes) {
  struct inodo inodo;
  unsigned int primerBL;

  // 1. Leer el inodo correspondiente
  if (leer_inodo(ninodo, &inodo) == FALLO)
    return FALLO;

  // 2. Comprobar que tiene permisos de escritura (asumiendo formato estándar:
  // 4-read, 2-write, 1-execute)
  if ((inodo.permisos & 2) != 2) {
    fprintf(stderr, "Error: el inodo no tiene permisos de escritura.\n");
    return FALLO;
  }

  // 3. No se puede truncar más allá del tamaño actual (EOF)
  if (nbytes > inodo.tamEnBytesLog) {
    fprintf(stderr, "Error: no se puede truncar más allá del EOF.\n");
    return FALLO;
  }

  // 4. Calcular el primer bloque lógico que vamos a liberar
  if (nbytes % BLOCKSIZE == 0) {
    primerBL = nbytes / BLOCKSIZE;
  } else {
    primerBL = nbytes / BLOCKSIZE + 1;
  }

  // 5. Liberar los bloques a partir de primerBL
  int liberados = liberar_bloques_inodo(primerBL, &inodo);
  if (liberados == FALLO)
    return FALLO;

  // 6. Actualizar las fechas y restar el tamaño del fichero
  inodo.mtime = time(NULL);
  inodo.ctime = time(NULL);
  inodo.tamEnBytesLog = nbytes;
  inodo.numBloquesOcupados -= liberados;

  // 7. Guardar el inodo
  if (escribir_inodo(ninodo, &inodo) == FALLO)
    return FALLO;

  return liberados;
}
