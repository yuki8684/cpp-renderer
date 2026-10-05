#pragma once

#include <cmath>
#include <limits>
#include <memory>
#include <random>

using std::shared_ptr;
using std::make_shared;

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

// 生成一个在 [min, max] 范围内的随机浮点数
inline double random_double(double min, double max) {
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937 generator; // 随机数生成器
    return min + (max - min) * distribution(generator);
}

// 生成一个在 [0, 1] 范围内的随机浮点数
inline double random_double() {
    return random_double(0.0, 1.0);
}

// 将角度转换为弧度
inline double degrees_to_radians(double degrees) {
    return degrees * pi / 180.0;
}

// 限制值在 [low, high] 范围内
inline double clamp(double x, double min, double max) {
    return x < min ? min : (x > max ? max : x);
}