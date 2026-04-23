// Autores: Miguel Amengual Baldó,Xavi García Lladó y Miguel García García.
#include "ficheros.h"
#include <stdio.h>

int main(int argc, char **argv) {
  /* Validación de argumentos:
     Comprobamos que el usuario ha introducido los 3 parámetros necesarios:
     el nombre del disco virtual, el texto a escribir y el modo de gestión de
     inodos.
  */
  if (argc < 4) {
    fprintf(stderr, RED "Sintaxis: escribir <nombre_dispositivo> <\"$(cat "
                        "fichero)\"> <diferentes_inodos>\n");
    fprintf(stderr, "Offsets: 9000, 209000, 30725000, 409605000, 480000000\n");
    fprintf(stderr, "Si diferentes_inodos=0 se reserva un solo inodo para "
                    "todos los offsets\n");
    printf(RESET "\n");
    return FALLO;
  }

  /* Montaje del dispositivo:
     Llamamos a bmount para abrir el fichero que simula nuestro disco.
     Sin este paso, no podemos acceder a los bloques físicos.
  */
  if (bmount(argv[1]) == FALLO)
    return FALLO;

  /* Inicialización de variables:
     Capturamos el texto del segundo argumento y convertimos el tercer argumento
     en un entero para saber si usaremos uno o varios inodos.
     Calculamos también la longitud del texto (nbytes).
  */
  char *texto = argv[2];
  int diferentes_inodos = atoi(argv[3]);
  int nbytes = strlen(texto);

  /* Configuración de pruebas:
     Definimos un array con 5 offsets estratégicos que forzarán al sistema
     a usar desde punteros directos hasta indirectos de nivel 3.
  */
  unsigned int offsets[] = {9000, 209000, 30725000, 409605000, 480000000};
  unsigned int ninodo;

  printf("longitud texto: %d\n", nbytes);

  /* Gestión de inodo único:
     Si el usuario eligió '0', reservamos un solo inodo de tipo fichero ('f')
     con permisos de lectura/escritura (6) antes de empezar a escribir.
  */
  if (diferentes_inodos == 0) {
    ninodo = reservar_inodo('f', 6);
  }

  /* Bucle principal de escritura:
     Recorremos los 5 offsets definidos anteriormente para realizar 5
     escrituras.
  */
  for (int i = 0; i < 5; i++) {
    /* Gestión de inodos múltiples:
       Si el usuario eligió '1', reservamos un inodo nuevo para cada
       iteración del bucle, creando así 5 ficheros distintos.
    */
    if (diferentes_inodos == 1) {
      ninodo = reservar_inodo('f', 6);
    }

    printf("\nNº inodo reservado: %d\n", ninodo);
    printf("offset: %u\n", offsets[i]);

    /* Escritura en el sistema de ficheros:
       Llamamos a mi_write_f pasándole el inodo, el texto y el offset actual.
       Esta función es la que realmente reparte los bytes por los bloques del
       disco.
    */
    int escritos = mi_write_f(ninodo, texto, offsets[i], nbytes);

    /* Obtención de estadísticas (stat):
       Tras escribir, leemos el estado del inodo para comprobar cómo ha crecido
       su tamaño lógico y cuántos bloques físicos ha ocupado realmente en el
       disco.
    */
    struct STAT stat;
    mi_stat_f(ninodo, &stat);

    printf("Bytes escritos: %d\n", escritos);
    printf("stat.tamEnBytesLog=%u\n", stat.tamEnBytesLog);
    printf("stat.numBloquesOcupados=%u\n", stat.numBloquesOcupados);
  }

  /* Desmontaje del dispositivo:
     Cerramos el descriptor del fichero para asegurar que todos los cambios
     se guardan correctamente en el disco virtual antes de salir.
  */
  bumount();
  return EXITO;
}
