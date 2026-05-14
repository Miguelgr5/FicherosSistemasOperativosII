/* semaforo_mutex_posix.h */
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define SEM_NAME "/mymutex" /* Usamos este nombre para el semáforo mutex */
#define SEM_INIT_VALUE 1    /* Valor inicial de los mutex */

sem_t *initSem();
void deleteSem(sem_t *sem);
void signalSem(sem_t *sem);
void waitSem(sem_t *sem);
