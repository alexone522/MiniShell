#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "parser.h"
#include <sys/types.h>  // Para pid_t
#include <unistd.h>     // Para fork, execvp
#include <sys/wait.h>   // Para waitpid
#include <string.h>     // Para strerror
#include <errno.h>      // Para la variable errno

#define BUFF 1024

int main() {    
    char input[BUFF]; // Buffer para la entrada del usuario
    int status;       // Variable para guardar el estado del proceso hijo

    // Ignorar señales SIGINT y SIGQUIT en la shell
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    while (1) { // Bucle principal de la shell
        printf("msh> "); // Mostrar el prompt
        fflush(stdout);  // Asegurarse de que se imprime inmediatamente

        // Leer entrada del usuario
        if (fgets(input, BUFF, stdin) == NULL) {
            printf("\n");
            break; // Salir si se encuentra EOF (Ctrl+D)
        }

        // Tokenizar la entrada del usuario
        tline *linea = tokenize(input);

        // Ignorar señales en cada iteración
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);

        // Si no hay comandos, continuar al siguiente ciclo
        if (linea == NULL || linea->ncommands == 0) {
            continue;
        }

        // Caso de un único mandato
        if (linea->ncommands == 1) {
            pid_t pid = fork(); // Crear un proceso hijo

            if (pid < 0) { // Error al crear el proceso
                fprintf(stderr, "ERROR al crear el hijo. Error: %s\n", strerror(errno));
                continue; // Continuar con el siguiente ciclo en caso de error
            }

            if (pid == 0) { // Proceso hijo
                // Restaurar señales predeterminadas en el hijo
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);

                // Ejecutar el comando
                execvp(linea->commands[0].filename, linea->commands[0].argv);

                // Si execvp falla, imprimir error y salir
                fprintf(stderr, "ERROR al ejecutar el comando '%s'. Error: %s\n",
                        linea->commands[0].filename, strerror(errno));
                exit(1);
            } else { // Proceso padre
                wait(&status); // Esperar al proceso hijo

                // Verificar el estado del proceso hijo
                if (WIFEXITED(status)) {
                    int exit_code = WEXITSTATUS(status);
                    if (exit_code != 0) {
                        fprintf(stderr, "El comando '%s' terminó con código de salida %d.\n",
                                linea->commands[0].filename, exit_code);
                    }
                } else {
                    fprintf(stderr, "El hijo no terminó normalmente.\n");
                }
            }
        }
        // Caso de dos mandatos (no implementado)
        else if (linea->ncommands == 2) {
        }
        // Caso de más de dos mandatos (no implementado)
        else {
        }
    }

    return 0;
}
