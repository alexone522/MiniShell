1. **Ejecutar un solo comando en foreground (con 0 o más argumentos)**  
   - **Puntuación**: 0.5 puntos  
   - [X] **Completado**  
   - Descripción: El intérprete ejecuta comandos simples con cualquier número de argumentos.

2. **Redirección de entrada y salida con un solo comando en foreground**  
   - **Puntuación**: 1 punto  
   - [X] **Completado**  
   - Descripción: Soporte para redirigir entrada desde un archivo y salida a un archivo en un solo comando.

3. **Ejecutar dos comandos en foreground con pipes (`|`), con redirección de entrada y salida**  
   - **Puntuación**: 1 punto  
   - [ ] **Completado**  
   - Descripción: Soporte para encadenar dos comandos con pipes y manejar redirecciones de entrada y salida.

4. **Ejecutar más de dos comandos en foreground con pipes (`|`), con redirección de entrada y salida**  
   - **Puntuación**: 4 puntos  
   - [ ] **Completado**  
   - Descripción: Soporte para encadenar más de dos comandos con pipes, gestionando redirecciones de entrada y salida.

5. **Comando `cd` (cambio de directorio)**  
   - **Puntuación**: 0.5 puntos  
   - [ ] **Completado**  
   - Descripción: Implementación del comando `cd` para cambiar de directorio, soportando rutas absolutas, relativas y la variable `HOME`.

6. **Ejecutar comandos en background con más de dos mandatos, redirección de entrada/salida y soporte para `jobs` y `fg`**  
   - **Puntuación**: 2 puntos  
   - [ ] **Completado**  
   - Descripción: Soporte para comandos en background, con gestión de procesos mediante `jobs` y `fg`.

7. **Manejo de señales SIGINT y SIGQUIT para procesos en background**  
   - **Puntuación**: 1 punto  
   - [ ] **Completado**  
   - Descripción: El minishell ignora señales SIGINT y SIGQUIT para procesos en background, pero sigue respondiendo a ellas en foreground.
