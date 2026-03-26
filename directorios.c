#include "directorios.h"
#define DEBUGN7
int extraer_camino(const char *camino, char *inicial, char *final, char *tipo) {
  if (camino[0] != '/')
    return FALLO;

  const char *segunda_barra = strchr(camino + 1, '/');

  if (segunda_barra) {
    // Hay más niveles (ej: /dir1/dir2)
    int longitud = segunda_barra - (camino + 1);
    strncpy(inicial, camino + 1, longitud);
    inicial[longitud] = '\0';
    strcpy(final, segunda_barra);
    *tipo = 'd'; // Si hay algo después de la segunda barra, el inicial es un
                 // directorio
  } else {
    // Es el último nivel (ej: /archivo o /directorio/)
    strcpy(inicial, camino + 1);
    strcpy(final, "");
    if (camino[strlen(camino) - 1] == '/') {
      *tipo = 'd';
      inicial[strlen(inicial) - 1] = '\0'; // Quitar la barra final de inicial
    } else {
      *tipo = 'f';
    }
  }
  return EXITO;
}
int buscar_entrada(const char *camino_directorio, unsigned int *p_inodo_dir,
                   unsigned int *p_inodo, unsigned int *p_entrada,
                   char reservar, unsigned char permisos) {
  struct inodo inodo_dir;
  struct entrada entrada;
  char inicial[TAMNOMBRE];
  char final[strlen(camino_directorio) + 1];
  char tipo;
  int num_entradas, n_entrada;

  memset(inicial, 0, TAMNOMBRE);
  memset(final, 0, strlen(camino_directorio) + 1);

  if (strcmp(camino_directorio, "/") == 0) {
    *p_inodo = 0; // Inodo raíz
    *p_entrada = 0;
    return EXITO;
  }

  if (extraer_camino(camino_directorio, inicial, final, &tipo) == FALLO) {
    return ERROR_CAMINO_INCORRECTO;
  }

#if defined(DEBUGN7)
  fprintf(stderr, "[buscar_entrada()→ inicial: %s, final: %s, reservar: %d]\n",
          inicial, final, reservar);
#endif

  if (leer_inodo(*p_inodo_dir, &inodo_dir) == FALLO)
    return FALLO;
  if ((inodo_dir.permisos & 4) == 0)
    return ERROR_PERMISO_LECTURA;

  num_entradas = inodo_dir.tamEnBytesLog / sizeof(struct entrada);
  n_entrada = 0;

  if (num_entradas > 0) {
    if (mi_read_f(*p_inodo_dir, &entrada, n_entrada * sizeof(struct entrada),
                  sizeof(struct entrada)) < 0)
      return FALLO;
    while (n_entrada < num_entradas && strcmp(inicial, entrada.nombre) != 0) {
      n_entrada++;
      if (n_entrada < num_entradas) {
        if (mi_read_f(*p_inodo_dir, &entrada,
                      n_entrada * sizeof(struct entrada),
                      sizeof(struct entrada)) < 0)
          return FALLO;
      }
    }
  }

  if (n_entrada == num_entradas) { // No existe la entrada
    switch (reservar) {
    case 0:
      return ERROR_NO_EXISTE_ENTRADA_CONSULTA;
    case 1:
      if (inodo_dir.tipo != 'd')
        return ERROR_NO_SE_PUEDE_CREAR_ENTRADA_EN_UN_FICHERO;
      if ((inodo_dir.permisos & 2) == 0)
        return ERROR_PERMISO_ESCRITURA;

      strcpy(entrada.nombre, inicial);
      if (tipo == 'd') {
        if (strcmp(final, "/") == 0 || strcmp(final, "") == 0) {
          entrada.ninodo = reservar_inodo('d', permisos);
#if defined(DEBUGN7)
          fprintf(stderr,
                  "[buscar_entrada()→ reservado inodo %u tipo d con permisos "
                  "%u para %s]\n",
                  entrada.ninodo, permisos, inicial);
#endif
        } else {
          return ERROR_NO_EXISTE_DIRECTORIO_INTERMEDIO;
        }
      } else {
        entrada.ninodo = reservar_inodo('f', permisos);
#if defined(DEBUGN7)
        fprintf(stderr,
                "[buscar_entrada()→ reservado inodo %u tipo f con permisos %u "
                "para %s]\n",
                entrada.ninodo, permisos, inicial);
#endif
      }

      if (mi_write_f(*p_inodo_dir, &entrada, n_entrada * sizeof(struct entrada),
                     sizeof(struct entrada)) < 0) {
        // Si falla la escritura, idealmente liberaríamos el inodo reservado
        return FALLO;
      }
#if defined(DEBUGN7)
      fprintf(stderr, "[buscar_entrada()→ creada entrada: %s, %u]\n",
              entrada.nombre, entrada.ninodo);
#endif
      break;
    }
  }

  // Comprobar si hemos terminado la ruta
  if (strcmp(final, "") == 0 || strcmp(final, "/") == 0) {
    if (n_entrada < num_entradas && reservar == 1)
      return ERROR_ENTRADA_YA_EXISTENTE;
    *p_inodo = entrada.ninodo;
    *p_entrada = n_entrada;
    return EXITO;
  } else {
    *p_inodo_dir = entrada.ninodo;
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
