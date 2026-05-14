#include "directorios.h"
// #define DEBUGN7
// #define DEBUGN9
// #define DEBUGCACHE
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
  // Verificación de seguridad básica
  if (camino == NULL || camino[0] != '/') {
    return FALLO;
  }

  // Buscamos la segunda aparición de '/' empezando desde la posición 1
  const char *segunda_barra = strchr(camino + 1, '/');

  if (segunda_barra != NULL) {
    // --- CASO DIRECTORIO ---
    // Ejemplo: "/dir1/fichero" o "/dir1/"

    // Calculamos cuántos caracteres hay entre la primera y segunda barra
    int longitud = segunda_barra - (camino + 1);

    strncpy(inicial, camino + 1, longitud);
    inicial[longitud] =
        '\0'; // Importante: strncpy no añade el nulo si llega al límite

    strcpy(final, segunda_barra);
    *tipo = 'd';
  } else {
    // --- CASO FICHERO (o último nivel) ---
    // Ejemplo: "/fichero"

    strcpy(inicial, camino + 1);
    strcpy(final, ""); // Cadena vacía
    *tipo = 'f';
  }

  return EXITO;
}
int buscar_entrada(const char *camino_directorio, unsigned int *p_inodo_dir,
                   unsigned int *p_inodo, unsigned int *p_entrada,
                   char reservar, unsigned char permisos) {
  struct inodo inodo_dir;
  struct entrada
      entradas[BLOCKSIZE /
               sizeof(struct entrada)]; // Buffer para un bloque de entradas
  char inicial[TAMNOMBRE];
  char final[strlen(camino_directorio) + 1];
  char tipo;
  int num_entradas, n_entrada;
  int error;

  memset(inicial, 0, TAMNOMBRE);
  memset(final, 0, strlen(camino_directorio) + 1);

  if (strcmp(camino_directorio, "/") == 0) {
    *p_inodo = 0; // Inodo raíz
    *p_entrada = 0;
    return EXITO;
  }

  if (extraer_camino(camino_directorio, inicial, final, &tipo) == FALLO) {
    error = ERROR_CAMINO_INCORRECTO;
    return error;
  }

#if defined(DEBUGN7)
  fprintf(stderr, "[buscar_entrada()→ inicial: %s, final: %s, reservar: %d]\n",
          inicial, final, reservar);
#endif

  if (leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO)
    return FALLO;
  if ((inodo_dir.permisos & 4) == 0) {
    error = ERROR_PERMISO_LECTURA;
    return error;
  }

  num_entradas = inodo_dir.tamEnBytesLog / sizeof(struct entrada);
  n_entrada = 0;

  // Lectura por bloques
  if (num_entradas > 0) {
    while (n_entrada < num_entradas) {
      // Si n_entrada es múltiplo de la cantidad de entradas que caben en un
      // bloque, leemos el bloque
      if (n_entrada % (BLOCKSIZE / sizeof(struct entrada)) == 0) {
        if (mi_read_f(*p_inodo_dir, entradas,
                      n_entrada * sizeof(struct entrada), BLOCKSIZE) < 0) {
          return FALLO;
        }
      }

      // Accedemos a la entrada correspondiente dentro del buffer usando el
      // operador %
      int indice_en_buffer = n_entrada % (BLOCKSIZE / sizeof(struct entrada));

      if (strcmp(inicial, entradas[indice_en_buffer].nombre) == 0) {
        // ¡Encontrado! Salimos del bucle
        break;
      }
      n_entrada++;
    }
  }

  if (n_entrada == num_entradas) { // No existe la entrada
    switch (reservar) {
    case 0:
      error = ERROR_NO_EXISTE_ENTRADA_CONSULTA;
      return error;
    case 1:
      if (inodo_dir.tipo != 'd') {
        error = ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
        return error;
      }
      if ((inodo_dir.permisos & 2) == 0) {
        error = ERROR_PERMISO_ESCRITURA;
        return error;
      }

      struct entrada nueva_entrada;
      strcpy(nueva_entrada.nombre, inicial);
      if (tipo == 'd') {
        if (strcmp(final, "/") == 0) {
          nueva_entrada.ninodo = reservar_inodo('d', permisos);
#if defined(DEBUGN7)
          fprintf(stderr,
                  "[buscar_entrada()→ reservado inodo %u tipo d con permisos "
                  "%u para %s]\n",
                  nueva_entrada.ninodo, permisos, inicial);
#endif
        } else {
          error = ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
          return error;
        }
      } else {
        nueva_entrada.ninodo = reservar_inodo('f', permisos);
#if defined(DEBUGN7)
        fprintf(stderr,
                "[buscar_entrada()→ reservado inodo %u tipo f con permisos %u "
                "para %s]\n",
                nueva_entrada.ninodo, permisos, inicial);
#endif
      }

      if (mi_write_f(*p_inodo_dir, &nueva_entrada,
                     n_entrada * sizeof(struct entrada),
                     sizeof(struct entrada)) < 0) {
        return FALLO;
      }
#if defined(DEBUGN7)
      fprintf(stderr, "[buscar_entrada()→ creada entrada: %s, %u]\n",
              nueva_entrada.nombre, nueva_entrada.ninodo);
#endif
      // Para que la parte final de la función funcione con la entrada recién
      // creada
      *p_inodo = nueva_entrada.ninodo;
      *p_entrada = n_entrada;
      return EXITO; // Salida directa tras crear
    }
  }

  // Si hemos terminado la ruta
  if (strcmp(final, "") == 0) {
    if (n_entrada < num_entradas && reservar == 1) {
      error = ERROR_ENTRADA_YA_EXISTENTE;
      return error;
    }
    // Obtenemos el inodo de la entrada encontrada en el buffer
    *p_inodo =
        entradas[n_entrada % (BLOCKSIZE / sizeof(struct entrada))].ninodo;
    *p_entrada = n_entrada;
    return EXITO;
  } else {
    *p_inodo_dir =
        entradas[n_entrada % (BLOCKSIZE / sizeof(struct entrada))].ninodo;
    return buscar_entrada(final, p_inodo_dir, p_inodo, p_entrada, reservar,
                          permisos);
  }
}
void mostrar_error_buscar_entrada(int error) {
  fprintf(stderr, RED);
  switch (error) {
  case ERROR_CAMINO_INCORRECTO:
    fprintf(stderr, "Error: Camino incorrecto.\n");
    break;
  case ERROR_PERMISO_LECTURA:
    fprintf(stderr, "Error: Permiso denegado de lectura.\n");
    break;
  case ERROR_NO_EXISTE_ENTRADA_CONSULTA:
    fprintf(stderr, "Error: No existe el archivo o el directorio.\n");
    break;
  case ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO:
    fprintf(stderr, "Error: No existe algún directorio intermedio.\n");
    break;
  case ERROR_PERMISO_ESCRITURA:
    fprintf(stderr, "Error: Permiso denegado de escritura.\n");
    break;
  case ERROR_ENTRADA_YA_EXISTENTE:
    fprintf(stderr, "Error: El archivo ya existe.\n");
    break;
  case ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO:
    fprintf(stderr, "Error: No es un directorio.\n");
    break;
  }
  fprintf(stderr, RESET);
}
int mi_creat(const char *camino, unsigned char permisos) {
  mi_waitSem();
  unsigned int p_inodo_dir = 0; // Empezamos la búsqueda desde el inodo raíz
  unsigned int p_inodo = 0; // Aquí nos devolverá el inodo del archivo creado
  unsigned int p_entrada =
      0; // Aquí nos devolverá el nº de entrada en el directorio padre

  // Llamamos a buscar_entrada
  // Pasar &p_inodo_dir permite que la función actualice por dónde va pasando
  int error =
      buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);

  if (error < 0) {
    mi_signalSem();
    // Si hay error (permisos, camino inexistente, etc), lo propagamos
    return error;
  }
  mi_signalSem();
  return 0; // Éxito
}
int mi_dir(const char *camino, char *buffer, char tipo, char flag) {
  unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
  int error;

  error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
  if (error < 0) {
    return error;
  }
  struct inodo inodo;
  if (leer_inodo(p_inodo, &inodo) == -1) {
    return -1;
  }
  if (inodo.tipo != tipo) {
    fprintf(stderr, "Error: la sintaxis no concuerda con el tipo.\n");
    return -1;
  }

  int num_entradas_mostradas = 0;
  char tmp[512];
  struct entrada entradas[BLOCKSIZE / sizeof(struct entrada)];
  memset(buffer, 0, 100 * 256);

  // CASO A: Es un FICHERO
  if (inodo.tipo == 'f') {
    if (flag == 'l') {
      formatear_linea_ls(tmp, &inodo, camino_o_nombre(camino));
    } else {
      // Formato simple: solo nombre con color
      sprintf(tmp, "%s%s%s\t", CYAN, camino_o_nombre(camino), RESET);
    }
    strcat(buffer, tmp);
    num_entradas_mostradas = 1;
  }
  // CASO B: Es un DIRECTORIO
  else {
    int offset = 0;
    int num_entradas_total = inodo.tamEnBytesLog / sizeof(struct entrada);
    int entradas_leidas = 0;

    while (entradas_leidas < num_entradas_total) {
      int leidos = mi_read_f(p_inodo, entradas, offset, BLOCKSIZE);
      if (leidos <= 0)
        break;

      int entradas_en_este_bloque = leidos / sizeof(struct entrada);

      for (int i = 0;
           i < entradas_en_este_bloque && entradas_leidas < num_entradas_total;
           i++) {
        if (entradas[i].ninodo != (unsigned int)-1) {
          memset(tmp, 0, sizeof(tmp));

          if (flag == 'l') { // FORMATO EXTENDIDO
            struct inodo inodo_hijo;
            leer_inodo(entradas[i].ninodo, &inodo_hijo);
            // El color se pone DENTRO de formatear_linea_ls
            formatear_linea_ls(tmp, &inodo_hijo, entradas[i].nombre);
          } else { // FORMATO SIMPLE
            // Necesitamos saber el tipo para el color
            struct inodo inodo_hijo;
            leer_inodo(entradas[i].ninodo, &inodo_hijo);
            char *color = (inodo_hijo.tipo == 'd') ? ORANGE : CYAN;
            sprintf(tmp, "%s%s%s\t", color, entradas[i].nombre, RESET);
          }

          strcat(buffer, tmp);
          num_entradas_mostradas++;
        }
        entradas_leidas++;
      }
      offset += BLOCKSIZE;
    }
  }
  return num_entradas_mostradas;
}
void formatear_linea_ls(char *linea, struct inodo *in, const char *nombre) {
  char tiempo[80];
  struct tm *tm;
  char p[4];

  // Colores
  char *color = (in->tipo == 'd') ? ORANGE : CYAN;

  // Permisos
  p[0] = (in->permisos & 4) ? 'r' : '-';
  p[1] = (in->permisos & 2) ? 'w' : '-';
  p[2] = (in->permisos & 1) ? 'x' : '-';
  p[3] = '\0';

  // Tiempo
  tm = localtime(&in->mtime);
  sprintf(tiempo, "%d-%02d-%02d %02d:%02d:%02d", tm->tm_year + 1900,
          tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

  // Línea final: solo el nombre lleva el color y el reset
  sprintf(linea, "%c\t%s\t%s\t%u\t%s%s%s\n", in->tipo, p, tiempo,
          in->tamEnBytesLog, color, nombre, RESET);
}
const char *camino_o_nombre(const char *camino) {
  // Buscamos la última barra
  const char *ultimo_slash = strrchr(camino, '/');

  if (!ultimo_slash) {
    return camino; // Si no hay barras, el nombre es el camino entero
  }

  // Si el camino termina en '/', como "/dir/", hay que buscar la barra anterior
  if (*(ultimo_slash + 1) == '\0') {
    // Un truco es copiar el camino, quitar la última barra y volver a buscar
    // O simplemente recorrer hacia atrás manualmente
    return "directorio"; // (Simplificación, mejor manejar el caso real)
  }

  return ultimo_slash + 1; // Devolvemos lo que hay después de la barra
}
int mi_chmod(const char *camino, unsigned char permisos) {
  unsigned int p_inodo_dir = 0;
  unsigned int p_inodo = 0;
  unsigned int p_entrada = 0;

  // 1. Buscamos el inodo asociado al camino (reservar = 0)
  int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
  if (error < 0) {
    return error; // Devolvemos el error de búsqueda (no existe, etc.)
  }

  // 2. Llamamos a la capa de ficheros para cambiar los permisos
  // mi_chmod_f es la función que realmente lee el inodo, cambia el campo y lo
  // escribe
  int resultado = mi_chmod_f(p_inodo, permisos);
  return resultado;
}
int mi_stat(const char *camino, struct STAT *p_stat) {
  unsigned int p_inodo_dir =
      0;                    // Inodo del directorio raíz para empezar a buscar
  unsigned int p_inodo = 0; // Aquí buscar_entrada nos devolverá el nº de inodo
  unsigned int p_entrada =
      0; // Variable necesaria para la firma de buscar_entrada

  // 1. Buscamos la entrada asociada al camino (reservar = 0 porque el archivo
  // debe existir)
  int error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);

  if (error < 0) {
    return error; // Devolvemos el código de error (ej: el archivo no existe)
  }

  // 2. Si la entrada existe, llamamos a la capa de ficheros pasándole el
  // p_inodo Le pasamos p_stat que ya es un puntero a la estructura que recibirá
  // los datos
  if (mi_stat_f(p_inodo, p_stat) == -1) {
    return -1;
  }

  // Retornamos el número de inodo por si el comando mi_stat.c quiere imprimirlo
  return p_inodo;
}

