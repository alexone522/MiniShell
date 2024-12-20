#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "parser.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define BUFF 1024

// Declaración de funciones

void hacer_cd(char **argv, int argc);        
void ejecutar_comandos_con_pipe(tline *linea);
void un_mandato(tline *linea);

// Función para ejecutar un solo comando
void un_mandato(tline *linea) {
    // Verificar si el comando tiene permisos de ejecución
    if (access(linea->commands[0].filename, X_OK) != 0) {
        fprintf(stderr, "ERROR: Comando '%s' no encontrado.\n", linea->commands[0].filename);
        return;
    }

    pid_t pid = fork(); // Crear un nuevo proceso
    if (pid < 0) { // Si hay un error al hacer el fork
        fprintf(stderr, "ERROR al crear el hijo: %s\n", strerror(errno));
        return;
    }

    if (pid == 0) { // Proceso hijo
        signal(SIGINT, SIG_DFL); // Restaurar el comportamiento de la señal SIGINT
        signal(SIGQUIT, SIG_DFL); // Restaurar el comportamiento de la señal SIGQUIT

        // Redirección de entrada desde un archivo si se especifica
        if (linea->redirect_input != NULL) {
            int fd_in = open(linea->redirect_input, O_RDONLY);
            if (fd_in < 0) {
                fprintf(stderr, "ERROR al abrir '%s' para lectura: %s\n", linea->redirect_input, strerror(errno));
                exit(1);
            }
            dup2(fd_in, STDIN_FILENO); // Redirigir entrada estándar
            close(fd_in);
        }

        // Redirección de salida a un archivo si se especifica
        if (linea->redirect_output != NULL) {
            int fd_out = open(linea->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out < 0) {
                fprintf(stderr, "ERROR al abrir '%s' para escritura: %s\n", linea->redirect_output, strerror(errno));
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO); // Redirigir salida estándar
            close(fd_out);
        }

        // Ejecutar el comando
        execvp(linea->commands[0].filename, linea->commands[0].argv);
        fprintf(stderr, "ERROR: No se pudo ejecutar el comando '%s': %s\n", linea->commands[0].filename, strerror(errno));
        exit(1);
    }

    // Esperar a que el proceso hijo termine
    waitpid(pid, NULL, 0);
}

// Función para ejecutar comandos con pipes
void ejecutar_comandos_con_pipe(tline *linea) {
    int num_pipes = linea->ncommands - 1; // Número de pipes necesarios
    int pipe_fd[num_pipes][2]; // Array para almacenar los descriptores de las tuberías
    pid_t pids[linea->ncommands]; // Array para almacenar los PIDs de los procesos hijos

    // Crear las tuberías necesarias
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipe_fd[i]) == -1) {
            fprintf(stderr, "ERROR al crear la tubería %d: %s\n", i, strerror(errno));
            return;
        }
    }

    // Crear un proceso para cada comando
    for (int i = 0; i < linea->ncommands; i++) {
        pids[i] = fork(); // Crear un nuevo proceso
        if (pids[i] < 0) { // Si hay un error al hacer el fork
            fprintf(stderr, "ERROR al crear el proceso %d: %s\n", i, strerror(errno));
            return;
        }

        if (pids[i] == 0) { // Proceso hijo
            signal(SIGINT, SIG_DFL); // Restaurar el comportamiento de la señal SIGINT
            signal(SIGQUIT, SIG_DFL); // Restaurar el comportamiento de la señal SIGQUIT

            // Redirección de entrada para los comandos intermedios
            if (i > 0) {
                dup2(pipe_fd[i - 1][0], STDIN_FILENO); // Leer desde el pipe anterior
            }
            // Redirección de salida para los comandos intermedios
            if (i < num_pipes) {
                dup2(pipe_fd[i][1], STDOUT_FILENO); // Escribir en el pipe siguiente
            }

            // Cerrar todas las tuberías
            for (int j = 0; j < num_pipes; j++) {
                close(pipe_fd[j][0]);
                close(pipe_fd[j][1]);
            }

            // Redirección de entrada si es el primer comando
            if (i == 0 && linea->redirect_input != NULL) {
                int fd_in = open(linea->redirect_input, O_RDONLY);
                if (fd_in < 0) {
                    fprintf(stderr, "ERROR al abrir '%s' para lectura: %s\n", linea->redirect_input, strerror(errno));
                    exit(1);
                }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }

            // Redirección de salida si es el último comando
            if (i == num_pipes && linea->redirect_output != NULL) {
                int fd_out = open(linea->redirect_output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_out < 0) {
                    fprintf(stderr, "ERROR al abrir '%s' para escritura: %s\n", linea->redirect_output, strerror(errno));
                    exit(1);
                }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }

            // Ejecutar el comando
            execvp(linea->commands[i].filename, linea->commands[i].argv);
            fprintf(stderr, "ERROR al ejecutar el comando '%s': %s\n", linea->commands[i].filename, strerror(errno));
            exit(1);
        }
    }

    // Cerrar todas las tuberías en el proceso principal
    for (int i = 0; i < num_pipes; i++) {
        close(pipe_fd[i][0]);
        close(pipe_fd[i][1]);
    }

    // Esperar a que todos los procesos hijos terminen
    for (int i = 0; i < linea->ncommands; i++) {
        waitpid(pids[i], NULL, 0);
    }
}

//Funcion "cd"
void hacer_cd(char **argv, int argc) {
    char *path = NULL;

    if (argc == 1) { // Sin argumentos: cambiar al directorio HOME
        path = getenv("HOME");
        if (path == NULL) {
            fprintf(stderr, "Error: No se pudo acceder a la variable de entorno HOME.\n");
            return;
        }
    } else if (argc == 2) { // Con un argumento: cambiar al directorio especificado
        path = argv[1];
    } else { // Más de un argumento: error
        fprintf(stderr, "Error: Demasiados argumentos para 'cd'. Uso: cd [directorio]\n");
        return;
    }

    // Intentar cambiar al directorio especificado
    if (chdir(path) != 0) {
        fprintf(stderr, "Error: No se pudo cambiar al directorio '%s'. %s\n", path, strerror(errno));
        return;
    }
}

// Función principal del shell
int main() {
    char input[BUFF];

    while (1) {
        printf("msh> ");
        fflush(stdout);

        // Leer la entrada del usuario
        if (fgets(input, BUFF, stdin) == NULL) {
            break; // Salir si se encuentra EOF
        }

        tline *linea = tokenize(input); // Tokenizar la entrada

        // Verificar si la línea contiene comandos
        if (!linea || !linea->commands || !linea->commands[0].filename) {
            continue;
        }

        // Si el comando es 'cd', llamamos a la función hacer_cd
        if(strcmp(linea->commands[0].argv[0], "cd") == 0){
            hacer_cd(linea->commands[0].argv, linea->commands[0].argc);
            continue;
        }

        // Ejecutar el comando de uno o más comandos con pipe
        if (linea->ncommands == 1) {
            un_mandato(linea);
        } else {
            ejecutar_comandos_con_pipe(linea);
        }
    }

    return 0;
}
