#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "parser.h"

#define BUFF 1024

int main(){    
    printf("msh> "); //prompt
    char input[BUFF];//buffer
    

    //Ignora las señales:
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    while(fgets(input, BUFF, stdin)){ //lee de teclado en cada iteración
        tline * linea = tokenize(input);//tokeniza

        if(linea->ncommands == 0){ 
            //si no tiene comandos imprime el prompt y espera otro comando
            printf("msh> ");
            continue;
        }
        if(linea->ncommands == 1){
            //caso de 1 mandato
        } else if (linea->ncommands == 2){
            //2 mandatos
        } else {
            //+ de 2 mandatos
        }
    }
    
    return 0;
}