int mi_touch(const char *camino, unsigned char permisos) {
  unsigned int p_inodo_dir = 0;
  unsigned int p_inodo = 0;
  unsigned int p_entrada = 0;

  // Llamamos a buscar_entrada con reservar = 1
  // IMPORTANTE: buscar_entrada detectará que el camino NO termina en '/'
  // y por lo tanto creará un inodo de tipo 'f'.
  int error =
      buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);

  if (error < 0) {
    return error; // Errores: ya existe, padre sin permisos, etc.
  }

  return 0;
}
#define USARCACHE 3 // 3: tabla LRU 2:FIFO
#define CACHE_SIZE 3
#if (USARCACHE == 2 || USARCACHE == 3)
static struct UltimaEntrada UltimasEntradas[CACHE_SIZE];
static int inicializada = 0; // Para limpiar la caché la primera vez
#if USARCACHE == 2
static int siguiente_fifo = 0;
#endif
#endif

void inicializar_cache() {
  if (!inicializada) {
    for (int i = 0; i < CACHE_SIZE; i++) {
      memset(UltimasEntradas[i].camino, 0, sizeof(UltimasEntradas[i].camino));
      UltimasEntradas[i].p_inodo = -1;
    }
    inicializada = 1;
  }
}

int mi_write(const char *camino, const void *buf, unsigned int offset,
             unsigned int nbytes) {
  unsigned int p_inodo_dir = 0;
  unsigned int p_inodo_fichero = 0;
  unsigned int p_entrada = 0;
  int error;
  int indice_cache = -1;
  // 1. INICIALIZAR CACHE
  inicializar_cache();
  //  2. BUSCAR EN CACHÉ (Estrategia LRU)
  for (int i = 0; i < CACHE_SIZE; i++) {
    if (strcmp(UltimasEntradas[i].camino, camino) == 0) {
      indice_cache = i;
      p_inodo_fichero = UltimasEntradas[i].p_inodo;
      break;
    }
  }

  if (indice_cache != -1) {
#if USARCACHE == 3
    // HIT: Actualizamos el sello de tiempo para LRU
    gettimeofday(&UltimasEntradas[indice_cache].ultima_consulta, NULL);
#endif

#if defined(DEBUGCACHE)
    fprintf(stderr, "[mi_write() → Utilizamos cache[%d]: %s]\n", indice_cache,
            camino);
#endif

  } else {
    // MISS: No está en caché, hay que buscarlo en el disco
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo_fichero,
                                &p_entrada, 0, 0)) < 0) {
      return error; // Error gestionado por buscar_entrada
    }

    // 3. ACTUALIZAR CACHÉ (Reemplazo LRU)
    int pos_a_reemplazar = 0;
