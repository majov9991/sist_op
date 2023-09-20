#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>

#include "command.h"
#include "execute.h"
#include "parser.h"
#include "parsing.h"
#include "builtin.h"


extern bool quit_bash;

static void show_prompt(void) {
    size_t size;
    size = pathconf(".", _PC_PATH_MAX);
    char *location = malloc(size);
    location = getcwd(location,size);
    if (location != NULL) {
        printf ("%s mybash> ", location);
    } else {
        perror("getcwd error");
    }
    free(location);
    fflush (stdout);
}

int main(int argc, char *argv[]) {
    pipeline pipe;
    Parser input;
    bool quit = false;
    input = parser_new(stdin);
    while (!quit) {
        waitpid(-1, NULL, 0);
        show_prompt();
        pipe = parse_pipeline(input);

        quit = parser_at_eof(input);
        
        if(pipe != NULL){
            execute_pipeline(pipe);
            pipe = pipeline_destroy(pipe); 
            // quit_bash == true cuando se hace run_exit
            quit = quit || quit_bash; 
        }
    }
    /* patch para memory leak de 1 byte en parser.h
    esta comentado porque deberia ser responsabilidad del TAD, puede causar double free si 
    se arregla el memory leak en el TAD Parser
    char *last_garbage = parser_last_garbage(input);
    free(last_garbage);
    */
    printf("\n");
    input = parser_destroy(input);

    return EXIT_SUCCESS;
}

