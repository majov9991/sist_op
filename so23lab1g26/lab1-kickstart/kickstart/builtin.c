#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "builtin.h"
#include "strextra.h"
#include "tests/syscall_mock.h"

bool quit_bash = false;
// verifica si el comando es un "exit"
static bool builtin_is_exit(scommand cmd){
    assert(cmd != NULL);

    return strcmp(scommand_front(cmd), "exit") == 0;
}

// verifica si el comando es un "cd"
static bool builtin_is_cd(scommand cmd){
    assert(cmd != NULL);

    return strcmp(scommand_front(cmd), "cd") == 0;
}

// verifica si el comando es un "help"
static bool builtin_is_help(scommand cmd){
    assert(cmd != NULL);

    return strcmp(scommand_front(cmd), "help") == 0;
}

//Indica si el comando alojado en `cmd` es un comando interno
bool builtin_is_internal(scommand cmd){
    assert(cmd != NULL);

    return (builtin_is_exit(cmd) || builtin_is_cd(cmd)) || builtin_is_help(cmd);
}

//Indica si el pipeline tiene solo un elemento y si este se corresponde a un comando interno.
bool builtin_alone(pipeline p){
    assert(p != NULL);

    return (pipeline_length(p) == 1 && builtin_is_internal(pipeline_front(p)));
}


// ejecuta el comando interno exit
static void builtin_run_exit(scommand cmd){
    assert(cmd != NULL && builtin_is_exit(cmd));
    quit_bash = true;

}

// ejecuta el comando interno cd
static void builtin_run_cd(scommand cmd){
    assert(cmd != NULL && builtin_is_cd(cmd));
    int return_value;
    
    scommand_pop_front(cmd);

    //El comando cd puede tomar cero ó un argumento
    //Verifico que la cantidad  de nodos de cmd sea menor o igual a 1
    if (scommand_length(cmd)<=1){
        
        //Defino un puntero que contendra la ruta
        char *path=NULL;

        //caso cuando hay dos nodos
        if (scommand_length(cmd)==1){
            
            path=scommand_front(cmd);

            //Si la ruta es null
            if (path==NULL) { 
                return_value=-1;
            }

            else{
                return_value=chdir(path);
            }
        }

        //Caso cuando hay un nodo
        else{
            //Este es el caso cuando se ejecuta cd solo,entonces se dirige al directorio de inicio
           
            //obtengo un puntero a la ruta del directorio de inicio     
            const char *homeDir = getenv("HOME");

            if (homeDir == NULL) {
                fprintf(stderr, "No se pudo obtener la ruta del directorio de inicio\n");        
            }
            return_value=chdir(homeDir);               

        }
        
        //caso cuanto el argumento de cd no existe
        if (return_value!=0){

            
            char *cmd_string = scommand_to_string(cmd);
            char *arg = strmerge("-mybash: cd: ", cmd_string);
            free(cmd_string);
            perror(arg);
            free(arg);
        }
    }

    //El caso cuando se tiene mas argumentos de lo esperado para el comando cd
    else{
        printf("-mybash: cd: demasiados argumentos\n");
    }
}


// ejecuta el comando interno help
static void builtin_run_help(scommand cmd){
    assert(cmd != NULL && builtin_is_help(cmd));

    //Se imprime informacion pedida

    printf("Nombre del shell: Mybash\n");
    printf("Autores: \n");

    //Incluir autores
    printf("Alexis Javier Vandevendes \n");
    printf("Eric Josias Rodriguez \n");
    printf("Maria Jose Villegas \n");
    printf("Tomás Joaquín Montes \n"); 
        

    printf("Comandos internos:\n");
    printf("                  cd: El comando cd permite navegar por el sistema de archivos y acceder a diferentes directorios\n");
    printf("                  help: El comando help se utiliza para proporcionar información sobre los comandos internos del shell y sus opciones\n");
    printf("                  exit: El comando exit en Linux se utiliza para finalizar la sesión actual en la línea de comandos o para salir del intérprete de comandos (shell) en el que te encuentras\n");
}

//Ejecuta un comando interno
//REQUIRES: {builtin_is_internal(cmd)}
 
void builtin_run(scommand cmd){
    assert(cmd != NULL && builtin_is_internal(cmd));
    
    if(builtin_is_cd(cmd))
        builtin_run_cd(cmd);

    else if (builtin_is_exit(cmd))
        builtin_run_exit(cmd);

    else if (builtin_is_help(cmd))
        builtin_run_help(cmd);        
 }
