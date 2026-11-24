# Proyecto Final - Computación Gráfica e Interacción Humano Computadora

**Alumno:** Daniel Imanol Gómez Varela - 319130377
**Semestre:** 2027-1 - Facultad de Ingeniería, UNAM

Recorrido 3D de Día de Muertos (atrio, iglesia, ofrenda y panteón) hecho en C++ con OpenGL en Visual Studio.

## Estructura del repositorio

```
Ejecutable/              Programa listo para correr (compilado en Release)
ProyectoFinal/           Código fuente, shaders y recursos
External Libraries/      GLFW, GLEW, GLM, Assimp y SOIL2
ProyectoFinal.sln        Solución de Visual Studio
armar_ejecutable.bat     Arma la carpeta Ejecutable después de compilar
```

## Descargar el proyecto

Se puede clonar el repositorio o bajarlo con **Code → Download ZIP** desde GitHub. Los modelos y texturas son archivos normales, así que las dos formas funcionan.

```
git clone https://github.com/danyhapp9/ProyectoFinalCompuGrafica_319130377.git
```

## Ejecutar el programa

1. Abrir la carpeta `Ejecutable/`.
2. Dar doble clic a `ProyectoFinal.exe`.

El programa debe abrirse desde su propia carpeta, porque busca los shaders, modelos e imágenes en `Shader/`, `assets/` e `Imagenes/` junto al `.exe`.

Si aparece un error de que falta `VCRUNTIME140.dll` o `MSVCP140.dll`, hay que instalar el **Microsoft Visual C++ Redistributable (x86)** desde la página de Microsoft.

## Compilar desde el código

1. Abrir `ProyectoFinal.sln` en Visual Studio 2022.
2. Elegir la configuración **Release | x86** (las librerías son de 32 bits).
3. Compilar con **Compilar → Recompilar solución**.
4. Dar doble clic a `armar_ejecutable.bat`, que vuelve a armar la carpeta `Ejecutable/`.

Para probar desde Visual Studio basta con F5 en **Debug | x86**.

Las librerías se referencian con `$(SolutionDir)` y todas las rutas de modelos, shaders e imágenes son relativas, así que no hay que configurar nada más.

## Controles

| Tecla | Acción |
|---|---|
| Clic izquierdo | Capturar el mouse para mover la cámara |
| W A S D / flechas | Caminar |
| Espacio | Saltar |
| F | Linterna |
| Q / E | Abrir o pasar ficha cultural / cerrar ficha |
| O | Abrir o cerrar el portón del panteón |
| C | Hacer doblar la campana |
| G | Mostrar u ocultar el fantasma |
| M | Pausar o reanudar las mariposas |
| Esc | Salir |

## Requisitos

- Windows 10 u 11
- Tarjeta gráfica con OpenGL 3.3 o superior
