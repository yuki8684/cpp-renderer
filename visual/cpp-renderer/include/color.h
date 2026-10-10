#pragma once

#include <iostream>

class Color {
public:
    Color() : r(0), g(0), b(0) {}
    Color(double red, double green, double blue) : r(red), g(green), b(blue) {}

    // Convert color to a format suitable for output (e.g., PPM)
    void write_color(std::ostream &out) const {
        out << static_cast<int>(255.999 * r) << ' '
            << static_cast<int>(255.999 * g) << ' '
            << static_cast<int>(255.999 * b) << '\n';
    }

    // Color addition
    Color operator+(const Color &other) const {
        return Color(r + other.r, g + other.g, b + other.b);
    }

    // Color multiplication by a scalar
    Color operator*(double scalar) const {
        return Color(r * scalar, g * scalar, b * scalar);
    }

    // Getters for color components
    double red() const { return r; }
    double green() const { return g; }
    double blue() const { return b; }

private:
    double r, g, b; // Color components
};