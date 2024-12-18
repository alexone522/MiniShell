#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "parser.h"
#include <sys/types.h>  // Para pid_t
#include <unistd.h>     // Para fork, execvp
#include <sys/wait.h>   // Para waitpid
#include <string.h>     // Para strerror
#include <errno.h>      // Para la variable errno
#include <fcntl.h>      ///2/ Para redirección de archivos

#define BUFF 1024

// Prototipos de funciones no implementadas
void hacer_cd(tline *linea);        // Implementar comando `cd`
void exec_background(tline *linea); // Implementar procesos en background
void jobs();                        // Implementar comando `jobs`
void fg(pid_t pid);                 // Implementar comando `fg`
void mas_mandatos(tline *linea);    // Implementar manejo de más de dos mandatos

void un_mandato(tline *linea) {
    if (linea->commands[0].filename == NULL) {
        fprintf(stderr, "Error: comando inválido o nulo.\n");
        return;
    }

    pid_t pid = fork();
    int status;

    if (pid < 0) {
        fprintf(stderr, "ERROR al crear el hijo. Error: %s\n", strerror(errno));
        return;
    }

    if (pid == 0) { // Proceso hijo
        // Restaurar señales predeterminadas
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        
        ///2/ Manejo de redirecciones de entrada y salida
        ///2/ Redirección de entrada
        if (linea->redirect_input != NULL) {
            int fd_in = open(linea->redirect_input, O_RDONLY);
            if (fd_in < 0) {
                fprintf(stderr, "ERROR al abrir el archivo de entrada '%s': %s\n",
                        linea->redirect_input, strerror(errno));
                exit(1);
            }
            dup2(fd_in, STDIN_FILENO); // Redirigir entrada estándar
            close(fd_in);
        }
        ///2/ Redirección de salida
        if (linea->redirect_output != NULL) {
            int fd_out = open(linea->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out < 0) {
                fprintf(stderr, "ERROR al abrir el archivo de salida '%s': %s\n",
                        linea->redirect_output, strerror(errno));
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO); // Redirigir salida estándar
            close(fd_out);
        }

        // Ejecutar el comando
        execvp(linea->commands[0].filename, linea->commands[0].argv);

        // Si execvp falla, imprimir error y salir
        fprintf(stderr, "ERROR al ejecutar el comando '%s': %s\n",
                linea->commands[0].filename, strerror(errno));
        exit(1);
    } else { // Proceso padre
        wait(&status); // Esperar al proceso hijo

        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0) {
                fprintf(stderr, "El comando '%s' terminó con código de salida %d.\n",
                        linea->commands[0].filename, exit_code);
            }
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "El comando '%s' fue terminado por la señal %d.\n",
                    linea->commands[0].filename, WTERMSIG(status));
        } else {
            fprintf(stderr, "El hijo no terminó normalmente.\n");
        }
    }
}

void dos_mandatos(tline *linea) {
    fprintf(stderr, "Manejo de dos mandatos aún no implementado.\n");
}

int main() {
    char input[BUFF]; // Buffer para la entrada del usuario
    int status;       // Variable para guardar el estado del proceso hijo

    // Ignorar señales SIGINT y SIGQUIT en la shell
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    while (1) {
        printf("msh> ");
        fflush(stdout);

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

        // Validar que el primer comando no sea nulo
        if (linea->commands[0].filename == NULL) {
            fprintf(stderr, "Error: comando inválido o nulo.\n");
            continue;
        }

        // Manejar diferentes casos según el número de mandatos
        if (linea->ncommands == 1) {
            un_mandato(linea);
        } else if (linea->ncommands == 2) {
            dos_mandatos(linea);
        } else {
            fprintf(stderr, "Manejo de más de dos mandatos aún no implementado.\n");
        }
    }

    return 0;
}

// Funciones no implementadas
void hacer_cd(tline *linea) {
    fprintf(stderr, "El comando 'cd' aún no está implementado.\n");
}

void exec_background(tline *linea) {
    fprintf(stderr, "Procesos en background aún no están implementados.\n");
}

void jobs() {
    fprintf(stderr, "El comando 'jobs' aún no está implementado.\n");
}

void fg(pid_t pid) {
    fprintf(stderr, "El comando 'fg' aún no está implementado.\n");
}

void mas_mandatos(tline *linea) {
    fprintf(stderr, "El manejo de más de dos mandatos aún no está implementado.\n");
}
