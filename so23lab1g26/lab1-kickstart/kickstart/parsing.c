#include <stdlib.h>
#include <stdbool.h>

#include "parsing.h"
#include "parser.h"
#include "command.h"

#include <assert.h>

bool first_command; //variable para trackear si un comando siendo parseado es el primero en el pipeline

static scommand parse_scommand(Parser p) {
    
    /* Devuelve NULL cuando hay un error de parseo */

    scommand new_scommand = scommand_new();

    arg_kind_t type = ARG_NORMAL; 
    bool error = false;

    parser_skip_blanks(p);
    char *arg = parser_next_argument(p, &type);
    error = (arg==NULL && type!=ARG_NORMAL);
    while(arg!=NULL) {
        // agrega en la parte del comando correspondiente de scommand segun el tipo
        if (type==ARG_NORMAL) {
            scommand_push_back(new_scommand, arg);
        }
        else if (type==ARG_INPUT) {
            scommand_set_redir_in(new_scommand, arg);
        }
        else if (type==ARG_OUTPUT) {
            scommand_set_redir_out(new_scommand, arg);
        } 
        parser_skip_blanks(p);
        type = ARG_NORMAL;
        arg = parser_next_argument(p, &type); 
        error = arg==NULL && type!=ARG_NORMAL; //salta error si intenta agregar un input o output con argumento NULL
    }
    
    // Destruye lo construido si no tiene comando o salto un error
    if (scommand_is_empty(new_scommand) || error) {
        if (error && first_command) {
            printf("-mybash: syntax error\n");
        }
        new_scommand = scommand_destroy(new_scommand);
    }
    first_command = false;
    return new_scommand;
}

//Esta funcion devuelve una lista de comandos y recibe como argumento la entrada de consola en forma de parser
pipeline parse_pipeline(Parser p) {
    assert(p!=NULL && !parser_at_eof(p));


    pipeline result = pipeline_new();
    scommand cmd = NULL;
    bool error = false, another_pipe=true;
    bool is_background = false;
    bool garbage;
    first_command = true;

    cmd = parse_scommand(p);
    error = (cmd==NULL); /* Comando inválido al empezar */
    /* Agrega comando a pipeline, se fija si hay un pipe y agrega su respectivo comando si lo hay*/
    while (another_pipe && !error) {
        pipeline_push_back(result, cmd);  
        parser_skip_blanks(p);
        parser_op_pipe(p, &another_pipe);
        if (another_pipe) {
            cmd = parse_scommand(p);
            error = (cmd==NULL);
            if (error) {
                printf("-mybash: syntax error\n");
            }
        }
    }

    /*Fijarse si ex background*/    
    parser_op_background(p, &is_background);
    pipeline_set_wait(result, !is_background);

    parser_garbage(p, &garbage); //el garbage se fija que no haya cosas entre un posible & y el fin de linea
    if (!error && garbage) {
        char *last_garbage = parser_last_garbage(p);
        printf("-mybash: syntax error cerca de token inesperado '%.1s'\n", last_garbage);
    }
    
    /*Ante cualquier error, destruye el pipeline construido*/
    if (error || garbage) {
        result = pipeline_destroy(result);
    }

    /* Tolerancia a espacios posteriores */
    /* Consumir todo lo que hay inclusive el \n */
    /* Si hubo error, hacemos cleanup */
    return result; 
}
