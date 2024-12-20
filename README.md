1. **Ejecutar un solo comando en foreground (con 0 o más argumentos)**  
   - **Puntuación**: 0.5 puntos  
   - [X] **Completado**  

2. **Redirección de entrada y salida con un solo comando en foreground**  
   - **Puntuación**: 1 punto  
   - [X] **Completado**  

3. **Ejecutar dos comandos en foreground con pipes (`|`), con redirección de entrada y salida**  
   - **Puntuación**: 1 punto  
   - [X] **Completado**  

4. **Ejecutar más de dos comandos en foreground con pipes (`|`), con redirección de entrada y salida**  
   - **Puntuación**: 4 puntos  
   - [X] **Completado**  

5. **Comando `cd` (cambio de directorio)**  
   - **Puntuación**: 0.5 puntos  
   - [X] **Completado**  

6. **Ejecutar comandos en background con más de dos mandatos, redirección de entrada/salida y soporte para `jobs` y `fg`**  
   - **Puntuación**: 2 puntos  
   - [+-] **Completado**  
   - Descripción: Soporte para comandos en background, con gestión de procesos mediante `jobs` y `fg`.

7. **Manejo de señales SIGINT y SIGQUIT para procesos en background**  
   - **Puntuación**: 1 punto  
   - [ ] **Completado**  
   - Descripción: El minishell ignora señales SIGINT y SIGQUIT para procesos en background, pero sigue respondiendo a ellas en foreground.
