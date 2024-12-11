#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "parser.h"

#define BUFF 1024

int main(){
    
    printf("msh> "); //prompt
    char input[BUFF];
    

    //Ignora las señales:
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    
    return 0;
}