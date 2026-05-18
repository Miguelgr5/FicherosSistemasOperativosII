#include "verificacion.h"

int main(int argc, char **argv) {
  // 1. Sintaxis: ./verificacion <disco> <directorio_simulacion>
  if (argc != 3) {
    fprintf(stderr, "Uso: ./verificacion <disco> <directorio_simulacion>\n");
    exit(1);
  }

  char *nombre_disco = argv[1];
  char *camino_simul = argv[2];

  // 2. Montar el disco
  if (bmount(nombre_disco) == -1) {
    fprintf(stderr, "Error al montar el disco\n");
    exit(1);
  }

  // 3. Obtener información del directorio de simulación
  struct STAT stat_simul;
  if (mi_stat(camino_simul, &stat_simul) == -1) {
    bumount();
    exit(1);
  }
  printf("%d", stat_simul.tamEnBytesLog);
  int num_entradas = stat_simul.tamEnBytesLog / sizeof(struct entrada);
  printf("Debug: tamEnBytesLog del directorio = %u\n",
         stat_simul.tamEnBytesLog);
  if (num_entradas != NUMPROCESOS) {
    fprintf(stderr, "Error: Se esperaban %d procesos, pero hay %d\n",
            NUMPROCESOS, num_entradas);
  }

  // 4. Crear el fichero de informe (informe.txt) dentro del directorio de
  // simulación
  char camino_informe[256];
  sprintf(camino_informe, "%sinforme.txt", camino_simul);
  if (mi_creat(camino_informe, 6) < 0) {
    bumount();
    exit(1);
  }

  // Leer las entradas del directorio
  struct entrada entradas[num_entradas];
  mi_read(camino_simul, entradas, 0, sizeof(entradas));

  int validados = 0;
  // 5. Recorrer cada entrada del directorio
  for (int i = 0; i < num_entradas; i++) {
    // Ignorar si no es un directorio de proceso (por ejemplo, el propio
    // informe.txt)
    if (strncmp(entradas[i].nombre, "proceso_", 8) != 0)
      continue;

    struct INFORMACION info;
    info.pid = atoi(entradas[i].nombre + 8);
    info.nEscrituras = 0;

    char camino_fichero[256];
    sprintf(camino_fichero, "%s%s/prueba.dat", camino_simul,
            entradas[i].nombre);

    struct REGISTRO reg;
    int offset = 0;
    // Leer el fichero prueba.dat secuencialmente
    while (mi_read(camino_fichero, &reg, offset, sizeof(struct REGISTRO)) > 0) {
      // Validar que el registro pertenece al proceso (por el PID)
      if (reg.pid == info.pid) {
        if (info.nEscrituras == 0) {
          info.PrimeraEscritura = info.UltimaEscritura = info.MenorPosicion =
              info.MayorPosicion = reg;
        } else {
          if (reg.nEscritura < info.PrimeraEscritura.nEscritura)
            info.PrimeraEscritura = reg;
          if (reg.nEscritura > info.UltimaEscritura.nEscritura)
            info.UltimaEscritura = reg;
          if (reg.nRegistro < info.MenorPosicion.nRegistro)
            info.MenorPosicion = reg;
          if (reg.nRegistro > info.MayorPosicion.nRegistro)
            info.MayorPosicion = reg;
        }
        info.nEscrituras++;
      }
      offset += sizeof(struct REGISTRO);
    }

    // 6. Preparar la salida por pantalla y para el fichero informe.txt
    char buffer[3500];
    char fecha_p[26], fecha_u[26], fecha_menor[26], fecha_mayor[26];

    strcpy(fecha_p, ctime(&info.PrimeraEscritura.fecha));
    fecha_p[24] = '\0';
    strcpy(fecha_u, ctime(&info.UltimaEscritura.fecha));
    fecha_u[24] = '\0';
    strcpy(fecha_menor, ctime(&info.MenorPosicion.fecha));
    fecha_menor[24] = '\0';
    strcpy(fecha_mayor, ctime(&info.MayorPosicion.fecha));
    fecha_mayor[24] = '\0';

    sprintf(buffer, "PID: %d\nNumero de escrituras: %u\n", info.pid,
            info.nEscrituras);
    sprintf(buffer + strlen(buffer), "Primera escritura\t%d\t%d\t%s\n",
            info.PrimeraEscritura.nEscritura, info.PrimeraEscritura.nRegistro,
            fecha_p);
    sprintf(buffer + strlen(buffer), "Ultima escritura\t%d\t%d\t%s\n",
            info.UltimaEscritura.nEscritura, info.UltimaEscritura.nRegistro,
            fecha_u);
    sprintf(buffer + strlen(buffer), "Menor posicion\t\t%d\t%d\t%s\n",
            info.MenorPosicion.nRegistro, info.MenorPosicion.nRegistro,
            fecha_menor);
    sprintf(buffer + strlen(buffer), "Mayor posicion\t\t%d\t%d\t%s\n\n",
            info.MayorPosicion.nRegistro, info.MayorPosicion.nRegistro,
            fecha_mayor);

    printf("[%d] %s", i, buffer); // Consola
    mi_write(camino_informe, buffer, strlen(buffer),
             strlen(buffer)); // Informe.txt (append simulado por offset)
    // Nota: En un sistema real usarías el tamaño actual del informe como offset
    // para el append.

    validados++;
  }

  printf("Total procesos validados: %d\n", validados);
  bumount();
  return 0;
}
