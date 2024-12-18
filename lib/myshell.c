#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "parser.h"
#include <dirent.h>     //Para DIR, opendir, closedir
#include <sys/types.h>  // Para pid_t
#include <unistd.h>     // Para fork, execvp
#include <sys/wait.h>   // Para waitpid
#include <string.h>     // Para strerror
#include <errno.h>      // Para la variable errno
#include <fcntl.h>      ///2/ Para redirección de archivos

#define BUFF 1024
#define MAX_PROCESOS_BG 100

//Variables globales para background:
pid_t bg_pids[MAX_PROCESOS_BG];
char *bg_commands[MAX_PROCESOS_BG];
int bg_count = 0;

// Prototipos de funciones no implementadas
void hacer_cd(char **argv, int argc);        // Implementar comando `cd`
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
            if(strcmp(linea->commands->argv[0], "cd" == 0)){
                hacer_cd(linea->commands[0].argv, linea->commands[0].argc);
                continue;
            } else if(strcmp(linea->commands->argv[0], "cd" == 0)) {
                jobs();
            } else{
                un_mandato(linea);
            }
        } else if (linea->ncommands == 2) {
            dos_mandatos(linea);
        } else {
            fprintf(stderr, "Manejo de más de dos mandatos aún no implementado.\n");
        }
    }

    return 0;
}

// Funciones no implementadas
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


void exec_background(tline *linea) {
    fprintf(stderr, "Procesos en background aún no están implementados.\n");
}

// Función para listar procesos en background
void jobs() {
    printf("Procesos en background:\n");
    for (int i = 0; i < bg_count; i++) {
        int status;
        pid_t result = waitpid(bg_pids[i], &status, WNOHANG); // Verificar el estado del proceso
        if (result == 0) {
            // Proceso sigue en ejecución
            printf("[%d] Running\t%s\n", bg_pids[i], bg_commands[i]);
        } else if (result > 0) {
            // Proceso terminó
            printf("[%d] Finished\t%s\n", bg_pids[i], bg_commands[i]);
            remove_background_process(bg_pids[i]); // Eliminarlo del arreglo
        }
    }
}

void fg(pid_t pid) {
    fprintf(stderr, "El comando 'fg' aún no está implementado.\n");
}

void mas_mandatos(tline *linea) {
    fprintf(stderr, "El manejo de más de dos mandatos aún no está implementado.\n");
}