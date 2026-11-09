// Bresenham Paint - OpenGL 3.3 + GLFW + GLAD
// Lienzo de celdas con grilla de fondo. Las figuras se rasterizan con Bresenham
// sobre la matriz de celdas y la matriz completa se sube como textura (GL_NEAREST).

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdio>
#include <vector>

#include "bresenham.hpp"

// ------------------------- Configuracion -------------------------
constexpr int GRID_W  = 80;   // celdas en X
constexpr int GRID_H  = 60;   // celdas en Y
constexpr int CELL_PX = 12;   // pixeles de pantalla por celda
constexpr size_t MAX_HISTORY = 64;

enum class Tool { Pencil, Line, Square, Triangle, Circle, Eraser };

static const char* TOOL_NAMES[] = {"Lapiz", "Linea", "Cuadrado", "Triangulo", "Circulo", "Borrador"};

static const Color PALETTE[8] = {
    {0, 0, 0},       {220, 40, 40},  {40, 160, 60},  {40, 90, 220},
    {240, 200, 30},  {150, 60, 190}, {240, 130, 30}, {120, 120, 120},
};
static const Color WHITE = {255, 255, 255};

// ------------------------- Estado de la app -------------------------
struct App {
    GLFWwindow* window = nullptr;
    Canvas canvas{GRID_W, GRID_H};
    std::vector<std::vector<uint8_t>> history;

    Tool tool = Tool::Pencil;
    int colorIdx = 0;
    bool showGrid = true;

    bool drawing = false;
    int startX = 0, startY = 0;   // celda donde se presiono el mouse
    int curX = 0, curY = 0;       // celda actual
    int lastX = 0, lastY = 0;     // ultima celda (para lapiz / borrador)
};

static void updateTitle(App& app) {
    char buf[200];
    std::snprintf(buf, sizeof(buf), "Bresenham Paint | %s | Color %d | Grilla: %s | celda (%d,%d)",
                  TOOL_NAMES[static_cast<int>(app.tool)], app.colorIdx + 1,
                  app.showGrid ? "ON" : "OFF", app.curX, app.curY);
    glfwSetWindowTitle(app.window, buf);
}

static void pushHistory(App& app) {
    app.history.push_back(app.canvas.pixels());
    if (app.history.size() > MAX_HISTORY) app.history.erase(app.history.begin());
}

static void undo(App& app) {
    if (app.history.empty()) return;
    app.canvas.pixels() = app.history.back();
    app.history.pop_back();
}

// Dibuja una figura en `cv` segun la herramienta
static void drawShape(Canvas& cv, Tool t, int x0, int y0, int x1, int y1, Color c, bool freeRect) {
    switch (t) {
        case Tool::Line:     bresenham::line(cv, x0, y0, x1, y1, c); break;
        case Tool::Square:
            if (freeRect) bresenham::rect(cv, x0, y0, x1, y1, c);   // SHIFT = rectangulo
            else          bresenham::square(cv, x0, y0, x1, y1, c);
            break;
        case Tool::Triangle: bresenham::triangle(cv, x0, y0, x1, y1, c); break;
        case Tool::Circle: {
            int r = static_cast<int>(std::lround(std::hypot(double(x1 - x0), double(y1 - y0))));
            bresenham::circle(cv, x0, y0, r, c);
            break;
        }
        default: break;
    }
}

// Trazo continuo (lapiz / borrador): linea de Bresenham entre dos celdas, con grosor
static void stroke(App& app, int x0, int y0, int x1, int y1) {
    bool eraser = app.tool == Tool::Eraser;
    Color c = eraser ? WHITE : PALETTE[app.colorIdx];
    int half = eraser ? 1 : 0;   // borrador 3x3, lapiz 1x1

    // Reutiliza la misma linea de Bresenham y estampa el pincel en cada punto
    bresenham::lineEach(x0, y0, x1, y1, [&](int x, int y) {
        for (int j = -half; j <= half; ++j)
            for (int i = -half; i <= half; ++i)
                app.canvas.plot(x + i, y + j, c);
        });
}

