#include "execute.h"
#include "builtin.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//librerias necesarias para usar fork, dup, pipe, etc
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "tests/syscall_mock.h"

static char** scommand_to_argv(scommand self) {
    assert(self != NULL);

    unsigned int n = scommand_length(self);
    char** argv = calloc(sizeof(char*), n + 1);

    if (argv != NULL) {
        for (unsigned int j = 0; j < n; j++) {
	        char * arg = strdup(scommand_front(self));
		scommand_pop_front(self);
                argv[j] = arg;

            assert(argv[j] != NULL);
            // Este assert verifica la parte de ⟨∀i ∈ 0..n-1 : argv[i] != NULL⟩
        }
        argv[n] = NULL;
    }

    assert(self != NULL && ((argv == NULL) != (scommand_is_empty(self) && argv != NULL && argv[n] == NULL)));
    return (argv);
}
// La funcion intercambia la entrada estandar por la del comando simple, si es NULL, no hace nada
static int change_file_desc_in(scommand cmd){
        assert (cmd != NULL); //Verifico que el cmd no sea NULL
        
        char* cmd_in = scommand_get_redir_in(cmd); // necesito el nombre de donde "leer" el archivo, asique lo coloco en cmd_in
        if (cmd_in != NULL) //cuando se leyò correctamente
        {
                int fd_in = open(cmd_in, O_RDONLY, S_IRUSR); // O_RDONLY quiere decir que solo lo vamos a leer. S_IRUSR quiere decir que usuario tiene permiso de leer

                if (fd_in == -1) // mensaje de error por si falla
                {
                        perror("el archivo no se ha leido correctamente");
                        return (EXIT_FAILURE);
                }
                
                int dup2_res = dup2(fd_in,STDIN_FILENO); // intercambiamos la entrada la "Entrada EStandar" por la del comando

                if (dup2_res == -1) //mensaje de error por si falla
                {
                        perror("dup2 ha fallado");
                        return(EXIT_FAILURE);
                }

                int res_close = close(fd_in); //Cerramos el fichero 
                if (res_close == -1) //mensaje de error
                {
                        perror("el archivo no se ha cerrado correctamente");
                        return (EXIT_FAILURE);
                }    
        } 
        return (EXIT_SUCCESS); 
}


//la funcion intercambia la salida estandar por la del comando, si es NULL, no hace nada. Util para cuando no necesitamos cambiar la salida.
static int change_file_desc_out(scommand cmd){
        assert(cmd != NULL); 
        
        char *cmd_out = scommand_get_redir_out(cmd); //necesitamos saber donde "dejar" el archivo o lo que necesite el comando, asi que lo colocamos en cmd_out
        if (cmd_out != NULL) //cuando tiene salida no NULA , es decir , cuando no se tenga que imprimir por pantalla
        {
                int fd_out = open(cmd_out, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR); //abrimos el fichero, primero lo creamos (si es que no existe), y lo escribimos, le damos ademas persimos de lectura y escritura con S_IRURS,S_IWURS

                if (fd_out == -1) // mensaje de error
                {
                        perror("El archivo de salida no se ha abierto correctamente");
                        return(EXIT_FAILURE);
                }

                int dup2_res = dup2(fd_out,STDOUT_FILENO); // intercambiamos la salida estandar por la del fichero que creamos o abrimos.

                if (dup2_res == -1) //mensaje de error
                {
                        perror("Dup2 ha fallado");
                        return(EXIT_FAILURE);
                }

                int res_close = close (fd_out); // cerramos el fichero
                if (res_close == -1) // mensaje de error
                {
                        perror("EL fichero no se ha cerrado bien");
                        return (EXIT_FAILURE);
                }
        }
        
        return (EXIT_SUCCESS);
}

//ejecuta un comando externo o uno simple, notar que no utiliza fork, pues esta funcion solo realiza los cambios de entrada salida y ejecuta, el fork lo haremos en otra funcion.
static void execute_scommand_ext(scommand cmd){
        assert(cmd != NULL && !scommand_is_empty(cmd));

        int redir_in = change_file_desc_in(cmd); //redireccionamos la entrada
        if (redir_in != EXIT_SUCCESS) //mensaje de error
        {
                exit(EXIT_FAILURE);
        }
        
        int redir_out = change_file_desc_out(cmd); // redireccionamos la salida
        if (redir_out != EXIT_SUCCESS)//mensaje de error
        {
                exit(EXIT_FAILURE);
        }
        
        char** str =scommand_to_argv(cmd); // creamos un puntero a "string" donde guardamos el comando
        if (str == NULL)
        {
                perror("calloc");
                exit(EXIT_FAILURE);
        }

        
        execvp(str[0],str); //ejecutamos el comando 


        // Si todo sale bien, esto no deberia ejecutarse
        perror(str[0]);
        
        exit(EXIT_FAILURE);
}


//la funcion ejecuta comandos distinguiendo si son builtin o no
static void exec_scommand(scommand cmd){
        assert(cmd != NULL);

        if (builtin_is_internal(cmd)) //si el comando es builtin se ejecuta
        {
                builtin_run(cmd); //ejecuta el builtin
                exit(EXIT_SUCCESS);
        }
        else if (!scommand_is_empty(cmd)) //si el comando no es vacio ejecuta el comando simple
        {
                execute_scommand_ext(cmd);
        }
        else // si ees vacio no hace nada
        {
                exit(EXIT_SUCCESS);
        }
        assert(false); // esto no deberia de saltar
}


