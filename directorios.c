#include "directorios.h"
#define DEBUGN7
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
        if (strcmp(final, "/") == 0 || strcmp(final, "") == 0) {
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
  if (strcmp(final, "") == 0 || strcmp(final, "/") == 0) {
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
  unsigned int p_inodo_dir = 0; // Empezamos la búsqueda desde el inodo raíz
  unsigned int p_inodo = 0; // Aquí nos devolverá el inodo del archivo creado
  unsigned int p_entrada =
      0; // Aquí nos devolverá el nº de entrada en el directorio padre

  // Llamamos a buscar_entrada
  // Pasar &p_inodo_dir permite que la función actualice por dónde va pasando
  int error =
      buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 1, permisos);

  if (error < 0) {
    // Si hay error (permisos, camino inexistente, etc), lo propagamos
    return error;
  }

  return 0; // Éxito
}
int mi_dir(const char *camino, char *buffer, char tipo, char flag) {
  unsigned int p_inodo_dir = 0, p_inodo = 0, p_entrada = 0;
  int error;

  error = buscar_entrada(camino, &p_inodo_dir, &p_inodo, &p_entrada, 0, 0);
  if (error < 0)
    return error;

  struct inodo inodo;
  if (leer_inodo(p_inodo, &inodo) == -1)
    return -1;

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
  return mi_chmod_f(p_inodo, permisos);
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