#if USARCACHE == 3
    struct timeval min_time;
    gettimeofday(&min_time, NULL); // Inicializar con el tiempo actual

    for (int i = 0; i < CACHE_SIZE; i++) {
      // Si hay un hueco libre, lo usamos
      if (UltimasEntradas[i].p_inodo == -1) {
        pos_a_reemplazar = i;
        break;
      }
      // Si no, buscamos el que tenga el tiempo más antiguo
      if (UltimasEntradas[i].ultima_consulta.tv_sec < min_time.tv_sec ||
          (UltimasEntradas[i].ultima_consulta.tv_sec == min_time.tv_sec &&
           UltimasEntradas[i].ultima_consulta.tv_usec < min_time.tv_usec)) {
        min_time = UltimasEntradas[i].ultima_consulta;
        pos_a_reemplazar = i;
      }
    }
#else
    pos_a_reemplazar = siguiente_fifo;
    siguiente_fifo = (siguiente_fifo + 1) % CACHE_SIZE;
#endif
#if defined(DEBUGN9)
    fprintf(stderr,
            ORANGE "[mi_write() → Actualizamos la caché de escritura]\n" RESET);
#endif
#if defined(DEBUGCACHE)
    fprintf(stderr, "[mi_write() → Reemplazamos cache[%d]: %s]\n",
            pos_a_reemplazar, camino);
