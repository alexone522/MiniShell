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
        //Caso de 2 mandatos    
        } else if (linea->ncommands == 2) {
            int pipe_fd[2]; // Descriptores de la tubería
            pid_t pid1, pid2; // Identificadores de los procesos hijos
            int status1, status2; // Estados para los hijos

            // Crear la tubería
            if (pipe(pipe_fd) == -1) {
                fprintf(stderr, "ERROR al crear la tubería. Error: %s\n", strerror(errno));
                continue; // Saltar al siguiente ciclo
            }

            // Crear el primer proceso (comando izquierdo)
            pid1 = fork();
            if (pid1 < 0) {
                fprintf(stderr, "ERROR al crear el primer hijo. Error: %s\n", strerror(errno));
                close(pipe_fd[0]);
                close(pipe_fd[1]);
                continue;
            }

            if (pid1 == 0) { // Proceso hijo 1
                signal(SIGINT, SIG_DFL);  // Restaurar señales
                signal(SIGQUIT, SIG_DFL);

                close(pipe_fd[0]); // Cerrar el extremo de lectura de la tubería
                if (dup2(pipe_fd[1], STDOUT_FILENO) == -1) { // Redirigir stdout
                    fprintf(stderr, "ERROR al redirigir la salida estándar. Error: %s\n", strerror(errno));
                    exit(1);
                }
                close(pipe_fd[1]); // Cerrar el extremo de escritura (ya redirigido)

                // Ejecutar el primer comando
                execvp(linea->commands[0].filename, linea->commands[0].argv);

                // Si execvp falla
                fprintf(stderr, "ERROR al ejecutar el comando '%s'. Error: %s\n",
                    linea->commands[0].filename, strerror(errno));
                    exit(1);
            }

            // Crear el segundo proceso (comando derecho)
            pid2 = fork();
            if (pid2 < 0) {
                fprintf(stderr, "ERROR al crear el segundo hijo. Error: %s\n", strerror(errno));
                close(pipe_fd[0]);
                close(pipe_fd[1]);
                continue;
            }

            if (pid2 == 0) { // Proceso hijo 2
                signal(SIGINT, SIG_DFL);  // Restaurar señales
                signal(SIGQUIT, SIG_DFL);

                close(pipe_fd[1]); // Cerrar el extremo de escritura de la tubería
                if (dup2(pipe_fd[0], STDIN_FILENO) == -1) { // Redirigir stdin
                    fprintf(stderr, "ERROR al redirigir la entrada estándar. Error: %s\n", strerror(errno));
                    exit(1);
                }
                close(pipe_fd[0]); // Cerrar el extremo de lectura (ya redirigido)

                // Ejecutar el segundo comando
                execvp(linea->commands[1].filename, linea->commands[1].argv);

                // Si execvp falla
                fprintf(stderr, "ERROR al ejecutar el comando '%s'. Error: %s\n",
                linea->commands[1].filename, strerror(errno));
                exit(1);
            }

            // Proceso padre
            close(pipe_fd[0]); // Cerrar ambos extremos de la tubería
            close(pipe_fd[1]);

            // Esperar a ambos procesos hijos
            if (waitpid(pid1, &status1, 0) == -1) {
                fprintf(stderr, "ERROR al esperar al primer hijo. Error: %s\n", strerror(errno));
            }
            if (waitpid(pid2, &status2, 0) == -1) {
                fprintf(stderr, "ERROR al esperar al segundo hijo. Error: %s\n", strerror(errno));
            }

            // Verificar los estados de los hijos
            if (WIFEXITED(status1) && WEXITSTATUS(status1) != 0) {
                fprintf(stderr, "El comando '%s' terminó con código de salida %d.\n",
                linea->commands[0].filename, WEXITSTATUS(status1));
            }
            if (WIFEXITED(status2) && WEXITSTATUS(status2) != 0) {
                fprintf(stderr, "El comando '%s' terminó con código de salida %d.\n",
                linea->commands[1].filename, WEXITSTATUS(status2));
            }
        }

        // Caso de más de dos mandatos (no implementado)
        else {
        }
    }

    return 0;
}
