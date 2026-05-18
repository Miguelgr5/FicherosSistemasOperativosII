#include "verificacion.h"

#define N_REGISTROS                                                            \
  256 // Optimización: Múltiplo de BLOCKSIZE (6 bloques de memoria)

int main(int argc, char **argv) {
  // 1. Comprobar sintaxis
  if (argc != 3) {
    fprintf(
        stderr,
        "Uso: ./verificacion <nombre_dispositivo> <directorio_simulación>\n");
    exit(1);
  }

  char *nombre_disco = argv[1];
  char *camino_simul = argv[2];

  // Montar el dispositivo virtual
  if (bmount(nombre_disco) == -1) {
    fprintf(stderr, "Error al montar el disco\n");
    exit(1);
  }

  // 2. Calcular entradas del directorio a partir del stat
  struct STAT stat_simul;
  if (mi_stat(camino_simul, &stat_simul) == -1) {
    fprintf(stderr, "Error al obtener stat del directorio de simulación\n");
    bumount();
    exit(1);
  }

  int num_entradas = stat_simul.tamEnBytesLog / sizeof(struct entrada);
  printf("dir_sim: %s\n", camino_simul);
  printf("numentradas: %d NUMPROCESOS: %d\n", num_entradas, NUMPROCESOS);

  // Si el número de entradas es incorrecto se aborta la ejecución
  // inmediatamente
  if (num_entradas != NUMPROCESOS) {
    fprintf(stderr,
            "Error: El número de entradas no coincide con NUMPROCESOS\n");
    bumount();
    exit(1);
  }

  // 3. Crear el fichero "informe.txt" dentro de la simulación
  char camino_informe[256];
  sprintf(camino_informe, "%sinforme.txt", camino_simul);
  if (mi_creat(camino_informe, 6) < 0) {
    fprintf(stderr, "Error al crear informe.txt\n");
    bumount();
    exit(1);
  }

  // MEJORA: Caché de directorios. Leemos todas las entradas a memoria principal
  // de un solo golpe.
  struct entrada *entradas = malloc(num_entradas * sizeof(struct entrada));
  if (mi_read(camino_simul, entradas, 0,
              num_entradas * sizeof(struct entrada)) < 0) {
    fprintf(stderr, "Error al leer las entradas del directorio\n");
    free(entradas);
    bumount();
    exit(1);
  }

  int offset_informe = 0; // Control estricto de desplazamiento para simular el
                          // append incremental

  // 4. Iterar sobre cada entrada guardada en el buffer
  for (int i = 0; i < num_entradas; i++) {
    // Ignorar de forma segura entradas que no pertenezcan a procesos (ej.
    // informe.txt)
    if (strncmp(entradas[i].nombre, "proceso_", 8) != 0) {
      continue;
    }

    struct INFORMACION info;
    // Extraer el PID del nombre usando el desplazamiento del prefijo "proceso_"
    info.pid = atoi(entradas[i].nombre + 8);
    info.nEscrituras = 0;

    char camino_fichero[256];
    sprintf(camino_fichero, "%s%s/prueba.dat", camino_simul,
            entradas[i].nombre);

    // Optimización masiva: Procesamiento secuencial mediante buffer de N
    // registros
    struct REGISTRO buffer_escrituras[N_REGISTROS];
    int offset_prueba = 0;
    int bytes_leidos;

    // Limpieza inicial del buffer de lectura
    memset(buffer_escrituras, 0, sizeof(buffer_escrituras));

    // Mientras haya datos que leer en prueba.dat
    while ((bytes_leidos = mi_read(camino_fichero, buffer_escrituras,
                                   offset_prueba, sizeof(buffer_escrituras))) >
           0) {
      int registros_leidos = bytes_leidos / sizeof(struct REGISTRO);

      for (int r = 0; r < registros_leidos; r++) {
        struct REGISTRO reg = buffer_escrituras[r];

        // Validar que el PID coincida para desechar la basura de los bloques
        // vacíos (ficheros dispersos)
        if (reg.pid == info.pid) {
          if (info.nEscrituras == 0) {
            // Inicialización con el primer registro válido encontrado
            info.PrimeraEscritura = reg;
            info.UltimaEscritura = reg;
            info.MenorPosicion = reg; // El primer registro leído es
                                      // inherentemente el de menor posición
            info.MayorPosicion = reg;
          } else {
            // Comprobaciones dinámicas por número de escritura
            if (reg.nEscritura < info.PrimeraEscritura.nEscritura) {
              info.PrimeraEscritura = reg;
            }
            if (reg.nEscritura > info.UltimaEscritura.nEscritura) {
              info.UltimaEscritura = reg;
            }
            // Actualización de la posición física más alta alcanzada
            if (reg.nRegistro > info.MayorPosicion.nRegistro) {
              info.MayorPosicion = reg;
            }
          }
          info.nEscrituras++;
        }
      }
      offset_prueba += bytes_leidos;
      memset(buffer_escrituras, 0,
             sizeof(buffer_escrituras)); // Limpieza cíclica obligatoria
    }

    // Mostrar por pantalla el progreso secuencial solicitado
    printf("%d) %u escrituras validadas en %s\n", i + 1, info.nEscrituras,
           camino_fichero);

    // 5. Formatear cadenas de texto con las marcas de tiempo legibles
    char buffer_texto[2048];
    char f_primera[30], f_ultima[30], f_menor[30], f_mayor[30];

    // Se emplea asctime() limpiando el salto de línea residual que genera la
    // función de sistema
    strcpy(f_primera, asctime(localtime(&info.PrimeraEscritura.fecha)));
    f_primera[strlen(f_primera) - 1] = '\0';

    strcpy(f_ultima, asctime(localtime(&info.UltimaEscritura.fecha)));
    f_ultima[strlen(f_ultima) - 1] = '\0';

    strcpy(f_menor, asctime(localtime(&info.MenorPosicion.fecha)));
    f_menor[strlen(f_menor) - 1] = '\0';

    strcpy(f_mayor, asctime(localtime(&info.MayorPosicion.fecha)));
    f_mayor[strlen(f_mayor) - 1] = '\0';

    // Composición exacta del bloque de texto según la plantilla exigida
    int len = sprintf(
        buffer_texto,
        "PID: %d\n"
        "Numero de escrituras: %u\n"
        "Primera Escritura\t%u\t%u\t%s\n"
        "Ultima Escritura\t%u\t%u\t%s\n"
        "Menor Posición\t%u\t%u\t%s\n"
        "Mayor Posición\t%u\t%u\t%s\n\n",
        info.pid, info.nEscrituras, info.PrimeraEscritura.nEscritura,
        info.PrimeraEscritura.nRegistro, f_primera,
        info.UltimaEscritura.nEscritura, info.UltimaEscritura.nRegistro,
        f_ultima, info.MenorPosicion.nEscritura, info.MenorPosicion.nRegistro,
        f_menor, info.MayorPosicion.nEscritura, info.MayorPosicion.nRegistro,
        f_mayor);

    // Volcado de datos al archivo acumulativo simulando un modo append preciso
    if (mi_write(camino_informe, buffer_texto, offset_informe, len) < 0) {
      fprintf(stderr, "Error al escribir en informe.txt\n");
      free(entradas);
      bumount();
      exit(1);
    }
    offset_informe += len; // Desplazamiento del puntero global de escritura
  }

  free(entradas);

  // Desmontar el dispositivo virtual
  if (bumount() == -1) {
    fprintf(stderr, "Error al desmontar el disco\n");
    exit(1);
  }

  return 0;
}
