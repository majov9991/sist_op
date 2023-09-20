#include <stdio.h> 
#include <glib.h>
#include <assert.h>
#include <stdlib.h>

#include "command.h"
#include "strextra.h"

/********** COMANDO SIMPLE **********/

/* Estructura correspondiente a un comando simple.
 * Es una 3-upla del tipo ([char*], char* , char*).
 */


struct scommand_s {
    GSList * args; // es una secuencia de palabras en donde la primera es el comando y las siguientes son sus argumentos
    char * redir_in; // redirector 2 
    char * redir_out; // redirector 1
};

scommand scommand_new(void){
    
	scommand scommand_new =malloc(sizeof(struct scommand_s)); 

	scommand_new-> args=NULL;
	scommand_new->redir_in=NULL;
	scommand_new->redir_out=NULL;


	assert(scommand_new != NULL && scommand_is_empty (scommand_new) 
						        && scommand_get_redir_in  (scommand_new)  == NULL 
	                            && scommand_get_redir_out (scommand_new) == NULL);
    return scommand_new;    
}

static void free_data(gpointer data) {
	// Se define free_data para que libere toda la infomación que tiene cada elemento de tipo GSList. 
	// Necesario para usar función g_slist_free_full	
	g_free(data);
}

scommand scommand_destroy(scommand self){
	assert(self!=NULL);

		//Libero la lista enlazada generada por GSlist
	//En este caso se utiliza la funcion g_slist_free_full a diferencia de g_slist_free ya que cada nodo contiene datos dinamicos
	//g_slist_free_full(self->args, g_free);
	g_slist_free_full(self->args,free_data);
	self->args=NULL;
	
	free(self->redir_in);
	self->redir_in=NULL;
	free(self->redir_out);
	self->redir_out=NULL;	
	
	free(self);  
	self=NULL;
	

	assert(self==NULL);
	return self;
}


//Agrega por detrás una cadena a la secuencia de cadenas.
void scommand_push_back(scommand self, char * argument){
		assert(self!=NULL && argument!=NULL);

		self->args=g_slist_append(self->args,argument);

		assert(!scommand_is_empty(self));		
}


//Quita la cadena de adelante de la secuencia de cadenas
void scommand_pop_front(scommand self){
	assert(self!=NULL && !scommand_is_empty(self));
	
	//obtenenmos la referencia al primer nodo
	GSList *first_node = self-> args;

	g_free(first_node->data);
	//Desvinculamos el primer nodo de la lista enlazada
    self->args = g_slist_delete_link(self->args,first_node);



    //Asignar el valor null al puntero hacia el primer nodo
    first_node=NULL;

}



void scommand_set_redir_in(scommand self, char * filename){
	assert(self!=NULL);
	if (self->redir_in!=NULL) {
		free(self->redir_in); //libero la memoria que ya estaba
	}
	self->redir_in= filename;
}	


void scommand_set_redir_out(scommand self, char * filename){
	assert(self!=NULL);
	
	if (self->redir_out!=NULL){
		free(self->redir_out); //libero la memoria que ya estaba
	}
	self->redir_out=filename;
}

bool scommand_is_empty(const scommand self){
	assert(self!=NULL);

	return  self->args==NULL;	
}


unsigned int scommand_length(const scommand self){
	assert(self!=NULL);

	//obtengo el tamaño de la lista atraves de la funcion de GSList
	unsigned int length = g_slist_length(self->args); 

	assert((length == 0) == scommand_is_empty(self)); 
	return length;
}



char * scommand_front(const scommand self){
	assert(self!=NULL && !scommand_is_empty(self));

	char *firstElement = (char*)self->args->data;

	assert(firstElement!=NULL); 
	return firstElement;
}



char * scommand_get_redir_in(const scommand self){
	assert(self!=NULL);

	return self->redir_in;
}


char * scommand_get_redir_out(const scommand self){
	assert(self!=NULL);

	return self->redir_out;
}


//merge str1 con str2 devolviendo el resultado en str1, libera la cadena vieja de str1 que quedo suelta
static char *self_strmerge(char *str1, char *str2) {
	
	char *aux_pointer = str1;
	str1 = strmerge(str1,str2);
	free(aux_pointer);
	return str1;
}

