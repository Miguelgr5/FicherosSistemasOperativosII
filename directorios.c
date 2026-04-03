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