static bool isFreehand(Tool t) { return t == Tool::Pencil || t == Tool::Eraser; }

// ------------------------- Callbacks -------------------------
static void cursorCallback(GLFWwindow* w, double mx, double my) {
    App& app = *static_cast<App*>(glfwGetWindowUserPointer(w));
    int ww, wh;
    glfwGetWindowSize(w, &ww, &wh);
    app.curX = std::clamp(static_cast<int>(std::floor(mx * GRID_W / ww)), 0, GRID_W - 1);
    app.curY = std::clamp(static_cast<int>(std::floor(my * GRID_H / wh)), 0, GRID_H - 1);

    if (app.drawing && isFreehand(app.tool)) {
        stroke(app, app.lastX, app.lastY, app.curX, app.curY);
        app.lastX = app.curX;
        app.lastY = app.curY;
    }
}

static void mouseCallback(GLFWwindow* w, int button, int action, int /*mods*/) {
    App& app = *static_cast<App*>(glfwGetWindowUserPointer(w));

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        pushHistory(app);
        app.drawing = true;
        app.startX = app.lastX = app.curX;
        app.startY = app.lastY = app.curY;
        if (isFreehand(app.tool)) stroke(app, app.curX, app.curY, app.curX, app.curY);
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE && app.drawing) {
        if (!isFreehand(app.tool)) {
            bool shift = glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                         glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
            drawShape(app.canvas, app.tool, app.startX, app.startY, app.curX, app.curY,
                      PALETTE[app.colorIdx], shift);
        }
        app.drawing = false;
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && app.drawing) {
        // Cancelar la operacion en curso
        undo(app);
        app.drawing = false;
    }
}

static void keyCallback(GLFWwindow* w, int key, int /*scancode*/, int action, int mods) {
    if (action != GLFW_PRESS) return;
    App& app = *static_cast<App*>(glfwGetWindowUserPointer(w));

    switch (key) {
        case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(w, GLFW_TRUE); return;
        case GLFW_KEY_P: app.tool = Tool::Pencil;   break;
        case GLFW_KEY_L: app.tool = Tool::Line;     break;
        case GLFW_KEY_S: app.tool = Tool::Square;   break;
        case GLFW_KEY_T: app.tool = Tool::Triangle; break;
        case GLFW_KEY_C: app.tool = Tool::Circle;   break;
        case GLFW_KEY_E: app.tool = Tool::Eraser;   break;
        case GLFW_KEY_G: app.showGrid = !app.showGrid; break;
        case GLFW_KEY_Z:
            if (mods & GLFW_MOD_CONTROL) undo(app);
            break;
        case GLFW_KEY_BACKSPACE:
        case GLFW_KEY_DELETE:
            pushHistory(app);
            app.canvas.clear();
            break;
        default:
            if (key >= GLFW_KEY_1 && key <= GLFW_KEY_8) app.colorIdx = key - GLFW_KEY_1;
            break;
    }
    updateTitle(app);
}

// ------------------------- Shaders -------------------------
static const char* VERT_SRC = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
out vec2 vUV;
void main() {
    vUV = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* FRAG_SRC = R"(#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uCanvas;
uniform vec2  uGrid;      // celdas (W, H)
uniform float uCellPx;    // pixeles de framebuffer por celda
uniform bool  uShowGrid;

void main() {
    // La fila 0 del canvas esta arriba; en OpenGL v=0 es abajo
    vec2 uv = vec2(vUV.x, 1.0 - vUV.y);
    vec3 col = texture(uCanvas, uv).rgb;

    if (uShowGrid) {
        vec2 cell = uv * uGrid;
        vec2 f  = fract(cell);
        vec2 id = floor(cell);
        float w = 1.0 / uCellPx;               // grosor de 1 pixel de pantalla

        bool lx = f.x < w;
        bool ly = f.y < w;
        bool major = (lx && mod(id.x, 10.0) < 0.5) || (ly && mod(id.y, 10.0) < 0.5);

        if (lx || ly) {
            vec3 g = major ? vec3(0.45, 0.50, 0.58) : vec3(0.72, 0.75, 0.80);
            col = mix(col, g, 0.6);
        }
    }
    FragColor = vec4(col, 1.0);
}
)";

static GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Error de shader:\n%s\n", log);
    }
    return s;
}

static GLuint makeProgram() {
    GLuint vs = compile(GL_VERTEX_SHADER, VERT_SRC);
    GLuint fs = compile(GL_FRAGMENT_SHADER, FRAG_SRC);
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, sizeof(log), nullptr, log);
        std::fprintf(stderr, "Error de link:\n%s\n", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

// ------------------------- main -------------------------
int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "No se pudo iniciar GLFW\n");
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);   // necesario en macOS
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    App app;
    app.window = glfwCreateWindow(GRID_W * CELL_PX, GRID_H * CELL_PX, "Bresenham Paint", nullptr, nullptr);
    if (!app.window) {
        std::fprintf(stderr, "No se pudo crear la ventana\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(app.window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::fprintf(stderr, "No se pudo cargar GLAD\n");
        return -1;
    }

    glfwSetWindowUserPointer(app.window, &app);
    glfwSetCursorPosCallback(app.window, cursorCallback);
    glfwSetMouseButtonCallback(app.window, mouseCallback);
    glfwSetKeyCallback(app.window, keyCallback);

    // Quad a pantalla completa: (x, y, u, v)
    const float quad[] = {
        -1.f, -1.f, 0.f, 0.f,
         1.f, -1.f, 1.f, 0.f,
        -1.f,  1.f, 0.f, 1.f,
         1.f,  1.f, 1.f, 1.f,
    };
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Textura del canvas (sin filtrado para ver las celdas nitidas)
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, GRID_W, GRID_H, 0, GL_RGB, GL_UNSIGNED_BYTE, app.canvas.data());

    GLuint prog = makeProgram();
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "uCanvas"), 0);
    glUniform2f(glGetUniformLocation(prog, "uGrid"), float(GRID_W), float(GRID_H));
    GLint locCellPx   = glGetUniformLocation(prog, "uCellPx");
    GLint locShowGrid = glGetUniformLocation(prog, "uShowGrid");

    std::puts("=== Bresenham Paint ===");
    std::puts("P lapiz | L linea | S cuadrado (SHIFT = rectangulo) | T triangulo | C circulo | E borrador");
    std::puts("1-8 colores | G grilla | Ctrl+Z deshacer | Backspace limpiar | Click derecho cancela | ESC salir");
    updateTitle(app);

    Canvas display(GRID_W, GRID_H);

    while (!glfwWindowShouldClose(app.window)) {
        glfwPollEvents();

        // Composicion: canvas confirmado + vista previa de la figura en curso
        display.pixels() = app.canvas.pixels();
        if (app.drawing && !isFreehand(app.tool)) {
            bool shift = glfwGetKey(app.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                         glfwGetKey(app.window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
            drawShape(display, app.tool, app.startX, app.startY, app.curX, app.curY,
                      PALETTE[app.colorIdx], shift);
        }

        glBindTexture(GL_TEXTURE_2D, tex);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, GRID_W, GRID_H, GL_RGB, GL_UNSIGNED_BYTE, display.data());

        int fbw, fbh;
        glfwGetFramebufferSize(app.window, &fbw, &fbh);
        glViewport(0, 0, fbw, fbh);
        glClearColor(1.f, 1.f, 1.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(prog);
        glUniform1f(locCellPx, float(fbw) / GRID_W);
        glUniform1i(locShowGrid, app.showGrid ? 1 : 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(app.window);
    }

    glDeleteTextures(1, &tex);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(prog);
    glfwDestroyWindow(app.window);
    glfwTerminate();
    return 0;
}
