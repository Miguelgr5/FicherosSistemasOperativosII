#define VERIFICACION_H

#include "simulacion.h"

struct INFORMACION {
  int pid;
  unsigned int nEscrituras; // Validadas
  struct REGISTRO PrimeraEscritura;
  struct REGISTRO UltimaEscritura;
  struct REGISTRO MenorPosicion;
  struct REGISTRO MayorPosicion;
};
