#define SIMULACION_H

#include "directorios.h"
#include <sys/types.h>
#include <time.h>

#define NUMPROCESOS 100
#define NUMESCRITURAS 50
#define REGMAX 500000

struct REGISTRO {
  time_t fecha;   // Precisión en segundos
  pid_t pid;      // PID del proceso que lo ha creado
  int nEscritura; // Número de escritura (1 a 50)
  int nRegistro;  // Posición lógica en el fichero
};
