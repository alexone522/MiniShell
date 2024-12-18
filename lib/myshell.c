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

void hacer_cd(tline *linea);
void un_mandato(tline *linea);
void dos_mandatos(tline *linea);
void mas_mandatos(tline *linea);
void exec_background(tline *line);
void jobs();
void fg(pid_t pid);


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
            un_mandato(linea);

        //Caso de 2 mandatos    
        } else if (linea->ncommands == 2) {
            dos_mandatos(linea);
        }

        // Caso de más de dos mandatos (no implementado)
        else {
            mas_mandatos(linea);
        }
    }

    return 0;
}

void exec_background(tline *line) {
    pid_t bg_pid = fork();

    if (bg_pid < 0) {
        fprintf(stderr, "Error al crear el proceso en background: %s\n", strerror(errno));
        exit(1);
    } else if (bg_pid == 0) { // Proceso hijo
        signal(SIGINT, SIG_DFL);  // Restaurar señales
        signal(SIGQUIT, SIG_DFL);

        if (line->ncommands == 1) {
            execvp(line->commands[0].argv[0], line->commands[0].argv);
        } else {
            fprintf(stderr, "No se soportan múltiples comandos aún.\n");
            exit(1);
        }

        fprintf(stderr, "Error al ejecutar el comando");
        exit(1);
    } else { // Proceso padre
        printf("[Proceso en background iniciado: PID %d]\n", bg_pid);
    }
}

void hacer_cd(tline *linea){
    char *path = NULL;

    // Si no se pasa argumento, usar el directorio HOME
    if (linea->commands[0].argc == 1) {
        path = getenv("HOME"); // Obtener la variable HOME
        if (path == NULL) {
            fprintf(stderr, "Error: No se pudo acceder a la variable de entorno HOME.\n");
            return;
        }
    } else if (linea->commands[0].argc == 2) {
        // Si hay un argumento, usarlo como path
        path = linea->commands[0].argv[1];
    } else {
        fprintf(stderr, "Error: Demasiados argumentos para 'cd'. Uso: cd [directorio]\n");
        return;
    }

    // Intentar cambiar al directorio especificado
    if (chdir(path) != 0) {
        fprintf(stderr, "'%s': Error. %s\n", path, strerror(errno));
    }
}

void un_mandato(tline *linea){
    pid_t pid = fork(); // Crear un proceso hijo
    int status;

    if (pid < 0) { // Error al crear el proceso
        fprintf(stderr, "ERROR al crear el hijo. Error: %s\n", strerror(errno));
        return; // Continuar con el siguiente ciclo en caso de error
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

void dos_mandatos(tline *linea){
    int pipe_fd[2]; // Descriptores de la tubería
    pid_t pid1, pid2; // Identificadores de los procesos hijos
    int status1, status2; // Estados para los hijos

    // Crear la tubería
    if (pipe(pipe_fd) == -1) {
        fprintf(stderr, "ERROR al crear la tubería. Error: %s\n", strerror(errno));
        return; // Saltar al siguiente ciclo
    }

    // Crear el primer proceso (comando izquierdo)
    pid1 = fork();
    if (pid1 < 0) {
        fprintf(stderr, "ERROR al crear el primer hijo. Error: %s\n", strerror(errno));
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return;
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
        return;
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

void mas_mandatos(tline *linea){
    fprintf(stderr, "El manejo de más de dos mandatos no está implementado.\n");
}

void jobs() {
    int status;
    pid_t pid;

    printf("Procesos en background:\n");
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("PID %d ha terminado.\n", pid);
    }
}

void fg(pid_t pid) {
    if (pid <= 0) {
        fprintf(stderr, "Debe especificar un PID válido para fg.\n");
        return;
    }

    printf("Moviendo PID %d al primer plano...\n", pid);
    int status;
    if (waitpid(pid, &status, 0) == -1) { // Esperamos al proceso
        perror("Error al traer el proceso al primer plano");
    } else {
        printf("Proceso PID %d finalizado.\n", pid);
    }
}

