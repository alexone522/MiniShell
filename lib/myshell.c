#include <stdio.h>      //printf, fprintf, fflush
#include <stdlib.h>     //malloc, free, exit, getenv
#include <signal.h>     //signal
#include "parser.h"     //tokenize
#include <sys/types.h>  // pid_t
#include <unistd.h>     //fork, execvp, chdir, getpid, dup2
#include <sys/wait.h>   //waitpid, WNOHANG, WIFEXITED, WIFSIGNALED
#include <string.h>     //strcmp, strcpy, strerror
#include <errno.h>      //errno
#include <fcntl.h>      //open, O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC, dup2


#define BUFF 1024
#define MAX_PROCESOS_BG 100

//Variables globales para background:
pid_t bg_pids[MAX_PROCESOS_BG];
char *bg_commands[MAX_PROCESOS_BG] ={NULL};
int bg_count = 0;

// Declaración de funciones
void hacer_cd(char **argv, int argc);        
void ejecutar_comandos_con_pipe(tline *linea);
void un_mandato(tline *linea);
void exec_background(tline *linea); 
void jobs();                        
void fg(char **argv, int argc);     
void remove_background_process(pid_t pid);
void add_background_process(pid_t pid, char *command);
void remove_job(pid_t pid);
void cleanup_background_jobs();

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
// Función para ejecutar un comando en background
void exec_background(tline *line) {
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    if (line == NULL || line->commands[0].argv[0] == NULL) {
        fprintf(stderr, "Error: línea o comando nulo.\n");
        return;
    }

    pid_t bg_pid = fork();
    if (bg_pid < 0) {
        fprintf(stderr, "Error al crear el proceso en background: %s\n", strerror(errno));
        return;
    } else if (bg_pid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);

        int dev_null = open("/dev/null", O_WRONLY);
        if (dev_null == -1) {
            perror("Error al abrir /dev/null");
            exit(1);
        }
        dup2(dev_null, STDOUT_FILENO);
        dup2(dev_null, STDERR_FILENO);
        close(dev_null);

        execvp(line->commands[0].argv[0], line->commands[0].argv);

        fprintf(stderr, "Error al ejecutar el comando en background: %s\n", strerror(errno));
        exit(1);
    } else {
        add_background_process(bg_pid, line->commands[0].argv[0]);
        printf("[Proceso en background iniciado: PID %d]\n", bg_pid);
    }
}

// Función para listar procesos en background
void jobs() {
    printf("Procesos en background:\n");
    for (int i = 0; i < bg_count; i++) {
        int status;
        pid_t result = waitpid(bg_pids[i], &status, WNOHANG);

        if (result == 0) {
            printf("[%d] Running\t%s\n", bg_pids[i], bg_commands[i]);
        } else if (result > 0) {
            printf("[%d] Finished\t%s\n", bg_pids[i], bg_commands[i]);
            free(bg_commands[i]);
            bg_commands[i] = NULL;
            remove_job(bg_pids[i]);
        } else {
            fprintf(stderr, "Error al verificar el estado del proceso con PID %d.\n", bg_pids[i]);
        }
    }
}

// Mover un proceso al foreground y restaurar terminal
void fg(char **argv, int argc) {
    if (argc == 0 || argv[0] == NULL) {
        fprintf(stderr, "Error: comando 'fg' no tiene argumentos.\n");
        return;
    }

    pid_t pid = 0;
    if (argc > 1) {
        pid = atoi(argv[1]);
        if (pid <= 0) {
            fprintf(stderr, "Error: PID inválido '%s'.\n", argv[1]);
            return;
        }
    } else {
        if (bg_count > 0) {
            pid = bg_pids[bg_count - 1];
        } else {
            fprintf(stderr, "Error: no hay procesos en background.\n");
            return;
        }
    }

    int found = 0;
    for (int i = 0; i < bg_count; i++) {
        if (bg_pids[i] == pid) {
            found = 1;
            break;
        }
    }
    if (!found) {
        fprintf(stderr, "Error: el proceso con PID %d no está en background.\n", pid);
        return;
    }

    printf("Moviendo proceso PID %d al primer plano...\n", pid);
    if (kill(pid, SIGCONT) == -1) {
        fprintf(stderr, "Error al enviar SIGCONT al proceso con PID %d: %s\n", pid, strerror(errno));
        return;
    }

    int status;
    if (waitpid(pid, &status, WUNTRACED) == -1) {
        fprintf(stderr, "Error al esperar al proceso con PID %d: %s\n", pid, strerror(errno));
    } else {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            remove_job(pid);
        }
        printf("Proceso PID %d finalizado.\n", pid);
    }
}

void remove_job(pid_t pid) {
    for (int i = 0; i < bg_count; i++) {
        if (bg_pids[i] == pid) {
            // Desplazar los elementos hacia la izquierda para eliminar el proceso
            for (int j = i; j < bg_count - 1; j++) {
                bg_pids[j] = bg_pids[j + 1];
                strcpy(bg_commands[j], bg_commands[j + 1]);
            }
            bg_count--; // Reducir el conteo de procesos en background
            break;
        }
    }
}

// Función para agregar un proceso a la lista de jobs
void add_background_process(pid_t pid, char *command) {
    if (bg_count < MAX_PROCESOS_BG) {
        bg_pids[bg_count] = pid;

        if (bg_commands[bg_count] != NULL) {
            free(bg_commands[bg_count]);
        }

        bg_commands[bg_count] = strdup(command);
        if (bg_commands[bg_count] == NULL) {
            fprintf(stderr, "Error al duplicar el comando\n");
            return;
        }

        bg_count++;
    } else {
        fprintf(stderr, "Error: se alcanzó el máximo de procesos en background (%d).\n", MAX_PROCESOS_BG);
    }
}


// Función para eliminar un proceso de la lista de jobs
void remove_background_process(pid_t pid) {
    for (int i = 0; i < bg_count; i++) {
        if (bg_pids[i] == pid) {
            free(bg_commands[i]);
            for (int j = i; j < bg_count - 1; j++) {
                bg_pids[j] = bg_pids[j + 1];
                bg_commands[j] = bg_commands[j + 1];
            }
            bg_count--;
            return;
        }
    }
    fprintf(stderr, "Error: PID %d no encontrado en la lista de procesos en background.\n", pid);
}

//Función para que no dé error el bg
void cleanup_background_jobs() {
    for (int i = 0; i < bg_count; i++) {
        if (bg_commands[i] != NULL) {
            free(bg_commands[i]);
            bg_commands[i] = NULL;
        }
    }
    bg_count = 0;
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
