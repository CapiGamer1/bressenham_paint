#pragma once
// Canvas de "pixeles grandes" (celdas) y algoritmos de rasterizacion de Bresenham.

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <vector>

struct Color {
    uint8_t r, g, b;
};

// Lienzo: una matriz W x H de celdas RGB. Cada celda es un "pixel" de la grilla.
class Canvas {
public:
    Canvas(int w, int h) : w_(w), h_(h), px_(static_cast<size_t>(w)* h * 3, 255) {}

    int width() const { return w_; }
    int height() const { return h_; }

    void clear(Color c = { 255, 255, 255 }) {
        for (size_t i = 0; i < px_.size(); i += 3) {
            px_[i] = c.r; px_[i + 1] = c.g; px_[i + 2] = c.b;
        }
    }

    void plot(int x, int y, Color c) {
        if (x < 0 || y < 0 || x >= w_ || y >= h_) return;
        size_t i = (static_cast<size_t>(y) * w_ + x) * 3;
        px_[i] = c.r; px_[i + 1] = c.g; px_[i + 2] = c.b;
    }

    const uint8_t* data() const { return px_.data(); }
    std::vector<uint8_t>& pixels() { return px_; }
    const std::vector<uint8_t>& pixels() const { return px_; }

private:
    int w_, h_;
    std::vector<uint8_t> px_;
};

namespace bresenham {

    // ---------------------------------------------------------------
    // Linea de Bresenham (version entera, funciona en los 8 octantes)
    // ---------------------------------------------------------------
    // Bresenham clasico con parametro de decision p (funciona en los 8 octantes).
    // Llama a plot(x, y) por cada punto de la linea.
    template <typename PlotFn>
    inline void lineEach(int x0, int y0, int x1, int y1, PlotFn plot) {
        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;   // direccion en X
        int sy = (y0 < y1) ? 1 : -1;   // direccion en Y

        int x = x0, y = y0;

        if (dx >= dy) {
            // Pendiente suave (|m| <= 1): X avanza siempre, Y avanza a veces
            int p = 2 * dy - dx;
            for (int i = 0; i <= dx; ++i) {
                plot(x, y);
                if (p >= 0) {
                    y += sy;
                    p -= 2 * dx;
                }
                p += 2 * dy;
                x += sx;
            }
        }
        else {
            // Pendiente empinada (|m| > 1): se intercambian los roles de X e Y
            int p = 2 * dx - dy;
            for (int i = 0; i <= dy; ++i) {
                plot(x, y);
                if (p >= 0) {
                    x += sx;
                    p -= 2 * dy;
                }
                p += 2 * dx;
                y += sy;
            }
        }
    }

    inline void line(Canvas& cv, int x0, int y0, int x1, int y1, Color c) {
        lineEach(x0, y0, x1, y1, [&](int x, int y) { cv.plot(x, y, c); });
    }

    // ---------------------------------------------------------------
    // Circulo de Bresenham (algoritmo del punto medio, simetria de 8 puntos)
    // ---------------------------------------------------------------
    inline void circle(Canvas& cv, int cx, int cy, int r, Color c) {
        if (r <= 0) { cv.plot(cx, cy, c); return; }

        auto plot8 = [&](int x, int y) {
            cv.plot(cx + x, cy + y, c); cv.plot(cx - x, cy + y, c);
            cv.plot(cx + x, cy - y, c); cv.plot(cx - x, cy - y, c);
            cv.plot(cx + y, cy + x, c); cv.plot(cx - y, cy + x, c);
            cv.plot(cx + y, cy - x, c); cv.plot(cx - y, cy - x, c);
            };

        int x = 0, y = r;
        int d = 3 - 2 * r;           // parametro de decision
        plot8(x, y);
        while (y >= x) {
            ++x;
            if (d > 0) {
                --y;
                d += 4 * (x - y) + 10;
            }
            else {
                d += 4 * x + 6;
            }
            plot8(x, y);
        }
    }

    // ---------------------------------------------------------------
    // Rectangulo (4 lineas de Bresenham)
    // ---------------------------------------------------------------
    inline void rect(Canvas& cv, int x0, int y0, int x1, int y1, Color c) {
        line(cv, x0, y0, x1, y0, c);
        line(cv, x1, y0, x1, y1, c);
        line(cv, x1, y1, x0, y1, c);
        line(cv, x0, y1, x0, y0, c);
    }

    // Cuadrado: el lado es el mayor de |dx| y |dy|, respetando la direccion del arrastre
    inline void square(Canvas& cv, int x0, int y0, int x1, int y1, Color c) {
        int dx = x1 - x0, dy = y1 - y0;
        int side = std::max(std::abs(dx), std::abs(dy));
        int sx = dx < 0 ? -1 : 1;
        int sy = dy < 0 ? -1 : 1;
        rect(cv, x0, y0, x0 + sx * side, y0 + sy * side, c);
    }

    // ---------------------------------------------------------------
    // Triangulo isosceles inscrito en la caja (x0,y0)-(x1,y1)
    // Vertice superior en el centro de la caja, base abajo. 3 lineas de Bresenham.
    // ---------------------------------------------------------------
    inline void triangle(Canvas& cv, int x0, int y0, int x1, int y1, Color c) {
        int ax = (x0 + x1) / 2, ay = y0;   // apice
        line(cv, ax, ay, x1, y1, c);
        line(cv, x1, y1, x0, y1, c);
        line(cv, x0, y1, ax, ay, c);
    }

} // namespace bresenham