char* scommand_to_string(const scommand self) {
    assert(self != NULL);

    GSList* lista = self->args; // en lista guardo el comando y los argumentos
    char* print_scommand = strdup("");
    // strdup duplica una cadena. 

    if (lista != NULL) {
        
        while (lista != NULL) {

        	//actualizo el puntero de print_scommand
	        print_scommand= self_strmerge(print_scommand, g_slist_nth_data(lista, 0u));
	        //en cada iteracion libero la memoria ocupada por el puntero aux
            print_scommand = self_strmerge(print_scommand, " ");
            lista = g_slist_next(lista);
        }
    }

    // si hay redireccion de salida entonces agrego un > seguido del redirector 
    if (self->redir_out != NULL) {
        print_scommand = self_strmerge(print_scommand, " > ");
	    print_scommand = self_strmerge(print_scommand, self->redir_out);
    }

    //  // si hay redireccion de entrada entonces agrego un > seguido del redirector 
    if (self->redir_in != NULL) {
        print_scommand= self_strmerge(print_scommand, " < ");
	    print_scommand = self_strmerge(print_scommand, self->redir_in);
    }


    assert(print_scommand == NULL || scommand_is_empty(self) ||
           scommand_get_redir_in(self) == NULL ||
           scommand_get_redir_out(self) == NULL || strlen(print_scommand) > 0);

    return (print_scommand);
}


/********** COMANDO PIPELINE **********/

/* Estructura correspondiente a un comando pipeline.
 * Es un 2-upla del tipo ([scommand], bool)
 */

struct pipeline_s {
    GSList * scmds;
    bool wait;
};



pipeline pipeline_new(void){
	
	pipeline new_pipeline=malloc(sizeof(struct pipeline_s));

	new_pipeline->scmds=NULL;	
	new_pipeline->wait=true; 

	assert(new_pipeline != NULL && pipeline_is_empty(new_pipeline)  && pipeline_get_wait(new_pipeline));
	return new_pipeline;
}

static void aux_pipeline_destroy(gpointer data) {
	// Definir free_data para que libere toda la infomación que tienen cada elemento de tipo GSList. 
	// Necesario para usar función g_slist_free_full	
	data=scommand_destroy(data);
	assert(data==NULL);
}


pipeline pipeline_destroy(pipeline self){
	assert(self!=NULL);

	g_slist_free_full(self->scmds,aux_pipeline_destroy); 
	free(self);
	self=NULL;
	
	assert(self==NULL);
	return self;
}


//Agrega por detrás un comando simple a la secuencia
void pipeline_push_back(pipeline self, scommand sc){
	assert(self!=NULL && sc!=NULL);

	//agrego el comando simple al final de la secuencia
	self->scmds=g_slist_append(self->scmds,sc);

	assert(!pipeline_is_empty(self));
}


//Quita el comando simple de adelante de la secuencia
void pipeline_pop_front(pipeline self){
	assert(self!=NULL && !pipeline_is_empty(self));
	//obtenenmos la referencia al primer nodo
	GSList *first_node = self->scmds;

	first_node->data=scommand_destroy(first_node->data);

	//Desvinculamos el primer nodo de la lista enlazada
    self->scmds = g_slist_delete_link(self->scmds,first_node);

    //Asignar el valor nll al puntero hacia el primer nodo
    first_node=NULL;
}

//Define si el pipeline tiene que esperar o no.
void pipeline_set_wait(pipeline self, const bool w){
	assert(self!=NULL);
	
	self->wait = w; 
}



bool pipeline_is_empty(const pipeline self){
	assert(self!=NULL);

	return self->scmds==NULL;
}

unsigned int pipeline_length(const pipeline self){
	assert(self!=NULL);

	//obtengo el tamaño de la lista atraves de la funcion de GSList
	unsigned int pipeline_length = g_slist_length(self->scmds); 

	assert((pipeline_length==0) == pipeline_is_empty(self));

	return pipeline_length;

}


//Devuelve el comando simple de adelante de la secuencia
scommand pipeline_front(const pipeline self){
	assert(self!=NULL && !pipeline_is_empty(self)); 

	scommand first_scommand=self->scmds->data;

	assert(first_scommand != NULL);
	return first_scommand;
}


bool pipeline_get_wait(const pipeline self){
	assert(self!=NULL);

	return self->wait;
}


char * pipeline_to_string(const pipeline self){
	assert(self!=NULL);

	//Creo un puntero a una cadena de caracteres que me va a servir para contener la representacion del pipeline	
	//Inicialmente es una cadena vacia
	//strdup() reserva memoria y tiene que ser liberada
	char * print_pipeline=strdup("");
	GSList *aux_pointer = self->scmds;

	//se asegura de que la lista de punteros a scommands exista
	if (aux_pointer!=NULL){
		
		while(aux_pointer!=NULL){
			char *cmd = scommand_to_string(aux_pointer->data);
			print_pipeline=self_strmerge(print_pipeline,cmd);
			free(cmd);

			if (aux_pointer->next!=NULL){
				print_pipeline=self_strmerge(print_pipeline," | ");
			}
			aux_pointer= g_slist_next(aux_pointer); //se pasa al siguiente nodo 				
		}

		//Si es false se agrega el &
		if (!pipeline_get_wait(self)){

			
			print_pipeline=self_strmerge(print_pipeline," & ");
		}

	}
	assert(pipeline_is_empty(self) || pipeline_get_wait(self) || strlen(print_pipeline)>0);

	return print_pipeline;
}