#endif
    // Insertar en la posición elegida
    strcpy(UltimasEntradas[pos_a_reemplazar].camino, camino);
    UltimasEntradas[pos_a_reemplazar].p_inodo = p_inodo_fichero;
#if USARCACHE == 3
    gettimeofday(&UltimasEntradas[pos_a_reemplazar].ultima_consulta, NULL);
#endif
  }
  // 4. ESCRITURA REAL
  // Llamamos a la capa de ficheros usando el inodo obtenido
  return mi_write_f(p_inodo_fichero, buf, offset, nbytes);
}
int mi_read(const char *camino, void *buf, unsigned int offset,
            unsigned int nbytes) {
  unsigned int p_inodo_dir = 0;
  unsigned int p_inodo_fichero = 0;
  unsigned int p_entrada = 0;
  int error;
  int indice_cache = -1;
  // 1. INICIALIZAR CACHÉ
  inicializar_cache();

  // 2. BUSCAR EN CACHÉ
  for (int i = 0; i < CACHE_SIZE; i++) {
    if (strcmp(UltimasEntradas[i].camino, camino) == 0) {
      indice_cache = i;
      p_inodo_fichero = UltimasEntradas[i].p_inodo;
      break;
    }
  }

  if (indice_cache != -1) {
#if (defined(DEBUGCACHE))
    fprintf(stderr, "[mi_read() → Utilizamos cache[%d]: %s]\n", indice_cache,
            camino);

#endif

#if USARCACHE == 3
    gettimeofday(&UltimasEntradas[indice_cache].ultima_consulta, NULL);
#endif
  } else {
    // MISS: No está en caché, hay que buscarlo en el dispositivo
    if ((error = buscar_entrada(camino, &p_inodo_dir, &p_inodo_fichero,
                                &p_entrada, 0, 0)) < 0) {
      return error;
    }

    // 2. ACTUALIZAR CACHÉ (Estrategia LRU)
    int pos_a_reemplazar = 0;
#if USARCACHE == 3
    struct timeval min_time;
    // Obtenemos el tiempo actual para comparar
    gettimeofday(&min_time, NULL);

    for (int i = 0; i < CACHE_SIZE; i++) {
      if (UltimasEntradas[i].p_inodo == -1) { // Hueco libre (caché no llena)
        pos_a_reemplazar = i;
        break;
      }
      // Buscamos la entrada con el timestamp más pequeño (la más antigua)
      if (UltimasEntradas[i].ultima_consulta.tv_sec < min_time.tv_sec ||
          (UltimasEntradas[i].ultima_consulta.tv_sec == min_time.tv_sec &&
           UltimasEntradas[i].ultima_consulta.tv_usec < min_time.tv_usec)) {
        min_time = UltimasEntradas[i].ultima_consulta;
        pos_a_reemplazar = i;
      }
    }
#else
    pos_a_reemplazar = siguiente_fifo;
    siguiente_fifo = (siguiente_fifo + 1) % CACHE_SIZE;
#endif
#if defined(DEBUGN9)
    fprintf(stderr,
            ORANGE "[mi_read() → Actualizamos la caché de lectura]\n" RESET);
#endif
#if defined(DEBUGCACHE)
    fprintf(stderr, "[mi_read() → Reemplazamos cache[%d]: %s]\n",
            pos_a_reemplazar, camino);
#endif
    strcpy(UltimasEntradas[pos_a_reemplazar].camino, camino);
    UltimasEntradas[pos_a_reemplazar].p_inodo = p_inodo_fichero;

#if USARCACHE == 3
    gettimeofday(&UltimasEntradas[pos_a_reemplazar].ultima_consulta, NULL);
#endif
  }
  // 3. LECTURA REAL
  return mi_read_f(p_inodo_fichero, buf, offset, nbytes);
}
int mi_link(const char *camino1, const char *camino2) {
  mi_waitSem();
  unsigned int p_inodo_dir1 = 0, p_inodo1 = 0, p_entrada1 = 0;
  unsigned int p_inodo_dir2 = 0, p_inodo2 = 0, p_entrada2 = 0;
  int error;

  // 1. Obtener el inodo del fichero original (camino1)
  // reservar = 0 porque el fichero ya debe existir.
  error = buscar_entrada(camino1, &p_inodo_dir1, &p_inodo1, &p_entrada1, 0, 0);
  if (error < 0) {
    mi_signalSem();
    return error; // Error: No existe el original o problema de permisos
  }

  // 2. Leer inodo original y comprobar permisos de lectura
  struct inodo inodo1;
  if (leer_inodo(p_inodo1, &inodo1) == -1) {
    mi_signalSem();
    return -1;
  }
  // Comprobar que sea un fichero (no enlazamos directorios)
  if (inodo1.tipo != 'f') {
    fprintf(stderr,
            RED "Error: mi_link solo permite enlaces entre ficheros.\n" RESET);
    mi_signalSem();
    return -1;
  }

  if ((inodo1.permisos & 4) == 0) { // Comprobar permiso de lectura (r--)
    fprintf(
        stderr, RED
        "Error: No hay permiso de lectura sobre el fichero original.\n" RESET);
    mi_signalSem();
    return -1;
  }

  // 3. Crear la entrada para el enlace (camino2)
  // reservar = 1 con permisos 6 (rw-) para crear la nueva entrada.
  // buscar_entrada devolverá error si el camino2 ya existe.
  error = buscar_entrada(camino2, &p_inodo_dir2, &p_inodo2, &p_entrada2, 1, 6);
  if (error < 0) {
    mi_signalSem();
    return error; // Error: Ya existe el enlace o ruta inválida
  }

  // 4. Asociar el inodo original a la nueva entrada
  struct entrada entrada2;
  // Leemos la entrada recién creada en el directorio padre
  if (mi_read_f(p_inodo_dir2, &entrada2, p_entrada2 * sizeof(struct entrada),
                sizeof(struct entrada)) < 0) {
    mi_signalSem();
    return -1;
  }

  // Cambiamos el inodo que buscar_entrada reservó por defecto por el inodo1
  entrada2.ninodo = p_inodo1;

  // Escribimos la entrada modificada en el directorio padre
  if (mi_write_f(p_inodo_dir2, &entrada2, p_entrada2 * sizeof(struct entrada),
                 sizeof(struct entrada)) < 0) {
    mi_signalSem();
    return -1;
  }

  // 5. Liberar el inodo que se creó por defecto para camino2
  // Ya no lo necesitamos porque ahora la entrada apunta a p_inodo1
  if (liberar_inodo(p_inodo2) == -1) {
    mi_signalSem();
    return -1;
  }
  // 6. Actualizar el inodo original (p_inodo1)
  inodo1.nlinks++;
  inodo1.ctime = time(NULL);
  if (escribir_inodo(p_inodo1, &inodo1) == -1) {
    mi_signalSem();
    return -1;
  }
  mi_signalSem();
  return 0;
}
int mi_unlink(const char *camino) {
  mi_waitSem();
  unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
  int error;

  // 1. Comprobar que la entrada camino exista y obtener p_entrada y p_inodo
  error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
  if (error < 0) {
    mi_signalSem();
    return error; // La entrada no existe o error de permisos
  }

  // 2. Leer ese inodo para verificar su tipo
  struct inodo inodo;
  if (leer_inodo(p_inodo, &inodo) == -1) {
    mi_signalSem();
    return -1;
  }

  // 3. Si es un directorio y no está vacío, no se puede borrar
  if (inodo.tipo == 'd' && inodo.tamEnBytesLog > 0) {
    fprintf(stderr, RED "Error: El directorio %s no está vacío.\n" RESET,
            camino);
    mi_signalSem();
    return -1;
  }

  // 4. Leer el inodo del directorio que contiene la entrada (p_inodo_dir)
  struct inodo inodo_dir_padre;
  if (leer_inodo(p_inodo_dir, &inodo_dir_padre) == -1) {
    mi_signalSem();
    return -1;
  }

  // 5. Obtener el número de entradas que tiene el directorio padre
  int num_entradas_total =
      inodo_dir_padre.tamEnBytesLog / sizeof(struct entrada);

  // 6. Gestionar la eliminación de la entrada sin dejar huecos
  // Si NO es la última entrada, movemos la última a la posición de la que
  // borramos
  if (p_entrada != (num_entradas_total - 1)) {
    struct entrada ultima_entrada;
    // Leer la última entrada
    if (mi_read_f(p_inodo_dir, &ultima_entrada,
                  (num_entradas_total - 1) * sizeof(struct entrada),
                  sizeof(struct entrada)) < 0) {
      mi_signalSem();
      return -1;
    }
    // Escribirla en la posición p_entrada
    if (mi_write_f(p_inodo_dir, &ultima_entrada,
                   p_entrada * sizeof(struct entrada),
                   sizeof(struct entrada)) < 0) {
      mi_signalSem();
      return -1;
    }
  }

  // 7. Truncar el inodo del directorio padre para eliminar la posición sobrante
  if (mi_truncar_f(p_inodo_dir, inodo_dir_padre.tamEnBytesLog -
                                    sizeof(struct entrada)) == -1) {
    mi_signalSem();
    return -1;
  }

  // 8. Decrementar el nº de enlaces del inodo p_inodo
  inodo.nlinks--;

  // 9. Si no quedan enlaces (nlinks == 0), liberar el inodo y su contenido
  if (inodo.nlinks == 0) {
    // liberar_inodo llamará a liberar_bloques_inodo para limpiar datos y
    // punteros
    if (liberar_inodo(p_inodo) == -1) {
      mi_signalSem();
      return -1;
    }
  } else {
    // Si aún quedan enlaces, actualizar ctime y guardar el inodo modificado
    inodo.ctime = time(NULL);
    if (escribir_inodo(p_inodo, &inodo) == -1) {
      mi_signalSem();
      return -1;
    }
  }
  mi_signalSem();
  return 0;
}
