#pragma once

#include "vec3.h"

// ============================================================
// 颜色类，用于表示颜色值及其操作
// ============================================================
class Color {
public:
    Color() : e{0, 0, 0} {}
    Color(double r, double g, double b) : e{r, g, b} {}

    // 获取颜色的红色分量
    double r() const { return e[0]; }
    // 获取颜色的绿色分量
    double g() const { return e[1]; }
    // 获取颜色的蓝色分量
    double b() const { return e[2]; }

    // 颜色的加法操作
    Color operator+(const Color& other) const {
        return Color(e[0] + other.e[0], e[1] + other.e[1], e[2] + other.e[2]);
    }

    // 颜色的乘法操作
    Color operator*(double scalar) const {
        return Color(e[0] * scalar, e[1] * scalar, e[2] * scalar);
    }

private:
    double e[3]; // 颜色的 RGB 分量
};