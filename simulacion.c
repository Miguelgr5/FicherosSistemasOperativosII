#include "simulacion.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int acabados = 0;

// Manejador de señal para enterrar procesos hijos y evitar zombies
void reaper() {
  pid_t ended;
  signal(SIGCHLD, reaper);
  while ((ended = waitpid(-1, NULL, WNOHANG)) > 0) {
    acabados++;
  }
}

int main(int argc, char **argv) {
  // 1. Sintaxis
  if (argc != 2) {
    fprintf(stderr, "Uso: ./simulacion <disco>\n");
    exit(1);
  }

  // 2. Preparar enterrador de hijos
  signal(SIGCHLD, reaper);

  // 3. Montar disco (Padre)
  if (bmount(argv[1]) == -1) {
    fprintf(stderr, "Error al montar el disco\n");
    exit(1);
  }

  // 4. Crear directorio raíz de la simulación con marca temporal
  time_t t = time(NULL);
  struct tm *tm = localtime(&t);
  char camino_simul[100];
  // IMPORTANTE: Termina en "/" para que mi_creat cree un DIRECTORIO
  sprintf(camino_simul, "/simul_%04d%02d%02d%02d%02d%02d/", tm->tm_year + 1900,
          tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

  if (mi_creat(camino_simul, 6) < 0) {
    fprintf(stderr, "Error al crear el directorio de simulación %s\n",
            camino_simul);
    bumount();
    exit(1);
  }

  printf(
      "*** SIMULACIÓN DE %d PROCESOS REALIZANDO CADA UNO %d ESCRITURAS ***\n",
      NUMPROCESOS, NUMESCRITURAS);

  // 5. Bucle de creación de procesos
  for (int p = 1; p <= NUMPROCESOS; p++) {
    pid_t pid = fork();

    if (pid == 0) { // --- PROCESO HIJO ---
      if (bmount(argv[1]) == -1)
        exit(1);

      // Crear directorio del proceso: proceso_PID/
      char camino_proceso[150];
      sprintf(camino_proceso, "%sproceso_%d/", camino_simul, getpid());
      if (mi_creat(camino_proceso, 6) < 0) {
        bumount();
        exit(1);
      }

      // Crear fichero: prueba.dat (sin barra final)
      char camino_fichero[200];
      sprintf(camino_fichero, "%sprueba.dat", camino_proceso);
      if (mi_creat(camino_fichero, 6) < 0) {
        bumount();
        exit(1);
      }

      // Semilla aleatoria
      srand(time(NULL) + getpid());

      for (int nescritura = 1; nescritura <= NUMESCRITURAS; nescritura++) {
        struct REGISTRO registro;
        registro.fecha = time(NULL);
        registro.pid = getpid();
        registro.nEscritura = nescritura;
        registro.nRegistro = rand() % REGMAX;

        // Escritura en posición aleatoria
        mi_write(camino_fichero, &registro,
                 registro.nRegistro * sizeof(struct REGISTRO),
                 sizeof(struct REGISTRO));

        usleep(50000); // 0,05 seg entre escrituras
      }

      // Mensaje de éxito al terminar las escrituras
      printf("[Proceso %d: Completadas %d escrituras en %s]\n", p,
             NUMESCRITURAS, camino_fichero);

      bumount();
      exit(0);
    }

    // El padre espera 0,15 seg para lanzar el siguiente
    usleep(150000);
  }

  // 6. Esperar a que todos los hijos terminen
  while (acabados < NUMPROCESOS) {
    pause();
  }

  printf("Simulación finalizada.\n");
  bumount();
  return 0;
}
