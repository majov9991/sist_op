/* Ejecuta comandos simples y pipelines.
 * No toca ningún comando interno.
 */

#ifndef EXECUTE_H
#define EXECUTE_H

#include "command.h"
#define READ_END 0  // para indicar si finalizamos la lectura de un "pipe" conectado por 2 procesos 
#define WRITE_END 1 // para indicar si finalizamos la escritura de un  "pipe" conectado por 2 procesos
#define MAX_SIZE 10000

void execute_pipeline(pipeline apipe);
/*
 * Ejecuta un pipeline, identificando comandos internos, forkeando, y
 *   redirigiendo la entrada y salida. puede modificar `apipe' en el proceso
 *   de ejecución.
 *   apipe: pipeline a ejecutar
 * Requires: apipe!=NULL
 */

#endif /* EXECUTE_H */