//ejecuta un comando simple,solo uno, notar que esta si utiliza fork y utiliza la funcion de arriba, pues es necesaria para hacerla mas "robusta"
static unsigned int simple_command(pipeline apipe){
        assert(apipe != NULL && pipeline_length(apipe) ==1); //debe haber solo 1 comando

        unsigned int res = 0u;
        if (builtin_is_internal(pipeline_front(apipe)))
        {
                builtin_run(pipeline_front(apipe));
        }
        else
        {
                int pid = fork(); //creamos un proceso 

                if (pid < 0) //mensaje de error
                {
                       perror("fork simple_command");
                       return res;
                }
                else if (pid == 0) //si estamos en el proceso hijo
                {
                       exec_scommand(pipeline_front(apipe)); //ejecuta el comando simple, y redirige entradas y salidas si es necesario.
                }
                else //el padre no hace nada
                {
                       res++; 
                }
        }
        
        assert(res == 1 || res == 0); // res solo puede tomar estos valores
        return res;
}


static unsigned int multiple_command(pipeline apipe){
        assert(apipe != NULL && pipeline_length(apipe) >= 2);
        unsigned int res = 0u;
        bool errors = false;
        unsigned int n_pipelines = pipeline_length(apipe) - 1u; 

        int* file_d = calloc(2 * n_pipelines, sizeof(int)); // Arreglo de todos los eventuales pipes, cada pipe corresponde a un arreglo de tamaño 2 y has n_pipelines
        
        if (file_d == NULL)
        {
                perror("ERROR CALLOC");
                return res;
        }

        for (unsigned int i = 0; i < n_pipelines; i++)
        {
                int res_pipe = pipe(file_d + i *2);

                if (res_pipe < 0) // en caso de error
                {
                        perror("error pipe multiple_command"); // mensaje de error

                        for (unsigned int j = 0; j < 2u * i; j++) //cierro todos los pipe
                        {
                                close(file_d[j]);
                        }
                        
                        free(file_d);
                        file_d = NULL;
                        return res;
                }

        }
         

        unsigned int index = 0;
        while (!pipeline_is_empty(apipe) && !errors)
        {
                int pid = fork();
                if (pid < 0) //si hay error en fork
                {
                        perror("error fork multiple_command");
                        errors = true;
                }
                else if (pid == 0) //si es el hijo
                {
                        if (pipeline_length(apipe) > 1) //si no es el ultimo comando en el pipeline
                        {
                                int res_dup = dup2(file_d[index + 1u],STDOUT_FILENO); //escribe en el pipe a su derecha
                                if (res_dup < 0) // caso de error
                                {
                                        perror("ERROR DUP");
                                        _exit(EXIT_FAILURE);
                                }     
                        }

                        if (index != 0) //si no es el primer comando en el pipeline
                        {
                                int res_dup = dup2(file_d[index - 2u],STDIN_FILENO); //lee del pipe a su izquierda
                                if (res_dup < 0) // caso de error
                                {
                                        perror("ERROR DUP");
                                        _exit(EXIT_FAILURE);
                                }
                                
                        }
                        for (unsigned int  i = 0; i < 2* n_pipelines; i++) //cierro todos los pipe
                        {
                                close(file_d[i]);
                        }
                        
                        exec_scommand(pipeline_front(apipe));    //ejecuto comando          
                }
                else if (pid > 0)//si es el padre
                {
                        pipeline_pop_front(apipe);
                        index = index + 2u;
                        res++;
                        
                }
        }
        
        for (unsigned int  i = 0; i < 2u * n_pipelines; i++)
        {
                close(file_d[i]); //cierro todos los pipe
        }
        
        free(file_d);
        file_d = NULL;

        return res;
}




// la funcion distingue entre si es un pipeline de un comando o multiples comandos.
static unsigned int foreground_pipeline(pipeline apipe){
        assert(apipe != NULL);
        unsigned int length = pipeline_length(apipe); //necesitamos saber si es un pipeline de un comando o 2.
        unsigned int res = 0u;
        if (length == 1u)
        {
                res = simple_command(apipe); //ejecutamos comando simple
        }
        else if (length >= 2u)
        {
                res = multiple_command(apipe); // ejecutamos comando de 2 o mas
        }
        
        assert(apipe != NULL);
        
        return res;
}
   

void execute_pipeline(pipeline apipe){
        assert(apipe != NULL);
        if (pipeline_get_wait(apipe)) //si es foreground entonces entra aca
        {
                unsigned int res = foreground_pipeline(apipe); //ejecuto en foreground
                while (res > 0u) //espero al hijo
                {
                        wait(NULL);
                        res--;
                }
        }
        else{ // si es en background entro aca y creo un proceso hijo
                int pid = fork();
                if (pid < 0) // por si falla el fork
                {
                        perror("ERROR FORK EXECUTE_PIPELINE");
                }
                else if (pid == 0)
                {
                        int background_set = setpgid(0,0); // cambia el process group del hijo
                        if (background_set==-1) 
                        {
                                perror("ERROR SETPGID EXECUTE_PIPELINE");
                        }
                        foreground_pipeline(apipe); //ejecutamos el comando
                        
                        exit(EXIT_SUCCESS);
                } 
                else 
                {       
                        printf("ID de proceso: %i\n", pid); // imprime ID del proceso
                } 
                  
        }
}




