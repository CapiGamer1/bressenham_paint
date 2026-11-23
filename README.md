# Bresenham Paint

Programa tipo Paint en **C++** con **OpenGL 3.3**, **GLFW** y **GLAD**. Dibuja líneas, cuadrados, triángulos y círculos sobre una grilla, calculados con los **algoritmos de Bresenham** (solo aritmética entera).

## Autor

- **Nombre:** Daniel Felipe Soler Miranda
- **Código de estudiante:** 6014340

## Controles

| Tecla / Mouse | Acción |
|---|---|
| `P` / `L` | Lápiz / Línea |
| `S` | Cuadrado (con `SHIFT`: rectángulo) |
| `T` / `C` | Triángulo / Círculo |
| `E` | Borrador |
| `1`–`8` | Colores |
| `G` | Mostrar u ocultar la grilla |
| `Ctrl+Z` | Deshacer |
| `Backspace` | Limpiar |
| Clic izquierdo (arrastrar) | Dibujar con vista previa |
| Clic derecho | Cancelar la figura en curso |
| `ESC` | Salir |

## Arquitectura

```
bresenham.hpp  → Canvas (matriz de celdas) + algoritmos de Bresenham
main.cpp       → Ventana, mouse/teclado, estado y render con OpenGL
glad.c         → Cargador de funciones de OpenGL
```

**Flujo:** el mouse y el teclado generan eventos → los callbacks de GLFW actualizan el estado → los algoritmos de Bresenham rellenan una matriz de 80×60 celdas → esa matriz se sube como textura (`GL_NEAREST`) → un shader la muestra en un rectángulo a pantalla completa y dibuja la grilla encima.

La GPU no dibuja las figuras, solo muestra la imagen. Las figuras las calcula Bresenham en la CPU.

**Puntos clave del diseño:**
- `Canvas::plot` es la única función que escribe en el lienzo y descarta los puntos fuera de los límites.
- La vista previa se dibuja en una copia del lienzo, así cancelar una figura no deja rastro.
- El deshacer guarda una copia del lienzo antes de cada acción.

## Algoritmos

- **Línea:** parámetro de decisión `p`. Si `dx ≥ dy`, `x` avanza siempre y `p` decide si `y` también avanza (`p` inicial = `2·dy − dx`); si no, se intercambian los ejes. Cubre los 8 octantes.
- **Círculo (punto medio):** parámetro de decisión `d` (inicial = `3 − 2r`) y simetría de 8 puntos. Si `d > 0`, baja `y` y `d += 4·(x − y) + 10`; si no, `d += 4·x + 6`.
- **Cuadrado, rectángulo y triángulo:** se arman con líneas de Bresenham.

## Compilación

Requiere C++17, GLFW y GLAD (OpenGL 3.3 Core). En Visual Studio (x64):

1. Agregar `main.cpp`, `bresenham.hpp` y `glad.c` al proyecto.
2. Incluir las carpetas `include` de GLFW y GLAD.
3. Enlazar `glfw3.lib` y `opengl32.lib`.
